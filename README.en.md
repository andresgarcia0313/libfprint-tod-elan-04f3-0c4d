# tod-elan0c4d - libfprint driver for the ELAN 04f3:0c4d fingerprint reader

Out-of-tree (TOD) libfprint module for the ELAN `04f3:0c4d` sensor, bcdDevice `0x2410`, found in the
Lenovo IdeaPad 5 14IAL7 (and possibly other 2022 Lenovo laptops). With stock libfprint the built-in
`elan` driver claims the device but enrolling fails ("Calibration failed" / unknown error). The
`elanmoc2` route does not work either: its match-on-chip mode needs SDCP with ELAN's private CA.

This module talks to the sensor as an image device (match on host):

- USB protocol reimplemented for interoperability from observed device behaviour; no vendor code,
  firmware or keys are included. Commands `42 01` (RUNIAP) and `40 27` (WDTRST) are never sent and a
  test guards the sources against them.
- Capture with recalibration and re-initialization when the analog front end returns zero or
  saturated frames, plus typed errors and retries; every step is logged in the `libfprint-elan0c4d`
  GLib domain (`G_MESSAGES_DEBUG=all`).
- Enroll in 12 touches with progress and retry hints (works with KDE Plasma and GNOME settings).
- Matching with SIFT keypoints and a RANSAC similarity check (OpenCV, the same approach as libfprint's
  SIGFM for small sensors). The stored template holds keypoint positions and descriptors only, never
  fingerprint images. Every C++ exception is caught at the C boundary and becomes a `GError`.

Measured on the real sensor (25 touches of one finger, 10 of another): the other finger never exceeded
3 consistent matches (threshold 8); the enrolled finger was accepted on 9 of 13 fresh touches.

## Install (Ubuntu 26.04 / Kubuntu, amd64)

    sudo apt install ./libfprint-2-tod1-elan0c4d_*_amd64.deb   # from the Releases page
    fprintd-list $USER    # must name "ELAN 04f3:0c4d (rev 0x2410)"

Enroll from System Settings → Users → "Configure fingerprint authentication" (KDE) or Settings → Users
(GNOME). The KDE lock screen then accepts the finger. Optional PAM profile for `sudo` and polkit
dialogs, skipping `sudo` without a terminal and SDDM: see `packaging/pam/` and the Spanish README.

## Build and test

    sudo apt install libfprint-2-tod-dev libopencv-dev meson
    meson setup build && ninja -C build && meson test -C build
    packaging/build-deb.sh      # release build, runs the tests, writes build-deb/*.deb

The tests use a scripted sensor and synthetic surfaces (no biometric data) and also pass with
`-Db_sanitize=address,undefined`.

## Status

Works on the author's machine for enroll, lock screen and sudo. Not yet reviewed by libfprint
upstream; feedback and testing on other 04f3:0c4d units are welcome.

License: LGPL-2.1-or-later.
