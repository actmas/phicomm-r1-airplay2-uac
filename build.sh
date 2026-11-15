#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

CLANG_BIN="${CLANG:-/usr/bin/clang}"
if ! command -v "$CLANG_BIN" >/dev/null 2>&1; then
    CLANG_BIN="$(command -v clang)"
fi

ARM_LD_BIN="${ARM_LD:-/opt/homebrew/bin/arm-linux-gnueabihf-ld}"
if ! command -v "$ARM_LD_BIN" >/dev/null 2>&1; then
    ARM_LD_BIN="$(command -v arm-linux-gnueabihf-ld)"
fi

echo "[*] Compiling r1_init_hook.c with ARM clang ($CLANG_BIN)..."
"$CLANG_BIN" -target armv7a-linux-gnueabi -mfloat-abi=soft -fPIE -fno-stack-protector -O2 \
  -Isrc -c src/r1_init_hook.c -o src/r1_init_hook.o

echo "[*] Linking r1_init_hook with libtinyalsa ($ARM_LD_BIN)..."
"$ARM_LD_BIN" -pie --dynamic-linker=/system/bin/linker \
  --allow-shlib-undefined -rpath-link=src -Lsrc -e main src/r1_init_hook.o -lc -ltinyalsa -lshairport -o src/r1_init_hook

echo "[*] Compiling airplay_server.c with ARM clang..."
"$CLANG_BIN" -target armv7a-linux-gnueabi -mfloat-abi=soft -fPIE -fno-stack-protector -O2 \
  -Isrc -c src/airplay_server.c -o src/airplay_server.o

echo "[*] Linking airplay_server with libtinyalsa and libshairport..."
"$ARM_LD_BIN" -pie --dynamic-linker=/system/bin/linker \
  --allow-shlib-undefined -rpath-link=src -Lsrc -e main src/airplay_server.o -lc -ltinyalsa -lshairport -o src/airplay_server

echo "[*] Patching ELF DT_FLAGS_1 and padding r1_init_hook..."
python3 -c '
import struct

for elf_path in ["src/r1_init_hook", "src/airplay_server"]:
    with open(elf_path, "rb") as f:
        data = bytearray(f.read())
    for i in range(0, len(data) - 8, 4):
        tag, val = struct.unpack("<II", data[i:i+8])
        if tag == 0x6ffffffb and val == 0x8000000:
            struct.pack_into("<II", data, i, tag, 0)
            print(f"    Cleared DT_FLAGS_1 in {elf_path}")
    with open(elf_path, "wb") as f:
        f.write(data)

with open("src/r1_init_hook", "rb") as f:
    data = f.read()
target_size = 887456
assert len(data) <= target_size
patched = data + b"\x00" * (target_size - len(data))
sectors = (target_size + 511) // 512
patched_padded = patched + b"\x00" * (sectors * 512 - target_size)
with open("r1_uac_firmware_hook.bin", "wb") as f:
    f.write(patched_padded)

print(f"[+] Build complete! hook={len(data)} bytes, padded_image={len(patched_padded)} bytes")
'
