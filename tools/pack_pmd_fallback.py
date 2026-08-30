#!/usr/bin/env python3
"""Baut ein minimales TPK2-Sprite (nur "Idle", 1 Standbild, keine Animation)
aus dem offiziellen PokeAPI-Pixelsprite, fuer Arten, die PMD SpriteCollab noch
nicht animiert hat (siehe tools/pack_pmd.py FALLOS-Liste).

  python3 tools/pack_pmd_fallback.py            # alle bekannten Luecken
  python3 tools/pack_pmd_fallback.py 514 516    # gezielt einzelne Dex-Nummern
"""
import os
import struct
import subprocess
import sys
from PIL import Image

sys.path.insert(0, os.path.dirname(__file__))

OUT = os.path.join(os.path.dirname(__file__), 'sdcard', 'mons')
SPRITE_URL = 'https://raw.githubusercontent.com/PokeAPI/sprites/master/sprites/pokemon'
ALPHA_T = 128

# aktuell (Stand dieser Session) fehlende Arten in PMD SpriteCollab
GAPS = [514, 516, 520, 522, 523, 538, 558, 564, 565, 588, 591, 592, 616, 626]


def rgb565(r, g, b):
    return (r >> 3) << 11 | (g >> 2) << 5 | (b >> 3)


def fetch(url, path):
    r = subprocess.run(['curl', '-sL', url, '-o', path], check=True)
    return os.path.getsize(path) > 200  # 404-Seiten sind winzig


def pack_one(dexnum, shiny):
    suffix = '-shiny' if shiny else ''
    url = f'{SPRITE_URL}{"/shiny" if shiny else ""}/{dexnum}.png'
    png = f'/tmp/fb_{dexnum}{suffix}.png'
    if not fetch(url, png):
        raise RuntimeError('kein PokeAPI-Sprite gefunden')
    im = Image.open(png).convert('RGBA')
    w, h = im.size
    if w > 255 or h > 255:
        im.thumbnail((255, 255))
        w, h = im.size

    colmap, pal, data = {}, [], bytearray()
    for px in im.getdata():
        if px[3] < ALPHA_T:
            data.append(0xFF)
            continue
        k = px[:3]
        if k not in colmap:
            if len(pal) >= 255:
                k2 = min(colmap, key=lambda c: sum((a - b) ** 2 for a, b in zip(c, k)))
                colmap[k] = colmap[k2]
            else:
                colmap[k] = len(pal)
                pal.append(k)
        data.append(colmap[k])

    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, f'p{"s" if shiny else ""}{dexnum:03d}.bin')
    with open(path, 'wb') as f:
        f.write(b'TPK2')
        f.write(struct.pack('<BH', 1, len(pal)))  # 1 accion: Idle
        for r, g, b in pal:
            f.write(struct.pack('<H', rgb565(r, g, b)))
        f.write(struct.pack('<4B', 0, w, h, 1))  # aid=0 (Idle), 1 frame
        f.write(struct.pack('<H', 400))          # Standbild, Dauer irrelevant
        f.write(bytes(data))
    kb = os.path.getsize(path) / 1024
    print(f"  -> p{'s' if shiny else ''}{dexnum:03d}.bin (Standbild): {len(pal)} Farben, {kb:.0f} KB")


if __name__ == '__main__':
    nums = [int(a) for a in sys.argv[1:]] if len(sys.argv) > 1 else GAPS
    fallos = []
    for n in nums:
        for shiny in (False, True):
            try:
                print(f"#{n:03d}{' shiny' if shiny else ''}")
                pack_one(n, shiny)
            except Exception as e:
                print(f"  FALLO: {e}")
                fallos.append((n, shiny))
    print(f"FALLOS: {fallos}" if fallos else "ALLE STANDBILDER ERSTELLT")
