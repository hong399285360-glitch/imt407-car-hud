# 硬件接线指南

> 安全第一！通电前请用万用表确认 VCC/GND 不短路、电平匹配。
> 引脚以 [`pinmap.md`](pinmap.md) 为准，该表与 `stm32f4xx_hal_msp.c` / `main.c` 严格同步。

## 一、总览

```
              ┌──────────── F407 IMT407 ─────────────┐
  ESP8266 ──→ │ UART4 (PA0/PA1)      SDIO (PC8-12/PD2)│ ←── MicroSD
  TJA1050 ──→ │ CAN1  (PB8/PB9)      SWD  (PA13/PA14) │ ←── CMSIS-DAP
              └──────────────────────────────────────┘
```

**注意**：本方案已移除 GPS (NEO-6M) 与 MPU6050 IMU，USART3 / I2C1 不再接线。
本板（启明欣欣 F407 轻奢版 V3.1）的 **CAN1 在 PB8/PB9**，不是常见的 PA11/PA12。

## 二、ESP8266 ESP-01S → F407 UART4

| ESP-01S | F407 | 说明 |
|---------|------|------|
| VCC (3.3V) | 3.3V | 供电，峰值 300mA，建议外接 AMS1117-3.3 |
| GND | GND | 共地 |
| TX | PA1 (UART4 RX) | ESP 发 → F407 收 |
| RX | PA0 (UART4 TX) | F407 发 → ESP 收 |
| EN (CH_PD) | 3.3V | 使能，必须拉高 |
| RST | PA8 | 复位控制（代码上电置高） |
| GPIO0 | 3.3V | 运行模式拉高 |
| GPIO2 | 3.3V | 拉高 |

**注意**：ESP8266 峰值电流 300mA，F407 板载 LDO 可能不够。建议外接 AMS1117-3.3 独立供电。
**注意**：UART4 RX 走 `DMA1 Stream2 Ch4` 循环模式 + IDLE 中断，PA1 不要被其它外设复用。

## 三、TJA1050 CAN 收发器 → F407 CAN1

### 模块侧（4 针排针）

| TJA1050 | F407 | 说明 |
|---------|------|------|
| VCC | 5V | 供电 |
| GND | GND | 共地 |
| TXD | PB9 (CAN1_TX) | F407 发 → TJA1050 收 |
| RXD | PB8 (CAN1_RX) | TJA1050 发 → F407 收 |

**交叉接线**：TJA1050 的 TXD 接 F407 的 CAN1_TX(PB9)，RXD 接 CAN1_RX(PB8)。

### OBD 侧（2 针接线端子）

| TJA1050 | OBD 跳线 | 颜色 |
|---------|---------|------|
| CAN-H | 蓝线 | 蓝 |
| CAN-L | 黄线 | 黄 |

### OBD-II 接口定义 (SAE J1962)

| 引脚 | 信号 | 说明 |
|------|------|------|
| 6 | CAN-H | 高速 CAN 高 |
| 14 | CAN-L | 高速 CAN 低 |
| 16 | +12V | 电池正极（**不为 HUD 供电**） |
| 4,5 | GND | 信号地 / 底盘地 |

**接线**：从 OBD-II 的第 6 脚和第 14 脚引出 CAN-H 和 CAN-L 到 TJA1050 的接线端子。

## 四、CMSIS-DAP 仿真器 → F407 SWD

> **本板标配 CMSIS-DAP，不是 ST-Link。** OpenOCD 必须用 `interface/cmsis-dap.cfg`。

| DAP | F407 | 说明 |
|-----|------|------|
| SWDIO | PA13 | SWD 数据 |
| SWCLK | PA14 | SWD 时钟 |
| 3V3 | 3V3 | 参考电平 |
| GND | GND | 共地 |

驱动：`CMSIS DAP仿真器\DAP串口驱动\stmcdc.inf`（见 `Docs/board_reference.md` 第 6 节）。
烧录命令见 `Docs/build_flash.md`。

## 五、电源方案

1. **车充 USB 5V/2A** → 给平板充电 + F407 供电
2. **AMS1117-3.3** → 独立给 ESP8266 供电（5V→3.3V）
3. **TVS 二极管 SMBJ24A** → 跨在 5V 输入端防浪涌
4. 所有模块 **共地**

## 六、安全红线

- **只读 CAN**，不注入 CAN 报文，不刷写 ECU
- **不碰高压系统**（400V 平台，橙色线束）
- **3.3V 排针别接 5V**（F407 I/O 是 3.3V）
- **TJA1050 用 5V 供电**，但 CAN 逻辑电平兼容 F407 的 3.3V
- **通电前用万用表检查 VCC/GND 不短路**
- **调试在停车状态下进行**

## 七、CAN 总线配置参数

| 参数 | 值 | 说明 |
|------|------|------|
| 波特率 | 500 kbps | ISO 15765-4 标准 |
| 采样点 | 85.7% | Prescaler=6, TQ=14, BS1=11, BS2=2 |
| 帧格式 | 标准帧 (11-bit ID) | 非扩展帧 |
| 请求 ID | 0x7DF | 功能寻址，广播给所有 ECU |
| 响应 ID | 0x7E8 | ECU 响应 |
| 超时 | 100ms | 单次 PID 请求超时 |
