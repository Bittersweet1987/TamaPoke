#include "pet.h"
#include "dex.h"
#include "audio.h"
#include "dayphase.h"
#include "party.h"
#include "learnsets_real.h"
#include <string.h>

static void loadDexBitmap(Preferences &prefs, const char *key, uint8_t *dst, size_t dstLen) {
  memset(dst, 0, dstLen);
  size_t stored = prefs.getBytesLength(key);
  if (!stored) return;
  size_t copyLen = stored < dstLen ? stored : dstLen;
  prefs.getBytes(key, dst, copyLen);
}

static const uint32_t STEP_DAILY_GOALS[STEP_DAILY_GOAL_COUNT] = {
  500UL, 2000UL, 5000UL
};
static const uint32_t STEP_TOTAL_GOALS[] = {
  10000UL, 50000UL, 100000UL, 250000UL
};

static uint8_t stepRewardBit(uint8_t index) {
  return index < STEP_DAILY_GOAL_COUNT ? (uint8_t)(1U << index) : 0;
}

static ExpeditionItem stepRewardItemFor(uint8_t index) {
  switch (index) {
    case 0: return EXP_ITEM_SNACK;
    case 1: return EXP_ITEM_ENERGY;
    case 2: return EXP_ITEM_TRAIN;
    default: return EXP_ITEM_NONE;
  }
}

void Pet::begin() {
  prefs.begin("tamapoke", false);
  dexPrefs.begin("tamapoke", false, "dexnvs");
  bool hadSave = prefs.getBool("init", false);
  saveLoadedFromNvs = hadSave;
  saveCreatedThisBoot = !hadSave;
  if (!hadSave) {
    prefs.putBool("init", true);
    newEgg();
  } else {
    load();
    backfillDexMonHistory();
    backfillDexMonHistoryAfterDexExpand();
    backfillDexMonHistoryAfterMovesField();
    backfillSentAwayFlags();
    {
      // Automatische Diagnose bei jedem Boot (keine interaktive Serial-Anfrage
      // noetig -- die Antwort auf Befehle ueber die native USB-CDC-Verbindung
      // dieses Boards ist unzuverlaessig, passiver Boot-Log dagegen nicht).
      // Nur bei Auffaelligkeiten ausgeben (Normalfall bleibt stumm), damit
      // das nicht zu dauerhaftem Rauschen im Log wird.
      uint16_t nCaught = 0, nCaughtEmpty = 0, nBred = 0, nBredEmpty = 0;
      for (int16_t d = 1; d <= DEX_COUNT; d++) {
        if (isCaught(d)) { nCaught++; if (dexMonsCaught[d].empty()) nCaughtEmpty++; }
        if (isRegistered(d)) { nBred++; if (dexMonsBred[d].empty()) nBredEmpty++; }
      }
      if (nCaughtEmpty > 0 || nBredEmpty > 0) {
        Serial.printf("WARNUNG boot-audit: %u/%u gefangene und %u/%u gezuechtete DexMon-Eintraege "
                      "sind trotz Bitmap-Flag leer (dexnvs evtl. wieder voll?)\n",
                      nCaughtEmpty, nCaught, nBredEmpty, nBred);
      }
    }
  }
  ensureStepDay();
  lastTick = millis();
}

void Pet::newEgg() {
  // Das bisherige Exemplar ist jetzt endgueltig weggeschickt: Eintrag final
  // sichern und fuer immer als "schon weggeschickt" markieren (siehe
  // canBeSentAway()) -- verhindert, dass man es nach einem Zurueckwechseln
  // (switchActiveTo()) ein zweites Mal wegschicken kann.
  if (speciesId >= 1 && speciesId <= DEX_COUNT) {
    syncOwnDexMon();
    dexMonsBred[speciesId].sentAway = true;
  }
  ceremony = CER_NONE;
  ceremonyUntil = 0;
  neglectTicks = 0;
  weight = 0;
  speciesId = -1;
  prevSpeciesId = -1;
  eggTarget = pickEggSpecies();  // especie oculta segun rareza y pokedex
  starterPick = (registeredCount() == 0);  // primera partida: el jugador elige inicial
  // sorteo shiny: 1/48 base, mejor con despedida y con racha/vinculo altos
  int shinyBase = (lastEnd == CER_FAREWELL ? 24 : 48) - careBonus();
  if (shinyBase < 8) shinyBase = 8;
  eggShiny = (random(shinyBase) == 0);
  eggTaps = 0;
  fullness = 80;
  joy = 80;
  energy = 80;
  hygiene = 100;
  poops = 0;
  ageMinutes = 0;
  lastPetInteractMinute = 0;
  evoDeclinedLv = 0;
  evoDeclinedAge = 0;
  farDeclinedAge = 0;
  careMistakes = 0;
  mistakeCooldown = 0;
  sleeping = false;
  save();
}

// progresion offline: el tiempo paso aunque estuviera apagado, pero con
// piedad — las barras bajan con suelo en 15 (vuelve hambriento, no muerto),
// sin descuidos ni escapadas en ausencia
static uint8_t dropTo(uint8_t v, uint8_t d, uint8_t fl) {
  if (v <= fl) return v;
  return (v - fl > d) ? v - d : fl;
}

void Pet::setClock(uint32_t nowEpoch) {
  lastSeenEpoch = nowEpoch;
  ensureStepDay();
  if (nowEpoch) save();  // persiste ya: un corte de luz no pierde la referencia
}

void Pet::syncClock(uint32_t nowEpoch) {
  uint32_t seen = prefs.getUInt("seen", 0);
  lastSeenEpoch = nowEpoch;
  ensureStepDay();
  if (nowEpoch == 0) return;
  uint32_t mins = (seen && nowEpoch > seen) ? (nowEpoch - seen) / 60 : 0;
  if (mins < 2 || ceremony != CER_NONE) {
    save();  // primera vez o sin tiempo que aplicar: solo persistir la hora
    return;
  }
  if (mins > 14UL * 24 * 60) mins = 14UL * 24 * 60;  // tope: 2 semanas

  for (uint32_t i = 0; i < mins; i++) {
    ageMinutes++;
    if (isEgg()) {
      if (ageMinutes >= 3) hatch();  // eclosiona en tu ausencia
      continue;
    }
    if (sleeping) {  // descanso: baja lento y con suelo, igual que en vivo
      energy = clamp100(energy + 6);
      if (ageMinutes % 2 == 0) {
        fullness = dropTo(fullness, 1, 30);
        joy = dropTo(joy, 1, 35);
      }
      if (ageMinutes % 3 == 0) hygiene = dropTo(hygiene, 1, 45);
      continue;
    }
    uint32_t minuteEpoch = seen + (i + 1UL) * 60UL;
    fullness = dropTo(fullness, nightFoodDrop(minuteEpoch), 15);
    energy = dropTo(energy, 1, 15);
    hygiene = dropTo(hygiene, 1, 15);
    joy = dropTo(joy, 1, 15);
  }
  if (!isEgg()) {
    if (!sleeping) {  // durmiendo no ensucia
      uint8_t p = poops + mins / 240;
      poops = p > 3 ? 3 : p;
    }
    // la evolucion NO se aplica offline: queda lista y la dispara el usuario
    // tocando al bicho cuando vuelve (para que vea la transformacion)
  }
  Serial.printf("offline: %u min aplicados (nv.%u)\n", mins, level());
  save();
}

bool Pet::update(uint32_t nowMs) {
  ensureStepDay();
  // fin de ceremonia: la criatura se va y queda un huevo nuevo
  if (ceremony != CER_NONE && deadlineReached(millis(), ceremonyUntil)) {
    newEgg();
    return true;
  }
  bool changed = false;
  while (nowMs - lastTick >= PET_TICK_MS) {
    lastTick += PET_TICK_MS;
    tick();
    changed = true;
  }
  return changed;
}

void Pet::tick() {
  if (ceremony != CER_NONE) return;  // el tiempo se detiene en la despedida
  ageMinutes++;
  checkLevelUpMoves();
  syncOwnDexMon();

  if (isEgg()) {
    if (ageMinutes >= 3) hatch();  // si no lo tocas, eclosiona solo a los 3 min
    return;
  }

  // el sueño es descanso: la energia se recupera y las necesidades bajan MUCHO
  // mas lento que despierto y con suelo (amanece pidiendo algo de mimo, no a
  // cero, sin descuidos ni escapadas). despierto: comida -2/min, hig/joy -1/min.
  // El peso aun se quema; la racha de buen cuidado (goodTicks) queda en pausa.
  if (sleeping) {
    energy = clamp100(energy + 6);
    if (weight > 0 && ageMinutes % 3 == 0) weight--;
    if (ageMinutes % 2 == 0) {                 // ~4x mas lento que despierto
      fullness = dropTo(fullness, 1, 30);
      joy = dropTo(joy, 1, 35);
    }
    if (ageMinutes % 3 == 0) hygiene = dropTo(hygiene, 1, 45);
    checkMedals();  // aun puede cruzar un nivel por edad mientras duerme
    if (++ticksSinceSave >= 5) pendingSave = true;
    return;
  }

  if (ageMinutes % MINUTES_PER_LEVEL == 0) sfxPlay(SFX_LEVEL);  // subio de nivel (despierto)

  fullness = clamp100(fullness - nightFoodDrop(lastSeenEpoch));
  energy = clamp100(energy - 1);
  if (fullness > 40 && poops < 3 && random(100) < 15) poops++;

  hygiene = clamp100(hygiene - 1 - 4 * poops);
  // el sobrepeso da pereza: la energia cae el doble
  if (weight > 50) energy = clamp100(energy - 1);
  if (weight > 0 && ageMinutes % 3 == 0) weight--;

  // la disciplina forja la defensa: 12 h seguidas bien cuidado = +1 DEF
  if (lowestStat() >= 40) {
    if (++goodTicks >= 720) {
      goodTicks = 0;
      if (trDef < 100) trDef++;
    }
  } else {
    goodTicks = 0;
  }

  int dJoy = -1;
  if (fullness < 30) dJoy -= 2;
  if (hygiene < 30) dJoy -= 2;
  joy = clamp100(joy + dJoy);

  // Descuido: dejar una estadistica por los suelos cuenta como error de
  // cuidado (con enfriamiento para no contar el mismo descuido cada minuto)
  if (mistakeCooldown > 0) mistakeCooldown--;
  if (lowestStat() <= 10 && mistakeCooldown == 0) {
    careMistakes++;
    mistakeCooldown = 30;
    if (bond > 3) bond -= 3;  // el descuido enfria el vinculo
  }

  checkMedals();  // la evolucion la dispara el usuario (canEvolveNow + tap), no el tick

  // abandono total: con TODO a cero durante una hora queda lista para escaparse;
  // NO se va sola, la dispara el usuario con el boton (final triste, lo presencia)
  if (fullness == 0 && joy == 0 && energy == 0 && hygiene == 0) {
    if (neglectTicks < RUNAWAY_TICKS) neglectTicks++;
  } else {
    neglectTicks = 0;  // un solo cuidado la salva
  }

  // ciclo completo (forma final + 7 dias): la despedida NO salta sola; queda
  // lista (canFarewellNow) y la dispara el usuario con el boton, para que la vea

  // autoguardado periodico: NO escribir a flash aqui (corre dentro del loop,
  // mientras se anima); solo marcar y dejar que el loop lo vuelque al atenuar
  if (++ticksSinceSave >= 5) pendingSave = true;
}

// vuelca el guardado periodico pendiente (lo llama el loop en un momento sin
// animacion para que el paron de la escritura a flash no se vea)
void Pet::flushSave() {
  if (pendingSave) save();
}

bool Pet::hasEvolutionPath(int16_t dex) const {
  if (dex < 1 || dex > DEX_COUNT) return false;
  for (uint16_t i = 0; i < EVOLUTION_RULE_COUNT; i++)
    if (EVOLUTION_RULES[i].from == dex) return true;
  return false;
}

// Quedan miembros sin registrar en la linea evolutiva de esta base? Se hace
// un pequeno recorrido por el grafo para cubrir ramas (Eevee, Tyrogue, etc.).
bool Pet::lineHasUnregistered(int16_t base) const {
  int16_t pending[12];
  uint8_t pendingCount = 0;
  bool seen[DEX_COUNT + 1] = { false };
  if (base >= 1 && base <= DEX_COUNT) pending[pendingCount++] = base;
  while (pendingCount) {
    int16_t cur = pending[--pendingCount];
    if (cur < 1 || cur > DEX_COUNT || seen[cur]) continue;
    seen[cur] = true;
    if (!isRegistered(cur)) return true;
    for (uint16_t i = 0; i < EVOLUTION_RULE_COUNT; i++) {
      const EvolutionRule &r = EVOLUTION_RULES[i];
      if (r.from != cur || seen[r.to]) continue;
      if (pendingCount < sizeof(pending) / sizeof(pending[0])) pending[pendingCount++] = r.to;
    }
  }
  return false;
}

uint8_t Pet::eggRarity() const {
  return (eggTarget >= 1 && eggTarget <= DEX_COUNT) ? DEX_TBL[eggTarget].rarity : R_COMUN;
}

// elige la especie del huevo: tirada de rareza (mejorada por una despedida
// completa, castigada por una escapada) y sesgo hacia lineas incompletas
int16_t Pet::pickEggSpecies() {
  // primera partida: inicial clasico
  if (registeredCount() == 0) {
    return CLASSIC_DEX[random(NUM_CLASSIC_DEX)];
  }

  uint8_t tier = R_COMUN;
  if (lastEnd != CER_RUNAWAY) {
    bool blessed = (lastEnd == CER_FAREWELL);
    int rare = (blessed ? 45 : 27) + careBonus();
    int leg = (registeredCount() >= 25) ? (blessed ? 10 : 3) + careBonus() / 3 : 0;
    int r = random(100);
    if (r < leg) tier = R_LEGENDARIO;
    else if (r < leg + rare) tier = R_RARO;
  }

  // candidatos del tier con linea incompleta; si no hay, baja de tier;
  // si la pokedex del tier esta completa, vale cualquiera del tier
  for (int pass = 0; pass < 2; pass++) {
    for (int t = tier; t >= R_COMUN; t--) {
      int16_t cand[DEX_COUNT];
      int n = 0;
      for (int16_t d = 1; d <= DEX_COUNT; d++) {
        if (DEX_TBL[d].rarity != t) continue;
        if (pass == 0 && !lineHasUnregistered(d)) continue;
        cand[n++] = d;
      }
      if (n > 0) return cand[random(n)];
    }
  }
  return CLASSIC_DEX[random(NUM_CLASSIC_DEX)];  // inalcanzable, por si acaso
}

void Pet::registerSpecies(int16_t dex) {
  if (dex < 1 || dex > DEX_COUNT) return;
  bool wasKnown = isRegistered(dex) || isCaught(dex);
  dexReg[(dex - 1) >> 3] |= (1 << ((dex - 1) & 7));
  if (shiny) dexShinyReg[(dex - 1) >> 3] |= (1 << ((dex - 1) & 7));
  if (!wasKnown) applyDexRewards();
}

// la racha y el vinculo mejoran el sorteo del huevo (0..~14)
int Pet::careBonus() const {
  int s = streak > 30 ? 30 : streak;
  return s / 3 + bond / 25;
}

uint8_t Pet::dailyGoalTarget(uint8_t goalType) const {
  switch (goalType) {
    case DAILY_GOAL_CARE: return 1;
    case DAILY_GOAL_PLAY: return 1;
    case DAILY_GOAL_BATTLE: return 1;
    case DAILY_GOAL_CATCH: return 5;
    case DAILY_GOAL_MEMO: return 3;
    default: return 1;
  }
}

bool Pet::dailyGoalComplete(uint8_t index) const {
  return index < DAILY_GOAL_COUNT && (dailyGoalDone & (1 << index));
}

void Pet::ensureDailyGoals() {
  if (isEgg() || ceremony != CER_NONE) return;
  uint32_t d = today();
  if (d == 0 || d == dailyGoalDay) return;
  static const uint8_t POOL[] = {
    DAILY_GOAL_CARE, DAILY_GOAL_PLAY, DAILY_GOAL_CATCH, DAILY_GOAL_MEMO, DAILY_GOAL_BATTLE
  };
  uint8_t seed = (uint8_t)((d + (speciesId > 0 ? speciesId : 0)) % 5);
  for (uint8_t i = 0; i < DAILY_GOAL_COUNT; i++) {
    dailyGoalType[i] = POOL[(seed + i) % 5];
    dailyGoalProgress[i] = 0;
  }
  dailyGoalDone = 0;
  dailyGoalDay = d;
  save();
}

void Pet::applyDailyReward() {
  joy = clamp100((int)joy + 4);
  addBond(1);
}

void Pet::noteDailyGoal(uint8_t goalType, uint8_t amount) {
  if (isEgg() || ceremony != CER_NONE || amount == 0) return;
  ensureDailyGoals();
  if (dailyGoalDay == 0) return;
  bool changed = false;
  for (uint8_t i = 0; i < DAILY_GOAL_COUNT; i++) {
    if (dailyGoalType[i] != goalType || dailyGoalComplete(i)) continue;
    uint8_t target = dailyGoalTarget(goalType);
    uint16_t next = (uint16_t)dailyGoalProgress[i] + amount;
    dailyGoalProgress[i] = next > target ? target : next;
    changed = true;
    if (dailyGoalProgress[i] >= target) {
      dailyGoalDone |= (1 << i);
      applyDailyReward();
      heartUntil = millis() + HEART_MS;
      sfxPlay(SFX_DAILY_GOAL);
    }
  }
  if (changed) save();
}

// primer cuidado del dia: avanza la racha y afianza el vinculo
void Pet::registerCare() {
  if (isEgg() || ceremony != CER_NONE) return;
  uint32_t d = today();
  if (d == 0 || d == lastCareDay) return;  // sin reloj, o ya conto hoy
  if (lastCareDay == 0 || d == lastCareDay + 1) {
    streak++;
  } else {
    streak = 1;        // hubo un hueco de dias
    lastMilestone = 0;
  }
  lastCareDay = d;
  bondToday = 0;
  if (streak > bestStreak) bestStreak = streak;
  bond = clamp100(bond + 4);
  uint16_t ms = (streak >= 100) ? 100 : (streak >= 30) ? 30
              : (streak >= 7)   ? 7   : (streak >= 3)  ? 3 : 0;
  if (ms > lastMilestone) {
    lastMilestone = ms;
    milestoneUntil = millis() + 4500;
  }
  checkMedals();
  save();
}

void Pet::addBond(uint8_t amt) {
  if (bondToday >= 8) return;  // tope diario: el vinculo no se farmea
  bond = clamp100(bond + amt);
  bondToday += amt;
}

void Pet::checkMedals() {
  if (isEgg()) return;
  uint16_t before = medals;
  if (level() >= 10) medals |= MED_LV10;
  if (level() >= 25) medals |= MED_LV25;
  if (level() >= 50) medals |= MED_LV50;
  if (berryKnown) medals |= MED_BERRY;
  if (streak >= 7) medals |= MED_STREAK7;
  if (bond >= 100) medals |= MED_BOND;
  if (!hasEvolutionPath(speciesId)) medals |= MED_FINAL;
  if (weight == 0 && level() >= 5 && careMistakes == 0) medals |= MED_FIT;
  uint16_t gained = medals & ~before;
  if (gained) {
    for (uint16_t m = gained; m; m &= (m - 1)) totalMedals++;
    newMedal = gained;
    medalUntil = millis() + 4000;
    if (!sleeping) sfxPlay(SFX_MEDAL);
    save();
  }
}

void Pet::rename(const char *name) {
  strncpy(nick, name, sizeof(nick) - 1);
  nick[sizeof(nick) - 1] = 0;
  save();
}

// Wie im echten Spiel wirkt sich das Level MULTIPLIKATIV auf den Basiswert
// aus (angelehnt an die offizielle Formel stat = (2*Basis*Level)/100 + Level),
// statt es nur additiv draufzuschlagen -- vorher war ein Lv.1-Bisasam kaum
// schwaecher als ein Lv.100-Bisaflor (Basiswert+1 vs. Basiswert+100), jetzt
// skaliert die Kampfstaerke wie erwartet deutlich mit dem Level.
static uint16_t calcStat(uint8_t base, uint8_t gene, uint8_t lvl, uint8_t tr) {
  uint32_t adjBase = (uint32_t)base * gene / 100;
  return (uint16_t)(adjBase * 2 * lvl / 100 + lvl + tr);
}

uint16_t Pet::atkStat() const {
  return isEgg() ? 0 : calcStat(DEX_TBL[speciesId].bAtk, geneAtk, level(), trAtk);
}
uint16_t Pet::defStat() const {
  return isEgg() ? 0 : calcStat(DEX_TBL[speciesId].bDef, geneDef, level(), trDef);
}
uint16_t Pet::speStat() const {
  return isEgg() ? 0 : calcStat(DEX_TBL[speciesId].bSpe, geneSpe, level(), trSpe);
}

uint16_t Pet::spaStat() const {
  return isEgg() ? 0 : calcStat(DEX_TBL[speciesId].bSpA, geneSpA, level(), trSpA);
}

uint16_t Pet::spdStat() const {
  return isEgg() ? 0 : calcStat(DEX_TBL[speciesId].bSpD, geneSpD, level(), trSpD);
}

DexMon Pet::rollFreshDexMon(int16_t dex, uint16_t atLevel, bool isShiny) const {
  DexMon m;
  m.level = atLevel < 1 ? 1 : atLevel;
  m.geneAtk = 90 + random(21);
  m.geneDef = 90 + random(21);
  m.geneSpe = 90 + random(21);
  m.geneSpA = 90 + random(21);
  m.geneSpD = 90 + random(21);
  m.geneHp  = 90 + random(21);
  m.shiny = isShiny ? 1 : 0;

  // Zufaellig, aber ECHT: alle Attacken sammeln, die die Art bis zu diesem
  // Level per Level-Aufstieg tatsaechlich gelernt haette, daraus 4 zufaellig
  // ziehen -- statt wie beim aktiven Haustier immer deterministisch die 4
  // hoechststufigen. Macht gefangene Exemplare individueller (wie
  // unterschiedlich "trainierte" wilde Tiere).
  if (dex >= 1 && dex <= DEX_COUNT) {
    uint16_t pool[64];
    uint8_t poolN = 0;
    uint16_t total = learnCount(dex);
    for (uint16_t i = 0; i < total && poolN < 64; i++) {
      uint8_t lvl = learnLevel(dex, i);
      if (lvl > m.level) continue;
      uint16_t mv = learnMove(dex, i);
      if (mv == 0) continue;
      bool dup = false;
      for (uint8_t p = 0; p < poolN; p++) if (pool[p] == mv) dup = true;
      if (!dup) pool[poolN++] = mv;
    }
    uint8_t take = poolN < MOVE_SLOTS ? poolN : MOVE_SLOTS;
    for (uint8_t s = 0; s < take; s++) {
      uint8_t pick = random(poolN - s);
      m.moves[s] = pool[pick];
      pool[pick] = pool[poolN - s - 1];  // swap-remove, ohne Zuruecklegen
    }
  }
  return m;
}

// Lernt Attacken, deren Levelvoraussetzung seit dem letzten Check erreicht
// wurde. Level-0-Eintraege (TM/Lernstein) werden hier bewusst ausgelassen --
// die muessten separat ausgewaehlt werden koennen, das ist noch nicht gebaut.
// Voller Vorrat (4 Attacken): die AELTESTE (Slot 0) wird verdraengt, alle
// anderen ruecken nach -- einfachste Regel, keine Auswahl-UI noetig.
void Pet::checkLevelUpMoves() {
  if (isEgg() || speciesId < 1 || speciesId > DEX_COUNT) return;
  if (pendingLearnMove) return;  // Dialog schon offen -- erst entscheiden lassen
  uint8_t curLevel = level();
  if (curLevel <= moveLevelChecked) return;
  uint8_t n = learnCount(speciesId);
  for (uint8_t i = 0; i < n; i++) {
    uint16_t mv = learnMove(speciesId, i);
    uint8_t lv = learnLevel(speciesId, i);
    if (lv == 0 || lv <= moveLevelChecked || lv > curLevel) continue;
    bool known = false;
    int freeSlot = -1;
    for (int s = 0; s < MOVE_SLOTS; s++) {
      if (moves[s] == mv) { known = true; break; }
      if (moves[s] == 0 && freeSlot < 0) freeSlot = s;
    }
    if (known) continue;
    if (freeSlot >= 0) {
      moves[freeSlot] = mv;
      continue;
    }
    // Alle 4 Slots vergeben: wie im echten Spiel NICHT mehr automatisch
    // ersetzen, sondern per Dialog fragen (siehe TamaPoke.ino). moveLevelChecked
    // bleibt bewusst VOR diesem Level stehen, bis der Spieler entscheidet --
    // declineLearnMove()/confirmLearnMove() schieben es dann auf lv nach.
    pendingLearnMove = mv;
    pendingLearnLevel = lv;
    return;
  }
  moveLevelChecked = curLevel;
}

void Pet::declineLearnMove() {
  if (!pendingLearnMove) return;
  moveLevelChecked = pendingLearnLevel;
  pendingLearnMove = 0;
  pendingLearnLevel = 0;
  save();
}

void Pet::confirmLearnMove(uint8_t replaceSlot) {
  if (!pendingLearnMove || replaceSlot >= MOVE_SLOTS) return;
  moves[replaceSlot] = pendingLearnMove;
  moveLevelChecked = pendingLearnLevel;
  pendingLearnMove = 0;
  pendingLearnLevel = 0;
  save();
}

uint16_t Pet::registeredCount() const {
  uint16_t n = 0;
  for (int i = 1; i <= DEX_COUNT; i++)
    if (isRegistered(i)) n++;
  return n;
}

uint16_t Pet::caughtCount() const {
  uint16_t n = 0;
  for (int i = 1; i <= DEX_COUNT; i++)
    if (isCaught(i)) n++;
  return n;
}

uint16_t Pet::knownDexCount() const {
  uint16_t n = 0;
  for (int i = 1; i <= DEX_COUNT; i++)
    if (isRegistered(i) || isCaught(i)) n++;
  return n;
}

uint8_t Pet::collectionRank() const {
  uint16_t known = knownDexCount();
  static const uint16_t GOALS[] = { 10, 25, 50, 100, 151, 160, 200, 251 };
  uint8_t rank = 0;
  for (uint8_t i = 0; i < sizeof(GOALS) / sizeof(GOALS[0]); i++)
    if (known >= GOALS[i]) rank = i + 1;
  return rank;
}

uint8_t Pet::unlockedCollectionFrameCount() const {
  return (uint8_t)(collectionRank() + 1);
}

bool Pet::setCollectionFrame(uint8_t frame) {
  if (frame >= unlockedCollectionFrameCount()) return false;
  if (collectionFrame == frame) return true;
  collectionFrame = frame;
  save();
  return true;
}

uint8_t Pet::nextDexGoal() const {
  static const uint16_t GOALS[] = { 10, 25, 50, 100, 151, 160, 200, 251 };
  uint16_t known = knownDexCount();
  for (uint8_t i = 0; i < sizeof(GOALS) / sizeof(GOALS[0]); i++)
    if (known < GOALS[i]) return (uint8_t)GOALS[i];
  return 251;
}

uint8_t Pet::applyDexRewards() {
  if (ceremony != CER_NONE || isEgg()) return 0;
  static const uint16_t GOALS[] = { 10, 25, 50, 100, 151, 160, 200, 251 };
  uint16_t known = knownDexCount();
  uint8_t reached = 0;
  for (uint8_t i = 0; i < sizeof(GOALS) / sizeof(GOALS[0]); i++) {
    uint8_t bit = 1 << i;
    if (known < GOALS[i] || (dexRewardMask & bit)) continue;
    dexRewardMask |= bit;
    reached = (uint8_t)GOALS[i];
    if (GOALS[i] == 10) joy = clamp100((int)joy + 5);
    else if (GOALS[i] == 25) addBond(2);
    else if (GOALS[i] == 50) {
      if (trAtk <= trDef && trAtk <= trSpe) trAtk = clamp100((int)trAtk + 1);
      else if (trDef <= trAtk && trDef <= trSpe) trDef = clamp100((int)trDef + 1);
      else trSpe = clamp100((int)trSpe + 1);
    } else if (GOALS[i] == 100) {
      addBond(4);
    } else {
      heartUntil = millis() + HEART_MS;
    }
  }
  if (reached) save();
  if (reached) {
    lastDexReward = reached;
    dexRewardUntil = millis() + 4200;
    sfxPlay(SFX_MEDAL);
  }
  return reached;
}

void Pet::registerCaught(int16_t dex, bool shinyVariant) {
  if (dex < 1 || dex > DEX_COUNT) return;
  bool wasKnown = isRegistered(dex) || isCaught(dex);
  dexCaught[(dex - 1) >> 3] |= (1 << ((dex - 1) & 7));
  if (shinyVariant) dexShinyReg[(dex - 1) >> 3] |= (1 << ((dex - 1) & 7));
  noteDailyGoal(DAILY_GOAL_CATCH, 1);
  if (!wasKnown) applyDexRewards();
  save();
}

uint8_t Pet::catchChanceForWild(int16_t wildDex, uint8_t wildLevel, uint8_t petLevel, bool closeWin) const {
  if (wildDex < 1 || wildDex > DEX_COUNT) return 0;
  const DexEntry &wild = DEX_TBL[wildDex];
  if (wild.rarity == R_LEGENDARIO) return 0;
  int chance = wild.rarity == R_RARO ? 28 : 55;
  int levelGap = (int)wildLevel - (int)(petLevel ? petLevel : 1);
  if (levelGap > 0) chance -= levelGap * 4;
  else if (levelGap < 0) chance += (-levelGap) * 2;
  if (closeWin) chance += 8;
  chance += bond / 20;
  chance += stepCatchBonus();
  if (wild.rarity == R_RARO && chance > 60) chance = 60;
  if (chance > 75) chance = 75;
  if (chance < 10) chance = 10;
  return (uint8_t)chance;
}

bool Pet::tryCatchWild(int16_t wildDex, uint8_t wildLevel, uint8_t petLevel, bool closeWin,
                       uint8_t luckRoll, bool shinyVariant) {
  uint8_t chance = catchChanceForWild(wildDex, wildLevel, petLevel, closeWin);
  if (chance == 0) return false;
  if ((luckRoll % 100) < chance) {
    registerCaught(wildDex, shinyVariant);
    joy = clamp100((int)joy + 4);
    addBond(1);
    save();
    return true;
  }
  return false;
}

// forma final que ya cumplio su ciclo (7 dias): lista para despedirse. La
// despedida la dispara el usuario con el boton (no salta sola, para que la vea)
bool Pet::canFarewellNow() const {
  return !isEgg() && !sleeping && ceremony == CER_NONE && canBeSentAway() &&
         !hasEvolutionPath(speciesId) && ageMinutes >= FAREWELL_AGE_MIN;
}

// abandono total durante 1h: lista para escaparse. La dispara el usuario con el
// boton (final triste); cuidarla un solo tick la salva (neglectTicks se resetea)
bool Pet::canRunawayNow() const {
  return !isEgg() && !sleeping && ceremony == CER_NONE && canBeSentAway() && neglectTicks >= RUNAWAY_TICKS;
}

// Siehe canBeSentAway(): das Exemplar, dessen "Reise" noch offen ist (also
// noch nicht per Abschied/Weglaufen/Freilassen beendet wurde). Das ist immer
// hoechstens eins -- ein neues Ei gibt es ja erst NACHDEM das vorherige
// weggeschickt wurde (siehe newEgg()).
int16_t Pet::homeSpeciesId() const {
  if (canBeSentAway()) return speciesId;  // das aktive Exemplar selbst ist es meistens
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    if (!dexMonsBred[d].empty() && !dexMonsBred[d].sentAway) return d;
  }
  return -1;
}

bool Pet::switchActiveTo(int16_t dex) {
  if (dex < 1 || dex > DEX_COUNT || dex == speciesId) return false;
  if (isEgg() || ceremony != CER_NONE) return false;
  const DexMon &target = dexMonsBred[dex];
  if (target.empty()) return false;

  // Das bisherige Exemplar zuerst sichern -- es bleibt ueber den Pokedex
  // jederzeit abrufbar (siehe switchActiveTo() beim naechsten Wechsel).
  if (speciesId >= 1 && speciesId <= DEX_COUNT) syncOwnDexMon();

  speciesId = dex;
  prevSpeciesId = -1;
  geneAtk = target.geneAtk; geneDef = target.geneDef; geneSpe = target.geneSpe;
  geneSpA = target.geneSpA; geneSpD = target.geneSpD;
  trAtk = target.trAtk; trDef = target.trDef; trSpe = target.trSpe;
  trSpA = target.trSpA; trSpD = target.trSpD;
  shiny = target.shiny;
  for (int i = 0; i < MOVE_SLOTS; i++) moves[i] = target.moves[i];
  moveLevelChecked = target.moveLevelChecked;
  bond = target.bond;
  medals = target.medals;
  snprintf(nick, sizeof(nick), "%s", target.nick);
  // Exaktes Alter, falls seit der letzten Sync-Runde bekannt; sonst (alte
  // Speicherstaende von vor diesem Feature) aus dem gespeicherten Level
  // rekonstruiert -- verliert hoechstens den Fortschritt innerhalb des
  // aktuellen Levels, nie den Level selbst.
  ageMinutes = target.ageMinutes ? target.ageMinutes
             : (uint32_t)(target.level > 1 ? target.level - 1 : 0) * MINUTES_PER_LEVEL;
  // Pflegewerte wie beim Schluepfen neu: waehrend der Bank-Zeit im Pokedex
  // (= Box) braucht ein Exemplar keine Pflege, siehe switchActiveTo()-Kommentar.
  fullness = 80; joy = 80; energy = 80; hygiene = 100;
  poops = 0; weight = 0; careMistakes = 0; mistakeCooldown = 0;
  berryKnown = false;
  sleeping = false;
  evoDeclinedLv = 0; evoDeclinedAge = 0; farDeclinedAge = 0;
  lastPetInteractMinute = 0;
  registerSpecies(speciesId);
  syncOwnDexMon();  // sofort zurueckspiegeln, damit die Karte nicht kurz leer wirkt
  save();
  return true;
}

void Pet::startFarewell() {
  if (isEgg() || ceremony != CER_NONE || !canBeSentAway()) return;
  lastEnd = CER_FAREWELL;
  ceremony = CER_FAREWELL;
  ceremonyUntil = millis() + CEREMONY_MS;
  heartUntil = ceremonyUntil;  // corazones durante toda la despedida
  sfxPlay(SFX_BYE);
  syncOwnDexMon();  // Level/Attacken sofort sichern, nicht erst beim naechsten Tick
  save();
}

void Pet::startRunaway() {
  if (isEgg() || ceremony != CER_NONE || !canBeSentAway()) return;
  lastEnd = CER_RUNAWAY;
  ceremony = CER_RUNAWAY;
  ceremonyUntil = millis() + CEREMONY_MS;
  sfxPlay(SFX_BYE);
  syncOwnDexMon();
  save();
}

void Pet::release() {
  if (isEgg() || ceremony != CER_NONE || !canBeSentAway()) return;
  lastEnd = CER_RELEASE;
  ceremony = CER_RELEASE;
  ceremonyUntil = millis() + CEREMONY_MS;
  heartUntil = ceremonyUntil;
  sfxPlay(SFX_BYE);
  syncOwnDexMon();
  save();
}

void Pet::hatch() {
  speciesId = eggTarget;
  shiny = eggShiny;
  // genes del individuo: 90-110% por stat (cada crianza es unica)
  geneAtk = 90 + random(21);
  geneDef = 90 + random(21);
  geneSpe = 90 + random(21);
  trAtk = trDef = trSpe = 0;
  moves[0] = moves[1] = moves[2] = moves[3] = 0;
  moveLevelChecked = 0;
  berryKnown = false;
  bond = 0;          // vinculo, medallas y nombre son del individuo
  bondToday = 0;
  medals = 0;
  newMedal = 0;
  nick[0] = 0;
  lastPetInteractMinute = 0;
  evoDeclinedLv = 0;
  evoDeclinedAge = 0;
  farDeclinedAge = 0;
  registerSpecies(speciesId);  // criado = registrado en la pokedex
  checkMedals();     // por si nace ya en forma final (legendario)
  checkLevelUpMoves();  // Startattacke(n) auf Level 1 sofort lernen
  applyPendingStepRewards();
  sfxPlay(SFX_HATCH);
  save();
}

// Los niveles y condiciones de evolucion viven en EVOLUTION_RULES para poder
// representar ramas y evoluciones de amistad/RTC sin romper partidas antiguas.
static bool evolutionRuleReady(const Pet &pet, const EvolutionRule &rule) {
  uint16_t need = (uint16_t)rule.minLevel + pet.careMistakes;
  if (need > 100 || pet.level() < (uint8_t)need) return false;
  switch (rule.condition) {
    case EVO_BOND:
      return pet.bond >= 50;
    case EVO_DAY_BOND: {
      int hour = sceneHourFromEpoch(pet.lastSeenEpoch);
      return pet.bond >= 50 && hour >= 6 && hour < 20;
    }
    case EVO_NIGHT_BOND: {
      int hour = sceneHourFromEpoch(pet.lastSeenEpoch);
      return pet.bond >= 50 && (hour < 6 || hour >= 20);
    }
    case EVO_ATK_GT_DEF:
      return pet.atkStat() > pet.defStat();
    case EVO_DEF_GT_ATK:
      return pet.defStat() > pet.atkStat();
    case EVO_ATK_EQ_DEF:
      return pet.atkStat() == pet.defStat();
    default:
      return true;
  }
}

uint8_t Pet::evolutionOptionCount() const {
  if (isEgg() || ceremony != CER_NONE || speciesId < 1 || speciesId > DEX_COUNT) return 0;
  uint8_t count = 0;
  int16_t seen[8] = { 0 };
  for (uint16_t i = 0; i < EVOLUTION_RULE_COUNT; i++) {
    const EvolutionRule &rule = EVOLUTION_RULES[i];
    if (rule.from != speciesId || !evolutionRuleReady(*this, rule)) continue;
    bool duplicate = false;
    for (uint8_t j = 0; j < count; j++) if (seen[j] == rule.to) duplicate = true;
    if (!duplicate && count < sizeof(seen) / sizeof(seen[0])) seen[count++] = rule.to;
  }
  return count;
}

int16_t Pet::evolutionOption(uint8_t index) const {
  if (isEgg() || ceremony != CER_NONE || speciesId < 1 || speciesId > DEX_COUNT) return -1;
  uint8_t count = 0;
  int16_t seen[8] = { 0 };
  for (uint16_t i = 0; i < EVOLUTION_RULE_COUNT; i++) {
    const EvolutionRule &rule = EVOLUTION_RULES[i];
    if (rule.from != speciesId || !evolutionRuleReady(*this, rule)) continue;
    bool duplicate = false;
    for (uint8_t j = 0; j < count; j++) if (seen[j] == rule.to) duplicate = true;
    if (duplicate || count >= sizeof(seen) / sizeof(seen[0])) continue;
    seen[count] = rule.to;
    if (count == index) return rule.to;
    count++;
  }
  return -1;
}

uint8_t Pet::evolutionRequiredLevel() const {
  if (isEgg() || speciesId < 1 || speciesId > DEX_COUNT) return 100;
  uint16_t best = 100;
  for (uint16_t i = 0; i < EVOLUTION_RULE_COUNT; i++) {
    const EvolutionRule &rule = EVOLUTION_RULES[i];
    if (rule.from != speciesId) continue;
    uint16_t need = (uint16_t)rule.minLevel + careMistakes;
    if (need < best) best = need;
  }
  return best > 100 ? 100 : (uint8_t)best;
}

bool Pet::evolutionUnlocked() const {
  return evolutionOptionCount() > 0;
}

bool Pet::canEvolveTo(int16_t target) const {
  if (!canEvolveNow() || target < 1 || target > DEX_COUNT) return false;
  for (uint8_t i = 0; i < evolutionOptionCount(); i++)
    if (evolutionOption(i) == target) return true;
  return false;
}

bool Pet::canEvolveNow() const {
  return evolutionUnlocked() && !sleeping && healthyStatCount() >= 3;
}

bool Pet::wantEvolveButton() const {
  if (sleeping || !evolutionUnlocked()) return false;
  if (level() >= 100) return ageMinutes >= evoDeclinedAge;
  return level() > evoDeclinedLv;
}

void Pet::declineEvolve() {
  if (level() >= 100) {
    evoDeclinedLv = 100;
    evoDeclinedAge = ageMinutes + 1440;
  } else {
    evoDeclinedLv = level();
  }
  save();
}

void Pet::evolveTo(int16_t target) {
  if (!canEvolveTo(target)) return;
  prevSpeciesId = speciesId;
  speciesId = target;
  registerSpecies(speciesId);
  sfxPlay(SFX_EVOLVE);
  evolveUntil = millis() + EVOLVE_ANIM_MS;
  save();
}

void Pet::evolve() {
  if (!canEvolveNow()) return;
  int16_t next = -1;
  for (uint8_t i = 0; i < evolutionOptionCount(); i++) {
    int16_t candidate = evolutionOption(i);
    if (candidate >= 1 && !isRegistered(candidate)) { next = candidate; break; }
    if (next < 0) next = candidate;
  }
  if (next >= 1) evolveTo(next);
}

void Pet::feed() {
  feedBerry(0);
}

void Pet::feedBerry(uint8_t color) {
  if (ceremony != CER_NONE) return;
  if (isEgg() || sleeping) return;
  if (lovesBerry(color)) {
    fullness = clamp100(fullness + 35);
    joy = clamp100(joy + 10);
    heartUntil = millis() + HEART_MS;  // "le encanta!"
    berryKnown = true;                 // descubierto: se muestra en la ficha
    addBond(2);
  } else {
    fullness = clamp100(fullness + 25);
  }
  eatUntil = millis() + EAT_ANIM_MS;
  registerCare();
  noteDailyGoal(DAILY_GOAL_CARE, 1);
  save();
}

void Pet::feedCandy() {
  if (ceremony != CER_NONE) return;
  if (isEgg() || sleeping) return;
  fullness = clamp100(fullness + 10);
  joy = clamp100(joy + 12);
  weight = clamp100(weight + 12);  // las chuches pasan factura
  eatUntil = millis() + EAT_ANIM_MS;
  registerCare();
  noteDailyGoal(DAILY_GOAL_CARE, 1);
  save();
}

void Pet::playResult(uint8_t score) {
  if (ceremony != CER_NONE || isEgg()) return;
  uint8_t v = trSpe + score / 5;  // jugar entrena la velocidad
  trSpe = v > 100 ? 100 : v;
  joy = clamp100(joy + 5 + (score > 15 ? 30 : score * 2));
  energy = dropTo(energy, 10 + score / 2, 5);
  fullness = dropTo(fullness, 5, 5);
  int burn = (int)weight - score * 2;  // el ejercicio quema peso
  weight = burn > 0 ? burn : 0;
  if (score >= 5) heartUntil = millis() + HEART_MS;
  if (score > gameHi) gameHi = score;  // nuevo record
  addBond(2);
  registerCare();
  noteDailyGoal(DAILY_GOAL_PLAY, 1);
  save();
}

uint8_t Pet::applyCatchResult(uint8_t score) {
  if (ceremony != CER_NONE || isEgg()) return 0;
  uint8_t gain = score / 3;
  if (gain > 12) gain = 12;
  uint8_t v = trSpe + gain;
  trSpe = v > 100 ? 100 : v;
  joy = clamp100(joy + 4 + (score > 12 ? 20 : score));
  energy = dropTo(energy, 8 + score / 3, 5);
  fullness = dropTo(fullness, 4, 5);
  int burn = (int)weight - score;
  weight = burn > 0 ? burn : 0;
  if (score >= 5) heartUntil = millis() + HEART_MS;
  if (score > catchHi) catchHi = score;
  addBond(1);
  registerCare();
  noteDailyGoal(DAILY_GOAL_CATCH, score);
  save();
  return gain;
}

uint8_t Pet::applyMemoResult(uint8_t rounds) {
  if (ceremony != CER_NONE || isEgg()) return 0;
  uint8_t gain = rounds / 2;
  if (gain > 10) gain = 10;
  uint8_t v = trDef + gain;
  trDef = v > 100 ? 100 : v;
  joy = clamp100(joy + 5 + (rounds > 8 ? 18 : rounds * 2));
  energy = dropTo(energy, 6 + rounds / 2, 5);
  fullness = dropTo(fullness, 3, 5);
  if (rounds >= 4) heartUntil = millis() + HEART_MS;
  if (rounds > memoHi) memoHi = rounds;
  addBond(2);
  registerCare();
  noteDailyGoal(DAILY_GOAL_MEMO, rounds);
  save();
  return gain;
}

uint8_t Pet::applyCleanResult(uint8_t score) {
  if (ceremony != CER_NONE || isEgg()) return 0;
  uint8_t gain = score / 2;
  if (gain > 18) gain = 18;
  hygiene = clamp100((int)hygiene + 20 + score * 3);
  joy = clamp100((int)joy + 3 + (score > 10 ? 12 : score));
  energy = dropTo(energy, 4 + score / 4, 8);
  if (poops && score >= 4) poops--;
  if (score >= 6) heartUntil = millis() + HEART_MS;
  if (score > cleanHi) cleanHi = score;
  addBond(1);
  registerCare();
  noteDailyGoal(DAILY_GOAL_CARE, 1);
  save();
  return gain;
}

uint8_t Pet::applyTypeResult(uint8_t score) {
  if (ceremony != CER_NONE || isEgg()) return 0;
  uint8_t gain = score / 4;
  if (gain > 10) gain = 10;
  uint8_t v = trAtk + gain;
  trAtk = v > 100 ? 100 : v;
  joy = clamp100((int)joy + 4 + (score > 12 ? 18 : score));
  energy = dropTo(energy, 5 + score / 3, 8);
  fullness = dropTo(fullness, 2, 5);
  if (score >= 5) heartUntil = millis() + HEART_MS;
  if (score > typeHi) typeHi = score;
  addBond(1);
  registerCare();
  noteDailyGoal(DAILY_GOAL_PLAY, 1);
  save();
  return gain;
}

bool Pet::applyPetEvent(uint8_t eventType) {
  if (ceremony != CER_NONE || isEgg()) return false;
  if (eventType == PET_EVENT_BERRY) {
    fullness = clamp100((int)fullness + 10);
    joy = clamp100((int)joy + 4);
  } else if (eventType == PET_EVENT_HEART) {
    joy = clamp100((int)joy + 6);
    addBond(1);
  } else if (eventType == PET_EVENT_SPARKLE) {
    joy = clamp100((int)joy + 5);
    if (energy <= hygiene) energy = clamp100((int)energy + 3);
    else hygiene = clamp100((int)hygiene + 3);
  } else {
    return false;
  }
  heartUntil = millis() + HEART_MS;
  registerCare();
  noteDailyGoal(DAILY_GOAL_CARE, 1);
  save();
  return true;
}

uint8_t Pet::interactPet(bool eveningBonus) {
  if (ceremony != CER_NONE || isEgg() || sleeping) return PET_INTERACT_NONE;
  uint32_t nowMinute = ageMinutes ? ageMinutes : 1;
  if (lastPetInteractMinute && nowMinute < lastPetInteractMinute + 10) {
    return PET_INTERACT_NONE;
  }
  lastPetInteractMinute = nowMinute;
  uint8_t result = PET_INTERACT_JOY;
  PetPersonality p = personality();
  int joyGain = (p == PERS_PLAYFUL) ? 4 : 2;
  joy = clamp100((int)joy + joyGain);
  if (p == PERS_LAZY) {
    energy = clamp100((int)energy + 2);
    result |= PET_INTERACT_ENERGY;
  }
  bool bondGain = eveningBonus || p == PERS_CALM || (p == PERS_BRAVE && battleWins > 0);
  if (bondGain) {
    uint8_t before = bond;
    addBond(1);
    if (bond > before) result |= PET_INTERACT_BOND;
  }
  heartUntil = millis() + HEART_MS;
  registerCare();
  noteDailyGoal(DAILY_GOAL_CARE, 1);
  save();
  return result;
}

bool Pet::applyShake() {
  if (ceremony != CER_NONE || isEgg() || sleeping) return false;
  uint32_t now = millis();
  if (deadlineActive(now, shakeReadyAt)) return false;
  uint32_t d = today();
  if (d != shakeDay) {
    shakeDay = d;
    shakeCountToday = 0;
  }
  if (d && shakeCountToday >= 8) return false;
  shakeReadyAt = now + 25000UL;
  if (d) shakeCountToday++;
  joy = clamp100((int)joy + 3);
  heartUntil = now + HEART_MS;
  pendingSave = true;
  return true;
}

void Pet::ensureStepDay() {
  uint32_t d = today();
  if (!d) return;
  if (!stepDay) {
    stepDay = d;
    pendingSave = true;
  } else if (stepDay != d) {
    stepDay = d;
    stepsToday = 0;
    stepDailyRewardMask = 0;
    pendingSave = true;
  }
}

uint32_t Pet::stepGoal(uint8_t index) const {
  return index < STEP_DAILY_GOAL_COUNT ? STEP_DAILY_GOALS[index] : 0;
}

bool Pet::stepGoalComplete(uint8_t index) const {
  uint8_t bit = stepRewardBit(index);
  return bit != 0 && (stepDailyRewardMask & bit) != 0;
}

uint8_t Pet::stepTrailRank() const {
  uint8_t rank = 0;
  for (uint8_t i = 0; i < sizeof(STEP_TOTAL_GOALS) / sizeof(STEP_TOTAL_GOALS[0]); i++) {
    if (stepsTotal >= STEP_TOTAL_GOALS[i]) rank = i + 1;
  }
  return rank;
}

uint16_t Pet::stepShinyChancePer4096() const {
  // Wilde Shinies bleiben selten: 8/4096 = 1/512 ohne Bewegung, bei
  // 5.000 Tagesschritten steigt der Tagesanteil auf maximal 32/4096.
  uint16_t units = 8;
  uint32_t daily = stepsToday / 210UL;
  if (daily > 24) daily = 24;
  units += (uint16_t)daily;
  // Gesamtmeilensteine geben einen kleinen, dauerhaften Trail-Vorteil.
  if (stepsTotal >= STEP_TOTAL_GOALS[0]) units += 1;
  if (stepsTotal >= STEP_TOTAL_GOALS[1]) units += 2;
  if (stepsTotal >= STEP_TOTAL_GOALS[2]) units += 2;
  if (stepsTotal >= STEP_TOTAL_GOALS[3]) units += 3;
  return units > 40 ? 40 : units;
}

uint8_t Pet::stepCatchBonus() const {
  uint32_t daily = stepsToday / 1000UL;
  if (daily > 5) daily = 5;
  uint8_t bonus = (uint8_t)daily;
  if (stepsTotal >= STEP_TOTAL_GOALS[2]) bonus++;
  if (stepsTotal >= STEP_TOTAL_GOALS[3]) bonus++;
  return bonus;
}

void Pet::recordStepReward(uint8_t index) {
  uint8_t bit = stepRewardBit(index);
  if (!bit || (stepDailyRewardMask & bit)) return;
  stepDailyRewardMask |= bit;

  ExpeditionItem item = stepRewardItemFor(index);
  if (item < EXP_ITEM_COUNT && canReceiveExpeditionItem(item)) {
    itemCounts[item]++;
  } else if (isEgg() || ceremony != CER_NONE) {
    // Ein Ei hat noch keine Stats. Die Belohnung wird nach dem Schlüpfen
    // erneut versucht, damit volle Taschen keinen Fortschritt verschlucken.
    pendingStepRewardMask |= bit;
  } else if (index == 0) {
    joy = clamp100((int)joy + 5);
  } else if (index == 1) {
    energy = clamp100((int)energy + 10);
  } else {
    uint8_t *stat = &trAtk;
    if (trDef < *stat && trDef <= trSpe) stat = &trDef;
    else if (trSpe < *stat && trSpe < trDef) stat = &trSpe;
    if (*stat < 100) *stat = clamp100((int)*stat + 2);
    else joy = clamp100((int)joy + 8);
  }

  lastStepRewardEvent = (uint8_t)(STEP_REWARD_SNACK + index);
  stepRewardUntil = millis() + 4500UL;
  pendingSave = true;
}

void Pet::applyPendingStepRewards() {
  if (!pendingStepRewardMask || isEgg() || ceremony != CER_NONE) return;
  uint8_t pending = pendingStepRewardMask;
  pendingStepRewardMask = 0;
  for (uint8_t i = 0; i < STEP_DAILY_GOAL_COUNT; i++) {
    uint8_t bit = stepRewardBit(i);
    if (!(pending & bit)) continue;
    ExpeditionItem item = stepRewardItemFor(i);
    if (item < EXP_ITEM_COUNT && canReceiveExpeditionItem(item)) itemCounts[item]++;
    else if (i == 0) joy = clamp100((int)joy + 5);
    else if (i == 1) energy = clamp100((int)energy + 10);
    else if (trAtk < 100) trAtk = clamp100((int)trAtk + 2);
    else if (trDef < 100) trDef = clamp100((int)trDef + 2);
    else if (trSpe < 100) trSpe = clamp100((int)trSpe + 2);
    else joy = clamp100((int)joy + 8);
    lastStepRewardEvent = (uint8_t)(STEP_REWARD_SNACK + i);
    stepRewardUntil = millis() + 4500UL;
  }
  pendingSave = true;
}

uint8_t Pet::applyWalk(uint16_t steps) {
  if (!steps) return 0;
  ensureStepDay();

  uint32_t oldToday = stepsToday;
  uint32_t oldTotal = stepsTotal;
  uint32_t maxU32 = 0xFFFFFFFFUL;
  stepsToday = (stepsToday > maxU32 - steps) ? maxU32 : stepsToday + steps;
  stepsTotal = (stepsTotal > maxU32 - steps) ? maxU32 : stepsTotal + steps;

  for (uint8_t i = 0; i < STEP_DAILY_GOAL_COUNT; i++)
    if (stepsToday >= STEP_DAILY_GOALS[i]) recordStepReward(i);
  for (uint8_t i = 0; i < sizeof(STEP_TOTAL_GOALS) / sizeof(STEP_TOTAL_GOALS[0]); i++) {
    uint8_t bit = (uint8_t)(1U << i);
    if (stepsTotal >= STEP_TOTAL_GOALS[i] && !(stepMilestoneMask & bit)) {
      stepMilestoneMask |= bit;
      lastStepRewardEvent = STEP_REWARD_TRAIL_RANK;
      stepRewardUntil = millis() + 4500UL;
      pendingSave = true;
    }
  }
  if ((oldToday / 100UL) != (stepsToday / 100UL) ||
      (oldTotal / 100UL) != (stepsTotal / 100UL)) pendingSave = true;

  // Schritte werden auch beim Ei und in der Zeremonie gezaehlt; nur die
  // bisherigen JOY/BOND-Wirkungen brauchen ein aktives Pokemon.
  if (ceremony != CER_NONE || isEgg()) return 0;

  uint32_t d = today();
  uint32_t hour = unixHourFromEpoch(lastSeenEpoch);
  if (d != walkDay) {
    walkDay = d;
    walkJoyToday = 0;
    walkBondToday = 0;
    walkJoyHour = 0;
    walkHour = hour;
    walkJoyBank = 0;
    walkBondBank = 0;
  } else if (hour != walkHour) {
    walkHour = hour;
    walkJoyHour = 0;
  }

  uint32_t joyBank = (uint32_t)walkJoyBank + steps;
  uint32_t bondBank = (uint32_t)walkBondBank + steps;
  uint8_t gained = 0;
  while (joyBank >= 40 && walkJoyToday < 18 && walkJoyHour < 6) {
    joyBank -= 40;
    joy = clamp100((int)joy + 1);
    walkJoyToday++;
    walkJoyHour++;
    gained++;
  }
  if (joyBank > 400) joyBank = 400;
  walkJoyBank = (uint16_t)joyBank;

  while (bondBank >= 150 && walkBondToday < 2) {
    uint8_t before = bond;
    addBond(1);
    if (bond == before) break;
    bondBank -= 150;
    walkBondToday++;
  }
  if (bondBank > 400) bondBank = 400;
  walkBondBank = (uint16_t)bondBank;

  if (gained || walkBondToday) pendingSave = true;
  return gained;
}

bool Pet::takeMorningGreeting() {
  uint32_t d = today();
  if (d == 0 || d == lastMorningDay) return false;
  if (dayPhaseFromEpoch(lastSeenEpoch) != 0) return false;
  lastMorningDay = d;
  if (!isEgg() && !sleeping && ceremony == CER_NONE) {
    joy = clamp100((int)joy + 2);
    heartUntil = millis() + HEART_MS;
  }
  save();
  return true;
}

PetPersonality Pet::personality() const {
  if (isEgg()) return PERS_BALANCED;
  if (weight >= 72 || energy <= 20) return PERS_LAZY;
  if (battleWins >= 8 || bestBattleStreak >= 4) return PERS_BRAVE;
  if (catchHi >= 18 || memoHi >= 8 || gameHi >= 24 || trSpe >= 55) return PERS_PLAYFUL;
  if ((bond >= 45 && careMistakes <= 1) || (streak >= 5 && careMistakes == 0)) return PERS_CALM;
  return PERS_BALANCED;
}

// saco de entrenamiento: los golpes entrenan la fuerza. Devuelve la subida.
uint8_t Pet::trainStrength(uint16_t hits) {
  if (ceremony != CER_NONE || isEgg()) return 0;
  uint8_t gain = hits / 4;          // ~4 golpes = 1 punto de entrenamiento
  if (gain > 18) gain = 18;         // tope por sesion: la FUE se forja a fuego lento
  uint8_t v = trAtk + gain;
  trAtk = v > 100 ? 100 : v;
  energy = dropTo(energy, 12, 5);   // cansa
  fullness = dropTo(fullness, 5, 5);
  int burn = (int)weight - hits / 3;  // tambien quema peso
  weight = burn > 0 ? burn : 0;
  joy = clamp100(joy + 6);
  if (hits >= 20) heartUntil = millis() + HEART_MS;
  if (hits > strHi) strHi = hits;   // record de golpes
  addBond(2);
  registerCare();
  save();
  return gain;
}

BattleReward Pet::applyBattleWin(int16_t wildDex, bool closeWin) {
  BattleReward reward;
  if (ceremony != CER_NONE || isEgg()) return reward;
  if (wildDex < 1 || wildDex > DEX_COUNT) wildDex = 1;
  const DexEntry &wild = DEX_TBL[wildDex];
  reward.amount = (wild.rarity == R_RARO) ? 2 : 1;
  if (closeWin) reward.amount++;
  if (wild.bAtk >= wild.bDef && wild.bAtk >= wild.bSpe) {
    reward.stat = BATTLE_REWARD_DEF;
    trDef = clamp100((int)trDef + reward.amount);
  } else if (wild.bDef >= wild.bAtk && wild.bDef >= wild.bSpe) {
    reward.stat = BATTLE_REWARD_ATK;
    trAtk = clamp100((int)trAtk + reward.amount);
  } else {
    reward.stat = BATTLE_REWARD_SPE;
    trSpe = clamp100((int)trSpe + reward.amount);
  }
  battleWins++;
  battleStreak++;
  if (battleStreak > bestBattleStreak) bestBattleStreak = battleStreak;
  joy = clamp100((int)joy + 8 + (closeWin ? 4 : 0));
  energy = dropTo(energy, 8, 20);
  fullness = dropTo(fullness, 3, 10);
  addBond(closeWin ? 3 : 2);
  // "Erfahrung": das Level ist normalerweise reine Spielzeit (siehe level()),
  // ein Sieg schenkt zusaetzliche Alters-Minuten und beschleunigt so das
  // naechste Level-Up spuerbar, ohne ein komplett eigenes XP-System noetig
  // zu machen. Jeder Sieg zaehlt gleich (kein Bonus fuer knappen Sieg), nur
  // die Seltenheit des Gegners entscheidet -- 1 bis 10 Minuten.
  ageMinutes += (wild.rarity == R_LEGENDARIO) ? 10 : (wild.rarity == R_RARO) ? 6 : 2;
  registerCare();
  noteDailyGoal(DAILY_GOAL_BATTLE, 1);
  save();
  return reward;
}

void Pet::applyBattleLoss() {
  if (ceremony != CER_NONE || isEgg()) return;
  battleLosses++;
  battleStreak = 0;
  joy = dropTo(joy, 12, 20);
  energy = dropTo(energy, 18, 20);
  fullness = dropTo(fullness, 4, 10);
  save();
}

uint8_t Pet::expeditionEnergyCost(uint8_t minutes) {
  if (minutes == 15) return 12;
  if (minutes == 30) return 20;
  if (minutes == 60) return 32;
  return 0xFF;
}

bool Pet::expeditionActive(uint32_t nowEpoch) const {
  return expeditionEndEpoch != 0 && nowEpoch < expeditionEndEpoch;
}

bool Pet::expeditionReady(uint32_t nowEpoch) const {
  return expeditionEndEpoch != 0 && nowEpoch >= expeditionEndEpoch;
}

uint8_t Pet::expeditionItemCount() const {
  uint8_t total = 0;
  for (uint8_t i = 0; i < EXP_ITEM_COUNT; i++) total += itemCounts[i];
  return total;
}

ExpeditionHudState Pet::expeditionHudState(uint32_t nowEpoch) const {
  if (isEgg() || ceremony != CER_NONE) return EXP_HUD_HIDDEN;
  if (expeditionReady(nowEpoch)) return EXP_HUD_READY;
  if (expeditionActive(nowEpoch)) return EXP_HUD_ACTIVE;
  return expeditionItemCount() ? EXP_HUD_BAG : EXP_HUD_HIDDEN;
}

bool Pet::canReceiveExpeditionItem(ExpeditionItem item) const {
  if (item >= EXP_ITEM_COUNT || itemCounts[item] >= EXP_ITEM_MAX) return false;
  return item != EXP_ITEM_TRAIN || trAtk < 100 || trDef < 100 || trSpe < 100;
}

bool Pet::expeditionInventoryFull() const {
  for (uint8_t i = 0; i < EXP_ITEM_COUNT; i++) {
    if (canReceiveExpeditionItem((ExpeditionItem)i)) return false;
  }
  return true;
}

bool Pet::canStartExpedition(uint8_t minutes, uint32_t nowEpoch) const {
  uint8_t cost = expeditionEnergyCost(minutes);
  return cost != 0xFF && nowEpoch != 0 && !isEgg() && !sleeping && ceremony == CER_NONE &&
         expeditionEndEpoch == 0 && energy >= cost && !expeditionInventoryFull();
}

uint8_t Pet::expeditionTrainingChance(uint8_t minutes) const {
  uint8_t base = minutes == 15 ? 8 : minutes == 30 ? 15 : minutes == 60 ? 25 : 0;
  if (!base) return 0;
  uint16_t avg = ((uint16_t)fullness + joy + energy + hygiene) / 4;
  if (avg >= 80 && bond >= 50) return minutes == 15 ? 18 : minutes == 30 ? 30 : 45;
  if (avg >= 60 && bond >= 20) return minutes == 15 ? 13 : minutes == 30 ? 22 : 35;
  return base;
}

bool Pet::startExpedition(uint8_t minutes, uint32_t nowEpoch, uint8_t luckRoll, uint8_t itemRoll) {
  if (!canStartExpedition(minutes, nowEpoch)) return false;

  uint8_t cost = expeditionEnergyCost(minutes);
  ExpeditionItem reward = EXP_ITEM_NONE;
  if (canReceiveExpeditionItem(EXP_ITEM_TRAIN) && luckRoll < expeditionTrainingChance(minutes)) {
    reward = EXP_ITEM_TRAIN;
  } else {
    const ExpeditionItem common[] = { EXP_ITEM_SNACK, EXP_ITEM_ENERGY, EXP_ITEM_CARE };
    uint8_t first = itemRoll % 3;
    for (uint8_t i = 0; i < 3; i++) {
      ExpeditionItem candidate = common[(first + i) % 3];
      if (canReceiveExpeditionItem(candidate)) {
        reward = candidate;
        break;
      }
    }
  }
  if (reward == EXP_ITEM_NONE) return false;

  energy -= cost;
  expeditionEndEpoch = nowEpoch + (uint32_t)minutes * 60UL;
  expeditionRewardItem = reward;
  save();
  return true;
}

ExpeditionItem Pet::claimExpedition(uint32_t nowEpoch) {
  if (!expeditionReady(nowEpoch)) return EXP_ITEM_NONE;
  ExpeditionItem reward = (ExpeditionItem)expeditionRewardItem;
  // Die urspruenglich zugeteilte Belohnung kann seit dem Start der Expedition
  // unerreichbar geworden sein (z.B. Training inzwischen bei 100/100/100, oder
  // das Fach bereits voll) -- ohne Ausweichoption bliebe die Expedition sonst
  // fuer immer unabholbar (Fehlerton, HUD zeigt weiter "bereit"). Dann wird,
  // genau wie beim Start, auf ein anderes verfuegbares Item ausgewichen.
  if (reward >= EXP_ITEM_COUNT || !canReceiveExpeditionItem(reward)) {
    reward = EXP_ITEM_NONE;
    for (uint8_t i = 0; i < EXP_ITEM_COUNT; i++) {
      if (canReceiveExpeditionItem((ExpeditionItem)i)) { reward = (ExpeditionItem)i; break; }
    }
  }
  if (reward == EXP_ITEM_NONE) return EXP_ITEM_NONE;
  itemCounts[reward]++;
  expeditionEndEpoch = 0;
  expeditionRewardItem = EXP_ITEM_NONE;
  save();
  return reward;
}

bool Pet::useExpeditionItem(ExpeditionItem item, int8_t trainingStat) {
  if (item >= EXP_ITEM_COUNT || itemCounts[item] == 0) return false;
  if (item == EXP_ITEM_SNACK) {
    fullness = clamp100((int)fullness + 25);
    joy = clamp100((int)joy + 5);
  } else if (item == EXP_ITEM_ENERGY) {
    energy = clamp100((int)energy + 30);
  } else if (item == EXP_ITEM_CARE) {
    hygiene = clamp100((int)hygiene + 30);
    if (poops > 0) poops--;
  } else {
    if (trainingStat == TRAIN_STAT_ATK && trAtk < 100) trAtk = clamp100((int)trAtk + 2);
    else if (trainingStat == TRAIN_STAT_DEF && trDef < 100) trDef = clamp100((int)trDef + 2);
    else if (trainingStat == TRAIN_STAT_SPE && trSpe < 100) trSpe = clamp100((int)trSpe + 2);
    else return false;
  }
  itemCounts[item]--;
  save();
  return true;
}

void Pet::play() {
  if (ceremony != CER_NONE) return;
  if (isEgg() || sleeping) return;
  joy = clamp100(joy + 25);
  energy = clamp100(energy - 10);
  fullness = clamp100(fullness - 5);
  heartUntil = millis() + HEART_MS;
  addBond(2);
  registerCare();
  save();
}

void Pet::toggleLight() {
  if (ceremony != CER_NONE) return;
  if (isEgg()) return;
  sleeping = !sleeping;
  save();
}

void Pet::clean() {
  if (ceremony != CER_NONE) return;
  poops = 0;
  hygiene = 100;
  addBond(1);
  registerCare();
  noteDailyGoal(DAILY_GOAL_CARE, 1);
  save();
}

void Pet::caress() {
  if (ceremony != CER_NONE) return;
  if (isEgg() || sleeping) return;
  joy = clamp100(joy + 5);
  heartUntil = millis() + HEART_MS;
  addBond(1);
  registerCare();
  noteDailyGoal(DAILY_GOAL_CARE, 1);
  save();
}

bool Pet::eggTap() {
  if (!isEgg()) return false;
  if (++eggTaps >= 3) { hatch(); return true; }
  save();
  return false;
}

PetMood Pet::mood() const {
  if (sleeping) return MOOD_SLEEPING;
  if (eating()) return MOOD_EATING;
  if (lowestStat() < 25) return MOOD_SAD;
  return MOOD_HAPPY;
}

void Pet::saveDexMons() {
  // putBytes() gibt die Anzahl geschriebener Bytes zurueck (0 bei Fehler,
  // z.B. wenn die dexnvs-Partition voll ist). Frueher wurde das nicht
  // geprueft -- ein voller Speicher liess frisch gefangene/gezuechtete Werte
  // beim naechsten Neustart wieder verschwinden, ohne jede Fehlermeldung
  // (siehe partitions.csv-Kommentar zu dexnvs).
  size_t wCgt = dexPrefs.putBytes("dmcgt", dexMonsCaught, sizeof(dexMonsCaught));
  size_t wBrd = dexPrefs.putBytes("dmbrd", dexMonsBred, sizeof(dexMonsBred));
  if (wCgt != sizeof(dexMonsCaught) || wBrd != sizeof(dexMonsBred)) {
    Serial.printf("FEHLER saveDexMons: dmcgt %u/%u Byte, dmbrd %u/%u Byte geschrieben "
                  "(dexnvs-Partition eventuell voll)\n",
                  (unsigned)wCgt, (unsigned)sizeof(dexMonsCaught),
                  (unsigned)wBrd, (unsigned)sizeof(dexMonsBred));
  }
}

// Laeuft genau einmal (Marker in "prefs"): Spielstaende von vor dem Fang/
// Zucht-Wertesystem haben fuer laengst registrierte/gefangene Arten keine
// DexMon-Werte, weil die nur beim Fangen/Schluepfen geschrieben werden (siehe
// offerDexMon() in TamaPoke.ino). Ohne Nachtrag blieben Pokedex-Seite 3 und
// "INS TEAM" fuer den kompletten Altbestand fuer immer leer.
void Pet::backfillDexMonHistory() {
  if (prefs.getBool("dexmig1", false)) return;
  prefs.putBool("dexmig1", true);
  bool changed = false;
  // Aktuelles Haustier: echte gelebte Werte statt Zufallswuerfel, es ist ja da.
  if (speciesId >= 1 && speciesId <= DEX_COUNT && isRegistered(speciesId) &&
      dexMonsBred[speciesId].empty()) {
    DexMon &m = dexMonsBred[speciesId];
    m.level = level();
    m.geneAtk = geneAtk; m.geneDef = geneDef; m.geneSpe = geneSpe;
    m.geneSpA = geneSpA; m.geneSpD = geneSpD;
    m.trAtk = trAtk; m.trDef = trDef; m.trSpe = trSpe;
    m.trSpA = trSpA; m.trSpD = trSpD;
    m.shiny = shiny ? 1 : 0;
    changed = true;
  }
  // Alle anderen laengst bekannten Arten: keine historischen Werte vorhanden,
  // also wie bei einem frischen Fang gewuerfelt, auf Hoehe des aktuellen Levels.
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    if (d == speciesId) continue;
    if (isRegistered(d) && dexMonsBred[d].empty()) {
      dexMonsBred[d] = rollFreshDexMon(d, level(), isShinyRegistered(d));
      changed = true;
    }
    if (isCaught(d) && dexMonsCaught[d].empty()) {
      dexMonsCaught[d] = rollFreshDexMon(d, level(), isShinyRegistered(d));
      changed = true;
    }
  }
  if (changed) saveDexMons();
}

// Laeuft genau einmal (eigener Marker "dexmig2"): Zwischen der Erweiterung
// des Dex auf 1025 Arten und dem Groessenmigrations-Fix in load() konnte ein
// normaler Autosave dexMonsCaught/dexMonsBred bereits leer in der NEUEN
// (groesseren) Groesse persistieren -- dann greift die dortige Migration
// nicht mehr (die gespeicherte Groesse passt ja schon), und die echten
// historischen Werte sind verloren. Fuellt genau wie backfillDexMonHistory()
// alle Luecken bei laengst bekannten Arten mit frisch gewuerfelten Werten auf
// Hoehe des aktuellen Levels, statt sie fuer immer leer zu lassen.
void Pet::backfillDexMonHistoryAfterDexExpand() {
  if (prefs.getBool("dexmig2", false)) return;
  prefs.putBool("dexmig2", true);
  bool changed = false;
  if (speciesId >= 1 && speciesId <= DEX_COUNT && isRegistered(speciesId) &&
      dexMonsBred[speciesId].empty()) {
    DexMon &m = dexMonsBred[speciesId];
    m.level = level();
    m.geneAtk = geneAtk; m.geneDef = geneDef; m.geneSpe = geneSpe;
    m.geneSpA = geneSpA; m.geneSpD = geneSpD;
    m.trAtk = trAtk; m.trDef = trDef; m.trSpe = trSpe;
    m.trSpA = trSpA; m.trSpD = trSpD;
    m.shiny = shiny ? 1 : 0;
    changed = true;
  }
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    if (d == speciesId) continue;
    if (isRegistered(d) && dexMonsBred[d].empty()) {
      dexMonsBred[d] = rollFreshDexMon(d, level(), isShinyRegistered(d));
      changed = true;
    }
    if (isCaught(d) && dexMonsCaught[d].empty()) {
      dexMonsCaught[d] = rollFreshDexMon(d, level(), isShinyRegistered(d));
      changed = true;
    }
  }
  if (changed) saveDexMons();
}

// Laeuft genau einmal (eigener Marker "dexmig4"): Als DexMon um ein eigenes
// moves[]-Feld erweitert wurde (siehe rollFreshDexMon()), aenderte sich
// sizeof(DexMon) ein weiteres Mal. Zwischen dieser Aenderung und dem dafuer
// noetigen Lade-Fix in load() konnte wieder ein Autosave dexMonsCaught/
// dexMonsBred leer in der (schon wieder neuen) Groesse persistieren -- exakt
// dasselbe Muster wie bei "dexmig2", nur mit einem eigenen Marker, weil
// "dexmig2" schon verbraucht ist und sonst nicht nochmal liefe. Gleiches
// Vorgehen: Luecken bei laengst bekannten Arten aus dem aktuellen Level
// nachwuerfeln, statt sie fuer immer leer zu lassen.
void Pet::backfillDexMonHistoryAfterMovesField() {
  if (prefs.getBool("dexmig4", false)) return;
  bool changed = false;
  if (speciesId >= 1 && speciesId <= DEX_COUNT && isRegistered(speciesId) &&
      dexMonsBred[speciesId].empty()) {
    DexMon &m = dexMonsBred[speciesId];
    m.level = level();
    m.geneAtk = geneAtk; m.geneDef = geneDef; m.geneSpe = geneSpe;
    m.geneSpA = geneSpA; m.geneSpD = geneSpD;
    m.trAtk = trAtk; m.trDef = trDef; m.trSpe = trSpe;
    m.trSpA = trSpA; m.trSpD = trSpD;
    m.shiny = shiny ? 1 : 0;
    changed = true;
  }
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    if (d == speciesId) continue;
    if (isRegistered(d) && dexMonsBred[d].empty()) {
      dexMonsBred[d] = rollFreshDexMon(d, level(), isShinyRegistered(d));
      changed = true;
    }
    if (isCaught(d) && dexMonsCaught[d].empty()) {
      dexMonsCaught[d] = rollFreshDexMon(d, level(), isShinyRegistered(d));
      changed = true;
    }
  }
  // Flag erst NACH der Schleife setzen (Absturzsicherheit: bricht die Schleife
  // vorzeitig ab, wird beim naechsten Boot einfach nochmal versucht statt
  // faelschlich als "erledigt" zu gelten).
  prefs.putBool("dexmig4", true);
  if (changed) saveDexMons();
}

void Pet::backfillSentAwayFlags() {
  if (prefs.getBool("dexmig5", false)) return;
  bool changed = false;
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    if (d == speciesId) continue;  // das aktuell aktive Exemplar: noch offen, siehe canBeSentAway()
    if (!dexMonsBred[d].empty() && !dexMonsBred[d].sentAway) {
      dexMonsBred[d].sentAway = true;
      changed = true;
    }
  }
  prefs.putBool("dexmig5", true);
  if (changed) saveDexMons();
}

// Wird aus tick() aufgerufen (guenstig: nur ein Struct-Update im RAM), die
// Persistenz laeuft ueber saveDexMons() aus save() mit -- keine eigene
// Schreibkadenz, um die grossen dexMons-Arrays nicht zusaetzlich abzunutzen.
void Pet::syncOwnDexMon() {
  if (speciesId < 1 || speciesId > DEX_COUNT) return;
  // Legt den Eintrag bei Bedarf an (frisch geschluepfte Tiere bekommen sonst
  // NIE einen dexMonsBred-Eintrag: hatch() registriert die Art nur im
  // dexReg-Bitmap, siehe registerSpecies(), erzeugt aber keinen Werte-Eintrag).
  DexMon &m = dexMonsBred[speciesId];
  m.level = level();
  m.geneAtk = geneAtk; m.geneDef = geneDef; m.geneSpe = geneSpe;
  m.geneSpA = geneSpA; m.geneSpD = geneSpD;
  m.trAtk = trAtk; m.trDef = trDef; m.trSpe = trSpe;
  m.trSpA = trSpA; m.trSpD = trSpD;
  m.shiny = shiny ? 1 : 0;
  // Attacken mitsynchronisieren: sonst nutzt die Party/der Arenakampf fuer
  // das aktive Haustier ein veraltetes Moveset aus einem frueheren Schnapp-
  // schuss (z.B. von rollFreshDexMon() beim Schluepfen), statt der Attacken,
  // die man ihm seither ueber die Pokedex-Tausch-Funktion tatsaechlich
  // gegeben hat.
  for (int i = 0; i < MOVE_SLOTS; i++) m.moves[i] = moves[i];
  // Fuer das Pet-Wechsel-Feature (switchActiveTo()): so laesst sich dieses
  // Exemplar spaeter exakt am selben Punkt (Alter/Level, Bindung, Spitzname,
  // Medaillen, bereits geprueftes Lernlisten-Level) fortsetzen, statt bei
  // jedem Wechsel wieder bei 0 anzufangen. sentAway wird NICHT hier gesetzt
  // (nur beim tatsaechlichen Wegschicken, siehe newEgg()).
  m.ageMinutes = ageMinutes;
  m.bond = bond;
  m.medals = medals;
  m.moveLevelChecked = moveLevelChecked;
  snprintf(m.nick, sizeof(m.nick), "%s", nick);
}

void Pet::save() {
  ticksSinceSave = 0;
  pendingSave = false;
  prefs.putUChar("full", fullness);
  prefs.putUChar("joy", joy);
  prefs.putUChar("ene", energy);
  prefs.putUChar("hyg", hygiene);
  prefs.putUChar("poop", poops);
  prefs.putUChar("wgt", weight);
  prefs.putUChar("gatk", geneAtk);
  prefs.putUChar("gdef", geneDef);
  prefs.putUChar("gspe", geneSpe);
  prefs.putUChar("tatk", trAtk);
  prefs.putBytes("moves", moves, sizeof(moves));
  prefs.putUChar("mvlvl", moveLevelChecked);
  prefs.putUShort("plmv", pendingLearnMove);
  prefs.putUChar("pllv", pendingLearnLevel);
  prefs.putUChar("tdef", trDef);
  prefs.putUChar("tspe", trSpe);
  prefs.putBool("bk", berryKnown);
  prefs.putBool("shy", shiny);
  prefs.putBool("eshy", eggShiny);
  prefs.putBool("stpk", starterPick);
  prefs.putBytes("dexsh", dexShinyReg, sizeof(dexShinyReg));
  prefs.putUInt("age", ageMinutes);
  prefs.putShort("dexn", speciesId);
  prefs.putShort("eggT2", eggTarget);
  prefs.putUChar("crack", eggTaps);
  prefs.putUChar("mist", careMistakes);
  prefs.putBool("sleep", sleeping);
  prefs.putUChar("lend", lastEnd);
  // millis()-Deadlines sind nach einem Neustart ungueltig. Der Marker sorgt
  // dafuer, dass ein bereits begonnener Abschied beim Boot sauber endet.
  prefs.putUChar("cerp", ceremony);
  if (lastSeenEpoch) prefs.putUInt("seen", lastSeenEpoch);
  prefs.putBytes("dexreg", dexReg, sizeof(dexReg));
  prefs.putBytes("dexcgt", dexCaught, sizeof(dexCaught));
  prefs.putUShort("strk", streak);
  prefs.putUShort("bstrk", bestStreak);
  prefs.putUInt("cday", lastCareDay);
  prefs.putUChar("bond", bond);
  prefs.putUShort("medal", medals);
  prefs.putUShort("tmedal", totalMedals);
  prefs.putUShort("mstone", lastMilestone);
  prefs.putUShort("ghi", gameHi);
  prefs.putUShort("shi", strHi);
  prefs.putUShort("chi", catchHi);
  prefs.putUShort("mhi", memoHi);
  prefs.putUShort("clhi", cleanHi);
  prefs.putUShort("tyhi", typeHi);
  prefs.putUShort("bwin", battleWins);
  prefs.putUShort("bloss", battleLosses);
  prefs.putUShort("bstk", battleStreak);
  prefs.putUShort("bbstk", bestBattleStreak);
  prefs.putBytes("badges2", badges, sizeof(badges));
  prefs.putUChar("cfrm", collectionFrame);
  prefs.putUInt("pimin", lastPetInteractMinute);
  prefs.putUChar("dxrew", dexRewardMask);
  prefs.putUInt("dgday", dailyGoalDay);
  prefs.putBytes("dgtype", dailyGoalType, sizeof(dailyGoalType));
  prefs.putBytes("dgprog", dailyGoalProgress, sizeof(dailyGoalProgress));
  prefs.putUChar("dgdone", dailyGoalDone);
  prefs.putBytes("items", itemCounts, sizeof(itemCounts));
  prefs.putUInt("exend", expeditionEndEpoch);
  prefs.putUChar("exrwd", expeditionRewardItem);
  prefs.putUInt("stday", stepDay);
  prefs.putUInt("stoday", stepsToday);
  prefs.putUInt("stotal", stepsTotal);
  prefs.putUChar("stdone", stepDailyRewardMask);
  prefs.putUChar("stmil", stepMilestoneMask);
  prefs.putUChar("stpend", pendingStepRewardMask);
  prefs.putUInt("lmday", lastMorningDay);
  prefs.putUInt("shkday", shakeDay);
  prefs.putUChar("shkcnt", shakeCountToday);
  prefs.putUInt("wday", walkDay);
  prefs.putUInt("whour", walkHour);
  prefs.putUChar("wjoy", walkJoyToday);
  prefs.putUChar("wjhr", walkJoyHour);
  prefs.putUChar("wbond", walkBondToday);
  prefs.putUChar("edlv", evoDeclinedLv);
  prefs.putUInt("edage", evoDeclinedAge);
  prefs.putString("nick", nick);
  saveDexMons();  // haelt den eigenen DexMon-Eintrag persistent synchron, siehe syncOwnDexMon()
}

void Pet::load() {
  fullness = prefs.getUChar("full", 80);
  joy = prefs.getUChar("joy", 80);
  energy = prefs.getUChar("ene", 80);
  hygiene = prefs.getUChar("hyg", 100);
  poops = prefs.getUChar("poop", 0);
  weight = prefs.getUChar("wgt", 0);
  geneAtk = prefs.getUChar("gatk", 0);
  geneDef = prefs.getUChar("gdef", 0);
  geneSpe = prefs.getUChar("gspe", 0);
  if (geneAtk == 0) {  // mascota anterior a los genes: tirada unica ahora
    geneAtk = 90 + random(21);
    geneDef = 90 + random(21);
    geneSpe = 90 + random(21);
  }
  trAtk = prefs.getUChar("tatk", 0);
  // Migration: "moves" war 4 Byte gross (uint8_t x4), bevor die Attacken-ID auf
  // uint16_t erweitert wurde (mehr als 255 Attacken im System, siehe
  // moves_real.h). Ein blindes getBytes() mit der NEUEN Groesse wuerde die
  // alten 4 Rohbytes falsch als 2 uint16_t-Werte interpretieren (siehe die
  // dexMonsCaught-Lehre weiter oben) -- daher hier explizit nach Groesse
  // unterscheiden statt stillschweigend falsche/verlorene Werte zu riskieren.
  size_t gotMoves = prefs.getBytesLength("moves");
  if (gotMoves == sizeof(moves)) {
    prefs.getBytes("moves", moves, sizeof(moves));
  } else if (gotMoves == sizeof(uint8_t) * MOVE_SLOTS) {
    uint8_t oldMoves[MOVE_SLOTS] = { 0 };
    prefs.getBytes("moves", oldMoves, sizeof(oldMoves));
    for (int i = 0; i < MOVE_SLOTS; i++) moves[i] = oldMoves[i];
  }
  moveLevelChecked = prefs.getUChar("mvlvl", 0);
  pendingLearnMove = prefs.getUShort("plmv", 0);
  pendingLearnLevel = prefs.getUChar("pllv", 0);
  trDef = prefs.getUChar("tdef", 0);
  trSpe = prefs.getUChar("tspe", 0);
  berryKnown = prefs.getBool("bk", false);
  shiny = prefs.getBool("shy", false);
  eggShiny = prefs.getBool("eshy", false);
  starterPick = prefs.getBool("stpk", false);
  loadDexBitmap(prefs, "dexsh", dexShinyReg, sizeof(dexShinyReg));
  ageMinutes = prefs.getUInt("age", 0);
  if (prefs.isKey("dexn")) {
    speciesId = prefs.getShort("dexn", -1);
    eggTarget = prefs.getShort("eggT2", 4);
  } else {
    // migracion desde la version con indices de flash (0-8)
    static const uint8_t OLD2DEX[9] = { 4, 5, 6, 1, 2, 3, 7, 8, 9 };
    int8_t old = prefs.getChar("spec", -1);
    speciesId = (old >= 0 && old < 9) ? OLD2DEX[old] : -1;
    int8_t oldT = prefs.getChar("eggT", 0);
    eggTarget = (oldT >= 0 && oldT < 9) ? OLD2DEX[oldT] : 4;
  }
  eggTaps = prefs.getUChar("crack", 0);
  careMistakes = prefs.getUChar("mist", 0);
  sleeping = prefs.getBool("sleep", false);
  lastEnd = prefs.getUChar("lend", CER_NONE);
  uint8_t pendingCeremony = prefs.getUChar("cerp", CER_NONE);
  loadDexBitmap(prefs, "dexreg", dexReg, sizeof(dexReg));
  loadDexBitmap(prefs, "dexcgt", dexCaught, sizeof(dexCaught));
  // Migration: dexMonsCaught/dexMonsBred waren [650] gross, bevor der Dex auf
  // 1025 Arten erweitert wurde -- UND DexMon selbst war kleiner, bevor es ein
  // eigenes moves[]-Feld bekam (siehe rollFreshDexMon()). Beide Aenderungen
  // aendern sizeof(...) des gespeicherten Blobs; eine exakte Groessenpruefung
  // ohne Fallback wuerde jede altere Variante stillschweigend verwerfen --
  // das ist bereits einmal passiert (kompletter Verlust der Fang-/Zucht-
  // Historie) und darf nicht nochmal passieren. Legacy-Layout ohne moves[]:
  struct DexMonNoMoves {
    uint16_t level = 0;
    uint8_t geneAtk = 100, geneDef = 100, geneSpe = 100;
    uint8_t geneSpA = 100, geneSpD = 100, geneHp = 100;
    uint8_t trAtk = 0, trDef = 0, trSpe = 0, trSpA = 0, trSpD = 0;
    uint8_t shiny = 0;
  };
  // Layout, bevor das Pet-Wechsel-Feature (ageMinutes/sentAway/bond/medals/
  // moveLevelChecked/nick) dazukam -- das ist der Stand, den JEDER aktuell
  // gespeicherte Spielstand noch hat (diese Migration betrifft also gerade
  // wirklich jeden, nicht nur sehr alte Speicherstaende).
  struct DexMonNoSwitch {
    uint16_t level = 0;
    uint8_t geneAtk = 100, geneDef = 100, geneSpe = 100;
    uint8_t geneSpA = 100, geneSpD = 100, geneHp = 100;
    uint8_t trAtk = 0, trDef = 0, trSpe = 0, trSpA = 0, trSpD = 0;
    uint8_t shiny = 0;
    uint16_t moves[MOVE_SLOTS] = { 0, 0, 0, 0 };
  };
  auto loadLegacyNoMoves = [](Preferences &p, const char *key, DexMon *dst, size_t count) {
    DexMonNoMoves *tmp = new DexMonNoMoves[count];
    p.getBytes(key, tmp, sizeof(DexMonNoMoves) * count);
    for (size_t i = 0; i < count; i++) {
      dst[i].level = tmp[i].level;
      dst[i].geneAtk = tmp[i].geneAtk; dst[i].geneDef = tmp[i].geneDef; dst[i].geneSpe = tmp[i].geneSpe;
      dst[i].geneSpA = tmp[i].geneSpA; dst[i].geneSpD = tmp[i].geneSpD; dst[i].geneHp = tmp[i].geneHp;
      dst[i].trAtk = tmp[i].trAtk; dst[i].trDef = tmp[i].trDef; dst[i].trSpe = tmp[i].trSpe;
      dst[i].trSpA = tmp[i].trSpA; dst[i].trSpD = tmp[i].trSpD;
      dst[i].shiny = tmp[i].shiny;
      // moves[] bleibt 0 -> partyBattleStats() faellt automatisch auf
      // deriveLevelMoves() zurueck, siehe TamaPoke.ino.
    }
    delete[] tmp;
  };
  auto loadLegacyNoSwitch = [](Preferences &p, const char *key, DexMon *dst, size_t count) {
    DexMonNoSwitch *tmp = new DexMonNoSwitch[count];
    p.getBytes(key, tmp, sizeof(DexMonNoSwitch) * count);
    for (size_t i = 0; i < count; i++) {
      dst[i].level = tmp[i].level;
      dst[i].geneAtk = tmp[i].geneAtk; dst[i].geneDef = tmp[i].geneDef; dst[i].geneSpe = tmp[i].geneSpe;
      dst[i].geneSpA = tmp[i].geneSpA; dst[i].geneSpD = tmp[i].geneSpD; dst[i].geneHp = tmp[i].geneHp;
      dst[i].trAtk = tmp[i].trAtk; dst[i].trDef = tmp[i].trDef; dst[i].trSpe = tmp[i].trSpe;
      dst[i].trSpA = tmp[i].trSpA; dst[i].trSpD = tmp[i].trSpD;
      dst[i].shiny = tmp[i].shiny;
      for (int s = 0; s < MOVE_SLOTS; s++) dst[i].moves[s] = tmp[i].moves[s];
      // ageMinutes/sentAway/bond/medals/moveLevelChecked/nick bleiben 0/leer;
      // switchActiveTo() leitet die Startzeit in diesem Fall aus level() ab.
    }
    delete[] tmp;
  };
  #define OLD_DEX_MON_COUNT_650 650
  #define CUR_DEX_MON_COUNT (DEX_COUNT + 1)
  size_t gotCgt = dexPrefs.getBytesLength("dmcgt");
  if (gotCgt == sizeof(dexMonsCaught))
    dexPrefs.getBytes("dmcgt", dexMonsCaught, sizeof(dexMonsCaught));
  else if (gotCgt == sizeof(DexMonNoSwitch) * CUR_DEX_MON_COUNT)
    loadLegacyNoSwitch(dexPrefs, "dmcgt", dexMonsCaught, CUR_DEX_MON_COUNT);
  else if (gotCgt == sizeof(DexMonNoSwitch) * OLD_DEX_MON_COUNT_650)
    loadLegacyNoSwitch(dexPrefs, "dmcgt", dexMonsCaught, OLD_DEX_MON_COUNT_650);
  else if (gotCgt == sizeof(DexMonNoMoves) * CUR_DEX_MON_COUNT)
    loadLegacyNoMoves(dexPrefs, "dmcgt", dexMonsCaught, CUR_DEX_MON_COUNT);
  else if (gotCgt == sizeof(DexMonNoMoves) * OLD_DEX_MON_COUNT_650)
    loadLegacyNoMoves(dexPrefs, "dmcgt", dexMonsCaught, OLD_DEX_MON_COUNT_650);
  size_t gotBrd = dexPrefs.getBytesLength("dmbrd");
  if (gotBrd == sizeof(dexMonsBred))
    dexPrefs.getBytes("dmbrd", dexMonsBred, sizeof(dexMonsBred));
  else if (gotBrd == sizeof(DexMonNoSwitch) * CUR_DEX_MON_COUNT)
    loadLegacyNoSwitch(dexPrefs, "dmbrd", dexMonsBred, CUR_DEX_MON_COUNT);
  else if (gotBrd == sizeof(DexMonNoSwitch) * OLD_DEX_MON_COUNT_650)
    loadLegacyNoSwitch(dexPrefs, "dmbrd", dexMonsBred, OLD_DEX_MON_COUNT_650);
  else if (gotBrd == sizeof(DexMonNoMoves) * CUR_DEX_MON_COUNT)
    loadLegacyNoMoves(dexPrefs, "dmbrd", dexMonsBred, CUR_DEX_MON_COUNT);
  else if (gotBrd == sizeof(DexMonNoMoves) * OLD_DEX_MON_COUNT_650)
    loadLegacyNoMoves(dexPrefs, "dmbrd", dexMonsBred, OLD_DEX_MON_COUNT_650);
  streak = prefs.getUShort("strk", 0);
  bestStreak = prefs.getUShort("bstrk", 0);
  lastCareDay = prefs.getUInt("cday", 0);
  bond = prefs.getUChar("bond", 0);
  medals = prefs.getUShort("medal", 0);
  totalMedals = prefs.getUShort("tmedal", 0);
  lastMilestone = prefs.getUShort("mstone", 0);
  gameHi = prefs.getUShort("ghi", 0);
  strHi = prefs.getUShort("shi", 0);
  catchHi = prefs.getUShort("chi", 0);
  memoHi = prefs.getUShort("mhi", 0);
  cleanHi = prefs.getUShort("clhi", 0);
  typeHi = prefs.getUShort("tyhi", 0);
  battleWins = prefs.getUShort("bwin", 0);
  battleLosses = prefs.getUShort("bloss", 0);
  battleStreak = prefs.getUShort("bstk", 0);
  bestBattleStreak = prefs.getUShort("bbstk", 0);
  if (prefs.getBytesLength("badges2") == sizeof(badges))
    prefs.getBytes("badges2", badges, sizeof(badges));
  collectionFrame = prefs.getUChar("cfrm", 0);
  if (collectionFrame >= unlockedCollectionFrameCount()) collectionFrame = 0;
  lastPetInteractMinute = prefs.getUInt("pimin", 0);
  dexRewardMask = prefs.getUChar("dxrew", 0);
  dailyGoalDay = prefs.getUInt("dgday", 0);
  size_t gotTypes = prefs.getBytes("dgtype", dailyGoalType, sizeof(dailyGoalType));
  size_t gotProg = prefs.getBytes("dgprog", dailyGoalProgress, sizeof(dailyGoalProgress));
  if (gotTypes != sizeof(dailyGoalType)) {
    dailyGoalType[0] = DAILY_GOAL_CARE;
    dailyGoalType[1] = DAILY_GOAL_PLAY;
    dailyGoalType[2] = DAILY_GOAL_CATCH;
  }
  if (gotProg != sizeof(dailyGoalProgress)) {
    dailyGoalProgress[0] = dailyGoalProgress[1] = dailyGoalProgress[2] = 0;
  }
  dailyGoalDone = prefs.getUChar("dgdone", 0);
  size_t gotItems = prefs.getBytes("items", itemCounts, sizeof(itemCounts));
  if (gotItems != sizeof(itemCounts)) memset(itemCounts, 0, sizeof(itemCounts));
  for (uint8_t i = 0; i < EXP_ITEM_COUNT; i++)
    if (itemCounts[i] > EXP_ITEM_MAX) itemCounts[i] = EXP_ITEM_MAX;
  expeditionEndEpoch = prefs.getUInt("exend", 0);
  expeditionRewardItem = prefs.getUChar("exrwd", EXP_ITEM_NONE);
  stepDay = prefs.getUInt("stday", 0);
  stepsToday = prefs.getUInt("stoday", 0);
  stepsTotal = prefs.getUInt("stotal", 0);
  stepDailyRewardMask = prefs.getUChar("stdone", 0) & ((1 << STEP_DAILY_GOAL_COUNT) - 1);
  stepMilestoneMask = prefs.getUChar("stmil", 0) & ((1 << (sizeof(STEP_TOTAL_GOALS) / sizeof(STEP_TOTAL_GOALS[0]))) - 1);
  pendingStepRewardMask = prefs.getUChar("stpend", 0) & ((1 << STEP_DAILY_GOAL_COUNT) - 1);
  lastMorningDay = prefs.getUInt("lmday", 0);
  shakeDay = prefs.getUInt("shkday", 0);
  shakeCountToday = prefs.getUChar("shkcnt", 0);
  walkDay = prefs.getUInt("wday", 0);
  walkHour = prefs.getUInt("whour", 0);
  walkJoyToday = prefs.getUChar("wjoy", 0);
  walkJoyHour = prefs.getUChar("wjhr", 0);
  walkBondToday = prefs.getUChar("wbond", 0);
  evoDeclinedLv = prefs.getUChar("edlv", 0);
  evoDeclinedAge = prefs.getUInt("edage", 0);
  if (expeditionEndEpoch == 0 || expeditionRewardItem >= EXP_ITEM_COUNT) {
    expeditionEndEpoch = 0;
    expeditionRewardItem = EXP_ITEM_NONE;
  }
  prefs.getString("nick", nick, sizeof(nick));
  // siembra: la mascota actual cuenta como criada (guardados antiguos)
  if (speciesId >= 1) registerSpecies(speciesId);
  ceremony = CER_NONE;
  ceremonyUntil = 0;
  if (!isEgg() && pendingCeremony >= CER_FAREWELL && pendingCeremony <= CER_RELEASE) {
    lastEnd = pendingCeremony;
    newEgg();
  }
}
