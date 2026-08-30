#pragma once
#include "dex.h"

// Echtes Kampfsystem, portiert von DylanPDao/TamaPoke (moves.h, MIT-Lizenz).
// Typwerte an unsere Reihenfolge angepasst (TYPE_NORMAL..TYPE_FAIRY statt T_NORMAL..T_FAIRY).

enum : uint8_t { MC_PHYS = 0, MC_SPEC = 1, MC_STATUS = 2 };

enum : uint8_t {
  EF_NONE = 0,
  EF_STAGE,       // statMask + stages auf das Ziel
  EF_RECOIL,      // param = Nenner (3 -> 1/3 des verursachten Schadens)
  EF_DRAIN,       // param = % des verursachten Schadens wird geheilt
  EF_FIXED_LVL,   // Schaden = Level des Angreifers, ignoriert Werte
  EF_FIXED,       // param = fester Schaden
  EF_PRIORITY,    // param = Prioritaet, handelt zuerst
  EF_NEVER_MISS,  // wuerfelt keine Genauigkeit
  EF_MULTI,       // 2-5 Treffer, power ist pro Treffer
  EF_HEAL,        // param = % der maximalen Vitalitaet geheilt
  EF_RECHARGE,    // naechste Runde ausgesetzt, angreifbar
  EF_CHARGE,      // Runde 1 = Aufladen; param 1 = unverwundbar waehrenddessen
};

enum : uint8_t { ST_ATK = 1, ST_DEF = 2, ST_SPA = 4, ST_SPD = 8, ST_SPE = 16 };
enum : uint8_t { TG_SELF = 0, TG_FOE = 1 };

enum : uint8_t {
  AIL_NONE = 0, AIL_PARA = 1, AIL_BURN = 2, AIL_POISON = 3,
  AIL_SLEEP = 4, AIL_FREEZE = 5, AIL_CONFUSE = 6,
};

struct MoveEntry {
  const char *name;
  uint8_t type;      // TYPE_*
  uint8_t cat;       // MC_*
  uint8_t power;     // 0 bei Status- und Festschaden-Attacken
  uint8_t acc;       // Prozent; 0 = kann nicht verfehlen
  uint8_t effect;    // EF_*
  int8_t  param;     // Nutzlast des Effekts
  uint8_t statMask;  // ST_* (nur bei EF_STAGE)
  int8_t  stages;    // +/- Stufen (nur bei EF_STAGE)
  uint8_t target;    // TG_*
  uint8_t ailment;   // AIL_*, 0 = keiner
  uint8_t ailChance; // Prozent, 0 = nie
};

// Attackennamen auf Deutsch (offizielle Spielenamen, nach bestem Wissen
// uebersetzt -- ohne Live-Abgleich gegen die Spiele, bei Zweifeln bitte
// melden). Vorher waren hier die englischen Originalnamen aus dem
// portierten DylanPDao/TamaPoke-Movepool stehengeblieben.
#define MOVE_COUNT 90
static const MoveEntry MOVE_TBL[MOVE_COUNT] = {
  { "-", TYPE_NORMAL, MC_STATUS, 0, 0, EF_NONE, 0, 0, 0, TG_SELF, AIL_NONE, 0 },  // 0
  { "TACKLE", TYPE_NORMAL, MC_PHYS, 40, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 1
  { "KRATZER", TYPE_NORMAL, MC_PHYS, 40, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 2
  { "STAMPFER", TYPE_NORMAL, MC_PHYS, 40, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 3
  { "PICKER", TYPE_NORMAL, MC_PHYS, 15, 85, EF_MULTI, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 4
  { "RUCKZUCKHIEB", TYPE_NORMAL, MC_PHYS, 40, 100, EF_PRIORITY, 1, 0, 0, TG_FOE, AIL_NONE, 0 },  // 5
  { "STERNSCHAUER", TYPE_NORMAL, MC_SPEC, 60, 0, EF_NEVER_MISS, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 6
  { "BODYCHECK", TYPE_NORMAL, MC_PHYS, 85, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 7
  { "KAMIKAZE", TYPE_NORMAL, MC_PHYS, 120, 100, EF_RECOIL, 3, 0, 0, TG_FOE, AIL_NONE, 0 },  // 8
  { "HYPERSTRAHL", TYPE_NORMAL, MC_SPEC, 150, 90, EF_RECHARGE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 9
  { "HACKEN", TYPE_FLYING, MC_PHYS, 35, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 10
  { "GLUT", TYPE_FIRE, MC_SPEC, 40, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_BURN, 10 },  // 11
  { "FEUERSCHLAG", TYPE_FIRE, MC_PHYS, 75, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_BURN, 10 },  // 12
  { "FLAMMENWURF", TYPE_FIRE, MC_SPEC, 90, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_BURN, 10 },  // 13
  { "FEUERSTURM", TYPE_FIRE, MC_SPEC, 110, 85, EF_NONE, 0, 0, 0, TG_FOE, AIL_BURN, 10 },  // 14
  { "BLASCHEN", TYPE_WATER, MC_SPEC, 30, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 15
  { "AQUAKNARRE", TYPE_WATER, MC_SPEC, 40, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 16
  { "WASSERFALL", TYPE_WATER, MC_PHYS, 80, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 17
  { "SURFER", TYPE_WATER, MC_SPEC, 90, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 18
  { "HYDROPUMPE", TYPE_WATER, MC_SPEC, 110, 80, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 19
  { "FUNKENSPRUNG", TYPE_ELECTRIC, MC_PHYS, 35, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_PARA, 20 },  // 20
  { "DONNERSCHOCK", TYPE_ELECTRIC, MC_SPEC, 40, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_PARA, 10 },  // 21
  { "DONNERSCHLAG", TYPE_ELECTRIC, MC_PHYS, 75, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_PARA, 10 },  // 22
  { "DONNERBLITZ", TYPE_ELECTRIC, MC_SPEC, 90, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_PARA, 10 },  // 23
  { "DONNER", TYPE_ELECTRIC, MC_SPEC, 110, 70, EF_NONE, 0, 0, 0, TG_FOE, AIL_PARA, 30 },  // 24
  { "ABSORBER", TYPE_GRASS, MC_SPEC, 20, 100, EF_DRAIN, 50, 0, 0, TG_FOE, AIL_NONE, 0 },  // 25
  { "RANKENHIEB", TYPE_GRASS, MC_PHYS, 45, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 26
  { "RASIERBLATT", TYPE_GRASS, MC_PHYS, 55, 95, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 27
  { "MEGASAUGER", TYPE_GRASS, MC_SPEC, 40, 100, EF_DRAIN, 50, 0, 0, TG_FOE, AIL_NONE, 0 },  // 28
  { "SOLARSTRAHL", TYPE_GRASS, MC_SPEC, 120, 100, EF_CHARGE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 29
  { "AURORASTRAHL", TYPE_ICE, MC_SPEC, 65, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 30
  { "EISHIEB", TYPE_ICE, MC_PHYS, 75, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_FREEZE, 10 },  // 31
  { "EISSTRAHL", TYPE_ICE, MC_SPEC, 90, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_FREEZE, 10 },  // 32
  { "BLIZZARD", TYPE_ICE, MC_SPEC, 110, 70, EF_NONE, 0, 0, 0, TG_FOE, AIL_FREEZE, 10 },  // 33
  { "KARATESCHLAG", TYPE_FIGHTING, MC_PHYS, 50, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 34
  { "ERDWURF", TYPE_FIGHTING, MC_PHYS, 0, 100, EF_FIXED_LVL, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 35
  { "UNTERWERFER", TYPE_FIGHTING, MC_PHYS, 80, 80, EF_RECOIL, 4, 0, 0, TG_FOE, AIL_NONE, 0 },  // 36
  { "HOCHSPRUNGKICK", TYPE_FIGHTING, MC_PHYS, 100, 90, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 37
  { "GIFTSTACHEL", TYPE_POISON, MC_PHYS, 15, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_POISON, 30 },  // 38
  { "SAEURE", TYPE_POISON, MC_SPEC, 40, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_POISON, 10 },  // 39
  { "SCHLAMM", TYPE_POISON, MC_SPEC, 65, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_POISON, 30 },  // 40
  { "SCHLAMMBOMBE", TYPE_POISON, MC_SPEC, 90, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_POISON, 30 },  // 41
  { "KNOCHENKEULE", TYPE_GROUND, MC_PHYS, 65, 85, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 42
  { "GRABENBAU", TYPE_GROUND, MC_PHYS, 80, 100, EF_CHARGE, 1, 0, 0, TG_FOE, AIL_NONE, 0 },  // 43
  { "ERDBEBEN", TYPE_GROUND, MC_PHYS, 100, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 44
  { "FLUEGELSCHLAG", TYPE_FLYING, MC_PHYS, 60, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 45
  { "BOHRPICKER", TYPE_FLYING, MC_PHYS, 80, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 46
  { "FLIEGEN", TYPE_FLYING, MC_PHYS, 90, 95, EF_CHARGE, 1, 0, 0, TG_FOE, AIL_NONE, 0 },  // 47
  { "PSYCHOWELLE", TYPE_PSYCHIC, MC_SPEC, 30, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 48
  { "VERWIRRUNG", TYPE_PSYCHIC, MC_SPEC, 50, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_CONFUSE, 10 },  // 49
  { "PSYSTRAHL", TYPE_PSYCHIC, MC_SPEC, 65, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_CONFUSE, 10 },  // 50
  { "PSYCHOKINESE", TYPE_PSYCHIC, MC_SPEC, 90, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 51
  { "KAEFERBISS", TYPE_BUG, MC_PHYS, 30, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 52
  { "NADELRAKETE", TYPE_BUG, MC_PHYS, 25, 95, EF_MULTI, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 53
  { "BLUTSAUGER", TYPE_BUG, MC_PHYS, 80, 100, EF_DRAIN, 50, 0, 0, TG_FOE, AIL_NONE, 0 },  // 54
  { "POWERHORN", TYPE_BUG, MC_PHYS, 120, 85, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 55
  { "KAEFERBRUMMEN", TYPE_BUG, MC_SPEC, 90, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 56
  { "KREUZSCHERE", TYPE_BUG, MC_PHYS, 80, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 57
  { "STEINHIEB", TYPE_FIGHTING, MC_PHYS, 40, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 58
  { "STEINWURF", TYPE_ROCK, MC_PHYS, 50, 90, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 59
  { "STEINHAGEL", TYPE_ROCK, MC_PHYS, 75, 90, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 60
  { "URGEWALT", TYPE_ROCK, MC_SPEC, 60, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 61
  { "LECKER", TYPE_GHOST, MC_PHYS, 30, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_PARA, 30 },  // 62
  { "NACHTSCHATTEN", TYPE_GHOST, MC_SPEC, 0, 100, EF_FIXED_LVL, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 63
  { "SCHATTENBALL", TYPE_GHOST, MC_SPEC, 80, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 64
  { "DRACHENZORN", TYPE_DRAGON, MC_SPEC, 0, 100, EF_FIXED, 40, 0, 0, TG_FOE, AIL_NONE, 0 },  // 65
  { "DRACHENKLAUE", TYPE_DRAGON, MC_PHYS, 80, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 66
  { "TOBTRUNK", TYPE_DRAGON, MC_PHYS, 120, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 67
  { "BISS", TYPE_DARK, MC_PHYS, 60, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 68
  { "KNIRSCHER", TYPE_DARK, MC_PHYS, 80, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 69
  { "EISENSCHAEDEL", TYPE_STEEL, MC_PHYS, 80, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 70
  { "CROSS-KANONE", TYPE_STEEL, MC_SPEC, 80, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 71
  { "FEE-FEUER", TYPE_FAIRY, MC_SPEC, 80, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 72
  { "RAUER KERL", TYPE_FAIRY, MC_PHYS, 90, 90, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 73
  { "MONDGEWALT", TYPE_FAIRY, MC_SPEC, 95, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 74
  { "SCHWERTTANZ", TYPE_NORMAL, MC_STATUS, 0, 0, EF_STAGE, 0, ST_ATK, 2, TG_SELF, AIL_NONE, 0 },  // 75
  { "KONZENTRATOR", TYPE_PSYCHIC, MC_STATUS, 0, 0, EF_STAGE, 0, ST_SPE, 2, TG_SELF, AIL_NONE, 0 },  // 76
  { "BARRIERE", TYPE_PSYCHIC, MC_STATUS, 0, 0, EF_STAGE, 0, ST_DEF, 2, TG_SELF, AIL_NONE, 0 },  // 77
  { "AMNESIE", TYPE_PSYCHIC, MC_STATUS, 0, 0, EF_STAGE, 0, ST_SPD, 2, TG_SELF, AIL_NONE, 0 },  // 78
  { "FINTE", TYPE_DARK, MC_STATUS, 0, 0, EF_STAGE, 0, ST_SPA, 2, TG_SELF, AIL_NONE, 0 },  // 79
  { "DRACHENTANZ", TYPE_DRAGON, MC_STATUS, 0, 0, EF_STAGE, 0, ST_ATK | ST_SPE, 1, TG_SELF, AIL_NONE, 0 },  // 80
  { "STAERKUNG", TYPE_FIGHTING, MC_STATUS, 0, 0, EF_STAGE, 0, ST_ATK | ST_DEF, 1, TG_SELF, AIL_NONE, 0 },  // 81
  { "KNURSCHER", TYPE_NORMAL, MC_STATUS, 0, 0, EF_STAGE, 0, ST_ATK, -1, TG_FOE, AIL_NONE, 0 },  // 82
  { "GRIMASSE", TYPE_NORMAL, MC_STATUS, 0, 0, EF_STAGE, 0, ST_DEF, -1, TG_FOE, AIL_NONE, 0 },  // 83
  { "KREISCHER", TYPE_NORMAL, MC_STATUS, 0, 0, EF_STAGE, 0, ST_DEF, -2, TG_FOE, AIL_NONE, 0 },  // 84
  { "FADENSCHUSS", TYPE_BUG, MC_STATUS, 0, 0, EF_STAGE, 0, ST_SPE, -1, TG_FOE, AIL_NONE, 0 },  // 85
  { "REGENERATION", TYPE_NORMAL, MC_STATUS, 0, 0, EF_HEAL, 50, 0, 0, TG_SELF, AIL_NONE, 0 },  // 86
  { "EI-BOMBE", TYPE_NORMAL, MC_STATUS, 0, 0, EF_HEAL, 50, 0, 0, TG_SELF, AIL_NONE, 0 },  // 87
  { "VERZWEIFLER", TYPE_NORMAL, MC_PHYS, 50, 0, EF_RECOIL, 4, 0, 0, TG_FOE, AIL_NONE, 0 },  // 88
  { "FINSTERAURA", TYPE_DARK, MC_SPEC, 80, 100, EF_NONE, 0, 0, 0, TG_FOE, AIL_NONE, 0 },  // 89
};
