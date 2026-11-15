<#
.SYNOPSIS
    Phicomm R1 (RK3229) One-Click AirPlay 2 + USB DAC (UAC1) Flash & Deploy Script for Windows
    斐讯 R1 (RK3229) Windows 一键刷入 AirPlay 2 + USB 声卡双模固化部署脚本
.PARAMETER Serial
    Optional ADB device serial number (or set $env:ADB_SERIAL)
#>
param(
    [string]$Serial = $env:ADB_SERIAL
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$FirmwareHook = Join-Path $ScriptDir "r1_uac_firmware_hook.bin"
$RebootLoader = Join-Path $ScriptDir "reboot_loader"

# Stock firmware 3448 /system/bin/busybox eMMC LBA
$TargetLba = "0x2b16e8"
$Sectors = "0x6c6" # 887,808 bytes (1734 sectors)

function Find-Tool {
    param([string[]]$Candidates)
    foreach ($c in $Candidates) {
        $cmd = Get-Command $c -ErrorAction SilentlyContinue
        if ($cmd) { return $cmd.Source }
        if (Test-Path $c) { return (Resolve-Path $c).Path }
    }
    return $null
}

$RkTool = Find-Tool @(
    (Join-Path $ScriptDir "rkdeveloptool.exe"),
    (Join-Path $ScriptDir "rkdeveloptool-win.exe"),
    "rkdeveloptool.exe",
    "rkdeveloptool"
)

$AdbBin = Find-Tool @(
    "adb.exe",
    "adb",
    (Join-Path $ScriptDir "platform-tools\adb.exe"),
    "$env:LOCALAPPDATA\Android\Sdk\platform-tools\adb.exe",
    "C:\platform-tools\adb.exe"
)

Write-Host "========================================================" -ForegroundColor Cyan
Write-Host "  Phicomm R1 AirPlay 2 + USB DAC (UAC1) Flash Tool (Win)" -ForegroundColor Cyan
Write-Host "========================================================" -ForegroundColor Cyan

if (-not (Test-Path $FirmwareHook)) {
    Write-Error "Missing firmware hook image: $FirmwareHook"
    exit 1
}

if (-not $RkTool) {
    Write-Host "[-] rkdeveloptool.exe not found in project root or PATH." -ForegroundColor Red
    Write-Host "    Please download rkdeveloptool.exe (and install Rockchip DriverAssitant)" -ForegroundColor Yellow
    Write-Host "    into $ScriptDir, or flash $FirmwareHook at LBA $TargetLba using RKDevTool GUI." -ForegroundColor Yellow
    exit 1
}

function Invoke-Adb {
    param([string[]]$ArgsList)
    $allArgs = @()
    if ($Serial) { $allArgs += @("-s", $Serial) }
    $allArgs += $ArgsList
    & $AdbBin @allArgs
}

# Step 1: Check Rockchip Loader / Maskrom mode
Write-Host "[*] Checking device connection..."
$devList = (& $RkTool ld 2>&1 | Out-String)

if ($devList -match "Loader|Maskrom") {
    Write-Host "[+] Device already in flash mode: $($devList.Trim())" -ForegroundColor Green
} else {
    Write-Host "[*] Not in flash mode yet. Attempting ADB reboot into Loader mode..."
    if (-not $AdbBin) {
        Write-Error "adb.exe not found. Install Android platform-tools or manually short test-points to enter Loader/Maskrom mode."
        exit 1
    }

    $state = (Invoke-Adb @("get-state") 2>$null | Out-String).Trim()
    if ($state -ne "device") {
        Write-Error "No unique online ADB device found. If multiple devices are connected, pass -Serial <serial>."
        exit 1
    }

    if (Test-Path $RebootLoader) {
        Invoke-Adb @("push", $RebootLoader, "/data/local/tmp/reboot_loader") | Out-Null
        Invoke-Adb @("shell", "su -c 'chmod 755 /data/local/tmp/reboot_loader && /data/local/tmp/reboot_loader'") | Out-Null
    }
    Invoke-Adb @("reboot", "bootloader") 2>$null | Out-Null

    Write-Host "[*] Waiting for device to enter Rockchip Loader mode (up to 15s)..."
    $entered = $false
    for ($i = 0; $i -lt 15; $i++) {
        Start-Sleep -Seconds 1
        $devList = (& $RkTool ld 2>&1 | Out-String)
        if ($devList -match "Loader|Maskrom") {
            $entered = $true
            break
        }
    }
    if (-not $entered) {
        Write-Error "Failed to enter Loader mode automatically. Check USB driver (Rockchip DriverAssitant) or replug USB."
        exit 1
    }
    Write-Host "[+] Entered flash mode: $($devList.Trim())" -ForegroundColor Green
}

# Step 2: Write firmware hook to eMMC
Write-Host "[*] Writing $FirmwareHook to eMMC LBA $TargetLba..."
& $RkTool wl $TargetLba $FirmwareHook
if ($LASTEXITCODE -ne 0) {
    Write-Error "rkdeveloptool wl failed with exit code $LASTEXITCODE"
    exit 1
}
Write-Host "[+] Write complete!" -ForegroundColor Green

# Step 3: Read back and verify byte-for-byte
Write-Host "[*] Reading back sectors for byte-for-byte verification..."
$VerifyTmp = Join-Path $ScriptDir ".verify_flash.bin"
if (Test-Path $VerifyTmp) { Remove-Item $VerifyTmp -Force }
& $RkTool rl $TargetLba $Sectors $VerifyTmp
if ($LASTEXITCODE -ne 0 -or -not (Test-Path $VerifyTmp)) {
    Write-Error "Readback failed! Do NOT reboot yet; re-run flashing."
    exit 1
}

$hashSrc = (Get-FileHash $FirmwareHook -Algorithm SHA256).Hash
$hashDst = (Get-FileHash $VerifyTmp -Algorithm SHA256).Hash
Remove-Item $VerifyTmp -Force

if ($hashSrc -ne $hashDst) {
    Write-Error "Verification mismatch! (expected $hashSrc, got $hashDst). Re-run flashing without rebooting."
    exit 1
}
Write-Host "[+] Verification 100% matched (SHA256: $hashSrc)!" -ForegroundColor Green

# Step 4: Reboot device
Write-Host "[*] Rebooting speaker..."
& $RkTool rd
Write-Host "[+] Speaker is rebooting. Waiting ~15-20s for Android ADB..."

# Step 5: Disable Phicomm cloud apps and deploy AirPlay 2 binaries
if (-not $AdbBin) {
    Write-Host "[!] adb.exe not found; skipping cloud service disable and AirPlay 2 file sync." -ForegroundColor Yellow
    exit 0
}

Invoke-Adb @("wait-for-device") 2>$null | Out-Null
Start-Sleep -Seconds 3

Write-Host "[*] Disabling obsolete Phicomm AI & stock player services..."
Invoke-Adb @("shell", "su -c 'CLASSPATH=/system/framework/pm.jar app_process /system/bin com.android.commands.pm.Pm disable com.phicomm.speaker.device'") | Out-Null
Invoke-Adb @("shell", "su -c 'CLASSPATH=/system/framework/pm.jar app_process /system/bin com.android.commands.pm.Pm disable com.phicomm.speaker.player'") | Out-Null

Write-Host "[*] Deploying AirPlay 2 binaries & config to /data/local/airplay2/..."
Invoke-Adb @("shell", "su -c 'mkdir -p /data/local/airplay2 && chmod 755 /data/local/airplay2'") | Out-Null

$nqptp = Join-Path $ScriptDir "bin\nqptp"
$shairport = Join-Path $ScriptDir "bin\shairport-sync"
$conf = Join-Path $ScriptDir "bin\shairport-sync.conf"

if (Test-Path $nqptp) {
    Invoke-Adb @("push", $nqptp, "/data/local/tmp/nqptp") | Out-Null
    Invoke-Adb @("shell", "su -c 'mv /data/local/tmp/nqptp /data/local/airplay2/nqptp && chmod 755 /data/local/airplay2/nqptp'") | Out-Null
}
if (Test-Path $shairport) {
    Invoke-Adb @("push", $shairport, "/data/local/tmp/shairport-sync") | Out-Null
    Invoke-Adb @("shell", "su -c 'mv /data/local/tmp/shairport-sync /data/local/airplay2/shairport-sync && chmod 755 /data/local/airplay2/shairport-sync'") | Out-Null
}
if (Test-Path $conf) {
    Invoke-Adb @("push", $conf, "/data/local/tmp/shairport-sync.conf") | Out-Null
    Invoke-Adb @("shell", "su -c 'mv /data/local/tmp/shairport-sync.conf /data/local/airplay2/shairport-sync.conf && chmod 644 /data/local/airplay2/shairport-sync.conf'") | Out-Null
}

Write-Host ""
Write-Host "========================================================" -ForegroundColor Green
Write-Host "  Deployment Complete! / 部署完成！" -ForegroundColor Green
Write-Host "========================================================" -ForegroundColor Green
Write-Host "1. USB DAC  : Plug & Play 'Phicomm R1 USB Audio Stream' (48kHz / 16-bit)"
Write-Host "2. AirPlay 2: Select 'R1-Music' on iOS/iPadOS/macOS over Wi-Fi"
Write-Host "3. Controls : Top touch ring (volume), center key (mute), Harman DSP preserved"
Write-Host "========================================================" -ForegroundColor Green
