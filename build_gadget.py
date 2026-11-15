#!/usr/bin/env python3
import os
import shutil
import subprocess

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_DIR = os.path.join(SCRIPT_DIR, "src")

CLANG_BIN = os.environ.get("CLANG") or shutil.which("clang") or "/usr/bin/clang"
ARM_LD_BIN = os.environ.get("ARM_LD") or shutil.which("arm-linux-gnueabihf-ld") or "/opt/homebrew/bin/arm-linux-gnueabihf-ld"

c_file = os.path.join(SRC_DIR, "uac1_gadget.c")
o_file = os.path.join(SRC_DIR, "uac1_gadget.o")
ko_file = os.path.join(SRC_DIR, "uac1_gadget.ko")
h_file = os.path.join(SRC_DIR, "uac1_gadget_blob.h")

print(f"[*] Compiling uac1_gadget.c with {CLANG_BIN} (ARM mode, no-pic)...")
subprocess.run([
    CLANG_BIN,
    "-target", "armv7a-linux-gnueabi",
    "-marm",
    "-fno-pic",
    "-fno-pie",
    "-fno-builtin",
    "-O2",
    "-c", c_file,
    "-o", o_file
], check=True)

print(f"[*] Linking relocatable kernel module uac1_gadget.ko with {ARM_LD_BIN}...")
subprocess.run([
    ARM_LD_BIN,
    "-r", o_file,
    "-o", ko_file
], check=True)

with open(ko_file, "rb") as f:
    ko_bytes = f.read()

print(f"[*] Generating {h_file} ({len(ko_bytes)} bytes)...")
lines = ["static const unsigned char uac1_gadget_ko[] = {"]
for i in range(0, len(ko_bytes), 16):
    chunk = ko_bytes[i:i+16]
    hex_str = ", ".join(f"0x{b:02x}" for b in chunk)
    lines.append(f"    {hex_str},")
lines.append("};")
lines.append(f"static const unsigned int uac1_gadget_ko_len = {len(ko_bytes)};\n")

with open(h_file, "w") as f:
    f.write("\n".join(lines))

print("[+] uac1_gadget_blob.h updated successfully!")
