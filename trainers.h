#pragma once
#include <stdint.h>

// Die komplette Ladder fuer 5 Regionen: je 8 Arenleiter + Top 4 + Meister,
// mit echten Teams/Leveln, portiert von DylanPDao/TamaPoke (trainers.h,
// MIT-Lizenz, dort per Hand aus den Original-/Remake-Spielen uebertragen und
// bei Kanto/Johto/Hoenn/Sinnoh gegen die pokered/pokecrystal/pokeemerald/
// pokeplatinum-Disassemblies verifiziert; Unova ist laut Quelle NICHT
// gegengeprueft, Level dort als Naeherung behandeln). Jedes Team ist eine
// Liste aus (Dex, Level)-Paaren -- die Attacken werden zur Laufzeit wie bei
// Wildpokemon aus der echten Lernliste abgeleitet (siehe wildBattleStats()
// in battle.cpp), damit hier keine Movesets von Hand gepflegt werden
// muessen.
//
// Hard Mode (siehe gymHardMode in TamaPoke.ino) nutzt dieselben Teams mit
// einem Level-Aufschlag statt eigener Tabellen -- im Original sind das
// perfekte IVs, die unser vereinfachtes Kampfsystem nicht kennt.
//
// Bewusst keine zusaetzliche Kappung: ein Leiter bringt immer sein ganzes
// Team, das eigene Team tritt komplett an -- Abnutzung ist die Schwierigkeit.

#define GYM_COUNT 13         // 8 Arenen + 4x Top-Vier + Meister (pro Region)
#define GYM_BADGE_COUNT 8    // davon zaehlen die ersten 8 als "Orden"
#define GYM_TEAM_MAX 6
#define GYM_REGION_COUNT 5

struct GymMon {
  int16_t dex;
  uint8_t level;
};

struct GymLeader {
  const char *name;
  uint8_t type;   // TYPE_* -- Spezialisierung, siehe types_real.h
  GymMon team[GYM_TEAM_MAX];
  uint8_t teamSize;
};

static const char *const GYM_REGION_NAMES[GYM_REGION_COUNT] = {
  "KANTO", "JOHTO", "HOENN", "SINNOH", "UNOVA",
};

// Echte deutsche Ordensnamen (nicht der Arenleiter-Name) je Region, Reihenfolge
// wie GYM_LEADERS_<REGION>[0..7]/badges.h. Ueber PokeWiki (deutsches
// Pokemon-Wiki) verifiziert; ASCII-transliteriert (Umlaute), da der GFX-Font
// keine Umlaute kann -- siehe dexName()/i18n.h fuer dasselbe Muster.
static const char *const GYM_BADGE_NAMES[GYM_REGION_COUNT][GYM_BADGE_COUNT] = {
  { "FELSORDEN", "QUELLORDEN", "DONNERORDEN", "FARBORDEN", "SEELENORDEN", "SUMPFORDEN", "VULKANORDEN", "ERDORDEN" },
  { "FLUEGELORDEN", "INSEKTORDEN", "BASISORDEN", "PHANTOMORDEN", "FAUSTORDEN", "STAHLORDEN", "EISORDEN", "DRACHENORDEN" },
  { "STEINORDEN", "KNOECHELORDEN", "DYNAMO-ORDEN", "HITZEORDEN", "BALANCEORDEN", "FEDERORDEN", "MENTALORDEN", "SCHAUERORDEN" },
  { "KOHLEORDEN", "WALDORDEN", "RELIKTORDEN", "BERGORDEN", "FENNORDEN", "MINENORDEN", "FIRNORDEN", "LICHTORDEN" },
  { "GRUNDORDEN", "GIFTORDEN", "KAEFERORDEN", "VOLTORDEN", "SEISMO-ORDEN", "JETORDEN", "LEGENDENORDEN", "WELLENORDEN" },
};

// Reihenfolge je Region = Kampfreihenfolge (Sieg n schaltet Gegner n+1 frei,
// siehe gymUnlocked() in TamaPoke.ino). Index 0-7 = Arenen (Seite "ARENEN"),
// 8-12 = Top Vier + Meister (Seite "TOP 4"). Deutsche Arenleiter-Namen bei
// Kanto (nur Rocko unterscheidet sich vom Englischen); die anderen Regionen
// behalten die Original-Namen, da im deutschen Spiel meist identisch.
static const GymLeader GYM_LEADERS_KANTO[GYM_COUNT] = {
  { "ROCKO",    TYPE_ROCK,     { { 74, 12 }, { 95, 14 } }, 2 },
  { "MISTY",    TYPE_WATER,    { { 120, 18 }, { 121, 21 } }, 2 },
  { "SURGE",    TYPE_ELECTRIC, { { 100, 21 }, { 25, 18 }, { 26, 24 } }, 3 },
  { "ERIKA",    TYPE_GRASS,    { { 71, 29 }, { 114, 24 }, { 45, 29 } }, 3 },
  { "KOGA",     TYPE_POISON,   { { 109, 37 }, { 89, 39 }, { 109, 37 }, { 110, 43 } }, 4 },
  { "SABRINA",  TYPE_PSYCHIC,  { { 64, 38 }, { 122, 37 }, { 49, 38 }, { 65, 43 } }, 4 },
  { "BLAINE",   TYPE_FIRE,     { { 58, 42 }, { 77, 40 }, { 78, 42 }, { 59, 47 } }, 4 },
  { "GIOVANNI", TYPE_GROUND,   { { 111, 45 }, { 51, 42 }, { 31, 44 }, { 34, 45 }, { 112, 50 } }, 5 },
  { "LORELEI",  TYPE_ICE,      { { 87, 52 }, { 91, 51 }, { 80, 52 }, { 124, 54 }, { 131, 54 } }, 5 },
  { "BRUNO",    TYPE_FIGHTING, { { 95, 51 }, { 107, 53 }, { 106, 53 }, { 95, 54 }, { 68, 56 } }, 5 },
  { "AGATHA",   TYPE_GHOST,    { { 94, 54 }, { 42, 54 }, { 93, 53 }, { 24, 56 }, { 94, 58 } }, 5 },
  { "LANCE",    TYPE_DRAGON,   { { 130, 56 }, { 148, 54 }, { 148, 54 }, { 142, 58 }, { 149, 60 } }, 5 },
  { "MEISTER",  TYPE_NORMAL,   { { 18, 61 }, { 65, 59 }, { 112, 61 }, { 59, 63 }, { 103, 61 }, { 9, 65 } }, 6 },
};

static const GymLeader GYM_LEADERS_JOHTO[GYM_COUNT] = {
  { "FALKNER",  TYPE_FLYING,   { { 16, 7 }, { 17, 9 } }, 2 },
  { "BUGSY",    TYPE_BUG,      { { 11, 14 }, { 14, 14 }, { 123, 16 } }, 3 },
  { "WHITNEY",  TYPE_NORMAL,   { { 35, 18 }, { 241, 20 } }, 2 },
  { "MORTY",    TYPE_GHOST,    { { 92, 21 }, { 93, 21 }, { 94, 25 }, { 93, 23 } }, 4 },
  { "CHUCK",    TYPE_FIGHTING, { { 57, 27 }, { 62, 30 } }, 2 },
  { "JASMINE",  TYPE_STEEL,    { { 81, 30 }, { 81, 30 }, { 208, 35 } }, 3 },
  { "PRYCE",    TYPE_ICE,      { { 86, 27 }, { 87, 29 }, { 221, 31 } }, 3 },
  { "CLAIR",    TYPE_DRAGON,   { { 148, 37 }, { 148, 37 }, { 148, 37 }, { 230, 40 } }, 4 },
  { "WILL",     TYPE_PSYCHIC,  { { 178, 40 }, { 124, 41 }, { 103, 41 }, { 80, 41 }, { 178, 42 } }, 5 },
  { "KOGA",     TYPE_POISON,   { { 168, 40 }, { 49, 41 }, { 205, 43 }, { 89, 42 }, { 169, 44 } }, 5 },
  { "BRUNO",    TYPE_FIGHTING, { { 237, 42 }, { 106, 42 }, { 107, 42 }, { 95, 43 }, { 68, 46 } }, 5 },
  { "KAREN",    TYPE_DARK,     { { 197, 42 }, { 45, 42 }, { 94, 45 }, { 198, 44 }, { 229, 47 } }, 5 },
  { "LANCE",    TYPE_DRAGON,   { { 130, 44 }, { 149, 47 }, { 149, 47 }, { 142, 46 }, { 6, 46 }, { 149, 50 } }, 6 },
};

// Hoenn: Smaragd-Fassung (Juan achte Arenaleiterin statt Wallace, dafuer
// Wallace als Meister -- entspricht keiner Rubin/Saphir-Fassung 1:1, siehe
// Original-Kommentar in DylanPDao/TamaPoke).
static const GymLeader GYM_LEADERS_HOENN[GYM_COUNT] = {
  { "ROXANNE",  TYPE_ROCK,     { { 74, 12 }, { 74, 12 }, { 299, 15 } }, 3 },
  { "BRAWLY",   TYPE_FIGHTING, { { 66, 16 }, { 307, 16 }, { 296, 19 } }, 3 },
  { "WATTSON",  TYPE_ELECTRIC, { { 100, 20 }, { 309, 20 }, { 82, 22 }, { 310, 24 } }, 4 },
  { "FLANNERY", TYPE_FIRE,     { { 322, 24 }, { 218, 24 }, { 323, 26 }, { 324, 29 } }, 4 },
  { "NORMAN",   TYPE_NORMAL,   { { 327, 27 }, { 288, 27 }, { 264, 29 }, { 289, 31 } }, 4 },
  { "WINONA",   TYPE_FLYING,   { { 333, 29 }, { 357, 29 }, { 279, 30 }, { 227, 31 }, { 334, 33 } }, 5 },
  { "TATE",     TYPE_PSYCHIC,  { { 344, 41 }, { 178, 41 }, { 337, 42 }, { 338, 42 } }, 4 },
  { "JUAN",     TYPE_WATER,    { { 370, 41 }, { 340, 41 }, { 364, 43 }, { 342, 43 }, { 230, 46 } }, 5 },
  { "SIDNEY",   TYPE_DARK,     { { 262, 46 }, { 275, 48 }, { 332, 46 }, { 342, 48 }, { 359, 49 } }, 5 },
  { "PHOEBE",   TYPE_GHOST,    { { 356, 48 }, { 354, 49 }, { 302, 50 }, { 354, 49 }, { 356, 51 } }, 5 },
  { "GLACIA",   TYPE_ICE,      { { 364, 50 }, { 362, 50 }, { 364, 52 }, { 362, 52 }, { 365, 53 } }, 5 },
  { "DRAKE",    TYPE_DRAGON,   { { 372, 52 }, { 334, 54 }, { 230, 53 }, { 330, 53 }, { 373, 55 } }, 5 },
  { "WALLACE",  TYPE_WATER,    { { 321, 57 }, { 73, 55 }, { 272, 56 }, { 340, 56 }, { 130, 56 }, { 350, 58 } }, 6 },
};

// Sinnoh: Platin-Reihenfolge (Fantina dritte statt fuenfte Arenaleiterin,
// siehe Original-Kommentar -- der Level-Anstieg ist sonst nicht monoton).
static const GymLeader GYM_LEADERS_SINNOH[GYM_COUNT] = {
  { "ROARK",    TYPE_ROCK,     { { 74, 12 }, { 95, 12 }, { 408, 14 } }, 3 },
  { "GARDENIA", TYPE_GRASS,    { { 387, 20 }, { 421, 20 }, { 407, 22 } }, 3 },
  { "FANTINA",  TYPE_GHOST,    { { 355, 24 }, { 93, 24 }, { 429, 26 } }, 3 },
  { "MAYLENE",  TYPE_FIGHTING, { { 307, 28 }, { 67, 29 }, { 448, 32 } }, 3 },
  { "WAKE",     TYPE_WATER,    { { 130, 33 }, { 195, 34 }, { 419, 37 } }, 3 },
  { "BYRON",    TYPE_STEEL,    { { 82, 37 }, { 208, 38 }, { 411, 41 } }, 3 },
  { "CANDICE",  TYPE_ICE,      { { 215, 40 }, { 221, 40 }, { 460, 42 }, { 478, 44 } }, 4 },
  { "VOLKNER",  TYPE_ELECTRIC, { { 135, 46 }, { 26, 46 }, { 405, 48 }, { 466, 50 } }, 4 },
  { "AARON",    TYPE_BUG,      { { 469, 49 }, { 212, 49 }, { 416, 50 }, { 214, 51 }, { 452, 53 } }, 5 },
  { "BERTHA",   TYPE_GROUND,   { { 340, 50 }, { 472, 53 }, { 450, 52 }, { 76, 52 }, { 464, 55 } }, 5 },
  { "FLINT",    TYPE_FIRE,     { { 229, 52 }, { 136, 55 }, { 78, 53 }, { 392, 55 }, { 467, 57 } }, 5 },
  { "LUCIAN",   TYPE_PSYCHIC,  { { 122, 53 }, { 196, 55 }, { 437, 54 }, { 65, 56 }, { 475, 59 } }, 5 },
  { "CYNTHIA",  TYPE_DRAGON,   { { 442, 58 }, { 407, 58 }, { 468, 60 }, { 448, 60 }, { 350, 58 }, { 445, 62 } }, 6 },
};

// Unova: B2W2, laut Original-Quelle NICHT gegen eine Disassembly verifiziert
// (kein Gen-5-Decomp verfuegbar) -- Level hier als Naeherung behandeln.
static const GymLeader GYM_LEADERS_UNOVA[GYM_COUNT] = {
  { "CHEREN",   TYPE_NORMAL,   { { 504, 11 }, { 506, 13 } }, 2 },
  { "ROXIE",    TYPE_POISON,   { { 109, 16 }, { 544, 18 } }, 2 },
  { "BURGH",    TYPE_BUG,      { { 541, 21 }, { 557, 21 }, { 542, 23 } }, 3 },
  { "ELESA",    TYPE_ELECTRIC, { { 587, 25 }, { 180, 25 }, { 596, 27 } }, 3 },
  { "CLAY",     TYPE_GROUND,   { { 552, 29 }, { 28, 29 }, { 530, 31 } }, 3 },
  { "SKYLA",    TYPE_FLYING,   { { 528, 33 }, { 227, 33 }, { 581, 35 } }, 3 },
  { "DRAYDEN",  TYPE_DRAGON,   { { 621, 43 }, { 330, 43 }, { 612, 46 } }, 3 },
  { "MARLON",   TYPE_WATER,    { { 537, 49 }, { 321, 49 }, { 593, 51 } }, 3 },
  { "SHAUNTAL", TYPE_GHOST,    { { 563, 56 }, { 426, 56 }, { 623, 56 }, { 609, 58 } }, 4 },
  { "GRIMSLEY", TYPE_DARK,     { { 510, 56 }, { 560, 56 }, { 553, 56 }, { 625, 58 } }, 4 },
  { "CAITLIN",  TYPE_PSYCHIC,  { { 518, 56 }, { 561, 56 }, { 579, 56 }, { 576, 58 } }, 4 },
  { "MARSHAL",  TYPE_FIGHTING, { { 297, 56 }, { 539, 56 }, { 534, 56 }, { 620, 58 } }, 4 },
  { "IRIS",     TYPE_DRAGON,   { { 635, 59 }, { 621, 57 }, { 306, 57 }, { 567, 57 }, { 131, 57 }, { 612, 59 } }, 6 },
};

static const GymLeader *const GYM_REGION_SETS[GYM_REGION_COUNT] = {
  GYM_LEADERS_KANTO, GYM_LEADERS_JOHTO, GYM_LEADERS_HOENN, GYM_LEADERS_SINNOH, GYM_LEADERS_UNOVA,
};
