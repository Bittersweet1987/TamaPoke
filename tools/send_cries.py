#!/usr/bin/env python3
"""Sendet die konvertierten Rufe (tools/sdcard/cries/*.pcm) per USB auf die
SD-Karte des Boards, ins Verzeichnis /cries. Nutzt dasselbe PUT-Protokoll wie
send_sd.py (siehe sdmon.cpp: sdSerialCommand).

  python3 tools/send_cries.py                     # Port automatisch suchen
  python3 tools/send_cries.py --port COM4          # Windows: expliziter Port
  python3 tools/send_cries.py --port /dev/cu.usbmodem101
"""
import argparse
import glob
import os
import sys
import time
import serial
from serial.tools import list_ports

ESPRESSIF_VID = 0x303A  # native USB der ESP32-S3 (siehe Geraete-Manager: VID_303A)


def find_port():
    for p in list_ports.comports():
        if p.vid == ESPRESSIF_VID:
            return p.device
    unix_ports = glob.glob('/dev/cu.usbmodem*')
    if unix_ports:
        return unix_ports[0]
    sys.exit("Board nicht gefunden - Port mit --port COM4 (o.ae.) angeben")


def wait_line(ser, expect, timeout=10):
    end = time.time() + timeout
    while time.time() < end:
        line = ser.readline().decode(errors='replace').strip()
        if not line:
            continue
        print(f"  Board: {line}")
        if line == expect:
            return True
        if line == 'ERR':
            return False
    return False


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--port')
    args = ap.parse_args()

    port = args.port or find_port()
    print(f"Port {port}")
    ser = serial.Serial(port, 115200, timeout=1)
    time.sleep(1.5)
    ser.reset_input_buffer()

    files = sorted(glob.glob(os.path.join(os.path.dirname(__file__), 'sdcard', 'cries', '*.pcm')))
    if not files:
        sys.exit("keine .pcm-Dateien - vorher tools/pack_cries.py ausfuehren")

    for path in files:
        size = os.path.getsize(path)
        name = f"cries/{os.path.basename(path)}"
        print(f"-> {name} ({size/1024:.0f} KB)")
        ser.write(f"PUT {name} {size}\n".encode())
        if not wait_line(ser, 'OK', 5):
            print("   Board hat PUT abgelehnt, weiter mit der naechsten Datei")
            continue
        t0 = time.time()
        ok = True
        with open(path, 'rb') as f:
            while chunk := f.read(2048):
                ser.write(chunk)
                ack = ''
                while ack not in ('#', 'ERR'):
                    ack = ser.readline().decode(errors='replace').strip()
                    if ack == '':
                        ok = False
                        break
                if not ok or ack == 'ERR':
                    ok = False
                    break
        if ok and wait_line(ser, 'DONE', 30):
            kbs = size / 1024 / max(0.01, time.time() - t0)
            print(f"   ok ({kbs:.0f} KB/s)")
        else:
            print("   Uebertragung fehlgeschlagen")
    print("fertig")


if __name__ == '__main__':
    main()
