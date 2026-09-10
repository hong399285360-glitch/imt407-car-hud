# 编译和烧录指南

> 烧录器是**本板标配的 CMSIS-DAP**，不是 ST-Link。
> 驱动：`CMSIS DAP仿真器\DAP串口驱动\stmcdc.inf`（资料路径见 `Docs/board_reference.md`）。
> OpenOCD 配置：`Scripts/openocd.cfg`（`interface/cmsis-dap.cfg` + `reset_config none`）。

## 前提条件

1. STM32CubeIDE for VS Code 扩展已安装（提供 OpenOCD）
2. GNU tools for STM32 (arm-none-eabi-gcc) 已通过 bundle manager 下载
3. CMSIS-DAP 仿真器已通过 SWD 连接到板子（SWDIO=PA13 / SWCLK=PA14 / GND / 3V3）
4. CMSIS-DAP 驱动已安装

## 编译

### 方式1: PowerShell 脚本
```powershell
cd d:\HONG\Doc\TRAE\imt407-car-hud
.\Scripts\build.ps1
```

脚本会先删除 `build/` 目录做全量重编，并自动从 `%LOCALAPPDATA%\stm32cube\bundles` 定位 cmake / ninja / arm-gcc。

### 方式2: VS Code 中
1. 打开 VS Code
2. 打开项目文件夹
3. Ctrl+Shift+P → "CMake: Build"

### 预期输出（2026-09-09 阶段14 实测）

```
[54/54] Linking C executable imt407-car-hud.elf; Generating bin/hex and printing size
   text    data     bss     dec     hex filename
  73536     480   41376  115392   1c2c0 D:/HONG/Doc/TRAE/imt407-car-hud/build/imt407-car-hud.elf

=== Build Complete ===
ELF: D:\HONG\Doc\TRAE\imt407-car-hud\build\imt407-car-hud.elf
Size: 508656 bytes
Done.
```

- 54 个编译单元，0 错误 0 警告
- Flash 占用 73.5KB / 1024KB，RAM (bss) 41.4KB / 192KB
- 改过 `WebUI/index.html` 后**必须先跑** `python Scripts/gen_web_page.py`，否则烧进去的还是旧页面

## 烧录

### 方式1: PowerShell 脚本
```powershell
.\Scripts\flash.ps1
```

### 方式2: OpenOCD 直接命令
```powershell
openocd -f Scripts/openocd.cfg -c "program build/imt407-car-hud.bin 0x08000000 verify reset exit"
```

### 预期输出
```
target stm32f4x.cpu: hardware has 6 breakpoints, 4 watchpoints
** Programming Started **
** Programming Finished **
** Verify OK **
** Reset **
shutdown command invoked
```

> 板子只引出 SWDIO/SWCLK/GND/VCC，没有独立 SRST 线，`openocd.cfg` 用 `reset_config none`（SYSRESETREQ 软复位），烧录后仍能正常复位运行。

## 调试

### VS Code 调试
1. 安装 Cortex-Debug 扩展（已安装）
2. F5 启动调试
3. 断点、单步、寄存器/内存查看

### 串口调试
UART4 (PA0/PA1) 已被 ESP8266 占用，板上**没有空闲的 TTL 串口**可用于 printf 调试。
需要看日志时可选：
- SWD + Cortex-Debug 查看变量（推荐）
- 借用 CMSIS-DAP 的虚拟串口（若你的 DAP 带该功能）
- 临时把 `printf` 重定向到 ESP8266 那条链路，但会与 AT 命令冲突，仅限单独调试

## 常见问题

### 找不到 arm-none-eabi-gcc
- 在 VS Code 中 Ctrl+Shift+P → "STM32: Install bundles"
- 选择 "gnu-tools-for-stm32" 安装

### OpenOCD 连接失败
- 检查 **CMSIS-DAP** 驱动（`stmcdc.inf`），不是 ST-Link 驱动
- 确认 SWDIO/SWCLK/GND 三线已接
- 尝试降低 adapter speed: 把 `openocd.cfg` 里 `adapter speed 4000` 改为 `1000`
- 确认 `openocd.cfg` 用的是 `interface/cmsis-dap.cfg` 而非 `stlink.cfg`

### 编译错误: 找不到 stm32f4xx_hal.h
- 确保已通过 CubeMX 生成 HAL 代码
- 确认 `Drivers/` 目录存在

### OBD 读取失败
- 极狐T1 可能使用扩展 PID，需实车确认
- PID 表在 `Core/Src/can_obd.c` 中，可调整请求的 Service/PID
- 先跑 `GET /scan` 做 PID 扫描，看哪些 PID 有响应
