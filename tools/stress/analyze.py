#!/usr/bin/env python3
"""Field-stress log analyzer for HyphenOS / HyphenConnect.

Reads a capture from capture.py (lines: "HH:MM:SS.mmm +<mono> | <device>") and
checks the invariants our hardening was supposed to deliver, printing a PASS/FAIL
report. Re-run any time during a test (cheap one-shot pass).

    python3 analyze.py [LOGFILE]   # default: stress-capture.log

Tuned to the device's real log vocabulary:
  reconnect backoff  : "[Runner] ... retry in <N> ms"   -> must GROW, cap 30 s
  rebuild / drop     : "[diag] rebuild ...", "mqtt loop detected dropped ..."
  recovery           : "[diag] connection established in <ms> ms (heap=<H>)"
  cellular link      : "[diag] cellular probe: reg=<r> signal=<s>"
  data path          : "Internet path test: got response" / failures
  heap               : "(heap=<H>)" and "MEMORY CHANGE: ... current <H> ... delta <d>"
  reboot             : ESP32 "rst:0x..(REASON)" AND the [SIMILIE] millis counter
                       collapsing back toward zero
  V4 time restore    : "Time restored from persist"
  V2 offline boot    : "proceeding offline after max attempts"
"""
import re
import sys

LOG = sys.argv[1] if len(sys.argv) > 1 else "stress-capture.log"

PREFIX = re.compile(r"^\d\d:\d\d:\d\d\.\d{3}\s+\+\s*([\d.]+)\s+\|\s?(.*)$")
SIMILIE_MS = re.compile(r"^(\d{6,12})\s+\[SIMILIE\]")

RE_RETRY = re.compile(r"retry in (\d+)\w*\s*ms")  # tolerate ArduinoLog "%lu" -> "500u ms"
RE_ESTAB = re.compile(r"connection established in (\d+)\w*\s*ms.*heap=(\d+)", re.I)
RE_REBUILD = re.compile(r"rebuild: tearing down.*attempts=(\d+).*heap=(\d+)", re.I)
RE_DROP = re.compile(r"mqtt loop detected dropped connection|processorLoops detected connection issue", re.I)
RE_CELL = re.compile(r"cellular probe: reg=(\d+)\s+signal=(\d+)", re.I)
RE_PATH_OK = re.compile(r"Internet path test:.*(200 OK|got response)", re.I)
RE_PATH_BAD = re.compile(r"Internet path test:.*(fail|timeout|error|no response)", re.I)
RE_MAINT_OK = re.compile(r"maintain: healthy \(heap=(\d+)\)", re.I)
RE_HEAP = re.compile(r"heap=(\d+)", re.I)
RE_MEM = re.compile(r"MEMORY CHANGE:.*current (\d+).*delta (-?\d+)", re.I)
RE_RESET = re.compile(r"rst:0x([0-9a-fA-F]+)\s*\(([^)]+)\)")
RE_TIME_RESTORE = re.compile(r"Time restored from persist", re.I)
# the authoritative V4 signal: raw Serial.printf in setup(), before ArduinoLog is up
RE_BOOT_RESTORE = re.compile(r"\[BOOT\] time restored=(\d+) hasTime=(\d+) unix=(\d+)")
RE_OFFLINE = re.compile(r"proceeding offline after max attempts", re.I)
RE_PUB_OK = re.compile(r"PUBLISHING STATUS: 1", re.I)

G, R, Y, D, O = "\033[32m", "\033[31m", "\033[33m", "\033[2m", "\033[0m"


def fmt(s):
    return f"{int(s)//60:02d}:{s%60:06.3f}"


def parse(path):
    ev = []
    last_ms = None
    try:
        with open(path, "r", errors="replace") as f:
            for ln in f:
                m = PREFIX.match(ln)
                if not m:
                    continue
                mono, text = float(m.group(1)), m.group(2)
                if text.startswith("[CAPTURE]"):
                    # only a mid-capture reopen indicates a real port loss (likely a
                    # power cycle). "open failed" is just the port not yet available
                    # (e.g. the IDE monitor still holding it) — not a reboot.
                    if "reopening" in text:
                        ev.append((mono, "__PORT_GAP__"))
                    continue
                # detect a device millis reset (reboot) via the [SIMILIE] counter
                sm = SIMILIE_MS.match(text)
                if sm:
                    cur = int(sm.group(1))
                    if last_ms is not None and cur + 30000 < last_ms:
                        ev.append((mono, f"__MS_RESET__ {last_ms}->{cur}"))
                    last_ms = cur
                ev.append((mono, text))
    except FileNotFoundError:
        sys.exit(f"log not found: {path}")
    return ev


def main():
    ev = parse(LOG)
    dev = [(t, x) for t, x in ev if not x.startswith("__")]
    if not dev:
        sys.exit(f"no device lines in {LOG} yet — capture attached? port freed?")
    span = dev[-1][0] - dev[0][0]

    retries, estab, rebuilds, drops = [], [], [], []
    cells, mems, heaps, resets, ms_resets = [], [], [], [], []
    port_gaps, time_restores, offlines, pubs, path_bad = [], [], [], [], []
    boot_restores = []
    for t, x in ev:
        if x.startswith("__MS_RESET__"):
            ms_resets.append((t, x)); continue
        if x == "__PORT_GAP__":
            if not port_gaps or t - port_gaps[-1] > 5:
                port_gaps.append(t)
            continue
        if m := RE_RETRY.search(x): retries.append((t, int(m.group(1))))
        if m := RE_ESTAB.search(x): estab.append((t, int(m.group(1)), int(m.group(2))))
        if m := RE_REBUILD.search(x): rebuilds.append((t, int(m.group(1)), int(m.group(2))))
        if RE_DROP.search(x): drops.append((t, x))
        if m := RE_CELL.search(x): cells.append((t, int(m.group(1)), int(m.group(2))))
        if RE_PATH_BAD.search(x): path_bad.append((t, x))
        if m := RE_MEM.search(x): mems.append((t, int(m.group(1)), int(m.group(2))))
        elif m := RE_MAINT_OK.search(x): heaps.append((t, int(m.group(1))))
        elif m := RE_HEAP.search(x): heaps.append((t, int(m.group(1))))
        if m := RE_RESET.search(x): resets.append((t, m.group(2)))
        if RE_TIME_RESTORE.search(x): time_restores.append((t, x))
        if m := RE_BOOT_RESTORE.search(x): boot_restores.append((t, int(m.group(1)), int(m.group(3))))
        if RE_OFFLINE.search(x): offlines.append((t, x))
        if RE_PUB_OK.search(x): pubs.append(t)

    # unify heap series (MEMORY CHANGE current + heap= samples)
    heap_series = sorted([(t, h) for t, h, _ in mems] + heaps)

    print(f"\n{'='*74}\nHyphenOS field-stress report  ({LOG})\n{'='*74}")
    print(f"span {fmt(span)}  | {len(dev)} device lines | {len(pubs)} successful publishes")

    res = []

    # boots ------------------------------------------------------------------
    # A single reboot emits several signals within a few seconds (rst banner,
    # millis-counter reset, sometimes a port gap). Coalesce them into one boot.
    signals = sorted([(t, f"rst({r})") for t, r in resets]
                     + [(t, x.replace('__MS_RESET__ ', 'millis-reset ')) for t, x in ms_resets]
                     + [(t, "port gap") for t in port_gaps])
    allboot = []
    for t, what in signals:
        if allboot and t - allboot[-1][0] < 8:
            allboot[-1][1].append(what)
        else:
            allboot.append([t, [what]])

    print(f"\n{D}-- boots / power events --{O}")
    for t, whats in allboot:
        print(f"  {fmt(t)}  reboot  [{', '.join(whats)}]")
    if not allboot:
        print("  (none — device stayed up the whole capture)")
    if port_gaps:
        print(f"  {D}(serial port gaps seen: {len(port_gaps)} — expected during power cycles){O}")
    loop = any(allboot[i+1][0]-allboot[i][0] < 45 for i in range(len(allboot)-1))
    res.append(("no reboot-loop", not loop,
                f"{len(allboot)} reboot(s)" + (", some <45s apart!" if loop else ", well spaced" if allboot else "")))

    # cellular link ----------------------------------------------------------
    print(f"\n{D}-- cellular link (reg/signal) --{O}")
    if cells:
        # report transitions only
        prev = None
        for t, reg, sig in cells:
            st = (reg, sig >= 99 or sig == 0)
            if st != prev:
                flag = R+"DOWN"+O if (reg == 0 or sig == 0 or sig >= 99) else G+"up"+O
                print(f"  {fmt(t)}  reg={reg} signal={sig}  [{flag}]")
                prev = st
        downs = [t for t, reg, sig in cells if reg == 0 or sig == 0 or sig >= 99]
        print(f"  ({len(cells)} probes, {len(downs)} showing link down)")
    else:
        print("  (no cellular probes parsed yet)")

    # backoff ----------------------------------------------------------------
    print(f"\n{D}-- reconnect backoff --{O}")
    if retries:
        epis, cur = [], []
        for t, n in retries:
            if cur and t - cur[-1][0] > 60:
                epis.append(cur); cur = []
            cur.append((t, n))
        if cur: epis.append(cur)
        grew_any, flat_any = False, False
        for k, e in enumerate(epis):
            ns = [n for _, n in e]
            grew = len(set(ns)) > 1 and ns == sorted(ns)
            grew_any |= grew
            if len(ns) >= 3 and len(set(ns)) == 1: flat_any = True
            tag = G+"grows"+O if grew else (R+"FLAT"+O if len(ns) >= 3 else Y+"short"+O)
            cap = ", capped@30s" if grew and max(ns) >= 30000 else ""
            print(f"  episode {k+1} @ {fmt(e[0][0])}: {ns} [{tag}{cap}]")
        ok = grew_any and not flat_any
        res.append(("backoff grows, not a storm", ok if (grew_any or flat_any) else None,
                    "exponential growth seen" if ok else
                    ("FLAT 500ms — OLD build / backoff inactive" if flat_any else "inconclusive")))
    else:
        print("  (no reconnect retries yet — pull the antenna to force a drop)")
        res.append(("backoff grows, not a storm", None, "no outage observed yet"))

    # recovery ---------------------------------------------------------------
    print(f"\n{D}-- recovery (rebuild/drop -> established) --{O}")
    faults = sorted([t for t, *_ in drops] + [t for t, *_ in rebuilds]
                    + [t for t, *_ in path_bad])
    for t, ms, h in estab:
        prior = [x for x in faults if x <= t]
        since = f"{t-prior[-1]:.1f}s after fault" if prior else "(initial)"
        print(f"  {fmt(t)}  established in {ms}ms  heap={h}  {since}")
    if not estab:
        print("  (no 'connection established' yet)")
    if rebuilds:
        res.append(("recovered after outage", len(estab) > 0,
                    f"{len(rebuilds)} rebuild(s), {len(estab)} reconnect(s)"))

    # time restore (V4) ------------------------------------------------------
    if allboot or boot_restores:
        print(f"\n{D}-- V4 time restore on boot --{O}")
        for t, restored, unix in boot_restores:
            tag = (G + f"restored, unix={unix}" + O) if restored else (R + "NOT restored (no time)" + O)
            print(f"  {fmt(t)}  [BOOT] {tag}")
        if not boot_restores:
            print("  ([BOOT] status line not seen — pre-fix firmware, or boot missed)")
        ok_restores = [b for b in boot_restores if b[1]]
        res.append(("time restored on boot (offline)", len(ok_restores) > 0,
                    f"{len(ok_restores)}/{len(boot_restores)} boot(s) had a valid clock before network"))

    if offlines:
        print(f"\n{D}-- V2 offline boot --{O}")
        for t, _ in offlines:
            print(f"  {fmt(t)}  proceeded offline after max attempts")
        res.append(("offline-proceed (no recursion)", True, f"{len(offlines)}x"))

    # heap -------------------------------------------------------------------
    print(f"\n{D}-- heap --{O}")
    if len(heap_series) >= 2:
        f0, fN = heap_series[0][1], heap_series[-1][1]
        lo = min(h for _, h in heap_series)
        worst_delta = min((d for *_, d in mems), default=0)
        print(f"  first={f0} last={fN} min={lo} samples={len(heap_series)} "
              f"net={fN-f0:+d} (worst single delta={worst_delta:+d})")
        leak = fN < f0 * 0.85 and span > 120
        res.append(("heap stable (no leak)", not leak,
                    f"net {fN-f0:+d} over {fmt(span)}"))
    else:
        print("  (need more heap samples)")

    print(f"\n{'='*74}\nINVARIANTS\n{'='*74}")
    for label, ok, detail in res:
        mark = G+"PASS"+O if ok else (Y+"n/a "+O if ok is None else R+"FAIL"+O)
        print(f"  [{mark}] {label:30s} {D}{detail}{O}")
    print()


if __name__ == "__main__":
    main()
