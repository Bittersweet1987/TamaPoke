#pragma once

#include <stdint.h>
#include "moves_real.h"

// Echtes, Attacken-basiertes Kampfsystem, portiert/inspiriert von
// DylanPDao/TamaPoke (MIT-Lizenz), an unsere Werte-/Typformel angepasst.
// Gilt sowohl fuer Wildkaempfe als auch (spaeter) Trainerkaempfe.

struct BattleStats {
  uint16_t hp;
  uint16_t atk, def, spa, spd, spe;
  uint8_t level;
  uint8_t type1 = 0, type2 = 0;
  uint16_t moves[4] = { 0, 0, 0, 0 };  // Index in MOVE_TBL; >255 moegliche Attacken (siehe moves_real.h)
};

struct BattleRuntime {
  BattleStats player, enemy;
  uint16_t playerHp, enemyHp, playerMaxHp, enemyMaxHp;
  uint8_t round = 0;
  uint8_t playerAilment = 0, enemyAilment = 0;        // AIL_*
  uint8_t playerAilCounter = 0, enemyAilCounter = 0;  // Schlaf/Frost: Runden bis Aufwachen/Tauen
  int8_t playerStage[5] = { 0, 0, 0, 0, 0 };  // ATK,DEF,SPA,SPD,SPE (siehe ST_* Reihenfolge)
  int8_t enemyStage[5] = { 0, 0, 0, 0, 0 };
  bool playerRecharge = false, enemyRecharge = false;
};

struct BattleMoveOutcome {
  bool acted;          // false: Status (Schlaf/Para/Frost/Recharge) hat den Zug blockiert
  bool missed;
  uint16_t damage;
  uint8_t effPct;      // 0/50/100/200/400 fuer die UI ("nicht sehr effektiv" etc.)
  bool fainted;
  bool battleEnded;
  bool playerWon;
  uint8_t newAilment;  // gerade zugefuegter Status, 0 wenn keiner
  bool selfHit;        // Verwirrung: sich selbst getroffen statt der Attacke
  bool healed;
  bool recoiled;
};

BattleStats wildBattleStats(int16_t dex, uint8_t level);
BattleRuntime beginBattleRuntime(const BattleStats &player, const BattleStats &enemy);
// Kompatibilitaet fuer das TYPE-Minispiel (Typvorteil-Quiz, nutzt keine
// BattleRuntime, nur eine reine Prozentzahl).
uint8_t battleTypeEffectPct(uint8_t attackType, uint8_t defendType1, uint8_t defendType2);
// side: true = der Spieler handelt, false = der Gegner
BattleMoveOutcome stepBattleMove(BattleRuntime &battle, bool playerSide, uint8_t moveSlot, uint8_t luckRoll);
uint8_t enemyChooseMove(const BattleRuntime &battle, uint8_t luckRoll);
bool playerActsFirst(const BattleRuntime &battle, uint8_t playerMove, uint8_t enemyMove, uint8_t luckRoll);
// Ende-der-Runde-Schaden (Verbrennung/Gift). true wenn dadurch besiegt.
bool applyEndOfTurnAilment(BattleRuntime &battle, bool playerSide);

// Unveraendert gegenueber dem bisherigen System (Wildbegegnungs-Auswahl).
bool canStartWildBattle(bool isEgg, bool sleeping, uint8_t ceremony);
uint8_t wildLevelFor(uint8_t petLevel, uint8_t luckRoll);
int16_t pickWildSpecies(uint8_t roll, uint8_t phase = 1);
// Ausgehend von der (immer zufaellig gewuerfelten) Basis-Art: liefert die
// hoechste Entwicklungsstufe, die bei diesem Level per Level-Bedingung schon
// erreicht sein koennte (Bindungs-/Tag-Nacht-/Statvergleich-Entwicklungen
// zaehlen bewusst nicht, da wilde Pokemon keine Bindungs-/Trainingswerte
// haben) -- so begegnet man im Wildkampf auch Weiterentwicklungen, aber nur
// wenn deren Level-Voraussetzung wirklich erfuellt ist.
int16_t wildEvolvedSpeciesForLevel(int16_t baseDex, uint8_t level);
