#!/usr/bin/env python3
"""Ersetzt learnsets_real.h komplett durch ECHTE Level-Aufstiegs-Lernlisten aus
PokeAPI -- nur fuer die 90 Attacken, die tatsaechlich in moves_real.h existieren,
mit dem echten Level, auf dem die jeweilige Art sie in den Spielen lernt.

Vorher waren die Lernlisten von DylanPDao/TamaPoke portiert und enthielten u.a.
erfundene "Bonus"-Eintraege (Level 0) ohne Bezug zu den echten Spielen. Diese
Version verwirft das und baut die Liste ausschliesslich aus PokeAPI-Level-
Aufstiegs-Daten (method="level-up") neu auf, fuer alle 1025 Arten (behebt
nebenbei die fehlende Abdeckung fuer Dex 650-1025).

Nutzung: python3 tools/gen_learnsets_real.py
Schreibt direkt nach ../learnsets_real.h.
"""
import json
import re
import sys
import time
import urllib.request

API = "https://pokeapi.co/api/v2"
HEADERS = {"User-Agent": "TamaPoke-learnset-gen/1.0 (+https://github.com/Bittersweet1987/TamaPoke)"}
DEX_COUNT = 1025

# Reihenfolge unserer 90 Attacken in MOVE_TBL (Index = Move-ID, siehe moves_real.h).
OUR_MOVES = [
    "-", "TACKLE", "KRATZER", "STAMPFER", "PICKER", "RUCKZUCKHIEB", "STERNSCHAUER",
    "BODYCHECK", "KAMIKAZE", "HYPERSTRAHL", "HACKEN", "GLUT", "FEUERSCHLAG",
    "FLAMMENWURF", "FEUERSTURM", "BLASCHEN", "AQUAKNARRE", "WASSERFALL", "SURFER",
    "HYDROPUMPE", "FUNKENSPRUNG", "DONNERSCHOCK", "DONNERSCHLAG", "DONNERBLITZ",
    "DONNER", "ABSORBER", "RANKENHIEB", "RASIERBLATT", "MEGASAUGER", "SOLARSTRAHL",
    "AURORASTRAHL", "EISHIEB", "EISSTRAHL", "BLIZZARD", "KARATESCHLAG", "ERDWURF",
    "UNTERWERFER", "HOCHSPRUNGKICK", "GIFTSTACHEL", "SAEURE", "SCHLAMM",
    "SCHLAMMBOMBE", "KNOCHENKEULE", "GRABENBAU", "ERDBEBEN", "FLUEGELSCHLAG",
    "BOHRPICKER", "FLIEGEN", "PSYCHOWELLE", "VERWIRRUNG", "PSYSTRAHL",
    "PSYCHOKINESE", "KAEFERBISS", "NADELRAKETE", "BLUTSAUGER", "POWERHORN",
    "KAEFERBRUMMEN", "KREUZSCHERE", "STEINHIEB", "STEINWURF", "STEINHAGEL",
    "URGEWALT", "LECKER", "NACHTSCHATTEN", "SCHATTENBALL", "DRACHENZORN",
    "DRACHENKLAUE", "TOBTRUNK", "BISS", "KNIRSCHER", "EISENSCHAEDEL",
    "CROSS-KANONE", "FEE-FEUER", "RAUER KERL", "MONDGEWALT", "SCHWERTTANZ",
    "KONZENTRATOR", "BARRIERE", "AMNESIE", "FINTE", "DRACHENTANZ", "STAERKUNG",
    "KNURSCHER", "GRIMASSE", "KREISCHER", "FADENSCHUSS", "REGENERATION",
    "EI-BOMBE", "VERZWEIFLER", "FINSTERAURA",
]

# Neueste zuerst: bei mehreren Fundstellen fuer dieselbe Attacke+Art wird die
# hoechstpriorisierte Version-Group genommen (Scarlet/Violet deckt aber nicht
# den ganzen Nationaldex ab, daher Fallback auf aeltere Spiele noetig).
VERSION_PRIORITY = [
    "scarlet-violet", "sword-shield", "ultra-sun-ultra-moon", "sun-moon",
    "omega-ruby-alpha-sapphire", "x-y", "black-2-white-2", "black-white",
    "heartgold-soulsilver", "platinum", "diamond-pearl", "firered-leafgreen",
    "emerald", "ruby-sapphire", "crystal", "gold-silver", "yellow", "red-blue",
]

UMLAUT = str.maketrans({"ä": "ae", "ö": "oe", "ü": "ue", "ß": "ss",
                         "Ä": "Ae", "Ö": "Oe", "Ü": "Ue"})


def ascii_fold_upper(s):
    s = s.translate(UMLAUT)
    s = re.sub(r"[^A-Za-z0-9 .,!?'\-]", "", s)
    return s.upper().strip()


def get(url, tries=4):
    req = urllib.request.Request(url, headers=HEADERS)
    for i in range(tries):
        try:
            with urllib.request.urlopen(req, timeout=20) as r:
                return json.loads(r.read().decode("utf-8"))
        except Exception as e:
            if i == tries - 1:
                raise
            time.sleep(1.5)


def main():
    print("Lade komplette Attackenliste von PokeAPI...", file=sys.stderr)
    all_moves = get(f"{API}/move?limit=3000")["results"]  # [{name, url}]

    print(f"Ordne {len(OUR_MOVES) - 1} bekannte Attacken ihren PokeAPI-Eintraegen zu...", file=sys.stderr)
    our_by_german = {name: idx for idx, name in enumerate(OUR_MOVES) if name != "-"}
    slug_to_our_id = {}  # PokeAPI-Slug -> unsere Move-ID
    matched_names = set()
    for i, m in enumerate(all_moves):
        try:
            data = get(m["url"])
        except Exception as e:
            print(f"# WARNUNG: Attacke {m['name']} nicht ladbar: {e}", file=sys.stderr)
            continue
        de = None
        for n in data.get("names", []):
            if n["language"]["name"] == "de":
                de = ascii_fold_upper(n["name"])
                break
        if de and de in our_by_german:
            slug_to_our_id[m["name"]] = our_by_german[de]
            matched_names.add(de)
        if (i + 1) % 150 == 0:
            print(f"  ...{i + 1}/{len(all_moves)} Attacken durchsucht", file=sys.stderr)
        time.sleep(0.02)

    missing = [n for n in our_by_german if n not in matched_names]
    print(f"Zugeordnet: {len(slug_to_our_id)} PokeAPI-Attacken -> {len(matched_names)}/{len(our_by_german)} unserer Attacken.",
          file=sys.stderr)
    if missing:
        print(f"# Nicht gefunden (bleiben ungenutzt): {missing}", file=sys.stderr)

    version_rank = {v: i for i, v in enumerate(VERSION_PRIORITY)}

    per_species = []  # Liste von Listen [(move_id, level), ...] pro Dex 1..DEX_COUNT
    for dex in range(1, DEX_COUNT + 1):
        try:
            poke = get(f"{API}/pokemon/{dex}/")
        except Exception as e:
            print(f"# FEHLER bei Dex {dex}: {e}", file=sys.stderr)
            per_species.append([])
            continue
        best = {}  # move_id -> (rank, level)  -- niedrigster rank gewinnt
        for mv in poke.get("moves", []):
            slug = mv["move"]["name"]
            if slug not in slug_to_our_id:
                continue
            move_id = slug_to_our_id[slug]
            for vgd in mv.get("version_group_details", []):
                if vgd["move_learn_method"]["name"] != "level-up":
                    continue
                vg = vgd["version_group"]["name"]
                rank = version_rank.get(vg, 999)
                lvl = vgd["level_learned_at"]
                if move_id not in best or rank < best[move_id][0]:
                    best[move_id] = (rank, lvl)
        entries = sorted(((mid, lvl) for mid, (rank, lvl) in best.items()), key=lambda e: e[1])
        per_species.append(entries)
        print(f"# {dex}/{DEX_COUNT} {poke['name']}: {len(entries)} Attacken", file=sys.stderr)
        time.sleep(0.02)

    total_entries = sum(len(e) for e in per_species)
    print(f"Fertig: {total_entries} Lerneintraege insgesamt fuer {DEX_COUNT} Arten. Schreibe learnsets_real.h...",
          file=sys.stderr)

    lines = []
    lines.append("#pragma once")
    lines.append("#include \"dex.h\"")
    lines.append("")
    lines.append("// Lernlisten (Level -> Attacke) fuer Dex 1-{}, GENERIERT von".format(DEX_COUNT))
    lines.append("// tools/gen_learnsets_real.py aus PokeAPI-Level-Aufstiegs-Daten (method=")
    lines.append("// \"level-up\"). Enthaelt nur Attacken, die tatsaechlich in moves_real.h")
    lines.append("// existieren, mit dem echten Spiellevel (neueste verfuegbare Version-Group")
    lines.append("// zuerst, siehe VERSION_PRIORITY im Generator-Skript). Keine erfundenen")
    lines.append("// Level-0-Bonus-Eintraege mehr wie in der fruehreren DylanPDao-Portierung.")
    lines.append("struct LearnEntry { uint8_t move; uint8_t level; };")
    lines.append("")

    flat = []
    offsets = [0]
    for entries in per_species:
        for mid, lvl in entries:
            lvl_clamped = max(1, min(255, lvl))
            flat.append((mid, lvl_clamped))
        offsets.append(len(flat))

    lines.append(f"static const LearnEntry LEARN_TBL[{max(1, len(flat))}] = {{")
    row = []
    for mid, lvl in flat:
        row.append(f"{{{mid},{lvl}}}")
        if len(row) == 12:
            lines.append("  " + ", ".join(row) + ",")
            row = []
    if row:
        lines.append("  " + ", ".join(row) + ",")
    if not flat:
        lines.append("  {0,0},")
    lines.append("};")
    lines.append("")

    lines.append(f"static const uint16_t LEARN_OFS[{DEX_COUNT + 1}] = {{")
    row = []
    for off in offsets:
        row.append(str(off))
        if len(row) == 16:
            lines.append("  " + ", ".join(row) + ",")
            row = []
    if row:
        lines.append("  " + ", ".join(row) + ",")
    lines.append("};")
    lines.append("")
    lines.append(f"#define LEARNSET_DEX_COUNT {DEX_COUNT}")
    lines.append("")
    lines.append("// Anzahl Lerneintraege einer Art.")
    lines.append("static inline uint8_t learnCount(int16_t dex) {")
    lines.append("  if (dex < 1 || dex > LEARNSET_DEX_COUNT) return 0;")
    lines.append("  return (uint8_t)(LEARN_OFS[dex] - LEARN_OFS[dex - 1]);")
    lines.append("}")
    lines.append("")
    lines.append("// i-ter Lerneintrag einer Art (0-basiert, siehe learnCount()).")
    lines.append("static inline uint8_t learnMove(int16_t dex, uint8_t i) {")
    lines.append("  if (dex < 1 || dex > LEARNSET_DEX_COUNT) return 0;")
    lines.append("  return LEARN_TBL[LEARN_OFS[dex - 1] + i].move;")
    lines.append("}")
    lines.append("static inline uint8_t learnLevel(int16_t dex, uint8_t i) {")
    lines.append("  if (dex < 1 || dex > LEARNSET_DEX_COUNT) return 0;")
    lines.append("  return LEARN_TBL[LEARN_OFS[dex - 1] + i].level;")
    lines.append("}")
    lines.append("")

    with open("../learnsets_real.h", "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    print(f"OK -> learnsets_real.h ({len(flat)} Eintraege, {DEX_COUNT} Arten)", file=sys.stderr)


if __name__ == "__main__":
    main()
