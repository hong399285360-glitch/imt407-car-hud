# build.ps1 - STM32F407 HUD 项目编译脚本
# 用法: .\Scripts\build.ps1

$ErrorActionPreference = "Stop"

$PROJECT_DIR = Split-Path -Parent $PSScriptRoot
$BUILD_DIR = Join-Path $PROJECT_DIR "build"

if (Test-Path $BUILD_DIR) {
    Remove-Item -Path $BUILD_DIR -Recurse -Force
}
New-Item -Path $BUILD_DIR -ItemType Directory -Force | Out-Null

# 定位工具链路径 (从 STM32Cube bundles 中查找)
$bundlesRoot = "$env:LOCALAPPDATA\stm32cube\bundles"

# CMake
$cmakeDir = Get-ChildItem -Path (Join-Path $bundlesRoot "cmake") -Directory -ErrorAction SilentlyContinue | Select-Object -First 1
if ($cmakeDir) {
    $cmakeBin = Join-Path $cmakeDir.FullName "bin"
    if (Test-Path $cmakeBin) { $env:PATH = "$cmakeBin;$env:PATH" }
}

# Ninja
$ninjaDir = Get-ChildItem -Path (Join-Path $bundlesRoot "ninja") -Directory -ErrorAction SilentlyContinue | Select-Object -First 1
if ($ninjaDir) {
    $ninjaBin = Join-Path $ninjaDir.FullName "bin"
    if (Test-Path $ninjaBin) { $env:PATH = "$ninjaBin;$env:PATH" }
}

# GNU Arm Embedded Toolchain
$armGccDir = Get-ChildItem -Path (Join-Path $bundlesRoot "gnu-tools-for-stm32") -Directory -ErrorAction SilentlyContinue | Select-Object -First 1
if ($armGccDir) {
    $armGccBin = Join-Path $armGccDir.FullName "bin"
    if (Test-Path $armGccBin) { $env:PATH = "$armGccBin;$env:PATH" }
}

# 验证工具链
Write-Host "=== 工具链检查 ===" -ForegroundColor Cyan
cmake --version | Select-Object -First 1
ninja --version
arm-none-eabi-gcc --version | Select-Object -First 1

# 生成内嵌网页源码 (web_page.c / web_page.h)
# 这两个文件在 .gitignore 中排除, 构建前必须从 WebUI/index.html 重新生成,
# 否则 cmake 的 file(GLOB) 扫不到 web_page.c, httpd.c 会链接失败.
Write-Host "`n=== 生成内嵌网页源码 ===" -ForegroundColor Cyan
python (Join-Path $PROJECT_DIR "Scripts/gen_web_page.py")
if ($LASTEXITCODE -ne 0) { Write-Host "生成 web_page.c 失败" -ForegroundColor Red; exit 1 }

# 构建
Write-Host "`n=== CMake Configure ===" -ForegroundColor Cyan
cmake -B $BUILD_DIR -S $PROJECT_DIR -G Ninja -DCMAKE_BUILD_TYPE=Debug
if ($LASTEXITCODE -ne 0) { Write-Host "CMake配置失败" -ForegroundColor Red; exit 1 }

Write-Host "`n=== Build ===" -ForegroundColor Cyan
cmake --build $BUILD_DIR -j8
if ($LASTEXITCODE -ne 0) { Write-Host "编译失败" -ForegroundColor Red; exit 1 }

Write-Host "`n=== Build Complete ===" -ForegroundColor Green

$elf = Join-Path $BUILD_DIR "imt407-car-hud.elf"
if (Test-Path $elf) {
    Write-Host "ELF: $elf" -ForegroundColor Green
    $size = (Get-Item $elf).Length
    Write-Host "Size: $size bytes" -ForegroundColor Green
}

Write-Host "Done." -ForegroundColor Green
