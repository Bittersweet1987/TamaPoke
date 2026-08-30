#!/usr/bin/env python3
"""Empaqueta todos los sprites de la SD (tools/sdcard/mons/*.bin) en el bundle
web/sprites.pak para que el instalador web los suba de un clic.

Formato TPAK (little-endian), sin cambios:
  char[4]  "TPAK"
  uint16   count
  count x { uint8 nameLen; char name[nameLen]; uint32 size }   (indice)
  ...datos de cada fichero, en el mismo orden...

Con los 1025 Pokemon el bundle completo pesa ~280 MB, muy por encima del
limite de 100 MB por archivo de GitHub (y Git LFS esta bloqueado en forks
publicos). Por eso el TPAK ya no se escribe como un solo fichero: se corta en
trozos crudos de <=90 MB (sprites.pak.1, .2, ...) que el instalador web
descarga en orden y concatena en memoria ANTES de interpretar el TPAK -- el
formato interno no cambia, solo su almacenamiento en el repo. Un manifiesto
`sprites.pak.json` lista las partes y el tamano total para la barra de progreso.

El instalador (web/index.html) descarga las partes, las concatena, parte el
TPAK resultante por el indice y manda cada fichero a la placa con el
protocolo PUT (igual que tools/send_sd.py).
"""
import glob
import json
import os
import struct
import sys

HERE = os.path.dirname(__file__)
MONS = os.path.join(HERE, 'sdcard', 'mons')
OUT = os.path.join(HERE, '..', 'web', 'sprites.pak')
GEN2_OUT = os.path.join(HERE, '..', 'web', 'sprites-gen2-update.pak')
PART_SIZE = 90 * 1024 * 1024  # unter GitHubs 100-MB-Einzeldatei-Grenze


def write_parts(out_path, data):
    base = os.path.basename(out_path)
    out_dir = os.path.dirname(out_path)
    # alte Teile/Manifeste dieses Namens erst entfernen (Anzahl kann sich aendern)
    for old in glob.glob(out_path + '.*'):
        os.remove(old)
    manifest_path = out_path + '.json'
    if os.path.exists(manifest_path):
        os.remove(manifest_path)

    parts = []
    for i in range(0, len(data), PART_SIZE) or [0]:
        chunk = data[i:i + PART_SIZE]
        idx = i // PART_SIZE + 1
        name = f'{base}.{idx}'
        with open(os.path.join(out_dir, name), 'wb') as f:
            f.write(chunk)
        parts.append(name)
    if not parts:  # leerer Bundle-Sonderfall, sollte hier nie vorkommen
        parts = [base + '.1']
        open(os.path.join(out_dir, parts[0]), 'wb').close()

    with open(manifest_path, 'w') as f:
        json.dump({'parts': parts, 'totalSize': len(data)}, f)
    return parts


def main():
    gen2_only = '--gen2' in sys.argv[1:]
    if gen2_only:
        files = [os.path.join(MONS, f'p{n:03d}.bin') for n in range(161, 252)]
        files += [os.path.join(MONS, f'ps{n:03d}.bin') for n in range(161, 252)]
        # thumbs.bin is a complete index and must accompany a partial sprite
        # update so the gallery can address all 251 entries.
        files.append(os.path.join(MONS, 'thumbs.bin'))
        out_path = GEN2_OUT
    else:
        files = sorted(glob.glob(os.path.join(MONS, '*.bin')))
        out_path = OUT
    if not files:
        raise SystemExit('no hay sprites en ' + MONS)
    names = ['mons/' + os.path.basename(f) for f in files]
    blobs = [open(f, 'rb').read() for f in files]

    header = bytearray()
    header += b'TPAK'
    header += struct.pack('<H', len(files))
    for name, blob in zip(names, blobs):
        nb = name.encode()
        header += struct.pack('<B', len(nb))
        header += nb
        header += struct.pack('<I', len(blob))
    data = bytes(header) + b''.join(blobs)

    parts = write_parts(out_path, data)

    total = sum(len(b) for b in blobs)
    print(f'{os.path.normpath(out_path)}: {len(files)} sprites, {total / 1048576:.1f} MB datos, '
          f'{len(data) / 1048576:.1f} MB total in {len(parts)} part(s): {", ".join(parts)}')


if __name__ == '__main__':
    main()
