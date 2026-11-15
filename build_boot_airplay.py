#!/usr/bin/env python3
import gzip
import io
import struct
import hashlib
import os

import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
BOOT_IN = sys.argv[1] if len(sys.argv) > 1 else os.environ.get("BOOT_IN", os.path.join(SCRIPT_DIR, "boot-original.bin"))
if not os.path.exists(BOOT_IN):
    fallback = os.path.join(SCRIPT_DIR, "../.backups/feixun-r1-2026-11-08/boot-checksum-repaired.bin")
    if os.path.exists(fallback):
        BOOT_IN = fallback
BOOT_OUT = os.path.join(SCRIPT_DIR, "boot_airplay.img")

print(f"[*] Reading source boot image: {BOOT_IN}")
with open(BOOT_IN, "rb") as f:
    boot_data = f.read()

page_sz = 16384
orig_hdr = bytearray(boot_data[:page_sz])
magic, ksize, kaddr, rsize, raddr, ssize, saddr, tagsaddr, page_size = struct.unpack_from('<8sIIIIIIII', orig_hdr)
assert magic.startswith(b"ANDROID!"), "Invalid Android boot image"

k_pages = (ksize + page_size - 1) // page_size
r_pages = (rsize + page_size - 1) // page_size
s_pages = (ssize + page_size - 1) // page_size

k_data = boot_data[page_size : page_size + ksize]
r_data = boot_data[(1 + k_pages) * page_size : (1 + k_pages) * page_size + rsize]
s_data = boot_data[(1 + k_pages + r_pages) * page_size : (1 + k_pages + r_pages) * page_size + ssize]

print(f"[*] Kernel: {ksize} bytes ({k_pages} pages)")
print(f"[*] Ramdisk: {rsize} bytes ({r_pages} pages)")
print(f"[*] Second Stage / DTB: {ssize} bytes ({s_pages} pages)")
print(f"[*] Page size: {page_size}")

# Verify original SHA1 hash
ctx = hashlib.sha1()
ctx.update(k_data)
ctx.update(struct.pack('<I', ksize))
ctx.update(r_data)
ctx.update(struct.pack('<I', rsize))
ctx.update(s_data)
ctx.update(struct.pack('<I', ssize))
ctx.update(orig_hdr[32:32 + 544])
assert ctx.digest() == orig_hdr[576:576+20], "Original boot checksum mismatch!"
print(f"[+] Verified source boot SHA1: {ctx.digest().hex()}")

print("[*] Unpacking ramdisk cpio...")
with gzip.open(io.BytesIO(r_data), 'rb') as f:
    cpio_raw = f.read()

idx = 0
records = []
while idx < len(cpio_raw):
    if cpio_raw[idx:idx+6] == b'070701':
        hdr_bytes = cpio_raw[idx:idx+110]
        namesize = int(hdr_bytes[94:102], 16)
        filesize = int(hdr_bytes[54:62], 16)
        name = cpio_raw[idx+110:idx+110+namesize-1].decode('latin1')
        hdr_size = (110 + namesize + 3) & ~3
        file_data = cpio_raw[idx+hdr_size:idx+hdr_size+filesize]
        records.append({
            'hdr': bytearray(hdr_bytes),
            'name': name,
            'data': file_data
        })
        idx += hdr_size + ((filesize + 3) & ~3)
        if name == 'TRAILER!!!':
            break
    else:
        break

print(f"[*] Parsed {len(records)} files in ramdisk")

# 1. Patch init.rk30board.rc
rc_patched = False
for r in records:
    if r['name'] == './init.rk30board.rc':
        old_text = r['data'].decode('latin1')
        target_str = "service r1_airplay2 /sbin/r1-airplay2-autostart.sh\n    class late_start\n    user root\n    group root audio inet net_admin\n    seclabel u:r:init:s0"
        new_str = "service r1_airplay2 /sbin/r1-airplay2-autostart.sh\n    class late_start\n    user shell\n    group shell audio inet net_admin input\n    seclabel u:r:shell:s0"
        assert target_str in old_text, "Target service block not found in init.rk30board.rc"
        new_text = old_text.replace(target_str, new_str)
        r['data'] = new_text.encode('latin1')
        rc_patched = True
        print("[+] Successfully patched init.rk30board.rc -> user shell, seclabel u:r:shell:s0")

assert rc_patched, "Failed to patch init.rk30board.rc"

# 2. Patch init.rc to set SELinux permissive at boot
initrc_patched = False
for r in records:
    if r['name'] == './init.rc':
        old_text = r['data'].decode('latin1')
        target_str = "    write /proc/1/oom_score_adj -1000\n"
        new_str = "    write /proc/1/oom_score_adj -1000\n    write /sys/fs/selinux/enforce 0\n"
        assert target_str in old_text, "Target oom_score_adj not found in init.rc"
        new_text = old_text.replace(target_str, new_str)
        r['data'] = new_text.encode('latin1')
        initrc_patched = True
        print("[+] Successfully patched init.rc -> write /sys/fs/selinux/enforce 0")

assert initrc_patched, "Failed to patch init.rc"

# 3. Patch sbin/r1-airplay2-autostart.sh
new_autostart_sh = """#!/system/bin/sh
export PATH=/system/bin:/system/xbin

# Launch airplay supervisor loop
while true; do
    if [ -x /data/local/tmp/airplay_server ]; then
        /data/local/tmp/airplay_server >> /data/local/tmp/airplay.log 2>&1
    elif [ -x /data/local/airplay2/airplay_server ]; then
        /data/local/airplay2/airplay_server >> /data/local/tmp/airplay.log 2>&1
    fi
    sleep 2
done
"""

sh_patched = False
for r in records:
    if r['name'] == './sbin/r1-airplay2-autostart.sh':
        r['data'] = new_autostart_sh.encode('latin1')
        sh_patched = True
        print("[+] Successfully updated sbin/r1-airplay2-autostart.sh")

assert sh_patched, "Failed to patch sbin/r1-airplay2-autostart.sh"

# 4. Set directory and file permissions in CPIO headers
for r in records:
    if r['name'] == './sbin':
        r['hdr'][14:22] = b'000041ED' # 040755
        print("[+] Set mode 0755 on ./sbin")
    elif r['name'] == './sbin/r1-airplay2-autostart.sh':
        r['hdr'][14:22] = b'000081ED' # 0100755
        print("[+] Set mode 0755 on ./sbin/r1-airplay2-autostart.sh")

# Re-serialize CPIO archive
print("[*] Rebuilding CPIO archive...")
new_cpio = bytearray()
for r in records:
    hdr_bytes = r['hdr']
    if r['name'] != 'TRAILER!!!':
        new_filesize = len(r['data'])
        hdr_bytes[54:62] = f"{new_filesize:08x}".upper().encode('ascii')
    new_cpio.extend(hdr_bytes)

    name_bytes = r['name'].encode('latin1') + b'\x00'
    new_cpio.extend(name_bytes)

    pad1 = ((110 + len(name_bytes) + 3) & ~3) - (110 + len(name_bytes))
    new_cpio.extend(b'\x00' * pad1)

    if len(r['data']) > 0:
        new_cpio.extend(r['data'])
        pad2 = ((len(r['data']) + 3) & ~3) - len(r['data'])
        new_cpio.extend(b'\x00' * pad2)

# Add trailer and 512-byte block padding
pad512 = (512 - (len(new_cpio) % 512)) % 512
new_cpio.extend(b'\x00' * pad512)

print(f"[*] New uncompressed ramdisk: {len(new_cpio)} bytes")

# Gzip compress ramdisk
new_ramdisk_gz = gzip.compress(bytes(new_cpio), compresslevel=9, mtime=0)
new_rsize = len(new_ramdisk_gz)
print(f"[*] New compressed ramdisk: {new_rsize} bytes")

# Update boot header ramdisk size at offset 16
struct.pack_into('<I', orig_hdr, 16, new_rsize)

# Recalculate Android Boot SHA1 checksum at offset 576
ctx = hashlib.sha1()
ctx.update(k_data)
ctx.update(struct.pack('<I', ksize))
ctx.update(new_ramdisk_gz)
ctx.update(struct.pack('<I', new_rsize))
ctx.update(s_data)
ctx.update(struct.pack('<I', ssize))
ctx.update(orig_hdr[32:32 + 544])
new_sha = ctx.digest()
print(f"[+] Recomputed boot image SHA1: {new_sha.hex()}")

orig_hdr[576:576 + 20] = new_sha
orig_hdr[576 + 20:576 + 32] = b'\x00' * 12

def pad_page(data, page_sz=16384):
    rem = len(data) % page_sz
    if rem > 0:
        return data + b'\x00' * (page_sz - rem)
    return data

# Build final boot image with Kernel + Ramdisk + Second Stage (DTB)
boot_out = bytearray()
boot_out.extend(orig_hdr) # 16384 bytes
boot_out.extend(pad_page(k_data, page_sz))
boot_out.extend(pad_page(new_ramdisk_gz, page_sz))
boot_out.extend(pad_page(s_data, page_sz))

target_total_size = len(boot_data)
if len(boot_out) < target_total_size:
    boot_out.extend(b'\x00' * (target_total_size - len(boot_out)))

with open(BOOT_OUT, "wb") as f:
    f.write(boot_out)

print(f"[+] Boot image built successfully: {BOOT_OUT} ({len(boot_out)} bytes, 0x{len(boot_out):x})")
