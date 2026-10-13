#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Manual check against the real sensor: open, N captures through enroll, close.

With the comparator stub every enroll ends in NOT_SUPPORTED right after a usable
frame, so that error means "captured". The driver waits until cancelled, like under
fprintd, so each touch is cancelled after limit_s. No image is stored.
Usage: FP_TOD_DRIVERS_DIR=<dir with the .so> G_MESSAGES_DEBUG=all capture-real.py [touches] [pause_s] [limit_s]
"""
import sys
import time

import gi

gi.require_version("FPrint", "2.0")
from gi.repository import FPrint, Gio, GLib  # noqa: E402

DEVICE_ERRORS = FPrint.DeviceError.quark()


def capture(dev, n, limit_s):
    """One enroll attempt cancelled after limit_s; returns a short verdict instead of raising."""
    started = time.monotonic()
    cancellable = Gio.Cancellable()
    timer = GLib.timeout_add_seconds(limit_s, lambda: cancellable.cancel() or False)
    try:
        dev.enroll_sync(FPrint.Print.new(dev), cancellable, None, None)
        verdict = "enrolled (comparator is no longer the stub?)"
    except GLib.Error as exc:
        if exc.matches(DEVICE_ERRORS, FPrint.DeviceError.NOT_SUPPORTED):
            verdict = "captured"
        elif exc.matches(Gio.io_error_quark(), Gio.IOErrorEnum.CANCELLED):
            verdict = f"no finger within {limit_s}s (cancelled)"
        else:
            verdict = f"failed {exc.domain}:{exc.code} {exc.message}"
    finally:
        if not cancellable.is_cancelled():
            GLib.source_remove(timer)
    print(f"RESULT touch {n}: {verdict} in {time.monotonic() - started:.1f}s", flush=True)
    return verdict == "captured"


def main(touches, pause_s, limit_s):
    ctx = FPrint.Context()
    ctx.enumerate()
    devices = [d for d in ctx.get_devices() if d.get_driver() == "elan0c4d-tod"]
    if not devices:
        print("RESULT elan0c4d-tod did not claim any device", flush=True)
        return 2
    dev = devices[0]
    try:
        dev.open_sync()
    except GLib.Error as exc:
        print(f"RESULT open failed: {exc.message}", flush=True)
        return 3
    ok = 0
    try:
        for n in range(1, touches + 1):
            ok += capture(dev, n, limit_s)
            time.sleep(pause_s)
    except KeyboardInterrupt:
        print("RESULT cancelled", flush=True)
    finally:
        try:
            dev.close_sync()
        except GLib.Error as exc:
            print(f"RESULT close failed: {exc.message}", flush=True)
    print(f"RESULT {ok}/{touches} captures", flush=True)
    return 0 if ok == touches else 1


if __name__ == "__main__":
    args = sys.argv[1:]
    sys.exit(main(int(args[0]) if args else 5, float(args[1]) if len(args) > 1 else 2.0,
                  int(args[2]) if len(args) > 2 else 30))
