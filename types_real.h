#pragma once
#include <stdint.h>
#include "dex.h"  // TYPE_NONE, TYPE_NORMAL, ... bereits vorhanden

// Typeneffektivitaet in Zehnteln (0 immun, 5 nicht sehr, 10 neutral, 20 super).
// Portiert von DylanPDao/TamaPoke (types.h), an unsere Typreihenfolge angepasst
// (TYPE_NONE=0 vorne, sonst identische Reihenfolge).
#define TYPE_COUNT_REAL 18
static const uint8_t TYPE_FX[TYPE_COUNT_REAL][TYPE_COUNT_REAL] = {
  { 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,  5,  0, 10, 10,  5, 10 },  // normal
  { 10,  5,  5, 10, 20, 20, 10, 10, 10, 10, 10, 20,  5, 10,  5, 10, 20, 10 },  // fire
  { 10, 20,  5, 10,  5, 10, 10, 10, 20, 10, 10, 10, 20, 10,  5, 10, 10, 10 },  // water
  { 10, 10, 20,  5,  5, 10, 10, 10,  0, 20, 10, 10, 10, 10,  5, 10, 10, 10 },  // electric
  { 10,  5, 20, 10,  5, 10, 10,  5, 20,  5, 10,  5, 20, 10,  5, 10,  5, 10 },  // grass
  { 10,  5,  5, 10, 20,  5, 10, 10, 20, 20, 10, 10, 10, 10, 20, 10,  5, 10 },  // ice
  { 20, 10, 10, 10, 10, 20, 10,  5, 10,  5,  5,  5, 20,  0, 10, 20, 20,  5 },  // fighting
  { 10, 10, 10, 10, 20, 10, 10,  5,  5, 10, 10, 10,  5,  5, 10, 10,  0, 20 },  // poison
  { 10, 20, 10, 20,  5, 10, 10, 20, 10,  0, 10,  5, 20, 10, 10, 10, 20, 10 },  // ground
  { 10, 10, 10,  5, 20, 10, 20, 10, 10, 10, 10, 20,  5, 10, 10, 10,  5, 10 },  // flying
  { 10, 10, 10, 10, 10, 10, 20, 20, 10, 10,  5, 10, 10, 10, 10,  0,  5, 10 },  // psychic
  { 10,  5, 10, 10, 20, 10,  5,  5, 10,  5, 20, 10, 10,  5, 10, 20,  5,  5 },  // bug
  { 10, 20, 10, 10, 10, 20,  5, 10,  5, 20, 10, 20, 10, 10, 10, 10,  5, 10 },  // rock
  {  0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 20, 10, 10, 20, 10,  5, 10, 10 },  // ghost
  { 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 20, 10,  5,  0 },  // dragon
  { 10, 10, 10, 10, 10, 10,  5, 10, 10, 10, 20, 10, 10, 20, 10,  5, 10,  5 },  // dark
  { 10,  5,  5,  5, 10, 20, 10, 10, 10, 10, 10, 10, 20, 10, 10, 10,  5, 20 },  // steel
  { 10,  5, 10, 10, 10, 10, 20,  5, 10, 10, 10, 10, 10, 10, 20, 20,  5, 10 },  // fairy
};

// TYPE_FX ist nach TYPE_NORMAL(1)..TYPE_FAIRY(18) indiziert (unser Offset -1 zum Array).
static inline uint16_t typeEffPct(uint8_t atk, uint8_t def1, uint8_t def2) {
  if (atk < TYPE_NORMAL || atk > TYPE_FAIRY) return 100;
  int ai = atk - TYPE_NORMAL, d1 = def1 - TYPE_NORMAL;
  if (d1 < 0 || d1 >= TYPE_COUNT_REAL) return 100;
  uint16_t e = TYPE_FX[ai][d1];
  int d2 = def2 - TYPE_NORMAL;
  e *= (d2 >= 0 && d2 < TYPE_COUNT_REAL && def2 != TYPE_NONE) ? TYPE_FX[ai][d2] : 10;
  return e;  // Zehntel x Zehntel = Prozent
}

static inline uint16_t typeEffVsDex(uint8_t atk, int16_t dex) {
  if (dex < 1 || dex > DEX_COUNT) return 100;
  return typeEffPct(atk, DEX_TBL[dex].type1, DEX_TBL[dex].type2);
}

// Sofortiger Typvorteil (STAB): 1.5x wenn die Attacke zu einem der eigenen Typen passt.
static inline bool hasStab(int16_t dex, uint8_t moveType) {
  if (dex < 1 || dex > DEX_COUNT) return false;
  return DEX_TBL[dex].type1 == moveType || DEX_TBL[dex].type2 == moveType;
}
