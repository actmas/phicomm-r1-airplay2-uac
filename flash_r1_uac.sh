#!/usr/bin/env bash
# ==============================================================================
# 斐讯 R1 (RK3229) 一键刷入 AirPlay 2 + USB 声卡双模固化部署脚本
# Phicomm R1 (RK3229) One-Click AirPlay 2 + USB DAC (UAC1) Flash & Deploy Script
# ==============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FIRMWARE_HOOK="$SCRIPT_DIR/r1_uac_firmware_hook.bin"
REBOOT_LOADER="$SCRIPT_DIR/reboot_loader"

# 固件 3448 下 /system/bin/busybox 的绝对 eMMC 扇区起始地址 (LBA)
# Absolute eMMC LBA of /system/bin/busybox on Phicomm R1 stock firmware 3448
TARGET_LBA="0x2b16e8"
SECTORS="0x6c6" # 887,808 字节 / bytes (1734 sectors)

# 自动选择 rkdeveloptool (支持 macOS 与 Linux)
OS_TYPE="$(uname -s)"
if [ "$OS_TYPE" = "Darwin" ]; then
    RKTOOL="$SCRIPT_DIR/rkdeveloptool-mac"
elif command -v rkdeveloptool >/dev/null 2>&1; then
    RKTOOL="$(command -v rkdeveloptool)"
else
    RKTOOL="$SCRIPT_DIR/rkdeveloptool"
fi

# 解析 ADB 命令与可选设备序列号 (可通过环境变量 ADB_SERIAL 或参数 -s 指定)
ADB_SERIAL="${ADB_SERIAL:-}"
if [ "$1" = "-s" ] && [ -n "$2" ]; then
    ADB_SERIAL="$2"
fi

find_adb() {
    if command -v adb >/dev/null 2>&1; then
        command -v adb
    elif [ -x "/opt/homebrew/bin/adb" ]; then
        echo "/opt/homebrew/bin/adb"
    elif [ -x "/usr/local/bin/adb" ]; then
        echo "/usr/local/bin/adb"
    elif [ -x "$HOME/Library/Android/sdk/platform-tools/adb" ]; then
        echo "$HOME/Library/Android/sdk/platform-tools/adb"
    else
        return 1
    fi
}

get_adb_cmd() {
    local adb_bin
    adb_bin="$(find_adb)" || return 1
    if [ -n "$ADB_SERIAL" ]; then
        echo "$adb_bin -s $ADB_SERIAL"
    else
        echo "$adb_bin"
    fi
}

echo "========================================================"
echo "  Phicomm R1 AirPlay 2 + USB DAC (UAC1) Flash Tool"
echo "========================================================"

# 检查核心文件
if [ ! -f "$RKTOOL" ] || [ ! -f "$FIRMWARE_HOOK" ]; then
    echo "[-] 缺少核心固化文件 ($RKTOOL 或 $FIRMWARE_HOOK)，请检查目录完整性。"
    echo "[-] Missing required files ($RKTOOL or $FIRMWARE_HOOK)."
    exit 1
fi
chmod +x "$RKTOOL"

# 步骤 1: 检查设备连接模式
echo "[*] 检测设备连接状态 / Checking device connection..."
DEV_LIST=$("$RKTOOL" ld 2>&1 || true)

if echo "$DEV_LIST" | grep -qiE "Loader|Maskrom"; then
    echo "[+] 检测到设备已处于烧录模式: $DEV_LIST"
else
    echo "[*] 未在烧录模式检测到设备，尝试通过 ADB 软重启进入 Loader 模式..."
    ADB=$(get_adb_cmd) || {
        echo "[-] 未找到 adb 命令，请安装 Android platform-tools 或手动短接进入 Loader/Maskrom 模式。"
        exit 1
    }

    DEVICE_STATE=$($ADB get-state 2>/dev/null || true)
    if [ "$DEVICE_STATE" != "device" ]; then
        echo "[-] ADB 未发现唯一在线设备。若连接了多台设备，请使用: ADB_SERIAL=<序列号> ./flash_r1_uac.sh"
        echo "[-] No single ADB device found. If multiple devices are connected, pass ADB_SERIAL=<serial>."
        exit 1
    fi

    echo "[*] 重启进入 Loader 模式 / Rebooting into Rockchip Loader mode..."
    if [ -f "$REBOOT_LOADER" ]; then
        $ADB push "$REBOOT_LOADER" /data/local/tmp/reboot_loader >/dev/null 2>&1 || true
        $ADB shell "su -c 'chmod 755 /data/local/tmp/reboot_loader && /data/local/tmp/reboot_loader'" >/dev/null 2>&1 || true
    fi
    $ADB reboot bootloader 2>/dev/null || true
    echo "[*] 等待设备重启进入 Loader 模式 (最多等待 12 秒)..."

    for i in {1..12}; do
        DEV_LIST=$("$RKTOOL" ld 2>&1 || true)
        if echo "$DEV_LIST" | grep -qiE "Loader|Maskrom"; then
            break
        fi
        sleep 1
    done

    if ! echo "$DEV_LIST" | grep -qiE "Loader|Maskrom"; then
        echo "[-] 未能自动进入 Loader 模式，请尝试拔插 USB 或手动短接进入 Loader/Maskrom 模式。"
        exit 1
    fi
    echo "[+] 成功进入烧录模式: $DEV_LIST"
fi

# 步骤 2: 刷入固化 Hook 镜像
echo "[*] 正在向 eMMC 扇区 $TARGET_LBA 写入固化镜像 ($FIRMWARE_HOOK)..."
"$RKTOOL" wl "$TARGET_LBA" "$FIRMWARE_HOOK"
echo "[+] 镜像写入完成！"

# 步骤 3: 逐字节校验
echo "[*] 正在回读并校验扇区数据完整性..."
VERIFY_TMP="$SCRIPT_DIR/.verify_flash.bin"
"$RKTOOL" rl "$TARGET_LBA" "$SECTORS" "$VERIFY_TMP"
DIFF_OUT=$(cmp "$FIRMWARE_HOOK" "$VERIFY_TMP" 2>&1 || true)
rm -f "$VERIFY_TMP"

if [ -n "$DIFF_OUT" ]; then
    echo "[-] 校验失败，写入数据不匹配！请勿重启设备，重新执行烧录。"
    exit 1
fi
echo "[+] 数据校验 100% 匹配！"

# 步骤 4: 重启设备
echo "[*] 正在重启音箱..."
"$RKTOOL" rd
echo "[+] 设备已开始重启，请等待约 15~20 秒进入系统。"

# 步骤 5: 等待 ADB 上线，禁用斐讯云服务并部署 AirPlay 2 组件
echo ""
echo "[*] 正在等待 ADB 服务重新上线以完成后续配置..."
ADB=$(get_adb_cmd) || {
    echo "[!] 未找到 adb，跳过系统精简与 AirPlay 2 二进制同步，请手动推送 bin/ 下文件至 /data/local/airplay2/。"
    exit 0
}

$ADB wait-for-device 2>/dev/null || true
sleep 3

echo "[*] 禁用斐讯 AI 语音、旧版播放器与蓝牙隐藏监控 (com.phicomm.speaker.device / player / systemtool)..."
$ADB shell "su -c 'CLASSPATH=/system/framework/pm.jar app_process /system/bin com.android.commands.pm.Pm disable com.phicomm.speaker.device'" || true
$ADB shell "su -c 'CLASSPATH=/system/framework/pm.jar app_process /system/bin com.android.commands.pm.Pm disable com.phicomm.speaker.player'" || true
$ADB shell "su -c 'CLASSPATH=/system/framework/pm.jar app_process /system/bin com.android.commands.pm.Pm disable com.phicomm.speaker.systemtool'" || true
$ADB shell "su -c 'settings put global wifi_suspend_optimizations_enabled 0'" || true

echo "[*] 部署 AirPlay 2 守护进程与配置至 /data/local/airplay2/..."
$ADB shell "su -c 'mkdir -p /data/local/airplay2 && chmod 755 /data/local/airplay2'" >/dev/null 2>&1 || true

if [ -f "$SCRIPT_DIR/bin/nqptp" ]; then
    $ADB push "$SCRIPT_DIR/bin/nqptp" /data/local/tmp/nqptp >/dev/null 2>&1 || true
    $ADB shell "su -c 'mv /data/local/tmp/nqptp /data/local/airplay2/nqptp && chmod 755 /data/local/airplay2/nqptp'" >/dev/null 2>&1 || true
fi

if [ -f "$SCRIPT_DIR/bin/shairport-sync" ]; then
    $ADB push "$SCRIPT_DIR/bin/shairport-sync" /data/local/tmp/shairport-sync >/dev/null 2>&1 || true
    $ADB shell "su -c 'mv /data/local/tmp/shairport-sync /data/local/airplay2/shairport-sync && chmod 755 /data/local/airplay2/shairport-sync'" >/dev/null 2>&1 || true
fi

if [ -f "$SCRIPT_DIR/bin/shairport-sync.conf" ]; then
    $ADB push "$SCRIPT_DIR/bin/shairport-sync.conf" /data/local/tmp/shairport-sync.conf >/dev/null 2>&1 || true
    $ADB shell "su -c 'mv /data/local/tmp/shairport-sync.conf /data/local/airplay2/shairport-sync.conf && chmod 644 /data/local/airplay2/shairport-sync.conf'" >/dev/null 2>&1 || true
fi

echo ""
echo "========================================================"
echo "  部署完成！/ Deployment Complete!"
echo "========================================================"
echo "1. USB 有线声卡：免驱识别为 'Phicomm R1 USB Audio Stream' (48kHz / 16-bit)"
echo "2. AirPlay 2：同一 Wi-Fi 下苹果设备可见 'R1-Music' (48kHz / 16-bit)"
echo "3. 物理控制：顶部环形滑块调节音量，中央按键一键静音，保留原厂哈曼卡顿 DSP"
echo "========================================================"
