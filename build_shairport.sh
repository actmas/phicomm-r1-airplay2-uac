#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SYSROOT="$SCRIPT_DIR/toolchain/sysroot"

export PATH="/opt/homebrew/bin:/opt/homebrew/opt/libtool/libexec/gnubin:$PATH"
export PKG_CONFIG="$(command -v pkg-config)"
export PKG_CONFIG_SYSROOT_DIR=""
export PKG_CONFIG_LIBDIR="$SYSROOT/usr/lib/pkgconfig"

export CC="$SCRIPT_DIR/toolchain/arm-cc"
export CXX="$SCRIPT_DIR/toolchain/arm-cxx"
export AR="${AR:-arm-linux-gnueabihf-ar}"
export RANLIB="${RANLIB:-arm-linux-gnueabihf-ranlib}"
export STRIP="${STRIP:-arm-linux-gnueabihf-strip}"

export CFLAGS="-O2 -I/usr/include/plist"
export LDFLAGS="-static"
export LIBS="-lssl -lcrypto"

cd "$SCRIPT_DIR/shairport-sync"

./configure \
  --host=arm-linux-musleabihf \
  --with-airplay-2 \
  --with-ssl=openssl \
  --with-tinysvcmdns \
  --with-pipe \
  --with-stdout \
  --with-metadata \
  --with-piddir=/dev/shm
