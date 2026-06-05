#!/usr/bin/env python3
"""Timestamped serial capture for field stress testing.

Reads the device serial line-by-line and writes each line prefixed with a host
wall-clock time AND a monotonic offset (seconds since capture start). The
monotonic offset is what the analyzer uses to measure intervals — reconnect
backoff growth, recovery time, reboot spacing — because the device's own log
lines are not wall-clock timestamped.

Usage:
    python3 capture.py [PORT] [BAUD] [OUTFILE]
Defaults: /dev/cu.usbserial-58960008911 115200 stress-capture.log
Append mode, line-buffered, mirrors to stdout. Reconnects if the port drops
(e.g. during a power cycle) so a single capture spans the whole test.
"""
import sys
import time
import datetime

try:
    import serial  # pyserial (bundled with PlatformIO's penv)
except ImportError:
    sys.stderr.write("pyserial not found; run with PlatformIO's python:\n"
                     "  ~/.platformio/penv/bin/python capture.py ...\n")
    sys.exit(2)

PORT = sys.argv[1] if len(sys.argv) > 1 else "/dev/cu.usbserial-58960008911"
BAUD = int(sys.argv[2]) if len(sys.argv) > 2 else 115200
OUT = sys.argv[3] if len(sys.argv) > 3 else "stress-capture.log"

t0 = time.monotonic()


def stamp(text):
    ts = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
    return f"{ts} +{time.monotonic() - t0:10.3f} | {text}\n"


def emit(f, text):
    line = stamp(text)
    f.write(line)
    sys.stdout.write(line)
    sys.stdout.flush()


def main():
    with open(OUT, "a", buffering=1) as f:
        emit(f, f"[CAPTURE] start port={PORT} baud={BAUD}")
        buf = b""
        ser = None
        while True:
            if ser is None:
                try:
                    # open without asserting DTR/RTS so re-attaching after a power
                    # cycle doesn't itself reset the ESP32 (we want the natural boot).
                    ser = serial.Serial()
                    ser.port = PORT
                    ser.baudrate = BAUD
                    ser.timeout = 1
                    ser.dtr = False
                    ser.rts = False
                    ser.open()
                    emit(f, "[CAPTURE] port opened")
                except Exception as e:
                    emit(f, f"[CAPTURE] open failed ({e}); retrying")
                    time.sleep(1)
                    continue
            try:
                data = ser.read(256)
            except Exception as e:
                emit(f, f"[CAPTURE] read error ({e}); reopening (power cycle?)")
                try:
                    ser.close()
                except Exception:
                    pass
                ser = None
                time.sleep(1)
                continue
            if not data:
                continue
            buf += data
            while b"\n" in buf:
                raw, buf = buf.split(b"\n", 1)
                emit(f, raw.decode("utf-8", "replace").rstrip("\r"))


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
