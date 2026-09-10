# 启明欣欣 STM32F407 轻奢版 V3.1 — 板子完整认识文档

> **资料根目录**: `E:\HONG\Down\启明欣欣STM32F407(轻奢版V3.1)学习资料`
> **板子图片**: `407轻奢版V3.1外设与接口介绍.jpg`
> **原理图**: `1 启明欣欣407开发板(轻奢版)V3.1原理图\启明欣欣STM32F407开发板(轻奢版V3.1)原理图.pdf`
> **创建日期**: 2026-09-08
> **最后更新**: 2026-09-08 (全部引脚从HAL库例程源码逐一确认)

## 1. 核心参数

| 参数 | 值 |
|------|-----|
| MCU | STM32F407ZGT6 (LQFP144) |
| Flash | 1 MB |
| SRAM | 192 KB (128KB主 + 64KB CCM) |
| 最高主频 | 168 MHz |
| HSE | 8 MHz 外部晶振 |
| 浮点 | Cortex-M4F (单精度FP) |
| 供电 | DC 6-24V宽压 / USB 5V |
| RTC电池 | CR1220纽扣电池座 |

## 2. 时钟配置 (HAL库, 从例程确认)

```
HSE = 8 MHz
PLL: M=8, N=336, P=2, Q=7
SYSCLK = 8/8*336/2 = 168 MHz
AHB = 168 MHz (HCLK)
APB1 = 42 MHz  (PCLK1: CAN1/2, UART4, USART2/3, I2C1, TIM2-7,12-14)
APB2 = 84 MHz  (PCLK2: USART1/6, SDIO, SPI1, TIM1/8-11, ADC1-3)
```

例程写法: `Stm32_Clock_Init(336,8,2,7);` 即 N=336, M=8, P=2, Q=7

## 3. 完整引脚映射 (HAL库例程源码逐一确认)

### 3.1 LED指示灯

| 功能 | 引脚 | 模式 | 电平 | 来源 |
|------|------|------|------|------|
| LED0 | PG13 | 推挽输出 | 高灭低亮 | HAL例程1 led.c |
| LED1 | PG14 | 推挽输出 | 同上 | 同上 |
| LED2 | PG15 | 推挽输出 | 同上 | 同上 |

### 3.2 蜂鸣器

| 功能 | 引脚 | 模式 | 电平 | 来源 |
|------|------|------|------|------|
| BEEP | **PG7** | 推挽输出 | 高响低停 | HAL例程2 beep.c |

> 注意: 之前文档写的PF8是错误的, HAL版蜂鸣器在PG7。

### 3.3 用户按键

| 功能 | 引脚 | 模式 | 来源 |
|------|------|------|------|
| KEY0 | **PF6** | 上拉输入 | HAL例程3 key.c |
| KEY1 | **PF7** | 上拉输入 | 同上 |
| KEY2 | **PF8** | 上拉输入 | 同上 |
| KEY3 | **PF9** | 上拉输入 | 同上 |
| 复位按键 | NRST | - | 硬件复位 |

> 注意: 之前文档写的PE2/3/4是标准库版本的, HAL版按键在PF6-9。
> PA0(KEY_UP/WK_UP)在图片中未单独标注, 且与UART4_TX冲突, 不使用。

### 3.4 串口 (8路全列表)

| 串口 | TX | RX | AF | 电平 | 接口形式 | 来源 |
|------|----|----|-----|------|---------|------|
| **USART1** | PA9 | PA10 | AF7 | RS232 | DB9 + 排针 | HAL例程6 usart1.c |
| **USART2** | PA2 | PA3 | AF7 | RS485 | 绿色端子 | HAL例程25 rs485.c |
| **USART3** | PC10 | PC11 | AF7 | TTL | 排针 | HAL例程13 usart3.c |
| **UART4** | **PA0** | **PA1** | **AF8** | TTL | **ESP8266接口** | ESP8266例程 usart4_wifi.c |
| UART5 | - | - | - | TTL | 排针 | 图片标注 |
| **USART6** | PC6 | PC7 | AF8 | TTL/RS232 | 排针 | HAL例程13 usart6.c |
| 串口2-232 | - | - | - | RS232 | DB9 | 图片标注(独立DB9) |

> **ESP8266**: 板载WiFi模块接口接在 **UART4 (PA0/PA1)** 上, 不是USART1!
> ESP8266例程使用标准库(SPL), 但引脚配置相同: PA0/PA1 AF8
> AT命令配置: CWMODE=3(AP模式), CWSAP="qiming_wifi","0123456789",11,4
> TCP服务器: CIPMUX=1, CIPSERVER=1,5000 (端口5000)

### 3.5 CAN总线

| 外设 | TX | RX | AF | 接口形式 | 来源 |
|------|----|----|-----|---------|------|
| **CAN1** | **PB9** | **PB8** | **AF9** | 绿色端子 | HAL例程16 can1.c |
| CAN2 | PB13 | PB12 | AF9 | 绿色端子 | HAL例程16 can2.c |

> 板载双路TJA1050 CAN收发器, 两路CAN都有独立端子
> CAN波特率: 500kbps (APB1=42M, SJW=1, BS1=7, BS2=6, BRP=6, 42M/((1+7+6)*6)=500K)

### 3.6 SD卡 (SDIO 4-bit)

| 功能 | 引脚 | AF | 来源 |
|------|------|-----|------|
| SDIO_D0 | PC8 | AF12 | 代码确认 |
| SDIO_D1 | PC9 | AF12 | 同上 |
| SDIO_D2 | PC10 | AF12 | 同上 |
| SDIO_D3 | PC11 | AF12 | 同上 |
| SDIO_CK | PC12 | AF12 | 同上 |
| SDIO_CMD | PD2 | AF12 | 同上 |

> SD卡槽在板子右侧上部, MicroSD规格, SDIO方式高速传输

### 3.7 SPI Flash (W25Q128, 16MB)

| 功能 | 引脚 | AF | 来源 |
|------|------|-----|------|
| SPI1_SCK | PB3 | AF5 | HAL例程15 |
| SPI1_MISO | PB4 | AF5 | 同上 |
| SPI1_MOSI | PB5 | AF5 | 同上 |
| Flash_CS | PB0 | 普通GPIO | 同上 |

> 贴片焊接在板子右侧, 用于扩展程序/数据存储

### 3.8 I2C EEPROM (AT24C02, 2Kb)

| 功能 | 引脚 | AF | 来源 |
|------|------|-----|------|
| I2C1_SCL | PB8 | AF4 | HAL例程14 |
| I2C1_SDA | PB9 | AF4 | 同上 |

> **冲突警告**: I2C1 和 CAN1 都用 PB8/PB9!
> I2C1: PB8/PB9 AF4 / CAN1: PB8/PB9 AF9
> 同一时刻只能选一个。本项目用CAN1, 不用I2C1, 无冲突。

### 3.9 DS18B20温度传感器

| 功能 | 引脚 | 模式 | 来源 |
|------|------|------|------|
| DQ | **PG11** | GPIO双向(单总线) | HAL例程22 DS18B20.h |

> 注意: 之前文档写的PA5是错误的, DS18B20在PG11。

### 3.10 红外接收 (HS0038)

| 功能 | 引脚 | 模式 | 来源 |
|------|------|------|------|
| IR_IN | PA8 | 输入(外部中断) | HAL例程23 |

> 支持NEC编码, 靠近复位按键

### 3.11 LCD显示 (FSMC)

| 功能 | 引脚 | AF |
|------|------|-----|
| FSMC_D0~D15 | PD0,1,4,5,7,8,9,10,11,14,15, PE7~15 | AF12 |
| FSMC_RS(A16) | PD11 | AF12 |
| FSMC_CS(NE1) | PD7 | AF12 |

> 支持2.8寸屏(ILI9341)和7寸屏(SSD1963/RA8875), 8080并口

### 3.12 USB

| 功能 | 引脚 | AF | 接口 |
|------|------|-----|------|
| USB OTG FS | PA11(DM)/PA12(DP) | AF10 | Mini USB |
| USB OTG HS | PB14(DM)/PB15(DP) | AF12 | micro USB |

> 注意: USB OTG FS的PA11/PA12与CAN1的默认引脚相同(但本板CAN1用PB8/PB9, 无冲突)

### 3.13 调试接口

| 接口 | 引脚 | 接口形式 |
|------|------|---------|
| SWDIO | PA13 | JTAG 20pin排针 |
| SWCLK | PA14 | 同上 |
| JTAG_TDI | PA15 | 同上 |
| JTAG_TDO | PB3 | 同上 |
| nRESET | NRST | 同上 |

> 兼容CMSIS DAP和JLink V8

### 3.14 电源系统

| 接口 | 位置 | 电压 | 说明 |
|------|------|------|------|
| DC输入 | 左下角 | 6-24V宽压 | 带极性保护, 经DC-DC降压 |
| USB供电 | 右侧 | 5V | Mini USB |
| 3.3V输出 | 排针 | 3.3V | 供外设使用 |
| 5V输出 | 排针 | 5V | 供外设使用 |
| GND | 排针 | 0V | 公共地 |

### 3.15 RTC

| 功能 | 说明 |
|------|------|
| CR1220电池座 | 板子中央偏右, 维持RTC走时 |
| RTC外设 | STM32内部RTC, LSE 32.768kHz |

## 4. 引脚冲突矩阵

| 引脚 | 功能1 | 功能2 | 本项目使用 | 说明 |
|------|-------|-------|---------|------|
| PA0 | UART4_TX | KEY_UP(WK_UP) | **UART4_TX** | 不能同时按键 |
| PA8 | 红外HS0038 | ESP8266_RST | ESP8266_RST | 红外不用 |
| PB8 | CAN1_RX | I2C1_SCL | **CAN1_RX** | I2C不用 |
| PB9 | CAN1_TX | I2C1_SDA | **CAN1_TX** | I2C不用 |
| PC10 | USART3_TX | SDIO_D2 | **SDIO_D2** | 串口3不用 |
| PC11 | USART3_RX | SDIO_D3 | **SDIO_D3** | 串口3不用 |

## 5. 本项目引脚分配总表

| 外设 | 引脚 | AF | 用途 |
|------|------|-----|------|
| **UART4** | PA0(TX)/PA1(RX) | AF8 | ESP8266 WiFi |
| **CAN1** | PB9(TX)/PB8(RX) | AF9 | TJA1050 → OBD-II |
| **SDIO** | PC8-12, PD2 | AF12 | MicroSD卡日志 |
| **GPIO** | PA8 | OUT | ESP8266 RST复位 |
| **GPIO** | PG13 | OUT | LED0(运行指示) |
| **SWD** | PA13/PA14 | - | CMSIS DAP烧录调试 |

## 6. 烧录方式

### 6.1 CMSIS DAP (板子标配仿真器)

板子配套CMSIS DAP仿真器, 通过SWD接口烧录。

**不是ST-Link! 必须使用CMSIS DAP配置。**

SWD接线:
| DAP | 开发板 |
|-----|--------|
| SWDIO | SWDIO (PA13) |
| SWCLK | SWCLK (PA14) |
| 3V3 | 3V3 |
| GND | GND |

驱动: `CMSIS DAP仿真器\DAP串口驱动\stmcdc.inf`

### 6.2 OpenOCD配置 (已修正)

```cfg
# CMSIS DAP (启明欣欣F407轻奢版V3.1标配仿真器)
source [find interface/cmsis-dap.cfg]
transport select swd
adapter speed 4000

source [find target/stm32f4x.cfg]

reset_config none
```

> 板子只引出 SWDIO/SWCLK/GND/VCC，没有独立 SRST 线，所以用 `reset_config none`（走 SYSRESETREQ 软复位）。
> 若你的 DAP 确实接了 nRESET，可改回 `reset_config srst_only srst_nogate`。
> 以 `Scripts/openocd.cfg` 为准。

### 6.3 串口ISP下载 (备选)

使用FlyMcu工具, 通过USART1(PA9/PA10), 需拉低BOOT0引脚。
工具路径: `6 常用软件\STM32F4串口下载软件（FLYMCU）`

### 6.4 烧录命令

```powershell
powershell -ExecutionPolicy Bypass -File Scripts\flash.ps1
```

## 7. CAN波特率计算

```
CAN1挂载在APB1上, PCLK1 = 42 MHz

例程参数: SJW=1TQ, BS1=7TQ, BS2=6TQ, BRP=6
Bit time = (1 + BS1 + BS2) * tQ = (1+7+6) * tQ = 14 * tQ
tQ = BRP / fPCLK = 6 / 42M = 0.1429 us
Bit rate = 1 / (14 * 0.1429 us) = 500 kbps

本项目代码参数: SJW=1TQ, BS1=11TQ, BS2=2TQ, BRP=6
Bit time = (1+11+2) * tQ = 14 * tQ  → 同样500kbps
```

## 8. 物理接线指南 (本项目)

### 8.1 TJA1050 CAN模块

| TJA1050 | F407板子 |
|---------|---------|
| VCC | 5V |
| GND | GND |
| TX | PB9 (CAN1_TX) |
| RX | PB8 (CAN1_RX) |
| CAN-H | OBD-II 引脚6 (蓝线) |
| CAN-L | OBD-II 引脚14 (黄线) |

> 注意: TX/RX不需要交叉, TJA1050的TX接MCU的TX(PB9), RX接MCU的RX(PB8)

### 8.2 ESP8266 (板载接口)

板子已内置ESP8266接口(UART4 PA0/PA1), 将ESP8266模块插入对应排针即可。
ESP8266_RST控制引脚: PA8。

### 8.3 MicroSD卡

板子右侧有MicroSD卡槽, 直接插卡, SDIO 4-bit模式。

### 8.4 CMSIS DAP烧录器

| DAP | 板子JTAG/SWD口 |
|-----|-----------|
| SWDIO | SWDIO |
| SWCLK | SWCLK |
| 3V3 | 3V3 |
| GND | GND |

## 9. 资料完整路径索引

### 9.1 顶层目录

```
E:\HONG\Down\启明欣欣STM32F407(轻奢版V3.1)学习资料\
├─ 0 开发板使用前先看.txt                    ← 先读
├─ 407轻奢版V3.1外设与接口介绍.jpg           ← 板子全貌图片
├─ 407轻奢版V3.1开发板程序下载教程.pdf       ← 烧录教程
├─ 启明407开发板(轻奢版)V3.1例程使用手册 .pdf ← 例程手册
├─ 启明欣欣STM32程序工程结构.pdf             ← 工程结构
├─ 1 启明欣欣407开发板(轻奢版)V3.1原理图\    ← 原理图PDF
├─ 2 各资源学习例程\                         ← 例程代码
├─ 3 启明欣欣_各扩展模块资料\                ← 扩展模块
├─ 4 SD卡根目录\                             ← SD卡示例文件
├─ 5 STCM32F4参考资料\                       ← 芯片手册
├─ 6 常用软件\                               ← 工具软件
├─ 7 相关资料\                               ← 进阶资料
└─ CMSIS DAP仿真器\                          ← DAP驱动
```

### 9.2 HAL库例程 (本项目以HAL库为准)

路径前缀: `2 各资源学习例程\HAL库版本\`

| 编号 | 例程名 | 关键引脚 | 本项目相关 |
|------|--------|---------|-----------|
| 1 | LED跑马灯 | PG13/14/15 | ✅ LED运行指示 |
| 2 | 蜂鸣器 | PG7 | - |
| 3 | 按键 | PF6/7/8/9 | - |
| 4 | 外部中断 | - | - |
| 5 | TFTLCD | FSMC | - |
| 6 | 串口1-RS232 | PA9/PA10 AF7 | - |
| 7 | 定时器中断 | - | - |
| 8 | PWM输出 | - | - |
| 9 | 独立看门狗 | - | - |
| 10 | ADC | - | - |
| 11 | DAC | - | - |
| 12 | 串口6-485 | PC6/PC7 | - |
| 13 | 各串口TTL | USART3: PC10/11, USART6: PC6/7 | 参考 |
| 14 | IIC_24C02 | PB8/PB9 AF4 | 引脚冲突参考 |
| 15 | SPI_W25Qxx | PB3/4/5 AF5 | - |
| **16** | **CAN1与CAN2** | **CAN1: PB8/PB9 AF9** | **✅ 核心** |
| 17 | DMA | - | 参考 |
| 18 | RTC实时时钟 | - | - |
| 19 | 汉字显示 | - | - |
| 20 | RTC农历显示 | - | - |
| 21 | 内部温度传感器 | - | - |
| 22 | DS18B20 | PG11 | - |
| 23 | 红外HS0038 | PA8 | 引脚冲突参考 |
| 24 | 触摸屏 | - | - |
| 25 | 232_485_can转换 | USART2: PA2/PA3 | 参考 |
| 26 | USB U盘(Host) | PA11/PA12 | - |

### 9.3 扩展模块资料

路径前缀: `3 启明欣欣_各扩展模块资料\`

| 模块 | 子路径 | 本项目 |
|------|--------|--------|
| **ESP8266** | `1、407轻奢版配套ESP8266资料\` | ✅ WiFi模块 |
| 蓝牙4.0 | `2、407轻奢版配套蓝牙4.0模块资料\` | - |
| 串口音频 | `3、串口音频模块资料\` | - |
| 2.8寸屏 | `4、2.8寸屏资料\` (ILI9341) | - |
| 7寸电容屏 | `5、SSD1963_7寸液晶电容屏资料\` | - |
| 7寸触摸屏 | `6、RA8875_7寸液晶触摸屏资料\` | - |

ESP8266资料详细:
- `模块规格书.pdf` — 模块硬件规格
- `AT指令集018.pdf` — ESP8266 AT命令完整手册
- `AT指令使用示例.pdf` — AT命令使用教程
- `ESP8266串口wifi模块使用手册.pdf` — 使用手册
- `手机APP经wifi控制开发板例程代码\` — 例程代码(SPL库)
- `启明欣欣WIFI.apk` — 手机APP

### 9.4 芯片参考资料

路径: `5 STCM32F4参考资料\`

| 文件 | 用途 |
|------|------|
| `STM32F407ZGT6数据手册.pdf` | 芯片引脚/电气参数 |
| `STM32F4xx中文参考手册.pdf` | 寄存器/外设详解 |
| `STM32F4xx英文参考手册.pdf` | 同上英文版 |
| `Cortex-M4 Devices Generic User Guide.pdf` | 内核手册 |
| `STM32F3与F4系列Cortex M4内核编程手册.pdf` | 编程手册 |
| `ST MCU 最新选型手册_201603.pdf` | 选型参考 |
| `1，STM32F4xx固件库\固体库手册stm32f4xx_dsp_stdperiph_lib_um.chm` | 标准库手册 |

### 9.5 进阶资料

路径: `7 相关资料\`

| 目录 | 用途 |
|------|------|
| `13，FreeRTOS学习资料` | **✅ 本项目使用FreeRTOS** |
| `5，FAT及FATFS资料` | **✅ 本项目使用FatFs** |
| `6，CAN学习资料` | **✅ 本项目使用CAN** |
| `3，LWIP学习资料` | 网络协议栈 |
| `1，UCOS学习资料` | RTOS学习 |
| `2，EMWIN学习资料` | GUI |
| `9，MDK手册` | Keil MDK |

### 9.6 工具软件

路径: `6 常用软件\`

| 工具 | 用途 |
|------|------|
| `MDK5\` | Keil5 IDE |
| `JLINK使用\` | JLink驱动 |
| `STM32F4串口下载软件（FLYMCU）` | 串口ISP下载 |
| `串口调试助手` | 串口调试 |
| `网络调试工具` | TCP/UDP调试 |

### 9.7 DAP仿真器

路径: `CMSIS DAP仿真器\`

| 文件 | 用途 |
|------|------|
| `启明欣欣CMSIS DAP使用手册.pdf` | DAP使用教程 |
| `DAP串口驱动\stmcdc.inf` | Windows驱动 |
| `DAP串口驱动\系统替换文件\` | Win7/8系统文件 |

## 10. 项目代码修正记录 (已完成)

| # | 文件 | 旧配置 | 正确配置 | 状态 |
|---|------|--------|---------|------|
| 1 | main.c | huart1 (USART1) | huart4 (UART4) | ✅ 已修 |
| 2 | hal_msp.c | USART1: PA9/PA10 AF7 | UART4: PA0/PA1 AF8 | ✅ 已修 |
| 3 | hal_msp.c | CAN1: PA11/PA12 AF9 | CAN1: PB8/PB9 AF9 | ✅ 已修 |
| 4 | it.c/it.h | USART1_IRQn/DMA2_Stream2 | UART4_IRQn/DMA1_Stream2 | ✅ 已修 |
| 5 | task_config.c | &huart1 | &huart4 | ✅ 已修 |
| 6 | main.h | huart1/hdma_usart1_rx | huart4/hdma_uart4_rx | ✅ 已修 |
| 7 | openocd.cfg | stlink.cfg | cmsis-dap.cfg | ✅ 已修 |
| 8 | CMakeLists.txt | 硬编码绝对路径 / 无浮点格式化 | 环境变量+回退探测 / 补 `-specs=nano.specs -u _printf_float` / 补 `stm32f4xx_hal_iwdg.c` | ✅ 已修 |

编译结果 (2026-09-09 阶段14): 54 编译单元, text=73536, data=480, bss=41376, 0错误 0警告
