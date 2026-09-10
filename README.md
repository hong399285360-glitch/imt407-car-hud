# IMT407 极狐T1 车载 HUD

基于 STM32F407 的新能源车载抬头显示：CAN 直读 OBD-II → ESP8266 开热点 → 平板浏览器渲染 → 挡风玻璃反射显示。

> 私有项目，当前处于「烧录线到货前的代码冻结」阶段。完整进度见 [TASKS.md](TASKS.md) / [PROGRESS.md](PROGRESS.md)。

## 硬件架构

```
        ┌──────────── F407 IMT407 ─────────────┐
OBD-II ─→ CAN1(PB8/PB9) + TJA1050            SDIO(PC8-12/PD2) ←── MicroSD 日志
        │                                      │
        └── UART4(PA0/PA1) ── ESP8266(AP模式) ──→ 平板浏览器 ──→ 挡风玻璃反射
```

- **主控**：启明欣欣 STM32F407 轻奢版 V3.1（LQFP144 / 1MB Flash / 192KB SRAM / 168MHz）
- **数据源**：CAN 直读 OBD-II（ISO 15765-4，Service 01 + 国标 Service 05），已彻底去掉 ELM327
- **无线**：ESP8266（ESP-01S）AP 模式，SSID `F407-HUD`，HTTP 服务 + WebSocket 式轮询 `/data`
- **日志**：MicroSD（FatFs），`hud_log.csv`（1s/行）+ `obd_scan.log`（诊断/告警文本）
- **已移除**：GPS(NEO-6M)、MPU6050 IMU（相关 `gps.c`/`imu.c` 保留为无调用死代码，链接期被 `--gc-sections` 丢弃）

## 引脚速查

| 外设 | 引脚 | 说明 |
|------|------|------|
| CAN1 | **PB8(RX) / PB9(TX)** AF9 | 不是 PA11/PA12（本板被 USB OTG 占用） |
| UART4 | **PA0(TX) / PA1(RX)** AF8 | ESP8266 |
| SDIO | PC8-12 + PD2 AF12 | MicroSD 4-bit |
| ESP RST | PA8（推挽输出） | 硬复位 |
| SWD | PA13 / PA14 | CMSIS-DAP 烧录 |

完整引脚/时钟/中断表见 [Docs/pinmap.md](Docs/pinmap.md)。

## 目录结构

```
Core/       固件源码（启动文件、链接脚本、HAL 回调、业务逻辑）
Docs/       板子资料、引脚、接线、烧录、上板自测清单
Scripts/    build.ps1 / flash.ps1 / gen_web_page.py / openocd.cfg
WebUI/      HUD 页面前端源 index.html（经 gen_web_page.py 生成 Core/Src/web_page.c）
Drivers/    ★ 不在此仓库，见下方「编译前置依赖」
```

## 编译前置依赖

本仓库**不含第三方库**（`Drivers/` 目录与 STM32Cube 软件 pack）。编译前需补齐：

| 依赖 | 来源 | CMake 定位方式 |
|------|------|----------------|
| STM32F4 HAL 驱动 | STM32CubeMX 生成 `Drivers/STM32F4xx_HAL_Driver` | `HAL_DIR`（`Drivers/` 下） |
| CMSIS Device (含 `system_stm32f4xx.c`) | STM32CubeMX 生成 `Drivers/CMSIS/Device/ST/STM32F4xx` | `DEV_DIR` |
| CMSIS Core 6.3.0 | STM32Cube 软件 pack | 环境变量 `CMSIS_CORE_DIR`，兜底 `%LOCALAPPDATA%\stm32cube\packs\...\CMSIS\6.3.0\CMSIS\Core\Include` |
| FreeRTOS 2.1.0 | STM32Cube 软件 pack | 环境变量 `FREERTOS_PACK_DIR`，兜底 `%LOCALAPPDATA%\stm32cube\packs\...\freertos\2.1.0` |
| FatFs | STM32CubeMX 生成 `Drivers/FatFs/source`（`ff.c/ffsystem.c/ffunicode.c` + `ffconf.h`） | `FATFS_DIR` |

> 用 STM32CubeMX 选 STM32F407ZGT6，开启 FreeRTOS + FATFS + SDIO + CAN + UART4 即可生成上述 `Drivers/`；CMSIS Core 与 FreeRTOS pack 通过 CubeMX「Manage software packs」安装。
> 若 pack 安装路径与默认不同，设置 `CMSIS_CORE_DIR` 和 `FREERTOS_PACK_DIR` 两个环境变量即可，无需改 CMakeLists。

## 编译 & 烧录

```powershell
# 编译（自动定位 cmake/ninja/arm-gcc 与软件 pack）
.\Scripts\build.ps1

# 烧录（CMSIS-DAP + SWD，不是 ST-Link；需 DSP 驱动 stmcdc.inf）
.\Scripts\flash.ps1
```

- 当前固件：**54 编译单元，text=73536 / data=480 / bss=41376，0 错误 0 警告**（阶段14 静态审计后）
- `Core/Src/web_page.c` 与 `Core/Inc/web_page.h` 是 `gen_web_page.py` 的生成物，**不入库**。首次 clone 编译前、以及每次改了 `WebUI/index.html` 后，都要先跑 `python Scripts/gen_web_page.py` 重新生成
- 烧录线接法、常见失败排查见 [Docs/build_flash.md](Docs/build_flash.md)

## 上板自测

烧录线到货后，照 [Docs/board_self_test.md](Docs/board_self_test.md) 逐步执行（每步有判据与排查表）：
A 编译烧录 → B SD 卡日志 → C WiFi/HTTP → D `/scan` 链路 → E CAN 回环 → F 实车 OBD。

## 文档索引

| 文档 | 内容 |
|------|------|
| [TASKS.md](TASKS.md) | 任务清单 + 当前状态速览（压缩后先读这里） |
| [PROGRESS.md](PROGRESS.md) | 完整变更日志与编译记录 |
| [OBD_PID_TABLE.md](OBD_PID_TABLE.md) | OBD-II Service 01/05 PID 与极狐T1 调查 |
| [Docs/pinmap.md](Docs/pinmap.md) | 权威引脚表 |
| [Docs/wiring_guide.md](Docs/wiring_guide.md) | 硬件接线 + 安全红线 |
| [Docs/board_reference.md](Docs/board_reference.md) | 开发板资料与路径 |
| [Docs/self_audit_2026-09-09.md](Docs/self_audit_2026-09-09.md) | 阶段13 全工程自查结论 |

## 安全红线

- 只读 CAN，不注入报文、不刷写 ECU
- 不碰 400V 高压系统（橙色线束）
- 调试在停车状态进行
