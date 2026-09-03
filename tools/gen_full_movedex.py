#!/usr/bin/env python3
"""Erweitert moves_real.h auf ALLE Attacken, die irgendeine der 1025 Arten per
Level-Aufstieg tatsaechlich lernt (statt nur 90 handverlesene), und regeneriert
learnsets_real.h passend dazu mit den echten Leveln.

Die urspruenglichen 90 handgefertigten Eintraege (Rekyl, Drain, Prioritaet,
Status-Stufen, Ladezeit usw.) bleiben UNVERAENDERT unter ihren alten IDs
erhalten. Alle neuen Attacken (die grosse Mehrheit) werden automatisch aus
PokeAPI abgeleitet: Typ/Schadensklasse/Staerke/Genauigkeit direkt uebernommen,
dazu einfache Effekte wo eindeutig erkennbar (Drain, Rekyl, Mehrfachtreffer,
Prioritaet, Heilung, ein einzelner Stat-Stufen-Wechsel, ein Status-Effekt).
Alles andere (Ladeangriffe, Erschoepfung, Fixschaden usw.) faellt bei neuen
Attacken auf einen einfachen Schadens-/Status-Wert ohne Zusatzeffekt zurueck --
das ist eine bewusste Vereinfachung, siehe README/CREDITS.

WICHTIG: Da es weit ueber 255 einzigartige Attacken gibt, wird die Attacken-ID
in dieser Generation zu einem 16-Bit-Wert -- der C++-Code (pet.h, battle.h/.cpp,
learnsets_real.h, TamaPoke.ino) muss dazu passend von uint8_t auf uint16_t fuer
Attacken-IDs umgestellt werden (separat, siehe AI_HANDOFF.md).

Nutzung: python3 tools/gen_full_movedex.py
Schreibt ../moves_real.h und ../learnsets_real.h neu (Backups vorher empfohlen).
"""
import json
import re
import sys
import time
import urllib.request

API = "https://pokeapi.co/api/v2"
HEADERS = {"User-Agent": "TamaPoke-movedex-gen/1.0 (+https://github.com/Bittersweet1987/TamaPoke)"}
DEX_COUNT = 1025

VERSION_PRIORITY = [
    "scarlet-violet", "sword-shield", "ultra-sun-ultra-moon", "sun-moon",
    "omega-ruby-alpha-sapphire", "x-y", "black-2-white-2", "black-white",
    "heartgold-soulsilver", "platinum", "diamond-pearl", "firered-leafgreen",
    "emerald", "ruby-sapphire", "crystal", "gold-silver", "yellow", "red-blue",
]

UMLAUT = str.maketrans({"ä": "ae", "ö": "oe", "ü": "ue", "ß": "ss",
                         "Ä": "Ae", "Ö": "Oe", "Ü": "Ue"})

TYPE_MAP = {
    "normal": "TYPE_NORMAL", "fire": "TYPE_FIRE", "water": "TYPE_WATER",
    "electric": "TYPE_ELECTRIC", "grass": "TYPE_GRASS", "ice": "TYPE_ICE",
    "fighting": "TYPE_FIGHTING", "poison": "TYPE_POISON", "ground": "TYPE_GROUND",
    "flying": "TYPE_FLYING", "psychic": "TYPE_PSYCHIC", "bug": "TYPE_BUG",
    "rock": "TYPE_ROCK", "ghost": "TYPE_GHOST", "dragon": "TYPE_DRAGON",
    "dark": "TYPE_DARK", "steel": "TYPE_STEEL", "fairy": "TYPE_FAIRY",
}
AIL_MAP = {
    "paralysis": "AIL_PARA", "burn": "AIL_BURN", "poison": "AIL_POISON",
    "sleep": "AIL_SLEEP", "freeze": "AIL_FREEZE", "confusion": "AIL_CONFUSE",
}
STAT_MASK = {
    "attack": "ST_ATK", "defense": "ST_DEF", "special-attack": "ST_SPA",
    "special-defense": "ST_SPD", "speed": "ST_SPE",
}


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
        except Exception:
            if i == tries - 1:
                raise
            time.sleep(1.5)


def parse_existing_moves():
    text = open("../moves_real.h", encoding="utf-8").read()
    m = re.search(r"MOVE_TBL\[MOVE_COUNT\] = \{(.*?)\n\};", text, re.S)
    rows = re.findall(r"\{ (.*?) \},", m.group(1))
    entries = []  # Liste von (name, raw_row_text)
    for r in rows:
        name = re.match(r'"([^"]*)"', r).group(1)
        entries.append((name, r))
    return entries


def derive_new_entry(move_data):
    """Baut eine MoveEntry-Zeile fuer eine neue (nicht handgefertigte) Attacke
    automatisch aus den PokeAPI-Rohdaten ab."""
    name = None
    for n in move_data.get("names", []):
        if n["language"]["name"] == "de":
            name = ascii_fold_upper(n["name"])
            break
    if not name:
        name = ascii_fold_upper(move_data["name"])
    mtype = TYPE_MAP.get(move_data["type"]["name"], "TYPE_NORMAL")
    dclass = move_data["damage_class"]["name"]
    cat = "MC_STATUS" if dclass == "status" else ("MC_PHYS" if dclass == "physical" else "MC_SPEC")
    power = move_data.get("power") or 0
    acc = move_data.get("accuracy")
    acc = 0 if acc is None else acc  # 0 = "kann nicht verfehlen" (bestehende Konvention)
    priority = move_data.get("priority") or 0
    meta = move_data.get("meta") or {}
    effect, param, statMask, stages, target, ailment, ailChance = "EF_NONE", 0, "0", 0, "TG_FOE", "AIL_NONE", 0

    if priority != 0:
        effect, param = "EF_PRIORITY", priority
    elif meta.get("drain", 0) > 0:
        effect, param = "EF_DRAIN", min(100, meta["drain"])
    elif meta.get("drain", 0) < 0:
        pct = abs(meta["drain"])
        effect, param = "EF_RECOIL", max(2, round(100 / pct)) if pct else 0
    elif meta.get("healing", 0) > 0:
        effect, param = "EF_HEAL", min(100, meta["healing"])
    elif meta.get("min_hits") and meta.get("max_hits"):
        effect = "EF_MULTI"
    elif move_data.get("accuracy") is None and power and power > 0:
        effect = "EF_NEVER_MISS"

    stat_changes = move_data.get("stat_changes") or []
    if effect == "EF_NONE" and len(stat_changes) == 1:
        sc = stat_changes[0]
        stat = STAT_MASK.get(sc["stat"]["name"])
        if stat:
            effect, statMask, stages = "EF_STAGE", stat, sc["change"]
            target = "TG_SELF" if (move_data.get("target", {}).get("name") in
                                    ("user", "user-and-allies", "users-field")) else "TG_FOE"

    ail = (move_data.get("meta") or {}).get("ailment", {}).get("name")
    if ail in AIL_MAP and effect == "EF_NONE":
        ailment = AIL_MAP[ail]
        ailChance = meta.get("ailment_chance") or 0
        if ailChance == 0 and dclass == "status":
            ailChance = 100  # reine Status-Attacke ohne Prozentangabe = garantiert

    if dclass == "status" and effect == "EF_NONE" and ailment == "AIL_NONE":
        target = "TG_SELF" if (move_data.get("target", {}).get("name") in
                                ("user", "user-and-allies", "users-field")) else "TG_FOE"

    return (f'"{name}", {mtype}, {cat}, {power}, {acc}, {effect}, {param}, '
            f'{statMask}, {stages}, {target}, {ailment}, {ailChance}')


def main():
    print("Lade bestehende 90 handgefertigte Attacken...", file=sys.stderr)
    existing = parse_existing_moves()
    existing_names = {name for name, _ in existing}

    print("Lade komplette Attackenliste von PokeAPI...", file=sys.stderr)
    all_moves = get(f"{API}/move?limit=3000")["results"]

    version_rank = {v: i for i, v in enumerate(VERSION_PRIORITY)}

    print("Sammle alle tatsaechlich per Level-Aufstieg gelernten Attacken (1025 Arten)...", file=sys.stderr)
    per_species_raw = []  # dex -> [(slug, level), ...]
    needed_slugs = set()
    for dex in range(1, DEX_COUNT + 1):
        try:
            poke = get(f"{API}/pokemon/{dex}/")
        except Exception as e:
            print(f"# FEHLER Dex {dex}: {e}", file=sys.stderr)
            per_species_raw.append([])
            continue
        best = {}
        for mv in poke.get("moves", []):
            slug = mv["move"]["name"]
            for vgd in mv.get("version_group_details", []):
                if vgd["move_learn_method"]["name"] != "level-up":
                    continue
                vg = vgd["version_group"]["name"]
                rank = version_rank.get(vg, 999)
                lvl = vgd["level_learned_at"]
                if slug not in best or rank < best[slug][0]:
                    best[slug] = (rank, lvl)
        entries = [(slug, lvl) for slug, (rank, lvl) in best.items()]
        per_species_raw.append(entries)
        needed_slugs.update(slug for slug, _ in entries)
        if dex % 100 == 0:
            print(f"  ...{dex}/{DEX_COUNT} Arten, {len(needed_slugs)} einzigartige Attacken bisher", file=sys.stderr)
        time.sleep(0.02)

    print(f"{len(needed_slugs)} einzigartige Attacken werden benoetigt. Lade Details + deutsche Namen...",
          file=sys.stderr)
    slug_by_name = {m["name"]: m for m in all_moves}
    slug_to_id = {}
    move_rows = [row for _, row in existing]  # ID 0..89 = die bestehenden, unveraendert
    name_to_id = {name: i for i, (name, _) in enumerate(existing)}

    count_new = 0
    for i, slug in enumerate(sorted(needed_slugs)):
        if slug not in slug_by_name:
            continue
        try:
            data = get(slug_by_name[slug]["url"])
        except Exception as e:
            print(f"# WARNUNG Attacke {slug} nicht ladbar: {e}", file=sys.stderr)
            continue
        de = None
        for n in data.get("names", []):
            if n["language"]["name"] == "de":
                de = ascii_fold_upper(n["name"])
                break
        if de and de in name_to_id:
            slug_to_id[slug] = name_to_id[de]  # deckt sich mit einer der 90 -> alte ID/Effekte behalten
        else:
            row = derive_new_entry(data)
            move_rows.append(row)
            slug_to_id[slug] = len(move_rows) - 1
            count_new += 1
        if (i + 1) % 100 == 0:
            print(f"  ...{i + 1}/{len(needed_slugs)} Attacken geladen ({count_new} neu abgeleitet)", file=sys.stderr)
        time.sleep(0.02)

    print(f"Attacken-Datenbank: {len(move_rows)} gesamt ({len(existing)} handgefertigt + {count_new} neu).",
          file=sys.stderr)

    with open("../moves_real.h", encoding="utf-8") as f:
        old_header = f.read()
    header_end = old_header.index("static const MoveEntry MOVE_TBL[MOVE_COUNT]")
    header = old_header[:header_end]
    header = re.sub(r"#define MOVE_COUNT \d+", f"#define MOVE_COUNT {len(move_rows)}", header)

    with open("../moves_real.h", "w", encoding="utf-8") as f:
        f.write(header)
        f.write("static const MoveEntry MOVE_TBL[MOVE_COUNT] = {\n")
        for i, row in enumerate(move_rows):
            f.write(f"  {{ {row} }},  // {i}\n")
        f.write("};\n")
    print("OK -> moves_real.h geschrieben.", file=sys.stderr)

    # ---- learnsets_real.h neu, mit den vollstaendigen IDs ----
    per_species = []
    for entries in per_species_raw:
        mapped = []
        for slug, lvl in entries:
            if slug in slug_to_id:
                mapped.append((slug_to_id[slug], max(1, min(255, lvl))))
        mapped.sort(key=lambda e: e[1])
        per_species.append(mapped)

    flat = []
    offsets = [0]
    for entries in per_species:
        flat.extend(entries)
        offsets.append(len(flat))

    lines = []
    lines.append("#pragma once")
    lines.append("#include \"dex.h\"")
    lines.append("")
    lines.append(f"// Lernlisten (Level -> Attacke) fuer Dex 1-{DEX_COUNT}, GENERIERT von")
    lines.append("// tools/gen_full_movedex.py aus PokeAPI-Level-Aufstiegs-Daten. Attacken-ID")
    lines.append("// ist ein uint16_t (mehr als 255 einzigartige Attacken im System, siehe")
    lines.append("// moves_real.h). Keine erfundenen Level-0-Bonus-Eintraege.")
    lines.append("struct LearnEntry { uint16_t move; uint8_t level; };")
    lines.append("")
    lines.append(f"static const LearnEntry LEARN_TBL[{max(1, len(flat))}] = {{")
    row = []
    for mid, lvl in flat:
        row.append(f"{{{mid},{lvl}}}")
        if len(row) == 10:
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
    lines.append("static inline uint16_t learnCount(int16_t dex) {")
    lines.append("  if (dex < 1 || dex > LEARNSET_DEX_COUNT) return 0;")
    lines.append("  return (uint16_t)(LEARN_OFS[dex] - LEARN_OFS[dex - 1]);")
    lines.append("}")
    lines.append("static inline uint16_t learnMove(int16_t dex, uint16_t i) {")
    lines.append("  if (dex < 1 || dex > LEARNSET_DEX_COUNT) return 0;")
    lines.append("  return LEARN_TBL[LEARN_OFS[dex - 1] + i].move;")
    lines.append("}")
    lines.append("static inline uint8_t learnLevel(int16_t dex, uint16_t i) {")
    lines.append("  if (dex < 1 || dex > LEARNSET_DEX_COUNT) return 0;")
    lines.append("  return LEARN_TBL[LEARN_OFS[dex - 1] + i].level;")
    lines.append("}")
    lines.append("")

    with open("../learnsets_real.h", "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    print(f"OK -> learnsets_real.h geschrieben ({len(flat)} Eintraege).", file=sys.stderr)


if __name__ == "__main__":
    main()
