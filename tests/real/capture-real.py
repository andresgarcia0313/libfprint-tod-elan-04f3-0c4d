#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Manual check against the real sensor: one full enroll as fprintd/KDE run it.

Prints every stage and every retry. While the comparator is the stub the enroll
ends in NOT_SUPPORTED after the last stage, which here means "all touches
captured". The driver waits until cancelled, like under fprintd, so the whole
enroll is cancelled after limit_s. No image is stored.
Usage: FP_TOD_DRIVERS_DIR=<dir with the .so> G_MESSAGES_DEBUG=all capture-real.py [limit_s]
"""
import sys
import time

import gi

gi.require_version("FPrint", "2.0")
from gi.repository import FPrint, Gio, GLib  # noqa: E402

STARTED = time.monotonic()


def progress(_dev, stage, _print, _data, error):
    """Called by libfprint after each accepted touch or retry."""
    when = time.monotonic() - STARTED
    if error:
        print(f"RESULT retry at {when:.1f}s: {error.message}", flush=True)
    else:
        print(f"RESULT stage {stage} accepted at {when:.1f}s", flush=True)


def enroll(dev, limit_s):
    """Run one enroll, cancelled after limit_s; return the verdict text."""
    cancellable = Gio.Cancellable()
    timer = GLib.timeout_add_seconds(limit_s, lambda: cancellable.cancel() or False)
    try:
        dev.enroll_sync(FPrint.Print.new(dev), cancellable, progress, None)
        return "enrolled"
    except GLib.Error as exc:
        if exc.matches(FPrint.DeviceError.quark(), FPrint.DeviceError.NOT_SUPPORTED):
            return "all touches captured (comparator still pending)"
        if exc.matches(Gio.io_error_quark(), Gio.IOErrorEnum.CANCELLED):
            return f"not finished within {limit_s}s (cancelled)"
        return f"failed {exc.domain}:{exc.code} {exc.message}"
    finally:
        if not cancellable.is_cancelled():
            GLib.source_remove(timer)


def main(limit_s):
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
    try:
        print(f"RESULT {dev.get_nr_enroll_stages()} stages: {enroll(dev, limit_s)}", flush=True)
    except KeyboardInterrupt:
        print("RESULT cancelled", flush=True)
    finally:
        try:
            dev.close_sync()
        except GLib.Error as exc:
            print(f"RESULT close failed: {exc.message}", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main(int(sys.argv[1]) if len(sys.argv) > 1 else 180))
