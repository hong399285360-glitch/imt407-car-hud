# IMT407 极狐T1 车载 HUD

一款基于 **STM32F407** 的开源车载抬头显示（HUD）方案：CAN 总线直读 OBD-II 数据 → ESP8266 开热点 → 平板浏览器渲染 HUD → 挡风玻璃反射显示。

针对极狐（ARCFOX）T1 新能源车设计，目标是把仪表盘上没有的电机功率、能量流、电池状态等信息，投影到驾驶视线正前方。

![HUD 界面预览](Docs/preview_hud.png)

## 为什么做这个

原车仪表信息有限，而新能源车最该被驾驶员看见的是**能量消耗状态**——急加速多费多少电、松油门回收多少、电池健康如何。市面上的 OBD 盒子依赖 ELM327 协议转换，慢且绕，本项目直接砍掉 ELM327，用 STM32F407 的 CAN 控制器**直读车辆总线**。

数据流只有一跳：

```
OBD-II 总线 ──CAN──> F407 解析 ──UART──> ESP8266 开热点 ──WiFi──> 平板浏览器渲染 ──反射──> 挡风玻璃
```

## 功能特性

- **CAN 直读 OBD-II**：ISO 15765-4 协议栈（单帧/多帧重组），Service 01 + 国标 Service 05，29 个 PID 自动扫描，已彻底移除 ELM327
- **HUD 界面（HTML5 Canvas）**：功率大字 + 能量流粒子动画为视觉中心，车速降为底部进度条，SOC/温度环形指示，Shy Tech 停车隐藏
- **驾驶评分**：急加速/急制动/能量回收/匀速工况实时计分
- **SD 卡日志**：CSV 每秒一行全量数据 + 文本诊断日志，离线分析驾驶行为
- **WiFi 无线下发**：ESP8266 AP 模式（SSID `F407-HUD`），HTTP `/data` 接口轮询刷新

## 硬件清单

| 部件 | 型号/说明 | 约价 |
|------|-----------|------|
| 主控板 | 启明欣欣 STM32F407 轻奢版 V3.1（LQFP144 / 1MB Flash / 192KB SRAM / 168MHz） | — |
| CAN 收发器 | TJA1050（5V 供电，连接 OBD 的 CAN-H/CAN-L） | ~¥5 |
| WiFi 模组 | ESP8266 ESP-01S（AT 固件，AP 模式） | ~¥10 |
| 显示 | 七彩虹 E708 Q1 平板 + 反射膜贴屏幕 | 旧物利用 |
| 存储 | MicroSD 卡（FatFs 日志） | — |
| 烧录 | CMSIS-DAP 调试器 + SWD 杜邦线 | ~¥15 |

完整接线图与安全红线见 [Docs/wiring_guide.md](Docs/wiring_guide.md)。

## 快速开始

### 1. 前置依赖

本仓库**不含第三方库**（`Drivers/` 目录），编译前需用 STM32CubeMX 补齐：

| 依赖 | 来源 |
|------|------|
| STM32F4 HAL 驱动 | CubeMX 生成 `Drivers/STM32F4xx_HAL_Driver` |
| CMSIS Device | CubeMX 生成 `Drivers/CMSIS/Device/ST/STM32F4xx` |
| CMSIS Core 6.3.0 | Cube 软件 pack（环境变量 `CMSIS_CORE_DIR`） |
| FreeRTOS 2.1.0 | Cube 软件 pack（环境变量 `FREERTOS_PACK_DIR`） |
| FatFs | CubeMX 生成 `Drivers/FatFs/source` |

> 用 CubeMX 选 STM32F407ZGT6，开启 FreeRTOS + FATFS + SDIO + CAN + UART4 即可生成。pack 安装路径非默认时，设置 `CMSIS_CORE_DIR` 和 `FREERTOS_PACK_DIR` 两个环境变量即可，无需改 CMakeLists。

### 2. 编译 & 烧录

```powershell
# 编译（自动定位 cmake/ninja/arm-gcc 与软件 pack）
.\Scripts\build.ps1

# 烧录（CMSIS-DAP + SWD，不是 ST-Link）
.\Scripts\flash.ps1
```

`build.ps1` 会在编译前自动把 `WebUI/index.html` 生成固件内嵌 C 源码（`Core/Src/web_page.c`，该文件不入库）；若手动调用 CMake，则需先运行 `python Scripts/gen_web_page.py`。

### 3. 上板自测

烧录线到手后，按 [Docs/board_self_test.md](Docs/board_self_test.md) 的 A~F 六个阶段逐步验证（每步有判据和排查表），从烧录、SD 卡日志、WiFi/HTTP、PID 扫描到 CAN 回环。

### 4. 实车接入

TJA1050 连接 OBD 座：`6=CAN-H`、`14=CAN-L`、`16=+12V`、`4/5=GND`。上电后固件自动扫描 29 个 PID 并写入 SD 卡，据此确认车辆实际响应的 PID 再微调 `Core/Src/can_obd.c`。

## 项目状态

| 里程碑 | 状态 |
|--------|------|
| 固件代码（协议栈/任务/日志/HTTP） | ✅ 完成 |
| HUD 界面（v7.2，含 Shy Tech / 能量流） | ✅ 完成 |
| 静态代码审计（两轮，0 错误 0 警告） | ✅ 完成 |
| 上板硬件验证（烧录/SD/WiFi/CAN 回环） | ⏳ 等烧录线到货 |
| 实车 OBD 联调 | ⏳ 等提车 |

最新固件：**54 编译单元，text=73,536 / data=480 / bss=41,376，0 错误 0 警告**（gcc 14.3.1, `-Os`）。

## 文档索引

| 文档 | 内容 |
|------|------|
| [Docs/pinmap.md](Docs/pinmap.md) | 权威引脚分配表 |
| [Docs/wiring_guide.md](Docs/wiring_guide.md) | 硬件接线 + 安全红线 |
| [Docs/board_self_test.md](Docs/board_self_test.md) | 上板自测清单（A~F 阶段） |
| [Docs/build_flash.md](Docs/build_flash.md) | 编译烧录全流程与排障 |
| [Docs/board_reference.md](Docs/board_reference.md) | 开发板硬件资料 |
| [OBD_PID_TABLE.md](OBD_PID_TABLE.md) | OBD-II PID 与极狐T1 调查 |
| [TASKS.md](TASKS.md) | 任务清单与路线图 |
| [PROGRESS.md](PROGRESS.md) | 开发变更日志 |
| [project.html](project.html) | 完整方案展示页（硬件架构/物料清单） |

## 安全声明

- **只读 CAN**：仅接收总线报文，不注入、不刷写 ECU
- **不碰高压**：远离 400V 橙色线束，所有调试在停车状态进行
- 改装车辆电气系统有风险，请自行评估并遵守当地法规

## License

[MIT](LICENSE) © 2026 hong399285360-glitch
