#include "party.h"
#include <stdlib.h>
#include <string.h>
#include "dex.h"
#include "pet.h"

Party party;

// Absichtlich derselbe NVS-Namespace wie das Haustier: ein Werksreset raeumt
// die Party mit auf, statt sie unangetastet zu lassen.
// Eigener Schluessel "party2": PartyMon wurde von einer Werte-Momentaufnahme
// auf einen reinen Verweis auf das Pokedex-Individuum umgebaut (anderes
// Speicherlayout) -- alte "party"-Bytes unter dem alten Schluessel bleiben
// harmlos ungenutzt liegen, statt als Datenmuell reinterpretiert zu werden.
void Party::begin() {
  for (auto &s : slots) s = PartyMon();
  prefs.begin("tamapoke", false);

  size_t stored = prefs.getBytesLength("party2");
  if (stored == sizeof(slots)) prefs.getBytes("party2", slots, sizeof(slots));

  for (auto &s : slots) {
    if (s.dex < 1 || s.dex > DEX_COUNT) s.dex = 0;
    s.nick[sizeof(s.nick) - 1] = 0;
  }
}

void Party::save() {
  prefs.putBytes("party2", slots, sizeof(slots));
}

uint8_t Party::count() const {
  uint8_t n = 0;
  for (auto &s : slots) if (!s.empty()) n++;
  return n;
}

int Party::firstFree() const {
  for (int i = 0; i < PARTY_SLOTS; i++)
    if (slots[i].empty()) return i;
  return -1;
}

bool Party::add(const PartyMon &m) {
  int i = firstFree();
  if (i < 0) return false;
  slots[i] = m;
  save();
  return true;
}

// Verknuepft ein Team-Mitglied mit einer bereits gespeicherten Pokedex-Art
// (Fang oder Zucht). Speichert keine Werte-Kopie -- Team/Kampf lesen ueber
// sourceOf() immer live, damit z.B. ein weiter wachsendes Haustier im Team
// mitzieht statt am Beitrittstag einzufrieren.
bool Party::addFromDex(int16_t dex, bool isCaught) {
  if (dex < 1 || dex > DEX_COUNT) return false;
  const DexMon &src = isCaught ? pet.dexMonsCaught[dex] : pet.dexMonsBred[dex];
  if (src.empty()) return false;
  if (isFull()) return false;
  for (auto &s : slots)  // schon im Team: keine zweite Kopie derselben Art zulassen
    if (!s.empty() && s.dex == dex) return false;

  PartyMon m;
  m.dex = dex;
  m.isCaught = isCaught;
  return add(m);
}

void Party::replaceAt(uint8_t i, const PartyMon &m) {
  if (i >= PARTY_SLOTS) return;
  slots[i] = m;
  save();
}

// Entfernt ein Team-Mitglied. Die Art bleibt im Pokedex verzeichnet -- daran
// aendert sich nichts, das war schon vorher unabhaengig von der Party.
void Party::releaseAt(uint8_t i) {
  if (i >= PARTY_SLOTS) return;
  slots[i] = PartyMon();
  save();
}

// Identisch zu calcStat() in pet.cpp: base * gen / 100 + Level + Training.
static uint16_t calcStat(uint8_t base, uint8_t gene, uint16_t lvl, uint8_t tr) {
  return (uint16_t)base * gene / 100 + lvl + tr;
}

uint16_t Party::atkOf(const PartyMon &m) const {
  if (m.empty()) return 0;
  const DexMon &s = sourceOf(m);
  return calcStat(DEX_TBL[m.dex].bAtk, s.geneAtk, s.level, s.trAtk);
}
uint16_t Party::defOf(const PartyMon &m) const {
  if (m.empty()) return 0;
  const DexMon &s = sourceOf(m);
  return calcStat(DEX_TBL[m.dex].bDef, s.geneDef, s.level, s.trDef);
}
uint16_t Party::speOf(const PartyMon &m) const {
  if (m.empty()) return 0;
  const DexMon &s = sourceOf(m);
  return calcStat(DEX_TBL[m.dex].bSpe, s.geneSpe, s.level, s.trSpe);
}
uint16_t Party::vitOf(const PartyMon &m) const {
  if (m.empty()) return 0;
  const DexMon &s = sourceOf(m);
  return calcStat(DEX_TBL[m.dex].bHp, s.geneHp, s.level, 10);
}
uint16_t Party::spaOf(const PartyMon &m) const {
  if (m.empty()) return 0;
  const DexMon &s = sourceOf(m);
  return calcStat(DEX_TBL[m.dex].bSpA, s.geneSpA, s.level, s.trSpA);
}
uint16_t Party::spdOf(const PartyMon &m) const {
  if (m.empty()) return 0;
  const DexMon &s = sourceOf(m);
  return calcStat(DEX_TBL[m.dex].bSpD, s.geneSpD, s.level, s.trSpD);
}
