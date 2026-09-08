<#
.SYNOPSIS
    LifeLine Hardware Ecosystem - Unified Firmware Build & Upload Script
.DESCRIPTION
    Compiles, uploads, or cleans firmware targets for all 4 LifeLine units:
    - lifeline_rx_pro (Base Station Receiver)
    - lifeline_tx_pro (Field Emergency Transmitter)
    - lifeline_tx_spu (Sensor Processing Unit)
    - lifeline_tx_ccu (Communication Controller Unit)
.EXAMPLE
    .\build.ps1              # Compiles all 4 firmware projects
    .\build.ps1 rx           # Compiles LifeLine RX Pro only
    .\build.ps1 tx           # Compiles LifeLine TX Pro only
    .\build.ps1 spu          # Compiles LifeLine SPU only
    .\build.ps1 ccu          # Compiles LifeLine CCU only
    .\build.ps1 rx -Upload   # Compiles and flashes RX Pro over USB
    .\build.ps1 clean        # Removes all .pio build caches
#>

[CmdletBinding()]
param(
    [ValidateSet("all", "rx", "tx", "spu", "ccu", "clean")]
    [string]$Target = "all",
    [switch]$Upload,
    [switch]$Monitor,
    [switch]$OTA
)

$ErrorActionPreference = "Stop"

# Locate PlatformIO CLI executable
$pioExe = "pio"
if (-not (Get-Command "pio" -ErrorAction SilentlyContinue)) {
    $customPio = "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"
    if (Test-Path $customPio) {
        $pioExe = $customPio
    } else {
        Write-Error "PlatformIO CLI (pio) was not found in PATH or ~/.platformio/penv/Scripts/."
        exit 1
    }
}

$Projects = [ordered]@{
    "rx"  = @{ Name = "LifeLine RX Pro Base Station";       Path = "lifeline_rx_pro" }
    "tx"  = @{ Name = "LifeLine TX Pro Field Transmitter";  Path = "lifeline_tx_pro" }
    "spu" = @{ Name = "LifeLine SPU Sensor Node";           Path = "lifeline_tx_spu" }
    "ccu" = @{ Name = "LifeLine CCU Communication Unit";    Path = "lifeline_tx_ccu" }
}

if ($Target -eq "clean") {
    Write-Host "`n[CLEAN] Cleaning .pio build artifacts across all projects..." -ForegroundColor Yellow
    foreach ($key in $Projects.Keys) {
        $projDir = Join-Path $PSScriptRoot $Projects[$key].Path
        $pioDir = Join-Path $projDir ".pio"
        if (Test-Path $pioDir) {
            Remove-Item -Recurse -Force $pioDir
            Write-Host "  [OK] Cleaned $pioDir" -ForegroundColor Green
        }
    }
    Write-Host "`nAll build artifacts cleaned successfully!`n" -ForegroundColor Green
    exit 0
}

$buildTargets = if ($Target -eq "all") { @("rx", "tx", "spu", "ccu") } else { @($Target) }

Write-Host "`n==============================================================" -ForegroundColor Cyan
Write-Host "          LIFELINE FIRMWARE BUILD SYSTEM                      " -ForegroundColor Cyan
Write-Host "==============================================================`n" -ForegroundColor Cyan

$results = [ordered]@{}

foreach ($t in $buildTargets) {
    $info = $Projects[$t]
    $projDir = Join-Path $PSScriptRoot $info.Path
    Write-Host "[BUILD] Compiling $($info.Name) in ./$($info.Path)..." -ForegroundColor Yellow

    $cmdArgs = @("run", "-d", $projDir)
    if ($OTA) {
        $cmdArgs += @("-e", "esp32dev_ota")
    }
    if ($Upload) {
        $cmdArgs += @("-t", "upload")
    }

    $startTime = Get-Date
    & $pioExe @cmdArgs
    $exitCode = $LASTEXITCODE
    $duration = [math]::Round(((Get-Date) - $startTime).TotalSeconds, 1)

    if ($exitCode -eq 0) {
        Write-Host "  [SUCCESS] $($info.Name) built in ${duration}s`n" -ForegroundColor Green
        $results[$info.Name] = "SUCCESS (${duration}s)"
    } else {
        Write-Host "  [FAILED] $($info.Name) (Exit Code: $exitCode)`n" -ForegroundColor Red
        $results[$info.Name] = "FAILED"
        if ($Target -ne "all") { exit $exitCode }
    }
}

Write-Host "`n==============================================================" -ForegroundColor Cyan
Write-Host "                     BUILD SUMMARY                            " -ForegroundColor Cyan
Write-Host "==============================================================" -ForegroundColor Cyan
foreach ($k in $results.Keys) {
    $statusColor = if ($results[$k].StartsWith("SUCCESS")) { "Green" } else { "Red" }
    Write-Host "  * $k : $($results[$k])" -ForegroundColor $statusColor
}
Write-Host "==============================================================`n" -ForegroundColor Cyan

if ($Monitor -and $buildTargets.Count -eq 1) {
    Write-Host "[MONITOR] Launching Serial Monitor (115200 baud)..." -ForegroundColor Cyan
    & $pioExe device monitor -b 115200
}
