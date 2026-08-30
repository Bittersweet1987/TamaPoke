#!/usr/bin/env python3
"""Erzeugt DEX_TBL/DEX_NAMES/EVOLUTION_RULES-Eintraege fuer Dex 650-1025
(Gen 6-9) direkt aus PokeAPI, im exakten Format der bestehenden dex.h.

Nutzung: python3 tools/gen_dex_650_1025.py > tools/dex_650_1025.txt
Das Ergebnis wird danach manuell/per Skript in dex.h eingefuegt (DEX_TBL-
Zeilen, DEX_NAMES-Spalten, EVOLUTION_RULES-Zeilen, siehe Kommentare im
Ausgabetext).
"""
import json
import re
import sys
import time
import urllib.request

LO, HI = 650, 1025
API = "https://pokeapi.co/api/v2"

TYPE_ACCENT = {
    "NORMAL": 0x8C4D, "FIRE": 0xEA87, "WATER": 0x4C98, "GRASS": 0x3C49,
    "ELECTRIC": 0xBCA1, "ICE": 0x4DB8, "FIGHTING": 0xA2A5, "POISON": 0x8A73,
    "GROUND": 0xB447, "PSYCHIC": 0xD28F, "BUG": 0x7CC4, "ROCK": 0x9407,
    "GHOST": 0x6AD3, "DRAGON": 0x5A98, "DARK": 0x5A6E, "STEEL": 0x7C73,
    "FAIRY": 0x8C4D, "FLYING": 0x8BF8, "NONE": 0x2946,
}
# type -> Biome-Index (0 pradera 1 playa 2 bosque 3 volcan 4 montana 5 nieve)
TYPE_BIOME = {
    "FIRE": 3, "WATER": 1, "ICE": 5, "ROCK": 4, "GROUND": 4, "STEEL": 4,
    "BUG": 2, "GRASS": 2, "POISON": 2, "GHOST": 2, "DARK": 2,
}

UMLAUT = str.maketrans({"ä": "ae", "ö": "oe", "ü": "ue", "ß": "ss",
                         "Ä": "Ae", "Ö": "Oe", "Ü": "Ue"})


def ascii_fold(s):
    s = s.translate(UMLAUT)
    s = s.replace("\n", " ").replace("", " ")
    s = re.sub(r"[^A-Za-z0-9 .,!?'\-]", "", s)
    return re.sub(r"\s+", " ", s).strip()


def get(url, tries=3):
    req = urllib.request.Request(url, headers={"User-Agent": "TamaPoke-dex-gen/1.0 (+https://github.com/ShadowEnemyx/TamaPoke)"})
    for i in range(tries):
        try:
            with urllib.request.urlopen(req, timeout=15) as r:
                return json.loads(r.read().decode("utf-8"))
        except Exception as e:
            if i == tries - 1:
                raise
            time.sleep(1.5)


def species_to_dex(species_url):
    # /pokemon-species/{id}/ -> id ist die Nummer, die wir als Dex nutzen
    m = re.search(r"/pokemon-species/(\d+)/", species_url)
    return int(m.group(1)) if m else None


def flatten_chain(node, out):
    dex = species_to_dex(node["species"]["url"])
    for ev in node.get("evolves_to", []):
        edex = species_to_dex(ev["species"]["url"])
        lvl = 0
        details = ev.get("evolution_details") or []
        if details:
            d = details[0]
            if d.get("min_level"):
                lvl = d["min_level"]
            elif d.get("item"):
                lvl = 30  # Stein-Entwicklung ~30, wie bei Dex 1-649 (siehe dex_data.py)
            elif d.get("trade_species") or d.get("known_move") or d.get("min_happiness"):
                lvl = 40  # Tausch/Freundschaft/Attacke ~40
            else:
                lvl = 32
        out.append((dex, edex, lvl))
        flatten_chain(ev, out)


def main():
    dex_lines = []
    name_cols = {0: [], 1: [], 2: [], 3: [], 4: [], 5: []}  # es,en,fr,de,it,pt
    evo_rules = []
    seen_chains = set()

    for dex in range(LO, HI + 1):
        try:
            poke = get(f"{API}/pokemon/{dex}/")
            species = get(f"{API}/pokemon-species/{dex}/")
        except Exception as e:
            print(f"# FEHLER bei Dex {dex}: {e}", file=sys.stderr)
            continue

        name_en = poke["name"].upper().replace("-", " ")
        types = [t["type"]["name"].upper() for t in sorted(poke["types"], key=lambda t: t["slot"])]
        type1 = types[0] if len(types) > 0 else "NORMAL"
        type2 = types[1] if len(types) > 1 else "NONE"
        stats = {s["stat"]["name"]: s["base_stat"] for s in poke["stats"]}
        hp, atk, dfs, spa, spd, spe = (stats.get("hp", 50), stats.get("attack", 50),
                                        stats.get("defense", 50), stats.get("special-attack", 50),
                                        stats.get("special-defense", 50), stats.get("speed", 50))

        names_by_lang = {n["language"]["name"]: n["name"] for n in species["names"]}
        de_name = names_by_lang.get("de", poke["name"].capitalize())
        for i, lang in enumerate(["es", "en", "fr", "de", "it", "pt-BR"]):
            nm = names_by_lang.get(lang) or names_by_lang.get("en") or poke["name"]
            name_cols[i].append(ascii_fold(nm).upper())

        desc = ""
        for entry in species.get("flavor_text_entries", []):
            if entry["language"]["name"] == "de":
                desc = ascii_fold(entry["flavor_text"])
                break
        if not desc:
            for entry in species.get("flavor_text_entries", []):
                if entry["language"]["name"] == "en":
                    desc = ascii_fold(entry["flavor_text"])
                    break
        if len(desc) > 140:
            desc = desc[:139].rsplit(" ", 1)[0] + "."

        if species.get("is_legendary") or species.get("is_mythical"):
            rarity = "R_LEGENDARIO"
        elif species.get("evolves_from_species"):
            rarity = "R_EVO"
        else:
            rarity = "R_COMUN"

        accent = TYPE_ACCENT.get(type1, 0x8C4D)
        biome = TYPE_BIOME.get(type1, 0)

        chain_url = species["evolution_chain"]["url"]
        evolves_to, evolve_level = 0, 0
        if chain_url not in seen_chains:
            seen_chains.add(chain_url)
            try:
                chain = get(chain_url)
                pairs = []
                flatten_chain(chain["chain"], pairs)
                for frm, to, lvl in pairs:
                    if frm and to:
                        evo_rules.append((frm, to, lvl))
            except Exception as e:
                print(f"# Evolutionskette fuer Dex {dex} fehlgeschlagen: {e}", file=sys.stderr)

        dex_lines.append(
            f'  {{ "{name_en}", 0, 0, {rarity}, 0x{accent:04X}, {hp}, {atk}, {dfs}, {spa}, {spd}, {spe}, '
            f'TYPE_{type1}, TYPE_{type2}, {biome}, "{desc}" }},  // {dex} {type1.lower()}/{type2.lower()}'
        )
        print(f"# {dex}/{HI} {name_en} ({de_name})", file=sys.stderr)
        time.sleep(0.03)

    # evolvesTo/evolveLevel direkt in die DEX_TBL-Zeilen eintragen (ersetzt die 0,0 Platzhalter)
    evo_map = {frm: (to, lvl) for frm, to, lvl in evo_rules}
    fixed_lines = []
    for i, line in enumerate(dex_lines):
        dex = LO + i
        if dex in evo_map:
            to, lvl = evo_map[dex]
            line = line.replace('", 0, 0, ', f'", {to}, {lvl}, ', 1)
        fixed_lines.append(line)

    print("// ===== DEX_TBL Zeilen 650-1025 (in DEX_TBL einfuegen) =====")
    print("\n".join(fixed_lines))
    print()
    print("// ===== EVOLUTION_RULES Zeilen 650-1025 (in EVOLUTION_RULES einfuegen) =====")
    for frm, to, lvl in evo_rules:
        print(f"  {{ {frm}, {to}, {lvl}, EVO_LEVEL }},")
    print()
    print("// ===== DEX_NAMES Spalten 650-1025 je Sprache (an die 6 bestehenden Zeilen anhaengen) =====")
    for i, lang in enumerate(["es", "en", "fr", "de", "it", "pt"]):
        joined = ", ".join(f'"{n}"' for n in name_cols[i])
        print(f"// {lang}:")
        print(joined + ",")


if __name__ == "__main__":
    main()
