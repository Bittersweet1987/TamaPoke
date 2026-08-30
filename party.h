#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "pet.h"  // MOVE_SLOTS, DexMon, extern Pet pet

// Die Party: aktive Kampf-Pokemon. Statt eines eigenen Lagers greift die Party
// direkt auf den bestehenden Pokedex zu -- jede bereits gezuechtete oder
// gefangene Art laesst sich zuweisen (Party::addFromDex). Entfernen loescht
// nur den Party-Platz; die Art bleibt unabhaengig davon im Pokedex verzeichnet
// (das war sie schon vorher, siehe pet.isRegistered()/isCaught()).
#define PARTY_SLOTS 6

// Ein aktives Party-Mitglied.
// Verweist nur auf ein Pokedex-Individuum (Fang oder Zucht), speichert selbst
// keine Werte-Momentaufnahme. So zeigen Team und Kampf immer den AKTUELLEN
// Stand von pet.dexMonsCaught/dexMonsBred -- levelt z.B. das gerade
// aufgezogene Haustier weiter, zieht das Team automatisch mit (vorher war
// hier beim Hinzufuegen eine Kopie eingefroren, die nie wieder aktualisiert
// wurde).
struct PartyMon {
  int16_t dex = 0;       // Pokedex-Nummer, 0 = leerer Platz
  bool isCaught = true;  // welche der zwei DexMon-Quellen gilt (siehe pet.h)
  char nick[12] = "";    // eigener Spitzname, unabhaengig vom Individuum

  bool empty() const { return dex < 1; }
};

class Party {
public:
  PartyMon slots[PARTY_SLOTS];

  void begin();                 // aus NVS laden
  uint8_t count() const;
  bool isFull() const { return count() >= PARTY_SLOTS; }
  int firstFree() const;        // Index des ersten leeren Platzes, -1 wenn voll
  bool add(const PartyMon &m);  // in den ersten freien Platz; false wenn voll
  // Neues Team-Mitglied direkt aus dem Pokedex erzeugen. isCaught waehlt die
  // Herkunft (Fang- vs. Zucht-Speicher, siehe pet.h DexMon). false wenn dort
  // noch kein Exemplar gespeichert ist oder das Team voll ist.
  bool addFromDex(int16_t dex, bool isCaught);
  void replaceAt(uint8_t i, const PartyMon &m);
  void releaseAt(uint8_t i);    // Platz freigeben -- die Art bleibt im Pokedex
  void save();

  // Das aktuelle Pokedex-Individuum hinter einem Team-Platz (live, keine
  // Kopie) -- liefert Level/Gene/Training/Shiny fuer Anzeige und Kampf.
  const DexMon &sourceOf(const PartyMon &m) const {
    return m.isCaught ? pet.dexMonsCaught[m.dex] : pet.dexMonsBred[m.dex];
  }

  // Kampfwerte eines Party-Mitglieds, dieselbe Formel wie beim aktiven Haustier
  uint16_t atkOf(const PartyMon &m) const;
  uint16_t defOf(const PartyMon &m) const;
  uint16_t speOf(const PartyMon &m) const;
  uint16_t vitOf(const PartyMon &m) const;
  uint16_t spaOf(const PartyMon &m) const;
  uint16_t spdOf(const PartyMon &m) const;

private:
  Preferences prefs;
};

extern Party party;
