#include "battle.h"
#include "dex.h"
#include "types_real.h"
#include "learnsets_real.h"
#include <Arduino.h>

namespace {

// Werte-Multiplikator fuer Statusstufen (-6..+6), klassische Formel.
uint16_t applyStage(uint16_t base, int8_t stage) {
  if (stage == 0) return base;
  if (stage > 0) return (uint16_t)((uint32_t)base * (2 + stage) / 2);
  return (uint16_t)((uint32_t)base * 2 / (2 - stage));
}

uint16_t statAfterStage(const BattleStats &s, const int8_t *stages, uint8_t which) {
  // which: 0=ATK 1=DEF 2=SPA 3=SPD 4=SPE
  uint16_t base = which == 0 ? s.atk : which == 1 ? s.def : which == 2 ? s.spa
                : which == 3 ? s.spd : s.spe;
  return applyStage(base, stages[which]);
}

void addStage(int8_t *stages, uint8_t statMask, int8_t delta) {
  if (statMask & ST_ATK) stages[0] = constrain(stages[0] + delta, -6, 6);
  if (statMask & ST_DEF) stages[1] = constrain(stages[1] + delta, -6, 6);
  if (statMask & ST_SPA) stages[2] = constrain(stages[2] + delta, -6, 6);
  if (statMask & ST_SPD) stages[3] = constrain(stages[3] + delta, -6, 6);
  if (statMask & ST_SPE) stages[4] = constrain(stages[4] + delta, -6, 6);
}

}  // namespace

BattleStats wildBattleStats(int16_t dex, uint8_t level) {
  if (dex < 1 || dex > DEX_COUNT) dex = 1;
  const DexEntry &entry = DEX_TBL[dex];
  uint8_t lvl = level ? level : 1;
  BattleStats stats = {};
  // Gleiche Level-Skalierung wie beim eigenen Haustier (siehe calcStat() in
  // pet.cpp): multiplikativ statt nur additiv, sonst waeren wilde Lv.1- und
  // Lv.100-Exemplare derselben Art kaum unterschiedlich stark.
  stats.atk = (uint16_t)((uint32_t)entry.bAtk * 2 * lvl / 100 + lvl);
  stats.def = (uint16_t)((uint32_t)entry.bDef * 2 * lvl / 100 + lvl);
  stats.spa = (uint16_t)((uint32_t)entry.bSpA * 2 * lvl / 100 + lvl);
  stats.spd = (uint16_t)((uint32_t)entry.bSpD * 2 * lvl / 100 + lvl);
  stats.spe = (uint16_t)((uint32_t)entry.bSpe * 2 * lvl / 100 + lvl);
  stats.hp  = 30 + (uint16_t)lvl * 5 + entry.bHp / 2;
  stats.level = lvl;
  stats.type1 = entry.type1;
  stats.type2 = entry.type2;
  // Wildpokemon greifen mit ihren echten Lernlisten-Attacken an (die
  // aktuellsten 4, die sie auf ihrem Level schon koennten).
  uint8_t n = learnCount(dex);
  uint8_t slot = 0;
  for (uint8_t i = 0; i < n && slot < 4; i++) {
    uint8_t lv = learnLevel(dex, i);
    if (lv == 0 || lv > lvl) continue;
    uint16_t mv = learnMove(dex, i);
    bool dup = false;
    for (uint8_t s = 0; s < slot; s++) if (stats.moves[s] == mv) dup = true;
    if (!dup) stats.moves[slot++] = mv;
  }
  if (slot == 0) stats.moves[0] = 1;  // Fallback: TACKLE-aehnliche erste Attacke
  return stats;
}

BattleRuntime beginBattleRuntime(const BattleStats &player, const BattleStats &enemy) {
  BattleRuntime battle = {};
  battle.player = player;
  battle.enemy = enemy;
  battle.playerMaxHp = player.hp;
  battle.enemyMaxHp = enemy.hp;
  battle.playerHp = battle.playerMaxHp;
  battle.enemyHp = battle.enemyMaxHp;
  return battle;
}

uint8_t battleTypeEffectPct(uint8_t attackType, uint8_t defendType1, uint8_t defendType2) {
  uint32_t eff = typeEffPct(attackType, defendType1, defendType2);
  return (uint8_t)(eff > 255 ? 255 : eff);
}

uint8_t enemyChooseMove(const BattleRuntime &battle, uint8_t luckRoll) {
  uint8_t count = 0;
  for (uint8_t i = 0; i < 4; i++) if (battle.enemy.moves[i] != 0) count++;
  if (count == 0) return 0;
  uint8_t pick = luckRoll % count;
  uint8_t seen = 0;
  for (uint8_t i = 0; i < 4; i++) {
    if (battle.enemy.moves[i] == 0) continue;
    if (seen == pick) return i;
    seen++;
  }
  return 0;
}

bool playerActsFirst(const BattleRuntime &battle, uint8_t playerMove, uint8_t enemyMove, uint8_t luckRoll) {
  int8_t pPrio = (battle.player.moves[playerMove] < MOVE_COUNT &&
                  MOVE_TBL[battle.player.moves[playerMove]].effect == EF_PRIORITY)
                 ? MOVE_TBL[battle.player.moves[playerMove]].param : 0;
  int8_t ePrio = (battle.enemy.moves[enemyMove] < MOVE_COUNT &&
                  MOVE_TBL[battle.enemy.moves[enemyMove]].effect == EF_PRIORITY)
                 ? MOVE_TBL[battle.enemy.moves[enemyMove]].param : 0;
  if (pPrio != ePrio) return pPrio > ePrio;
  uint16_t pSpe = statAfterStage(battle.player, battle.playerStage, 4);
  uint16_t eSpe = statAfterStage(battle.enemy, battle.enemyStage, 4);
  if (pSpe != eSpe) return pSpe > eSpe;
  return (luckRoll % 2) == 0;  // exaktes Gleichstand: Zufall
}

static BattleMoveOutcome doMove(BattleRuntime &battle, bool playerSide, uint8_t moveSlot, uint8_t luckRoll) {
  BattleMoveOutcome out = {};
  BattleStats &attacker = playerSide ? battle.player : battle.enemy;
  BattleStats &defender = playerSide ? battle.enemy : battle.player;
  int8_t *atkStage = playerSide ? battle.playerStage : battle.enemyStage;
  int8_t *defStage = playerSide ? battle.enemyStage : battle.playerStage;
  uint16_t &atkHp = playerSide ? battle.playerHp : battle.enemyHp;
  uint16_t &defHp = playerSide ? battle.enemyHp : battle.playerHp;
  uint16_t defMaxHp = playerSide ? battle.enemyMaxHp : battle.playerMaxHp;
  uint8_t &ailment = playerSide ? battle.playerAilment : battle.enemyAilment;
  uint8_t &ailCounter = playerSide ? battle.playerAilCounter : battle.enemyAilCounter;
  bool &recharge = playerSide ? battle.playerRecharge : battle.enemyRecharge;

  // Status, der den Zug blockiert.
  if (recharge) { recharge = false; out.acted = false; return out; }
  if (ailment == AIL_SLEEP) {
    if (ailCounter > 0) { ailCounter--; out.acted = false; return out; }
    ailment = 0;
  }
  if (ailment == AIL_FREEZE) {
    if ((luckRoll % 100) >= 20) { out.acted = false; return out; }
    ailment = 0;  // aufgetaut
  }
  if (ailment == AIL_PARA && (luckRoll % 100) < 25) { out.acted = false; return out; }
  if (ailment == AIL_CONFUSE) {
    if (ailCounter > 0) ailCounter--;
    if (ailCounter == 0) ailment = 0;
    if ((luckRoll % 100) < 33) {
      // trifft sich selbst: leichter fester Schaden, kein Attackeneffekt
      uint16_t selfDmg = (uint16_t)(defMaxHp / 12 + 1);
      atkHp = atkHp > selfDmg ? atkHp - selfDmg : 0;
      out.acted = true;
      out.selfHit = true;
      out.fainted = (atkHp == 0);
      out.battleEnded = out.fainted;
      out.playerWon = playerSide ? false : true;
      return out;
    }
  }

  out.acted = true;
  uint16_t moveId = attacker.moves[moveSlot];
  if (moveId == 0 || moveId >= MOVE_COUNT) return out;
  const MoveEntry &mv = MOVE_TBL[moveId];

  uint8_t acc = mv.acc;
  bool alwaysHits = (mv.effect == EF_NEVER_MISS) || acc == 0;
  if (!alwaysHits && (luckRoll % 100) >= acc) {
    out.missed = true;
    return out;
  }

  // Status-Attacken (kein Schaden): nur Stufeneffekt oder eigene Heilung.
  if (mv.cat == MC_STATUS) {
    if (mv.effect == EF_STAGE) {
      addStage(mv.target == TG_SELF ? atkStage : defStage, mv.statMask, mv.stages);
    } else if (mv.effect == EF_HEAL) {
      uint16_t heal = (uint16_t)((uint32_t)(playerSide ? battle.playerMaxHp : battle.enemyMaxHp) * mv.param / 100);
      atkHp = (uint16_t)min((uint32_t)atkHp + heal, (uint32_t)(playerSide ? battle.playerMaxHp : battle.enemyMaxHp));
      out.healed = true;
    }
    if (mv.ailment != AIL_NONE && (luckRoll % 100) < mv.ailChance) {
      uint8_t &targetAil = mv.target == TG_SELF ? ailment : (playerSide ? battle.enemyAilment : battle.playerAilment);
      uint8_t &targetCounter = mv.target == TG_SELF ? ailCounter : (playerSide ? battle.enemyAilCounter : battle.playerAilCounter);
      if (targetAil == AIL_NONE) {
        targetAil = mv.ailment;
        targetCounter = (mv.ailment == AIL_SLEEP) ? 1 + (luckRoll % 3) : (mv.ailment == AIL_CONFUSE ? 2 + (luckRoll % 3) : 0);
        out.newAilment = mv.ailment;
      }
    }
    return out;
  }

  // Schadensberechnung.
  uint16_t damage = 0;
  if (mv.effect == EF_FIXED) {
    damage = (uint16_t)mv.param;
  } else if (mv.effect == EF_FIXED_LVL) {
    damage = attacker.level;
  } else {
    uint16_t atkStat = mv.cat == MC_PHYS ? statAfterStage(attacker, atkStage, 0) : statAfterStage(attacker, atkStage, 2);
    uint16_t defStat = mv.cat == MC_PHYS ? statAfterStage(defender, defStage, 1) : statAfterStage(defender, defStage, 3);
    if (defStat == 0) defStat = 1;
    uint32_t base = (uint32_t)mv.power * atkStat / defStat;
    base = base / 3 + 2;  // auf unsere kleinere Werte-Skala gebracht

    uint16_t eff = typeEffPct(mv.type, defender.type1, defender.type2);
    bool stab = (mv.type == attacker.type1 || mv.type == attacker.type2);
    base = base * eff / 100;
    if (stab) base = base * 3 / 2;
    if (mv.effect == EF_MULTI) {
      uint8_t hits = 2 + (luckRoll % 4 == 0 ? 3 : (luckRoll % 3 == 0 ? 1 : 0));  // meist 2-3, selten mehr
      base *= hits;
    }
    damage = (uint16_t)min(base, (uint32_t)9999);
    out.effPct = (uint8_t)min((uint32_t)eff, (uint32_t)400);
  }

  if (damage < 1 && mv.power > 0) damage = 1;
  defHp = defHp > damage ? defHp - damage : 0;
  out.damage = damage;
  out.fainted = (defHp == 0);

  if (mv.effect == EF_RECOIL && mv.param > 0) {
    uint16_t recoil = (uint16_t)(damage / mv.param);
    if (recoil < 1) recoil = 1;
    atkHp = atkHp > recoil ? atkHp - recoil : 0;
    out.recoiled = true;
  } else if (mv.effect == EF_DRAIN) {
    uint16_t drain = (uint16_t)((uint32_t)damage * mv.param / 100);
    uint16_t atkMax = playerSide ? battle.playerMaxHp : battle.enemyMaxHp;
    atkHp = (uint16_t)min((uint32_t)atkHp + drain, (uint32_t)atkMax);
    out.healed = drain > 0;
  } else if (mv.effect == EF_RECHARGE) {
    recharge = true;
  }

  if (!out.fainted && mv.effect == EF_STAGE && mv.target == TG_FOE) {
    addStage(defStage, mv.statMask, mv.stages);
  }
  if (!out.fainted && mv.ailment != AIL_NONE && (luckRoll % 100) < mv.ailChance) {
    uint8_t &defAilment = playerSide ? battle.enemyAilment : battle.playerAilment;
    uint8_t &defCounter = playerSide ? battle.enemyAilCounter : battle.playerAilCounter;
    if (defAilment == AIL_NONE) {
      defAilment = mv.ailment;
      defCounter = (mv.ailment == AIL_SLEEP) ? 1 + (luckRoll % 3) : (mv.ailment == AIL_CONFUSE ? 2 + (luckRoll % 3) : 0);
      out.newAilment = mv.ailment;
    }
  }

  if (out.fainted) {
    out.battleEnded = true;
    out.playerWon = playerSide;
  }
  return out;
}

BattleMoveOutcome stepBattleMove(BattleRuntime &battle, bool playerSide, uint8_t moveSlot, uint8_t luckRoll) {
  return doMove(battle, playerSide, moveSlot, luckRoll);
}

bool applyEndOfTurnAilment(BattleRuntime &battle, bool playerSide) {
  uint8_t ail = playerSide ? battle.playerAilment : battle.enemyAilment;
  if (ail != AIL_BURN && ail != AIL_POISON) return false;
  uint16_t &hp = playerSide ? battle.playerHp : battle.enemyHp;
  uint16_t maxHp = playerSide ? battle.playerMaxHp : battle.enemyMaxHp;
  uint16_t dmg = (uint16_t)(maxHp / 8 + 1);
  hp = hp > dmg ? hp - dmg : 0;
  return hp == 0;
}

bool canStartWildBattle(bool isEgg, bool sleeping, uint8_t ceremony) {
  return !isEgg && !sleeping && ceremony == 0;
}

uint8_t wildLevelFor(uint8_t petLevel, uint8_t luckRoll) {
  int base = (int)(petLevel ? petLevel : 1);
  int delta;
  if (luckRoll < 55) delta = (int)(luckRoll % 3) - 1;
  else if (luckRoll < 85) delta = -2 - (int)(luckRoll % 3);
  else delta = 2 + (int)(luckRoll % 2);
  int level = base + delta;
  if (level < 1) level = 1;
  return level > 100 ? 100 : (uint8_t)level;
}

static bool wildTypePreferred(uint8_t type1, uint8_t type2, uint8_t phase) {
  auto has = [&](uint8_t t) { return type1 == t || type2 == t; };
  switch (phase) {
    case 0: return has(TYPE_GRASS) || has(TYPE_FLYING) || has(TYPE_NORMAL) || has(TYPE_BUG);
    case 2: return has(TYPE_WATER) || has(TYPE_FLYING) || has(TYPE_FIRE);
    case 3: return has(TYPE_GHOST) || has(TYPE_POISON) || has(TYPE_BUG);
    default: return true;
  }
}

int16_t pickWildSpecies(uint8_t roll, uint8_t phase) {
  int16_t pool[DEX_COUNT];
  int count = 0;
  uint8_t targetRarity = (roll % 100) < 25 ? R_RARO : R_COMUN;

  auto fill = [&](uint8_t rarity, bool prefer) {
    count = 0;
    for (int16_t dex = 1; dex <= DEX_COUNT; dex++) {
      const DexEntry &entry = DEX_TBL[dex];
      if (entry.rarity != rarity) continue;
      if (prefer && !wildTypePreferred(entry.type1, entry.type2, phase)) continue;
      pool[count++] = dex;
    }
  };

  fill(targetRarity, true);
  if (count == 0) fill(targetRarity, false);
  if (count == 0 && targetRarity == R_RARO) {
    fill(R_COMUN, true);
    if (count == 0) fill(R_COMUN, false);
  }
  return count > 0 ? pool[roll % count] : 1;
}

int16_t wildEvolvedSpeciesForLevel(int16_t baseDex, uint8_t level) {
  int16_t cur = baseDex;
  for (uint8_t guard = 0; guard < 4; guard++) {
    int16_t next = 0;
    for (uint16_t i = 0; i < EVOLUTION_RULE_COUNT; i++) {
      const EvolutionRule &r = EVOLUTION_RULES[i];
      if (r.from == cur && r.condition == EVO_LEVEL && level >= r.minLevel) { next = r.to; break; }
    }
    if (next == 0 || next == cur) break;
    cur = next;
  }
  return cur;
}
