#!/usr/bin/env bash
# Builds libfprint-2-tod1-elan0c4d_<version>_<arch>.deb from this tree (release, stripped).
# Runtime dependencies come from the libraries the module really links (ldd + dpkg -S).
# Usage: packaging/build-deb.sh [outdir]   (default: build-deb/)
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
out="${1:-$root/build-deb}"
arch="$(dpkg --print-architecture)"
multiarch="$(dpkg-architecture -qDEB_HOST_MULTIARCH)"
version="$(meson introspect --projectinfo "$root/meson.build" | python3 -c 'import json,sys; print(json.load(sys.stdin)["version"])')"
pkg="libfprint-2-tod1-elan0c4d"
stage="$out/stage"

log() { printf '%s [build-deb] %s\n' "$(date +%T)" "$*"; }

rm -rf "${stage:?ruta vacia}"
log "compilando $version para $arch"
meson setup --wipe "$out/meson" "$root" --buildtype=release --prefix=/usr --libdir="lib/$multiarch" >/dev/null
ninja -C "$out/meson" >/dev/null
meson test -C "$out/meson" --no-rebuild --print-errorlogs >/dev/null || { log "las pruebas fallan: no se empaqueta"; exit 1; }
DESTDIR="$stage" meson install -C "$out/meson" --no-rebuild --strip >/dev/null

so="$stage/usr/lib/$multiarch/libfprint-2/tod-1/libelan0c4d-tod.so"
[ -f "$so" ] || { log "no se instaló $so"; exit 1; }
# Only the libraries the module names itself (NEEDED), not everything they pull in.
needed="$(objdump -p "$so" | awk '/NEEDED/{print $2}')"
depends="$(for lib in $needed; do ldconfig -p | awk -v l="$lib" '$1 == l && /x86-64/{print $NF; exit}'; done \
  | xargs -r -n1 readlink -f | xargs -r dpkg -S 2>/dev/null | cut -d: -f1 | sort -u \
  | grep -vxE 'libc6|libfprint-2-tod1' | paste -sd, | sed 's/,/, /g')"
# Built and tested against this libfprint: apt keeps it back instead of loading the module into a
# libfprint whose internals this code was never run with (same upstream series only).
tod_ver="$(dpkg-query -W -f='${Version}' libfprint-2-tod1)"
tod_min="${tod_ver%%-*}"
tod_max="$(printf '%s' "$tod_min" | sed -E 's/^([0-9]+:)?([0-9]+)\.([0-9]+).*/\1\2.\3/' | awk -F. '{print $1"."$2+1"~"}')"
log "depende de: $depends; libfprint-2-tod1 >= $tod_min y << $tod_max"

mkdir -p "$stage/DEBIAN" "$stage/usr/share/doc/$pkg"
cp "$root/README.md" "$stage/usr/share/doc/$pkg/"
cat > "$stage/DEBIAN/control" <<CONTROL
Package: $pkg
Version: $version
Architecture: $arch
Maintainer: Andres Garcia <andresgarcia0313@gmail.com>
Depends: libfprint-2-tod1 (>= $tod_min), libfprint-2-tod1 (<< $tod_max), $depends
Section: libs
Priority: optional
Description: libfprint TOD driver for the ELAN 04f3:0c4d fingerprint reader
 Out-of-tree libfprint module for the ELAN 04f3:0c4d (bcdDevice 0x2410) found
 in Lenovo IdeaPad 5 14IAL7. Captures, enrolls in 12 touches and matches with
 SIFT keypoints. fprintd picks it over the built-in elan driver.
CONTROL
# fprintd keeps the old module loaded until it restarts; it is D-Bus activated again on next use.
for script in postinst postrm; do
  printf '#!/bin/sh\nset -e\nsystemctl try-restart fprintd.service 2>/dev/null || true\n' > "$stage/DEBIAN/$script"
  chmod 0755 "$stage/DEBIAN/$script"
done
deb="$out/${pkg}_${version}_${arch}.deb"
dpkg-deb --root-owner-group --build "$stage" "$deb" >/dev/null
log "paquete: $deb"
