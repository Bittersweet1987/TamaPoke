#!/usr/bin/env python3
"""Holt die Pokemon-Rufe von PokeAPI/cries und wandelt sie in rohes PCM16
mono/16kHz um (das Format, das audio.cpp/cryPlay() ueber den ES8311-Codec
abspielt). Ergebnis liegt unter tools/sdcard/cries/pNNN.pcm, genau wie
pack_pmd.py die Sprites unter tools/sdcard/mons/ ablegt.

  python3 tools/pack_cries.py          # alle 251 (Gen 1+2)
  python3 tools/pack_cries.py 7 25     # nur Squirtle und Pikachu

Braucht ffmpeg im PATH (nur zum Konvertieren, nicht auf dem Board noetig).
Danach hochladen mit:
  python3 tools/send_cries.py
oder ins Web-Installer-Bundle packen (siehe README fuer sprites.pak - Cries
laufen analog dazu ueber eine eigene Sende-Route, siehe send_cries.py).
"""
import os
import subprocess
import sys
import urllib.request

CRY_URL_LEGACY = "https://raw.githubusercontent.com/PokeAPI/cries/main/cries/pokemon/legacy/{n}.ogg"
CRY_URL_LATEST = "https://raw.githubusercontent.com/PokeAPI/cries/main/cries/pokemon/latest/{n}.ogg"
OUT_DIR = os.path.join(os.path.dirname(__file__), 'sdcard', 'cries')


def fetch_ogg(n, dest):
    # Gen 1-5 (<=649) haben eigene "legacy"-Rufe; ab Gen 6 gibt es nur "latest".
    for url in (CRY_URL_LEGACY.format(n=n), CRY_URL_LATEST.format(n=n)):
        subprocess.run(['curl', '-sL', url, '-o', dest], check=True)
        if os.path.getsize(dest) >= 100:  # PokeAPI liefert bei 404 eine kurze HTML-Fehlerseite
            return True
    if os.path.exists(dest):
        os.remove(dest)
    return False


def to_pcm(ogg_path, pcm_path):
    r = subprocess.run(
        ['ffmpeg', '-y', '-loglevel', 'error', '-i', ogg_path,
         '-ac', '1', '-ar', '16000', '-f', 's16le', '-acodec', 'pcm_s16le', pcm_path],
        capture_output=True)
    return r.returncode == 0


def main():
    nums = [int(a) for a in sys.argv[1:]] if len(sys.argv) > 1 else list(range(1, 650))
    os.makedirs(OUT_DIR, exist_ok=True)

    ok, missing = 0, []
    for n in nums:
        ogg = os.path.join(OUT_DIR, f'_{n}.ogg')
        pcm = os.path.join(OUT_DIR, f'p{n:03d}.pcm')
        try:
            if not fetch_ogg(n, ogg):
                missing.append(n)
                continue
            if to_pcm(ogg, pcm):
                ok += 1
                print(f"  #{n:03d} -> {os.path.basename(pcm)} ({os.path.getsize(pcm)/1024:.0f} KB)")
            else:
                missing.append(n)
        finally:
            if os.path.exists(ogg):
                os.remove(ogg)

    print(f"fertig: {ok} Rufe, {len(missing)} fehlend {missing if missing else ''}")


if __name__ == '__main__':
    main()
