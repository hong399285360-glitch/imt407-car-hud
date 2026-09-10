# flash.ps1 - STM32F407 烧录脚本
# 用法: .\Scripts\flash.ps1
# 前提: CMSIS-DAP 仿真器已连接到板子 SWD 口 (不是 ST-Link)

$ErrorActionPreference = "Stop"

$PROJECT_DIR = Split-Path -Parent $PSScriptRoot
$BUILD_DIR = Join-Path $PROJECT_DIR "build"
$ELF_FILE = Join-Path $BUILD_DIR "imt407-car-hud.elf"
$BIN_FILE = Join-Path $BUILD_DIR "imt407-car-hud.bin"
$OPENOCD_CFG = Join-Path $PSScriptRoot "openocd.cfg"

# 定位 OpenOCD
$OPENOCD = Get-Command openocd -ErrorAction SilentlyContinue
if (-not $OPENOCD) {
    $extPath = Join-Path $env:USERPROFILE ".vscode\extensions"
    $extDirs = Get-ChildItem -Path $extPath -Filter "stmicroelectronics.stm32cube-ide-*" -Directory -ErrorAction SilentlyContinue
    foreach ($d in $extDirs) {
        $exe = Join-Path $d.FullName "tools\bin\openocd.exe"
        if (Test-Path $exe) { $OPENOCD_EXE = $exe; break }
    }
} else {
    $OPENOCD_EXE = $OPENOCD.Source
}

if (-not (Test-Path $BIN_FILE)) {
    Write-Host "错误: 找不到 $BIN_FILE，请先编译" -ForegroundColor Red
    exit 1
}

Write-Host "=== Flashing ===" -ForegroundColor Cyan
Write-Host "目标: STM32F407ZGT6" -ForegroundColor Gray
Write-Host "固件: $BIN_FILE" -ForegroundColor Gray
Write-Host "OpenOCD: $OPENOCD_EXE" -ForegroundColor Gray
Write-Host ""

if ($OPENOCD_EXE -and (Test-Path $OPENOCD_EXE)) {
    & $OPENOCD_EXE -f $OPENOCD_CFG -c "program $BIN_FILE 0x08000000 verify reset exit"
} else {
    Write-Host "错误: 找不到 OpenOCD，请确保 STM32CubeIDE 扩展已安装工具链" -ForegroundColor Red
    Write-Host "或手动安装 OpenOCD 并添加到 PATH" -ForegroundColor Yellow
    exit 1
}

if ($LASTEXITCODE -eq 0) {
    Write-Host "=== Flash Complete ===" -ForegroundColor Green
} else {
    Write-Host "=== Flash Failed ===" -ForegroundColor Red
}
