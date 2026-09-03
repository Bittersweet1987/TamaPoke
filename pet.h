#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "time_utils.h"
#include "dex.h"

// 1 tick = 1 minuto de juego. Baja este valor para probar mas rapido
// (p. ej. 5000UL = las estadisticas caen 12x mas rapido).
#define PET_TICK_MS 60000UL
// Minutos de juego por nivel. Con 60, CHARMANDER evoluciona a las ~16 h
// de juego con cuidado perfecto. Baja a 1 para ver evoluciones al momento.
#define MINUTES_PER_LEVEL 60
#define EAT_ANIM_MS 2500UL
#define HEART_MS 1500UL
#define EVOLVE_ANIM_MS 5200UL              // animacion de evolucion (mas larga = mas epica)
#define MOVE_SLOTS 4  // Attacken-Slots; auch von party.h genutzt (dort nicht nochmal definiert)
#define CEREMONY_MS 10000UL                // duracion de la despedida en pantalla
#define FAREWELL_AGE_MIN (3UL * 24 * 60)   // se despide a los 3 dias de juego (en forma final)
#define RUNAWAY_TICKS 60                   // se escapa tras 1 h con TODO a cero

// ceremonias de fin de ciclo
enum : uint8_t { CER_NONE = 0, CER_FAREWELL, CER_RUNAWAY, CER_RELEASE };

enum PetMood : uint8_t { MOOD_HAPPY, MOOD_SAD, MOOD_EATING, MOOD_SLEEPING };
enum PetEventType : uint8_t { PET_EVENT_BERRY = 0, PET_EVENT_HEART, PET_EVENT_SPARKLE };
enum PetPersonality : uint8_t {
  PERS_BALANCED = 0,
  PERS_PLAYFUL,
  PERS_BRAVE,
  PERS_CALM,
  PERS_LAZY,
};

enum PetInteractResult : uint8_t {
  PET_INTERACT_NONE = 0,
  PET_INTERACT_JOY = 1 << 0,
  PET_INTERACT_BOND = 1 << 1,
  PET_INTERACT_ENERGY = 1 << 2,
};

enum DailyGoalType : uint8_t {
  DAILY_GOAL_CARE = 0,
  DAILY_GOAL_PLAY,
  DAILY_GOAL_BATTLE,
  DAILY_GOAL_CATCH,
  DAILY_GOAL_MEMO,
};
#define DAILY_GOAL_COUNT 3

enum ExpeditionItem : uint8_t {
  EXP_ITEM_SNACK = 0,
  EXP_ITEM_ENERGY,
  EXP_ITEM_CARE,
  EXP_ITEM_TRAIN,
  EXP_ITEM_NONE = 0xFF,
};
#define EXP_ITEM_COUNT 4
#define EXP_ITEM_MAX 80

// Belohnungen der taeglichen Schritt-Meilensteine. Die Werte werden nur
// kurzzeitig fuer die Anzeige gespeichert; der eigentliche Fortschritt liegt
// in stepDailyRewardMask/stepMilestoneMask.
enum StepRewardEvent : uint8_t {
  STEP_REWARD_NONE = 0,
  STEP_REWARD_SNACK,
  STEP_REWARD_ENERGY,
  STEP_REWARD_TRAIN,
  STEP_REWARD_TRAIL_RANK,
};
#define STEP_DAILY_GOAL_COUNT 3

// Zustand fuer den kleinen Expeditions-Hinweis auf dem Hauptscreen.
enum ExpeditionHudState : uint8_t {
  EXP_HUD_HIDDEN = 0,
  EXP_HUD_ACTIVE,
  EXP_HUD_READY,
  EXP_HUD_BAG,
};

enum TrainingStat : int8_t {
  TRAIN_STAT_ATK = 0,
  TRAIN_STAT_DEF,
  TRAIN_STAT_SPE,
};

// medallas del individuo (bitmask)
enum : uint16_t {
  MED_LV10 = 1 << 0, MED_LV25 = 1 << 1, MED_LV50 = 1 << 2,
  MED_BERRY = 1 << 3, MED_STREAK7 = 1 << 4, MED_BOND = 1 << 5,
  MED_FINAL = 1 << 6, MED_FIT = 1 << 7,
};
#define MED_COUNT 8

enum BattleRewardStat : uint8_t {
  BATTLE_REWARD_NONE = 0,
  BATTLE_REWARD_ATK,
  BATTLE_REWARD_DEF,
  BATTLE_REWARD_SPE,
};

// Ein gespeichertes Exemplar einer Art (Fang ODER Zucht, siehe pet.h).
// level == 0 bedeutet "noch kein Exemplar dieser Herkunft gespeichert".
struct DexMon {
  uint16_t level = 0;
  uint8_t geneAtk = 100, geneDef = 100, geneSpe = 100;
  uint8_t geneSpA = 100, geneSpD = 100, geneHp = 100;
  uint8_t trAtk = 0, trDef = 0, trSpe = 0, trSpA = 0, trSpD = 0;
  uint8_t shiny = 0;
  // Zufaellig, aber ECHT: 4 Attacken, die die Art bis zu diesem Level per
  // Level-Aufstieg tatsaechlich gelernt haette (siehe rollFreshDexMon()).
  // Nur bei frisch erzeugten Exemplaren (Fang/Zucht) befuellt; alte
  // gespeicherte Exemplare bleiben 0 = "-" (Migration nicht noetig, da
  // Party-Kaempfe bei 0 auf deriveLevelMoves() zurueckfallen, siehe party.cpp).
  uint16_t moves[MOVE_SLOTS] = { 0, 0, 0, 0 };
  // Nur fuer gezuechtete Exemplare (dexMonsBred) relevant, siehe
  // Pet::switchActiveTo()/newEgg()/canBeSentAway(): das exakte Spielalter
  // (fuer nahtloses Weiterleveln beim Zurueckwechseln), ob dieses Exemplar
  // schon einmal weggeschickt wurde (dann nie wieder moeglich), sowie
  // Bindung/Spitzname/Medaillen des Individuums.
  uint32_t ageMinutes = 0;
  bool sentAway = false;
  uint8_t bond = 0;
  uint16_t medals = 0;
  uint8_t moveLevelChecked = 0;
  char nick[12] = "";
  bool empty() const { return level == 0; }
};

struct BattleReward {
  BattleRewardStat stat = BATTLE_REWARD_NONE;
  uint8_t amount = 0;
};

class Pet {
public:
  // Estadisticas 0..100
  uint8_t fullness = 80;  // comida
  uint8_t joy = 80;       // felicidad
  uint8_t energy = 80;    // energia
  uint8_t hygiene = 100;  // limpieza
  uint8_t poops = 0;      // cacas en pantalla (max 3)
  uint8_t weight = 0;     // 0-100: las chuches engordan, el minijuego quema
  // genes (90-110%, se tiran al eclosionar) y entrenamiento (0-100)
  uint8_t geneAtk = 100, geneDef = 100, geneSpe = 100;
  uint8_t geneSpA = 100, geneSpD = 100;  // fuer das echte Kampfsystem (Party-Portierung)
  uint8_t trAtk = 0, trDef = 0, trSpe = 0;
  uint8_t trSpA = 0, trSpD = 0;
  // Attacken, die das aktuelle Pokemon kennt (0 = leerer Slot). Wird beim
  // Levelaufstieg automatisch befuellt, siehe checkLevelUpMoves() in pet.cpp.
  uint16_t moves[MOVE_SLOTS] = { 0, 0, 0, 0 };  // Index in MOVE_TBL (siehe moves_real.h)
  uint8_t moveLevelChecked = 0;  // bis zu diesem Level wurden Lernlisten schon geprueft
  // Wie im echten Spiel: sind schon 4 Attacken vergeben, wird eine neu
  // erreichte Lernlisten-Attacke NICHT mehr automatisch ueberschrieben,
  // sondern erst per Dialog angeboten (siehe hasPendingLearnMove() unten,
  // UI in TamaPoke.ino). 0 = kein Dialog offen.
  uint16_t pendingLearnMove = 0;
  uint8_t pendingLearnLevel = 0;
  bool berryKnown = false;  // ya descubrio su baya favorita
  bool shiny = false;       // variante de color rara (se sortea en el huevo)
  uint32_t ageMinutes = 0;
  int16_t speciesId = -1;      // numero de Pokedex (1-DEX_COUNT), -1 = huevo
  int16_t prevSpeciesId = -1;  // para la animacion de evolucion
  uint8_t careMistakes = 0;   // descuidos: cada uno retrasa la evolucion 1 nivel
  bool sleeping = false;
  uint32_t lastSeenEpoch = 0;   // ultima hora RTC vista (para progresion offline)
  uint8_t ceremony = CER_NONE;  // despedida/escapada/liberacion en curso
  uint8_t lastEnd = CER_NONE;   // como acabo la anterior (afecta al huevo)
  uint8_t dexReg[DEX_BITMAP_BYTES] = { 0 };       // pokedex de criados
  uint8_t dexShinyReg[DEX_BITMAP_BYTES] = { 0 };  // shiny criado o capturado
  uint8_t dexCaught[DEX_BITMAP_BYTES] = { 0 };    // pokedex de salvajes capturados

  // Gespeicherte Werte je Art, GETRENNT nach Herkunft (Fang vs. Zucht). Index
  // 0 unbenutzt, 1..DEX_COUNT je Art. Wird beim Fangen/Schluepfen befuellt;
  // bei einem Duplikat entscheidet der Spieler per Vergleichsdialog, welches
  // Exemplar bleibt (siehe TamaPoke.ino dexCompare*). Party::addFromDex()
  // liest aus diesen Speichern statt jedes Mal neu zu wuerfeln.
  DexMon dexMonsCaught[DEX_COUNT + 1];
  DexMon dexMonsBred[DEX_COUNT + 1];
  // racha de cuidado diario (del jugador: persiste entre crianzas)
  uint16_t streak = 0, bestStreak = 0;
  uint32_t lastCareDay = 0;
  // vinculo (del bicho: sube lento con cuidado, se resetea al nacer otro)
  uint8_t bond = 0;
  char nick[12] = "";    // apodo (vacio = nombre de especie)
  // medallas: del individuo + contador acumulado entre todas las crianzas
  uint16_t medals = 0, totalMedals = 0;
  uint16_t newMedal = 0;   // recien conseguida(s), para celebrar
  uint16_t lastMilestone = 0;  // hito de racha ya celebrado
  uint16_t gameHi = 0;     // record del minijuego (del jugador)
  uint16_t strHi = 0;      // record de golpes al saco
  uint16_t catchHi = 0;    // record de capturas del minijuego catch
  uint16_t memoHi = 0;     // record de rondas del minijuego memo
  uint16_t cleanHi = 0;    // record del minijuego clean
  uint16_t typeHi = 0;     // record del minijuego type match
  uint16_t battleWins = 0, battleLosses = 0;
  uint16_t battleStreak = 0, bestBattleStreak = 0;
  // Sieg-Marker je Region und Schwierigkeit: Bit n = Gegner n besiegt (8
  // Arenen + Top Vier + Meister, siehe GYM_COUNT/GYM_REGION_COUNT in
  // trainers.h; spielerweit, wie streak/medals ueberlebt es eine neue
  // Zucht). Kein Statuseffekt -- schaltet nur den naechsten Gegner frei,
  // genau wie im Original DylanPDao/TamaPoke. [region][0]=normal,[region][1]=hart.
  uint16_t badges[5][2] = { { 0 } };
  bool hasBadge(uint8_t region, bool hard, uint8_t i) const {
    return region < 5 && i < 13 && (badges[region][hard ? 1 : 0] & (1u << i));
  }
  void winBadge(uint8_t region, bool hard, uint8_t i) {
    if (region < 5 && i < 13) badges[region][hard ? 1 : 0] |= (1u << i);
  }
  uint8_t badgeCount(uint8_t region, bool hard) const {
    uint8_t n = 0;
    for (uint8_t i = 0; i < 13; i++) if (hasBadge(region, hard, i)) n++;
    return n;
  }
  uint8_t collectionFrame = 0;  // 0=Basis, weitere Rahmen ueber Dex-Meilensteine
  uint32_t lastPetInteractMinute = 0;
  uint8_t dexRewardMask = 0;
  uint32_t dailyGoalDay = 0;
  uint8_t dailyGoalType[DAILY_GOAL_COUNT] = { DAILY_GOAL_CARE, DAILY_GOAL_PLAY, DAILY_GOAL_CATCH };
  uint8_t dailyGoalProgress[DAILY_GOAL_COUNT] = { 0, 0, 0 };
  uint8_t dailyGoalDone = 0;
  uint8_t itemCounts[EXP_ITEM_COUNT] = { 0 };
  uint32_t expeditionEndEpoch = 0;
  uint8_t expeditionRewardItem = EXP_ITEM_NONE;
  // Schrittfortschritt des Spielers: heute wird am RTC-Tag zurueckgesetzt,
  // der Gesamtwert bleibt ueber Eier und neue Pokemon erhalten.
  uint32_t stepsToday = 0;
  uint32_t stepsTotal = 0;
  uint32_t stepDay = 0;
  uint8_t stepDailyRewardMask = 0;
  uint8_t stepMilestoneMask = 0;
  bool saveLoadedFromNvs = false;
  bool saveCreatedThisBoot = false;

  void begin();                 // carga estado de NVS (o crea el primer huevo)
  bool update(uint32_t nowMs);  // true cuando avanzo el estado del bicho

  // Acciones (botones tactiles)
  void feed();              // baya roja (compatibilidad)
  void feedBerry(uint8_t color);  // 0 roja, 1 azul, 2 verde
  void feedCandy();
  bool lovesBerry(uint8_t color) const {
    return !isEgg() && (speciesId % 3) == color;  // gusto oculto por especie
  }
  void playResult(uint8_t score);  // recompensa del minijuego (entrena VEL)
  uint8_t applyCatchResult(uint8_t score);
  uint8_t applyMemoResult(uint8_t rounds);
  uint8_t applyCleanResult(uint8_t score);
  uint8_t applyTypeResult(uint8_t score);
  bool applyPetEvent(uint8_t eventType);
  uint8_t interactPet(bool eveningBonus);
  bool applyShake();
  uint8_t applyWalk(uint16_t steps);
  uint32_t stepGoal(uint8_t index) const;
  bool stepGoalComplete(uint8_t index) const;
  uint8_t stepTrailRank() const;
  uint16_t stepShinyChancePer4096() const;
  uint8_t stepCatchBonus() const;
  bool showStepReward() const { return deadlineActive(millis(), stepRewardUntil); }
  uint8_t lastStepReward() const { return lastStepRewardEvent; }
  bool takeMorningGreeting();
  PetPersonality personality() const;
  void ensureDailyGoals();
  uint8_t dailyGoalTarget(uint8_t goalType) const;
  bool dailyGoalComplete(uint8_t index) const;
  uint8_t trainStrength(uint16_t hits);  // saco de entrenamiento (entrena FUE)
  BattleReward applyBattleWin(int16_t wildDex, bool closeWin);
  void applyBattleLoss();

  // Expediciones: el premio se sortea y guarda al salir para que un reinicio
  // no permita repetir la tirada. Los rolls son inyectables para pruebas nativas.
  static uint8_t expeditionEnergyCost(uint8_t minutes);
  bool expeditionActive(uint32_t nowEpoch) const;
  bool expeditionReady(uint32_t nowEpoch) const;
  uint8_t expeditionItemCount() const;
  ExpeditionHudState expeditionHudState(uint32_t nowEpoch) const;
  bool expeditionInventoryFull() const;
  bool canStartExpedition(uint8_t minutes, uint32_t nowEpoch) const;
  uint8_t expeditionTrainingChance(uint8_t minutes) const;
  bool startExpedition(uint8_t minutes, uint32_t nowEpoch, uint8_t luckRoll, uint8_t itemRoll = 0);
  ExpeditionItem claimExpedition(uint32_t nowEpoch);
  bool useExpeditionItem(ExpeditionItem item, int8_t trainingStat = -1);

  // stats de combate: base real de gen 1 x genes + nivel + entrenamiento
  uint16_t atkStat() const;
  uint16_t defStat() const;
  uint16_t speStat() const;
  uint16_t spaStat() const;  // Spezial-Angriff (echtes Kampfsystem)
  uint16_t spdStat() const;  // Spezial-Verteidigung (echtes Kampfsystem)

  // Wuerfelt ein frisches Exemplar (fuer Fang/Zucht-Speicherung, siehe DexMon).
  DexMon rollFreshDexMon(int16_t dex, uint16_t atLevel, bool isShiny) const;
  // Prueft Lernlisten fuer neu erreichte Level und lernt faellige Attacken
  // automatisch (voller Vorrat -> die aelteste wird ersetzt). Wird aus tick()
  // aufgerufen, ist aber public falls die UI nach einem Levelsprung (z.B.
  // Offline-Fortschritt) manuell nachtriggert werden soll.
  void checkLevelUpMoves();
  // Dialog "Attacke X erlernen?" (siehe pendingLearnMove oben).
  bool hasPendingLearnMove() const { return pendingLearnMove != 0; }
  void declineLearnMove();                  // "Nein": Attacke bleibt ungelernt
  void confirmLearnMove(uint8_t replaceSlot); // "Ja" + gewaehlter Slot: ersetzt moves[replaceSlot]
  void saveDexMons();  // separat, da die zwei Arrays gross sind (nur bei Aenderung aufrufen)
  // Einmaliger Nachtrag fuer Spielstaende von vor dem Fang/Zucht-Wertesystem:
  // traegt fuer bereits registrierte/gefangene Arten ohne DexMon-Werte welche
  // nach, damit Pokedex-Seite 3 und "INS TEAM" auch ohne Neufang funktionieren.
  void backfillDexMonHistory();
  void backfillDexMonHistoryAfterDexExpand();
  void backfillDexMonHistoryAfterMovesField();
  // Einmaliger Nachtrag fuers Pet-Wechsel-Feature: Spielstaende von VOR
  // diesem Feature kennen "sentAway" noch nicht (Default false ueberall).
  // Jeder gezuechtete Eintrag, der nicht das aktuell aktive Exemplar ist,
  // MUSS aber schon einmal weggeschickt worden sein -- sonst gaebe es ja gar
  // kein neues (aktuelles) Ei/Exemplar. Siehe canBeSentAway().
  void backfillSentAwayFlags();
  // Haelt dexMonsBred[speciesId] mit dem gerade aufgezogenen Haustier
  // synchron (Level/Gene/Training/Shiny). Ohne das blieb der Pokedex-Eintrag
  // (und alles, was von dort liest -- Werte-Seite, Team) auf dem Stand vom
  // Schluepfen/Nachtrag eingefroren, statt mit dem Haustier mitzuwachsen.
  void syncOwnDexMon();
  void play();
  void toggleLight();  // dormir / despertar
  void clean();
  void caress();  // tocar al bicho
  bool eggTap();  // tocar el huevo: 3 toques y eclosiona (true si acaba de eclosionar)
  void newEgg();   // empezar de cero con un inicial aleatorio
  void release();  // soltar (pulsacion larga + confirmar)
  void syncClock(uint32_t nowEpoch);  // aplica el tiempo transcurrido apagado
  void setClock(uint32_t nowEpoch);   // fija la hora sin aplicar progresion
  void startFarewell();  // tambien usable desde la consola serie (BYE)
  void startRunaway();   // tambien usable desde la consola serie (RUN)

  bool isEgg() const { return speciesId < 0; }
  uint8_t eggCracks() const { return eggTaps; }
  bool eating() const { return deadlineActive(millis(), eatUntil); }
  bool showHeart() const { return deadlineActive(millis(), heartUntil); }
  bool evolving() const { return deadlineActive(millis(), evolveUntil); }
  float evolveT() const {     // progreso de la animacion de evolucion 0..1
    uint32_t n = millis();
    uint32_t left = deadlineRemaining(n, evolveUntil);
    return 1.0f - (float)left / (float)EVOLVE_ANIM_MS;
  }
  bool evolutionUnlocked() const;  // nivel/condicion: al menos una forma disponible
  bool canEvolveNow() const;  // lista: unlocked + wach + 3 de 4 valores > 40
  uint8_t evolutionOptionCount() const;
  int16_t evolutionOption(uint8_t index) const;
  uint8_t evolutionRequiredLevel() const;
  bool canEvolveTo(int16_t target) const;
  void evolveTo(int16_t target);  // transforma a un objetivo elegido
  void evolve();                  // compatibilidad: elige una opcion disponible
  bool canFarewellNow() const;  // forma final + 7 dias: lista para despedirse (boton)
  bool canRunawayNow() const;   // abandono total 1h: lista para escaparse (boton triste)
  // Pet-Wechsel: ein bereits einmal weggeschicktes Exemplar (Abschied/
  // Weglaufen/Freilassen) darf das kein zweites Mal werden, auch wenn man
  // es per switchActiveTo() zurueckgeholt hat.
  bool canBeSentAway() const {
    return speciesId >= 1 && speciesId <= DEX_COUNT && !dexMonsBred[speciesId].sentAway;
  }
  // Die Art des Exemplars, dessen "Reise" noch offen ist (also noch nicht
  // weggeschickt wurde) -- das ist immer genau eins, siehe canBeSentAway().
  // -1, wenn es (noch) keins gibt (z.B. ganz frisches Spiel).
  int16_t homeSpeciesId() const;
  // Wechselt das aktive (im Hauptbildschirm lebende) Exemplar zu einer
  // bereits gefangenen ODER gezuechteten Art aus dem Pokedex. Das bisherige
  // Exemplar wird zuerst gesichert (bleibt also abrufbar), das neue setzt
  // exakt beim zuletzt gespeicherten Stand fort (Level/Alter, Gene,
  // Training, Attacken, Bindung, Spitzname, Medaillen). Pflegewerte
  // (Hunger/Freude/Energie/Hygiene) starten neu, wie beim Schluepfen --
  // waehrend ein Exemplar "auf der Bank" ist, braucht es keine Pflege
  // (siehe Pokedex als Box). false, wenn dex ungueltig/leer/schon aktiv ist.
  bool switchActiveTo(int16_t dex);
  // el usuario decide en un dialogo; "mantener/quedaros" pospone y re-ofrece luego
  bool wantEvolveButton() const;
  bool wantFarewellButton() const { return canFarewellNow() && ageMinutes >= farDeclinedAge; }
  void declineEvolve();
  void declineFarewell() { farDeclinedAge = ageMinutes + 1440; } // re-ofrece dentro de 1 dia
  // primera partida: el jugador elige inicial (Bulbasaur/Charmander/Squirtle)
  bool awaitingStarter() const { return starterPick; }
  void chooseStarter(int16_t dex) { eggTarget = dex; starterPick = false; save(); }
  void factoryReset() { prefs.clear(); }  // borra la NVS (test: comando serie WIPE)
  void dbgRunawayReady() { fullness = joy = energy = hygiene = 0; neglectTicks = RUNAWAY_TICKS; }  // test
  uint8_t level() const {
    uint32_t raw = 1UL + ageMinutes / MINUTES_PER_LEVEL;
    return raw > 100UL ? 100 : (uint8_t)raw;
  }
  bool isRegistered(int16_t dex) const {
    return dex >= 1 && dex <= DEX_COUNT && (dexReg[(dex - 1) >> 3] & (1 << ((dex - 1) & 7)));
  }
  bool isCaught(int16_t dex) const {
    return dex >= 1 && dex <= DEX_COUNT && (dexCaught[(dex - 1) >> 3] & (1 << ((dex - 1) & 7)));
  }
  bool isShinyRegistered(int16_t dex) const {
    return dex >= 1 && dex <= DEX_COUNT && (dexShinyReg[(dex - 1) >> 3] & (1 << ((dex - 1) & 7)));
  }
  uint16_t registeredCount() const;
  uint16_t caughtCount() const;
  uint16_t knownDexCount() const;
  uint8_t collectionRank() const;
  uint8_t unlockedCollectionFrameCount() const;
  bool setCollectionFrame(uint8_t frame);
  void registerCaught(int16_t dex, bool shinyVariant = false);
  uint8_t nextDexGoal() const;
  uint8_t applyDexRewards();
  uint8_t catchChanceForWild(int16_t wildDex, uint8_t wildLevel, uint8_t petLevel, bool closeWin) const;
  bool tryCatchWild(int16_t wildDex, uint8_t wildLevel, uint8_t petLevel, bool closeWin,
                    uint8_t luckRoll, bool shinyVariant = false);
  bool lineHasUnregistered(int16_t base) const;
  bool hasEvolutionPath(int16_t dex) const;
  uint8_t eggRarity() const;       // rareza del huevo actual (sin revelar especie)
  int16_t pickEggSpecies();        // publica para poder simular tiradas (EGGS)
  uint8_t healthyStatCount() const {
    return (uint8_t)((fullness > 40 ? 1 : 0) + (joy > 40 ? 1 : 0) +
                     (energy > 40 ? 1 : 0) + (hygiene > 40 ? 1 : 0));
  }
  uint8_t lowestStat() const { return min(min(fullness, joy), min(energy, hygiene)); }
  PetMood mood() const;
  // progreso de la ceremonia de despedida/escapada, 0..1 (para animarla)
  float ceremonyT() const {
    if (ceremony == CER_NONE) return 0.0f;
    uint32_t n = millis();
    uint32_t left = deadlineRemaining(n, ceremonyUntil);
    return 1.0f - (float)left / (float)CEREMONY_MS;
  }

  // racha / vinculo / medallas / nombre
  void rename(const char *name);
  bool hasMedal(uint16_t m) const { return medals & m; }
  bool showMedal() const { return deadlineActive(millis(), medalUntil); }
  bool showMilestone() const { return deadlineActive(millis(), milestoneUntil); }
  bool showDexReward() const { return deadlineActive(millis(), dexRewardUntil); }
  uint8_t lastDexRewardGoal() const { return lastDexReward; }
  int careBonus() const;  // mejora del huevo por racha + vinculo

  // guardado periodico diferido: tick() marca pendiente y el loop lo vuelca
  // cuando la pantalla esta atenuada/apagada (la escritura a flash congela
  // ~1s ambos cores: asi no se ve ni corta el tactil)
  bool savePending() const { return pendingSave; }
  void flushSave();
  // Sofortiges Speichern von aussen (z.B. nach einem Arenensieg mit direkter
  // Trainings-Aenderung) -- wie applyBattleWin() es intern schon tut.
  void saveNow() { save(); }

private:
  Preferences prefs;
  Preferences dexPrefs;  // eigene "dexnvs"-Partition fuer dexMonsCaught/dexMonsBred (siehe partitions.csv)
  uint32_t lastTick = 0;
  uint32_t eatUntil = 0;
  uint32_t heartUntil = 0;
  uint32_t evolveUntil = 0;
  int16_t eggTarget = 1;       // dex oculto que saldra del huevo
  bool eggShiny = false;       // sorpresa sorteada al crear el huevo
  uint8_t eggTaps = 0;
  uint8_t mistakeCooldown = 0;
  uint8_t ticksSinceSave = 0;
  bool pendingSave = false;     // guardado periodico pendiente de volcar
  uint8_t evoDeclinedLv = 0;    // "mantener forma": no ofrecer hasta el siguiente nivel
  uint32_t evoDeclinedAge = 0;  // auf Lv.100: wieder anbieten ab diesem Alter
  uint32_t farDeclinedAge = 0;  // "quedaros juntos": no ofrecer despedida hasta esta edad
  bool starterPick = false;     // primera partida: esperando que el jugador elija inicial
  uint8_t neglectTicks = 0;
  uint16_t goodTicks = 0;  // racha bien cuidado: forja la DEF
  uint32_t ceremonyUntil = 0;
  uint8_t bondToday = 0;       // tope diario de subida de vinculo
  uint32_t medalUntil = 0;     // celebracion de medalla en pantalla
  uint32_t milestoneUntil = 0; // celebracion de hito de racha
  uint32_t dexRewardUntil = 0;
  uint8_t lastDexReward = 0;
  uint32_t lastMorningDay = 0;
  uint32_t shakeReadyAt = 0;
  uint32_t shakeDay = 0;
  uint8_t shakeCountToday = 0;
  uint16_t walkJoyBank = 0;
  uint16_t walkBondBank = 0;
  uint32_t walkDay = 0;
  uint32_t walkHour = 0;
  uint8_t walkJoyToday = 0;
  uint8_t walkJoyHour = 0;
  uint8_t walkBondToday = 0;
  uint32_t stepRewardUntil = 0;
  uint8_t lastStepRewardEvent = STEP_REWARD_NONE;
  uint8_t pendingStepRewardMask = 0;

  uint32_t today() const { return lastSeenEpoch ? lastSeenEpoch / 86400 : 0; }
  void registerCare();   // primer cuidado del dia: racha + vinculo
  void addBond(uint8_t amt);
  void noteDailyGoal(uint8_t goalType, uint8_t amount);
  void applyDailyReward();
  void recordStepReward(uint8_t index);
  void applyPendingStepRewards();
  void ensureStepDay();
  void checkMedals();
  void tick();
  void hatch();
  void registerSpecies(int16_t dex);
  bool canReceiveExpeditionItem(ExpeditionItem item) const;
  void save();
  void load();
  static uint8_t clamp100(int v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }
};

extern Pet pet;
