# 引脚分配参考

> **本文件是权威引脚表**，内容与代码严格一致，来源：
> - `Core/Src/stm32f4xx_hal_msp.c`（GPIO AF / DMA / NVIC 配置）
> - `Core/Src/main.c`（`SystemClock_Config` / `MX_*_Init`）
> - `Docs/board_reference.md`（开发板资料核对）
>
> 最后校准：2026-09-09（自查阶段 13.14 / P2-8）
>
> **两个最容易踩的坑（本板为启明欣欣 F407 轻奢版 V3.1，不是通用 F407 开发板）：**
> 1. **CAN1 在 PB8(RX)/PB9(TX)**，不是 PA11/PA12 —— 本板 PA11/PA12 被 USB OTG 占用。
> 2. **ESP8266 接在 UART4 (PA0/PA1)**，不是 USART1。
>
> GPS (NEO-6M) 与 MPU6050 已从本方案中彻底移除，相关引脚/外设不再占用。

## 一、外设引脚分配

| 外设 | 总线 | 引脚 | AF | 连接模块 | 参数 |
|------|------|------|----|----------|------|
| UART4 TX | APB1 (42MHz) | PA0 | AF8 | ESP8266 RX | 115200 8N1 |
| UART4 RX | APB1 (42MHz) | PA1 | AF8 | ESP8266 TX | DMA1 Stream2 Ch4, 循环模式 |
| CAN1 RX | APB1 (42MHz) | PB8 | AF9 | TJA1050 RXD | 500kbps, GPIO 上拉, `CAN1_RX0_IRQn` |
| CAN1 TX | APB1 (42MHz) | PB9 | AF9 | TJA1050 TXD | 500kbps |
| SDIO D0 | AHB1 (168MHz) | PC8 | AF12 | MicroSD D0 | 4-bit 总线 |
| SDIO D1 | AHB1 (168MHz) | PC9 | AF12 | MicroSD D1 | |
| SDIO D2 | AHB1 (168MHz) | PC10 | AF12 | MicroSD D2 | |
| SDIO D3 | AHB1 (168MHz) | PC11 | AF12 | MicroSD D3 | |
| SDIO CK | AHB1 (168MHz) | PC12 | AF12 | MicroSD CLK | `ClockDiv=0` → 24MHz |
| SDIO CMD | AHB1 (168MHz) | PD2 | AF12 | MicroSD CMD | |
| GPIO 输出 | — | PA8 | — | ESP8266 RST | 推挽输出, 上电置高 |
| SWDIO | SYS | PA13 | AF0 | CMSIS-DAP SWDIO | 调试/烧录 |
| SWCLK | SYS | PA14 | AF0 | CMSIS-DAP SWCLK | 调试/烧录 |
| HSE | — | OSC_IN / OSC_OUT | — | 8MHz 无源晶振 | PLL 输入 |

**接线方向注意**：CAN 收发器是交叉接线 —— MCU 的 CAN1_TX(PB9) 接 TJA1050 的 TXD，MCU 的 CAN1_RX(PB8) 接 TJA1050 的 RXD。

## 二、时钟树

```
HSE 8MHz → PLL(M=8, N=336, P=2, Q=7) → SYSCLK 168MHz
  ├─ HCLK  = 168MHz (AHB1)  → SDIO / DMA
  ├─ APB1  = 42MHz          → CAN1 / UART4   (APB1CLKDivider = HCLK/4)
  └─ APB2  = 84MHz          → 本方案未使用    (APB2CLKDivider = HCLK/2)
```

## 三、CAN 波特率计算

```
APB1 = 42MHz
Prescaler = 6            → CAN 时钟 = 42MHz / 6 = 7MHz
TQ = 1 / 7MHz = 142.8ns
SJW = 1TQ, BS1 = 11TQ, BS2 = 2TQ  → 每比特总 TQ = 1 + 11 + 2 = 14
Bit rate = 7MHz / 14 = 500kbps
采样点 = (1 + 11) / 14 = 85.7%   (ISO 15765-4 要求 75%~87.5%)
```

## 四、DMA 通道分配

| DMA | Stream | Channel | 外设 | 方向 | 模式 | 中断优先级 |
|-----|--------|---------|------|------|------|-----------|
| DMA1 | Stream2 | Ch4 | UART4 RX | Periph→Mem | 循环 (CIRCULAR) | 5 |

**SDIO 不使用 DMA** —— `main.c` 中 `hsd` 未挂 DMA 句柄，`diskio.c` 走 `HAL_SD_ReadBlocks/WriteBlocks` 阻塞式读写。

**CAN1 使用中断接收**（`CAN1_RX0_IRQn`），不占用 DMA。

**所有 DMA 缓冲区必须放主 SRAM (0x20000000~0x2001BFFF)，不能放 CCM (0x10000000)** —— CCM 无法被 DMA 访问。

## 五、中断优先级

| 中断 | 优先级 | 说明 |
|------|--------|------|
| `SDIO_IRQn` | 5 | SD 卡传输完成 (阶段14: 4→5, 4 违反 configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY) |
| `UART4_IRQn` | 5 | ESP8266 IDLE 帧结束检测 |
| `DMA1_Stream2_IRQn` | 5 | UART4 RX DMA |
| `CAN1_RX0_IRQn` | 5 | OBD-II 报文接收 |
| `PendSV_IRQn` | 15 (最低) | FreeRTOS 任务切换 |

## 六、已移除的外设（勿再接线）

| 外设 | 原计划引脚 | 状态 |
|------|-----------|------|
| GPS NEO-6M | USART3 PB10/PB11 | 已 pass。MSP 配置与调用已删除；`Core/Src/gps.c` 仍存在但无任何调用者 |
| MPU6050 IMU | I2C1 PB6/PB7 | 已 pass。MSP 配置与调用已删除；`Core/Src/imu.c` 仍存在但无任何调用者 |
| USART1 | PA9/PA10 | 未使用（历史上曾误用于 ESP8266） |

> `gps.c` / `imu.c` 因 `CMakeLists.txt` 用 `file(GLOB Core/Src/*.c)` 被一并编译，但链接期 `--gc-sections` 已把它们的 `.text/.data/.bss` 全部丢弃（map 文件实测均为 0），**不占用固件空间**。清理与否见 `TASKS.md`。