#!/usr/bin/env python3
"""Decode the watch battery log (raw 'batlog' partition dump) to CSV + summary.

Usage: batlog_decode.py batlog.bin [out.csv]
Record layout mirrors firmware/main/batlog.h: v2 = 32 bytes (current), v1 =
16 bytes (older dumps). Little-endian.
"""
import struct
import sys
from datetime import datetime

REC1 = struct.Struct("<IIHbBBBBB")        # time seq mv pct flags screen bright period crc
REC2 = struct.Struct("<IIHbBBBBBHH11sB")   # ... period ver sleep_pm sleeps rsv crc
F_EXT, F_BLE, F_BOOT, F_PLUG, F_UNPLUG, F_USB = 0x01, 0x02, 0x04, 0x08, 0x10, 0x20


def crc8(data):
    c = 0
    for b in data:
        c ^= b
        for _ in range(8):
            c = ((c << 1) ^ 0x07) & 0xFF if c & 0x80 else (c << 1) & 0xFF
    return c


def records_v2(blob):
    out = []
    for off in range(0, len(blob) - 31, 32):
        raw = blob[off:off + 32]
        if raw == b"\xff" * 32 or crc8(raw[:31]) != raw[31]:
            continue
        t, seq, mv, pct, flags, screen, bright, period, ver, spm, n, rsv, _ = REC2.unpack(raw)
        if ver != 2:
            continue
        out.append(dict(seq=seq, time=t, mv=mv, pct=pct, flags=flags, screen_s=screen,
                        bright=bright, period_s=period, sleep_pm=spm, sleeps=n,
                        locks=pm_locks(rsv)))
    return out


def pm_locks(rsv):
    """Diagnostic field (firmware pm_top_locks): name1[4] pct1, then either a
    2nd lock name2[4] pct2 (older builds) or BLE conn params itvl u16, latency
    u16, 0xC1 marker."""
    if rsv[0] == 0xFF:
        return ""
    out = []
    name = rsv[0:4].split(b"\0")[0].decode(errors="replace")
    if name:
        out.append(f"{name}:{rsv[4]}%")
    if rsv[9] == 0xC1:
        itvl, lat = struct.unpack("<HH", rsv[5:9])
        out.append(f"conn {itvl * 1.25:.1f}ms lat{lat}")
    elif rsv[9] != 0xFF:
        name2 = rsv[5:9].split(b"\0")[0].decode(errors="replace")
        if name2:
            out.append(f"{name2}:{rsv[9]}%")
    return " ".join(out)


def records_v1(blob):
    out = []
    for off in range(0, len(blob) - 15, 16):
        raw = blob[off:off + 16]
        if raw == b"\xff" * 16 or crc8(raw[:15]) != raw[15]:
            continue
        t, seq, mv, pct, flags, screen, bright, period, _ = REC1.unpack(raw)
        out.append(dict(seq=seq, time=t, mv=mv, pct=pct, flags=flags, screen_s=screen,
                        bright=bright, period_s=period, sleep_pm=None, sleeps=None, locks=""))
    return out


def records(blob):
    # A dump holds one layout; a stray chunk can pass the other's CRC by chance,
    # so keep whichever layout decodes more records.
    out = max(records_v2(blob), records_v1(blob), key=len)
    out.sort(key=lambda r: r["seq"])
    return out


def event_name(f):
    return ("boot" if f & F_BOOT else "plug" if f & F_PLUG
            else "unplug" if f & F_UNPLUG else "")


def fmt_time(t):
    return datetime.fromtimestamp(t).strftime("%Y-%m-%d %H:%M:%S") if t > 1_600_000_000 else f"t={t}"


def summary(recs):
    print(f"{len(recs)} records", file=sys.stderr)
    if not recs:
        return
    print(f"from {fmt_time(recs[0]['time'])} to {fmt_time(recs[-1]['time'])}", file=sys.stderr)
    # On-battery runs: consecutive records without external power or a reboot.
    runs, cur = [], []
    for r in recs:
        if r["time"] < 1_600_000_000:  # clock not set yet (no RTC/phone time)
            if len(cur) > 1:
                runs.append(cur)
            cur = []
            continue
        if r["flags"] & (F_EXT | F_BOOT) or r["pct"] < 0:
            if len(cur) > 1:
                runs.append(cur)
            cur = [] if r["flags"] & F_EXT or r["pct"] < 0 else [r]
            continue
        cur.append(r)
    if len(cur) > 1:
        runs.append(cur)
    print("\non-battery runs:", file=sys.stderr)
    for run in runs:
        a, b = run[0], run[-1]
        hours = (b["time"] - a["time"]) / 3600
        if hours <= 0:
            continue
        secs = sum(r["period_s"] for r in run) or 1
        on = sum(r["screen_s"] for r in run)
        sl = [r for r in run if r["sleep_pm"] is not None and r["period_s"]]
        sleep_txt = ""
        if sl:
            ls = sum(r["sleep_pm"] * r["period_s"] for r in sl) / sum(r["period_s"] for r in sl) / 10
            wk = sum(r["sleeps"] for r in sl) / sum(r["period_s"] for r in sl)
            sleep_txt = f"  light-sleep {ls:4.1f}%  {wk:5.1f} sleeps/s"
        print(f"  {fmt_time(a['time'])}  {hours:6.2f} h  {a['pct']:3d}% -> {b['pct']:3d}%  "
              f"{a['mv']} -> {b['mv']} mV  {(a['pct'] - b['pct']) / hours:5.2f} %/h  "
              f"{(a['mv'] - b['mv']) / hours:6.1f} mV/h  screen on {100 * on / secs:4.1f}%"
              f"{sleep_txt}",
              file=sys.stderr)


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    recs = records(open(sys.argv[1], "rb").read())
    out = open(sys.argv[2], "w") if len(sys.argv) > 2 else sys.stdout
    out.write("seq,time,mv,pct,ext_power,usb,ble,event,screen_on_s,brightness,period_s,"
              "light_sleep_pct,sleeps,pm_locks\n")
    for r in recs:
        f = r["flags"]
        ls = "" if r["sleep_pm"] is None else f"{r['sleep_pm'] / 10:.1f}"
        n = "" if r["sleeps"] is None else r["sleeps"]
        out.write(f"{r['seq']},{fmt_time(r['time'])},{r['mv']},{r['pct']},"
                  f"{int(bool(f & F_EXT))},{int(bool(f & F_USB))},{int(bool(f & F_BLE))},"
                  f"{event_name(f)},{r['screen_s']},{r['bright']},{r['period_s']},{ls},{n},"
                  f"{r['locks']}\n")
    summary(recs)


if __name__ == "__main__":
    main()
