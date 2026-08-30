#!/usr/bin/env python3
"""Fuegt die generierten Dex 650-1025 Daten in dex.h ein.
Nutzung: python3 tools/merge_dex_650_1025.py
Liest tools/../dex.h und dex_650_1025_output.txt (fest verdrahtet, einmaliger Gebrauch).
"""
import re

import os
BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEX_H = os.path.join(BASE, "dex.h")
SRC = os.path.join(BASE, "tools", "dex_650_1025_output.txt")

with open(SRC, encoding="utf-8") as f:
    src = f.read()

sections = src.split("// ===== ")
dex_tbl_lines = []
evo_lines = []
name_cols = {}  # lang -> list of quoted-name strings joined already

for sec in sections:
    if sec.startswith("DEX_TBL"):
        body = sec.split("=====", 1)[1]
        dex_tbl_lines = [l for l in body.strip("\n").split("\n") if l.strip()]
    elif sec.startswith("EVOLUTION_RULES"):
        body = sec.split("=====", 1)[1]
        evo_lines = [l for l in body.strip("\n").split("\n") if l.strip()]
    elif sec.startswith("DEX_NAMES"):
        body = sec.split("=====", 1)[1]
        lines = body.strip("\n").split("\n")
        cur_lang = None
        for l in lines:
            l = l.rstrip()
            if l.startswith("// "):
                cur_lang = l[3:].strip().rstrip(":")
            elif l.strip():
                name_cols[cur_lang] = l.rstrip(",")

assert len(dex_tbl_lines) == 376, f"DEX_TBL: {len(dex_tbl_lines)} lines, erwartet 376"
print(f"DEX_TBL: {len(dex_tbl_lines)} Zeilen, EVOLUTION_RULES: {len(evo_lines)} Zeilen")
for lang in ("es", "en", "fr", "de", "it", "pt"):
    assert lang in name_cols, f"Sprache {lang} fehlt"
    n = name_cols[lang].count('",') + 1
    print(f"DEX_NAMES[{lang}]: {n} Namen")

with open(DEX_H, encoding="utf-8") as f:
    text = f.read()

# 1) DEX_COUNT 649 -> 1025
text = text.replace("#define DEX_COUNT 649", "#define DEX_COUNT 1025", 1)

# 2) DEX_TBL: vor der ersten "};" nach "static const DexEntry DEX_TBL" einfuegen
marker = "static const DexEntry DEX_TBL[DEX_COUNT + 1] = {"
idx = text.index(marker)
close_idx = text.index("\n};", idx)
insertion = "\n" + "\n".join(dex_tbl_lines)
text = text[:close_idx] + insertion + text[close_idx:]

# 3) EVOLUTION_RULES: vor der schliessenden "};" einfuegen, Count aktualisieren
marker2 = "static const EvolutionRule EVOLUTION_RULES[] = {"
idx2 = text.index(marker2)
close_idx2 = text.index("\n};", idx2)
insertion2 = "\n" + "\n".join(evo_lines)
text = text[:close_idx2] + insertion2 + text[close_idx2:]

old_count = 320
new_count = old_count + len(evo_lines)
text = text.replace(f"#define EVOLUTION_RULE_COUNT {old_count}", f"#define EVOLUTION_RULE_COUNT {new_count}", 1)

# 4) DEX_NAMES: pro Sprachzeile die neuen Namen vor dem schliessenden "},\n" der jeweiligen Zeile anhaengen
lang_order = ["ES", "EN", "FR", "DE", "IT", "PT"]
lang_key = {"ES": "es", "EN": "en", "FR": "fr", "DE": "de", "IT": "it", "PT": "pt"}
for lang in lang_order:
    tag = f"// {lang}\n  {{ "
    idx3 = text.index(tag)
    # Ende dieser Zeile finden: sucht "},\n" das die Zeile schliesst (Zeile endet mit '},' gefolgt von Zeilenumbruch und "  // " oder "};")
    line_end = text.index("},\n", idx3)
    new_names = name_cols[lang_key[lang]]
    text = text[:line_end] + ", " + new_names + text[line_end:]

with open(DEX_H, "w", encoding="utf-8") as f:
    f.write(text)

print("dex.h aktualisiert.")
