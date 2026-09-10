# IMT407 极狐T1 车载HUD项目 - 进度跟踪

> 项目启动: 2026-09-05
> 完整方案文档: [project.html](./project.html)
> 架构决策参考: [.trae/documents/imt407-car-hud-project-preparation.md](./.trae/documents/imt407-car-hud-project-preparation.md)
> 逐条任务清单: [TASKS.md](./TASKS.md)

## 里程碑

| # | 阶段 | 状态 | 完成日期 |
|---|------|------|----------|
| 0 | 方案整合 + 文档输出 | DONE | 2026-09-05 |
| 1 | 工具链安装 | DONE | 2026-09-05 |
| 2 | 项目骨架目录 | DONE | 2026-09-05 |
| 3 | HAL库 + FreeRTOS + 初始化代码 | DONE | 2026-09-05 |
| 4 | 业务逻辑模块代码 | DONE | 2026-09-05 |
| 5 | HTML5 HUD仪表盘 | DONE | 2026-09-05 |
| 6 | 构建脚本 | DONE | 2026-09-05 |
| 7 | 文档 | DONE | 2026-09-05 |
| 8 | 首次编译验证 | **DONE** | 2026-09-05 |
| 9 | 代码一致性审查 + 修复 | DONE | 2026-09-08 |
| 10 | HUD 界面迭代 (v4.0 → v7.2) | DONE | 2026-09-08 |
| 11 | 硬件验证 (烧录/上板) | ⬜ 待烧录线 + 提车 | — |
| 12 | 体验优化 | ⬜ 待实车 | — |
| 13 | 代码自查修复 (P0~P3) | **DONE** | 2026-09-09 |
| 14 | 第二轮深度 debug (静态审计 P0/P1) | **DONE** | 2026-09-09 |

## 首次编译结果

**编译状态: ✅ 通过**

| 指标 | 值 |
|------|-----|
| 编译器 | arm-none-eabi-gcc 14.3.1 (GNU Tools for STM32) |
| 目标芯片 | STM32F407ZGT6 (Cortex-M4F, 168MHz) |
| 编译源文件 | 50 个 (.c + .s) |
| text (代码) | 60,640 bytes (~59 KB) |
| data (已初始化数据) | 480 bytes |
| bss (未初始化数据) | 42,144 bytes (~41 KB) |
| 总计 RAM | ~42 KB |
| Flash 占用 | ~60 KB |
| ELF 文件 | 518 KB |
| BIN 文件 | 59.7 KB |
| HEX 文件 | 168 KB |

### 模块组成
- **Core 业务层**: 15个C文件 (CAN_OBD, WiFi, HTTP, Energy, SDLog, 等；`gps.c`/`imu.c` 已无调用者，被 `--gc-sections` 丢弃)
- **HAL 驱动层**: 16个C文件 (GPIO, UART, CAN, SDIO, DMA, RCC, 等)
- **FreeRTOS**: 9个C文件 (任务, 队列, 信号量, 定时器, 等)
- **FatFs 文件系统**: 3个C文件 (ff.c, ffsystem.c, ffunicode.c)
- **系统/启动**: system_stm32f4xx.c + startup_stm32f407xx.s

## 最新编译结果 (2026-09-09, 阶段14 修复后)

**编译状态: ✅ 通过 (0 错误 0 警告)**

| 指标 | 值 |
|------|-----|
| 编译单元 | 54 个 |
| text (代码) | 73,536 bytes (~71.8 KB) |
| data (已初始化数据) | 480 bytes |
| bss (未初始化数据) | 41,376 bytes (~40.4 KB) |
| dec | 115,392 bytes |
| ELF 文件 | 508,656 bytes |

> 校验命令: `Scripts/build.ps1`
> 阶段间变化: text 62,672 → 73,536 (主要来自 `-u _printf_float` 引入的浮点格式化)

## 已创建文件清单

### Core/Inc (15个头文件)
- shared_data.h, uart_ringbuf.h, wifi.h, obd.h, gps.h, imu.h, energy.h
- httpd.h, sdlog.h, task_config.h, main.h, stm32f4xx_it.h, stm32f4xx_hal_conf.h
- FreeRTOSConfig.h, cmsis_os2.h

### Core/Src (15个源文件)
- shared_data.c, uart_ringbuf.c, wifi.c, obd.c, gps.c, imu.c, energy.c
- httpd.c, sdlog.c, task_config.c, main.c, stm32f4xx_it.c
- stm32f4xx_hal_msp.c, syscalls.c, diskio.c, cmsis_os2.c

### Drivers
- **STM32F4xx_HAL_Driver**: Inc/ + Src/ (完整HAL库)
- **CMSIS/Device/ST/STM32F4xx**: 设备头文件 + system_stm32f4xx.c
- **FatFs/source**: ff.c, ffsystem.c, ffunicode.c, ffconf.h, diskio.h

### WebUI
- index.html (性能风格HUD仪表盘: G值球, 功率弧, 罗盘, RPM, 电压电流)

### Scripts
- build.ps1 (自动定位STM32 bundle工具链 + CMake + Ninja)
- flash.ps1, openocd.cfg

### 构建配置
- CMakeLists.txt, arm-toolchain.cmake, STM32F407ZGTx_FLASH.ld
- startup_stm32f407xx.s

### Docs
- wiring_guide.md, pinmap.md, build_flash.md

### 项目根目录
- .gitignore, TASKS.md, PROGRESS.md, project.html

## 变更日志

### 2026-09-09 (阶段14 第二轮深度 debug — 静态审计修复)
> 无烧录线/无实车条件下能做的全部代码级修复, 每批改完立即编译自查
> 逐条任务见 [TASKS.md](./TASKS.md) 阶段14 章节

**P0 致命 (4项)**
- CAN 数据可信性: 请求/响应配对校验 (响应须回显本次 SID+PID)、ISO 15765-2 连续帧 SN 连续性校验、负响应 0x7F 立即失败、`obd_valid` 真三态
- 锁与阻塞 I/O 解耦: TaskOBD/TaskSDLog/`/data` 路由锁内只 memcpy 快照, CAN 15 个 PID 串行请求与阻塞式 f_write 移到锁外; `SharedData_MergeOBD` 只覆盖 OBD 自有字段
- FatFs 并发串行化: `sdlog.c` 新增 `s_sd_mutex` 覆盖全部 FatFs 调用 (FF_FS_REENTRANT=0, FatFs 自身无互斥)
- RTOS 对象创建失败处理: `osMutexNew`/`osThreadNew` 返回值检查, 失败进安全态而非裸跑

**P1 高 (4项)**
- newlib-nano 浮点: `CMakeLists.txt` 补 `-u _printf_float`, nm 确认 `_dtoa_r` 进镜像 (此前 `%.1f` 输出空); `syscalls.c` `_write` 改空实现
- 中断优先级 + 看门狗: SDIO_IRQn 优先级 4→5 (4 违反 configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY); 启用 IWDG + 新建 `watchdog.c/h` (OBD/Energy/HTTPD 三任务心跳, 20s 启动宽限期, 调试冻结)
- ESP8266 链路健壮性: 5 条 AT 逐条校验 + `init_fail_step` 落 SD 日志、PA8 硬复位替代 `AT+RST`、环缓 overflow 告警、HTTP 各响应路径检查发送返回值
- 文档不实记录修正: `obd_valid` 描述、FatFs 码页 936→932、`board_reference.md` CMakeLists 行、`OBD_PID_TABLE.md` CAN 引脚 PA11/PA12→PB8/PB9

**编译验证**
- 最终: **54 编译单元, text=73536, data=480, bss=41376, dec=115392, ELF=508656, 0 错误 0 警告**
- 阶段间 text 变化 62,672 → 73,536 (主要来自 `-u _printf_float` 引入的浮点格式化)

### 2026-09-09 (阶段13 全工程自查 + P0~P3 修复)
> 详见 [Docs/self_audit_2026-09-09.md](./Docs/self_audit_2026-09-09.md)
> 自查结论: **固件即使烧进去也拿不到真实数据** —— 两条最关键的中断链路写好了函数但从未接线

**P0 致命 (4项)**
- P0-1 CAN 接收链路接线: 补 `HAL_CAN_RxFifo0MsgPendingCallback()` → `CAN_OBD_RxHandler()`，此前 CAN1_RX0 中断收到的帧无人取走，15 个 PID 全部超时
- P0-2 UART4 IDLE 中断: `UART4_IRQHandler()` 中显式判 IDLE 并调 `UART_RingBuf_OnIDLE()`，此前既中断风暴又不搬数据，WiFi 全废
- P0-3 HTTP 任务栈 2048 → 4096 (`task_config.h`)，原峰值约 2800B 必然溢出
- P0-4 HUD 页面内嵌固件: `WebUI/index.html` → `Core/Src/web_page.c` C 常量数组 (19475 bytes)，`GET /` 直接返回 HTML，不再 302 到裸 JSON

**P1 高 (5项)**
- P1-1 `obd_valid` 连续 3 轮无有效响应归零 (此前只置 1 从不归零；阶段14 已升级为真三态: 本轮有响应→1 / 连续 3 轮全失败→0 / 未达阈值→保持)
- P1-2 前端 `mockMode` 默认改为 `false` (此前默认开模拟数据，最危险的假阳性)
- P1-3 断线检测改为"最近 3 秒内是否成功收到 `/data` 且 JSON 解析成功"
- P1-4 `timestamp` 去掉硬编码 1722528000，改为运行时长语义
- P1-5 `ESP8266_SendData()` 改为等待 `>` 提示再发 payload、等待 `SEND OK` 超时返回 (此前固定 `HAL_Delay(50)`，大包必丢)

**P2 中 (9项)**
- P2-1 `CAN_OBD_ScanPIDs()` 接线 (HTTP 触发 + 启动扫描，结果写 SD 卡)
- P2-2 `resp.len >= 2` 长度守卫，防 uint8 下溢越界
- P2-3 `HTTPD_BuildJSON` snprintf 截断 clamp 到 `buf_len-1`
- P2-4 `disk_initialize()` 补 `HAL_SD_ConfigWideBusOperation(SDIO_BUS_WIDE_4B)`
- P2-5 `sdlog.c` `f_open` 返回值校验 + `f_close` 收尾，防掉电损坏 FAT
- P2-6 MSP 栈 `_Min_Stack_Size` 0x400 → 0x800
- P2-7 `CMakeLists.txt` 去掉硬编码绝对路径，改环境变量 + 回退探测
- P2-8 重写 `Docs/pinmap.md` / `Docs/wiring_guide.md` (UART4 PA0/PA1、CAN1 PB8/PB9、CMSIS-DAP)，顺带修正 `Docs/build_flash.md` 的 ST-Link 表述与 `Docs/board_reference.md` 的 `reset_config` 矛盾
- P2-9 `Scripts/openocd.cfg` + `flash.ps1` 统一为 CMSIS-DAP / `reset_config none`

**P3 低 (3项)**
- canvas 尺寸仅在变化时重置 + `devicePixelRatio` 适配
- 粒子 `shadowBlur` 改为预渲染贴图，降手机端负载
- Shy 面板 0~5km/h 过渡区改分段渐变，消除半透明叠加重叠

**误报纠正 (3项，已核实推翻，未改动)**
- FPU 未使能 → 错，`SystemInit()` 已有 CPACR 配置
- SDIO `ClockDiv=0` 超规范 → 不准确，实为 24MHz (`PLL48CLK/(ClockDiv+2)`)
- CAN 采样点应 87.5% → 不成立，当前 85.7% 是该时钟下最接近 ISO 11898 区间的取值

**编译验证**
- 首次编译暴露 `sdlog.c` 真实错误 (`f_printf` 返回值语义误用 `!= FR_OK`)，修正为 `< 0`
- 最终结果: **52 单元，text=62672, data=104, bss=41024, 0 错误 0 警告**
- 遗留: `Core/Src/gps.c` / `imu.c` 被 `file(GLOB Core/Src/*.c)` 吸入编译，但无任何调用者且被 `--gc-sections` 清零 (map 文件实测 `.text/.data/.bss` 均为 0)，不占固件空间

### 2026-09-07 (CAN直读方案)
- **架构级变更: ELM327 → CAN直读**
  - 删除旧obd.c/h, 新增can_obd.c/h (480行)
  - 实现ISO 15765-4 CAN协议栈 (单帧/首帧/连续帧/流控帧)
  - 实现ISO 15765-2多帧重组逻辑
  - 实现Service 01 + Service 05双协议PID请求
  - 实现PID扫描模式 (29个PID自动扫描, SD卡记录)
  - 国标Service 05 PID解析公式全部实现
- **硬件变更: UART2/ELM327 → CAN1/TJA1050**
  - main.c: 删除USART2, 添加CAN1 (500kbps; 引脚后修正为 PB8/PB9, 见 Docs/pinmap.md)
  - hal_msp.c: 删除USART2 MSP, 添加CAN MSP (AF9)
  - it.c/it.h: 删除DMA1_Stream5/USART2中断, 添加CAN1_RX0中断
  - main.h: 删除huart2/hdma_usart2, 添加hcan1
  - task_config.c: OBD_Init→CAN_OBD_Init, OBD_Update→CAN_OBD_Update
  - CMakeLists.txt: 添加stm32f4xx_hal_can.c
  - hal_conf.h: 启用HAL_CAN_MODULE_ENABLED + #include
- **数据源修复:**
  - 电压: PID 0x42(12V) → S05 PID 0x1B(高压母线电压)
  - 电流: 无PID → S05 PID 0x1C(母线电流)
  - 功率: V×I反算(=0) → S05 PID 0x1C 或 V×I正向计算
  - 电机转速: S01 0x0C(可能不支持) → S05 PID 0x1A(国标)
  - 电机温度: 无 → S05 PID 0x07(国标)
  - SOC: S01 0x5B(不确定) → S05 PID 0x03(国标, 精度0.392%)
  - 油门: S01 0x11(节气门) → S05 PID 0x09(加速踏板, 精度0.392%)
- **energy.c修复:** 硬编码dt=0.1s → HAL_GetTick()真实时间间隔
- **shared_data.h新增字段:** motor_temp, igbt_temp, cell_vmax, cell_vmin, brake
- **httpd.c JSON新增:** motor_temp, soc, brake, obd_valid字段
- **sdlog新增:** SDLog_Log函数(文本日志, 用于PID扫描结果记录)
- **编译验证通过:** 51个源文件, text=62KB, bss=40KB

### 2026-09-05 (后期)
- **首次编译成功!** 50个源文件，Flash占用~60KB，RAM占用~42KB
- 下载并集成 FatFs R0.16 文件系统 (从 elm-chan.org 官网)
- 配置 ffconf.h: 启用 f_printf/float/long long，代码页932 (`ffconf.h:87`)，无RTC模式
- 实现 diskio.c 对接 STM32 HAL SDIO 驱动
- 修复 uart_ringbuf.c: 改用标准 HAL_UART_Receive_DMA + 空闲中断 (STM32F4无ReceiveToIdle_DMA)
- 修复 stm32f4xx_it.c: 添加 FreeRTOS 中断处理函数声明
- 添加 stm32f4xx_ll_sdmmc.c 底层SDIO驱动 (HAL SD驱动依赖)
- 添加 syscalls.c newlib-nano 系统调用存根 (_sbrk, _write, _read, 等)
- 修复链接顺序问题: 使用 target_link_libraries 确保库在目标文件之后
- 更新 build.ps1: 自动从 STM32 bundle 目录定位 CMake/Ninja/GCC 工具链
- 工具链路径: `C:\Users\Administrator\AppData\Local\stm32cube\bundles\`
  - cmake 4.3.1, ninja 1.13.2, gnu-tools-for-stm32 14.3.1

### 2026-09-05 (早期)
- 整合多轮讨论, 输出完整项目文档 `project.html`
- 确定最终方案: F407+ESP8266 WiFi → 七彩虹E708 Q1平板浏览器 → 屏幕反射HUD
- 确定BOM清单(约55元), 接口分配, 开发路线5步
- 创建 TASKS.md 逐条任务清单
- 安装 VS Code + STM32CubeIDE扩展包 v3.10.0
- 创建项目目录结构
- 编写全部业务逻辑模块代码
- 编写 HTML5 HUD仪表盘 (性能风格升级版)
- 编写构建脚本和文档

### 2026-09-08 (代码一致性审查 + 修复5个问题)
- **P0: fetchData URL不匹配** — HUD请求'data.json'但httpd.c路由是'GET /data' → 改为'/data'
- **P0: 功率双重计算** — can_obd.c从PID读到vd->power, energy.c又用V×I/1000覆盖 → 改为PID优先
- **P1: GPS/IMU残余代码** — task_config.c/main.c/hal_msp.c/it.c/main.h仍初始化huart3/hi2c1 → 全部清除
  - 删除: USART3初始化/I2C1初始化/DMA1_Stream1中断/USART3中断/I2C1中断/GPS任务/IMU任务
  - task_config.h: 删除TASK_GPS_STACK/TASK_IMU_STACK/TASK_GPS_PRIO/TASK_IMU_PRIO和函数声明
- **P1: mockData缺字段** — 缺current/brake赋值 → 从power和voltage反算current, 回充时随机生成brake
- **P2: SDLog格式污染** — SDLog_Log文本日志混在CSV数据行中 → 改用独立文件obd_scan.log
- **编译验证通过:** 51源文件, text=40000(↓23KB), bss=40000 — 链接器移除了未调用的GPS/IMU代码
- **功率弧放大:** 半径从字号×0.62→字号×1.5, 弧直径=h×0.48 > 文字宽h×0.40, 文字完全在弧内
- **SOC/温度环放大:** SOC半径0.13H→0.17H, 温度0.105H→0.14H, 环内加黑色填充让文字背景干净
- **环内文字居中:** 从textBaseline='middle'(Canvas对数字偏上)改为'alphabetic'+手动偏移sz*0.35
- **粒子遮罩:** 椭圆形羽化半透明遮罩(径向渐变0.7→0.5→0透明), 粒子路过文字时被柔和压暗
- **车速进度条避让:** 两端自动避开SOC/温度环(socGap/tempGap=环半径×1.3), 不再撞环
- **编译验证通过:** 51源文件, text=63196, bss=40872

### 2026-09-08 (HUD v7.1 混合视觉: 弧+环+大字)
- **功率:** 240°弧形仪表盘(背景灰+前景色按比例填充), 大字在弧中央, 一眼看踩多深
- **SOC:** 恢复环形, 环填充=SOC%, 大字在环中央
- **温度:** 环形, 环填充=(temp-20)/100映射, 大字在环中央
- **能量流:** 保持无框文字链+粒子(不需要框)
- **低速过渡:** SOC/温度环同步淡出, 面板淡入, 无重叠

### 2026-09-08 (HUD v7.0 无框设计)
- **v6.0问题复盘:** 参数被框进小框框(BAT/MOTOR方框/面板边框)文字被压缩; 16:9窗口下B=min(w,h)导致字号灾难性小; 空位浪费
- **v7.0设计原则: 无框+超大字+填满屏幕**
  - 废除所有方框/边框/圆环, 文字直接悬浮
  - 字号基于H且比例放大2-3倍: 功率0.24H(原0.14B) / 能量流0.09H(原0.038B) / SOC 0.11H(原0.04B)
  - 能量流改为文字链: BAT 384V ●●▶ MOTOR 26kW ●●▶ 88, 粒子在文字间隙流动
  - SOC环→超大"78%"文字; 面板边框→5行大字直接淡入
- **自检(1000×560):** 功率134px/能量流50px/SOC62px; 相邻间距≥9px; 粒子区与文字宽度measureText验证600px极窄也不重叠

### 2026-09-08 (HUD v6.0 完全重做)
- **v5.0问题复盘:** 节点标签与数值重叠 / 能量流条太窄中间大片黑 / 温度贴边被裁 / 字号小间距大
- **v6.0布局数学 (1080p):**
  - 功率151px@0.19H, DRIVE标签28px@0.09H (间距18px)
  - 能量流 0.86W×0.20B @0.385H, 节点标签nodeY-0.26H/数值nodeY+0.24H (间距24px)
  - 车速70px@0.56H + 进度条@0.63H
  - SOC环r=0.055B@左下(0.10W,0.80H), 最高温度@右下(0.90W,0.80H)
- **防重叠措施:** kW/km-h单位用measureText实测宽度定位; 温度只显示MAX一个值; 全部middle baseline
- **Shy Tech交叉淡入淡出:** 低速时驾驶元素(功率/能量流/车速)淡出, 信息面板淡入, 不再半透明叠盖

### 2026-09-08 (HUD v5.0 极简重设计)
- **设计理念全面重构: 基于行业HUD标准研究**
  - 研究BMW Shy Tech / 奔驰AR-HUD / Polestar HUD案例
  - 遵循希克定律: 核心信息1-3个, 最关键只有1个
  - 遵循国标T/ITS 0222: 数字≥30弧分, 4种语义色
- **核心改为能量流 (新能源差异化):**
  - T1: 功率大字 (中央最大, 0.09×base ≈ 97px@1080p)
  - 核心可视化: 电池→电机→车轮粒子动画
  - T2: 车速降为底部进度条 (不抢视觉中心)
  - T3: SOC环形图标+温度图标 (角落, 异常才高亮)
- **Shy Tech状态机:**
  - 行驶(speed>5): 只显示功率+能量流
  - 蠕行(0<speed≤5): 保持行驶状态
  - 停车(speed=0持续3秒): 渐入展开面板(电池健康/能耗/行程/评分)
- **颜色减到4种语义:** 白(功率) / 青(驱动) / 绿(回充+正常) / 红(警告)
- **标签全部砍掉:** 不再显示BATTERY/TEMP/TRIP等文字标签, 用位置+颜色区分

### 2026-09-08 (HUD v4.3 易读性修复)
- 修复6个文字重叠: middle baseline / 行高1.6× / 面板紧凑 / 自适应缩放

### 2026-09-07 (功能升级: 5大新功能 + HUD v4.0)
- **决策: 去掉GPS和IMU模块**
  - GPS: 速度OBD有, 航向需移动, 天线破孔新车不干
  - IMU: G值球/坡度砍掉, 省模块+接线
  - 保留: ESP8266(WiFi) + TJA1050(CAN OBD) + MicroSD(日志)
- **新增5大功能 (纯OBD数据驱动):**
  1. 能量流可视化: 电池→电机→车轮粒子动画, 蓝=驱动/绿=回充
  2. 电池健康监控: 单体Vmax/Vmin/压差ΔV/温差ΔT, 压差>100mV标红
  3. 温度三件套: 电机/IGBT/电池温度, 各自阈值预警
  4. 驾驶效率评分: 100分制, 急加速-2/急制动-3/回收+1/匀速+0.5, ABCDE等级
  5. 行程统计: 总能耗/总回充/距离/平均能耗, SD卡CSV导出25字段
- **代码改动 (6个文件):**
  - shared_data.h: 新增cell_delta_v/batt_tmax/batt_tmin/batt_delta_t/drive_score/drive_grade
  - can_obd.c: 新增IGBT/制动/单体电压/电池温度PID请求, 修复电机温度赋值bug
  - energy.c: 重写评分体系(5秒评估周期), 行程统计累加逻辑
  - energy.h: EnergyState新增评分/计数器/历史数据字段
  - sdlog.c: CSV 25字段, 含电池健康/温度/评分/行程统计
  - httpd.c: JSON新增cell_vmax/cell_dv/motor_temp/igbt_temp/drive_score等
  - index.html: HUD v4.0 完全重写, 去掉G值球/指南针, 新增5大功能UI
- **编译验证通过:** 51个源文件, text=63KB, bss=40KB

### 2026-09-07 (HUD v3.0 + 文档更新)
- **HUD v3.0 数据无效状态处理:**
  - 新增connStatus连接状态管理 (OBD/GPS)
  - OBD断线时所有OBD数据标灰, 显示"--", 面板边框变暗
  - 左上角新增连接状态指示器: OBD绿灯/红灯, GPS绿灯/红灯
  - mockData新增模拟断连逻辑: 每30秒断5秒, 可直观看到断线效果
- **HUD新增显示项:**
  - SOC百分比 (左侧面板, <20%变红)
  - 电机温度 (右侧面板, <60绿/<90黄/>90红)
  - 制动踏板 (替代油门/回收, 有制动时红色高亮)
  - "NO DATA"标签 (OBD断线时功率区显示)
- **mockData对齐:**
  - 新增motor_temp/soc/brake/obd_valid字段
  - 删除旧的regen_pct/throttle逻辑, 改为从数据源直接获取
- **文档更新:**
  - wiring_guide.md: ELM327章节 → TJA1050 CAN收发器接线图 + OBD-II引脚定义
  - pinmap.md: USART2(PA2/PA3) → CAN1(引脚后修正为 PB8/PB9, AF9), 新增CAN波特率计算
  - 安全红线: "只读OBD" → "只读CAN", 新增TJA1050供电说明

## 下一步行动

### 等烧录线到货 (需要硬件)
1. CMSIS-DAP 接 SWD (PA13/PA14/GND/3V3)，`Scripts/build.ps1` → `Scripts/flash.ps1` 烧录
2. 板载 CAN1 ↔ CAN2 回环自测 (先不接车，验证 500kbps + ISO 15765 收发链路)
3. 验证 UART4/ESP8266 通信、SDIO/FatFs 挂载、WiFi/HTTP 服务、`GET /scan` PID 扫描

### 提车后验证 (需要实车)
1. TJA1050 接 OBD-II (6=CAN-H, 14=CAN-L, 16=+12V, 4/5=GND)，接线: TJA1050 + ESP8266 + MicroSD (GPS/IMU 已 pass)
2. **PID扫描**: 触发扫描，SD 卡记录 29 个 PID 响应情况
3. 确认极狐T1响应哪些 Service 05 PID
4. 根据扫描结果调整 can_obd.c (禁用无响应PID)
5. HUD实车联调: 平板浏览器访问, 调字号/布局
6. 日间/夜间模式切换
