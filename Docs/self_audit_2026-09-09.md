# IMT407 全工程自查报告

> 日期: 2026-09-09
> 方式: 静态代码审查（烧录线未到，无法上板验证）
> 范围: Core/Src + Core/Inc 全部业务代码、WebUI/index.html、构建链路（CMake/ld/openocd）、Docs 文档一致性

---

## 结论摘要

| 级别 | 数量 | 性质 |
|------|------|------|
| P0 致命 | 4 | 功能链路完全不通，上板必然失败 |
| P1 高 | 5 | 数据错误 / 体验崩坏，会误导判断 |
| P2 中 | 9 | 健壮性、性能、工程化缺陷 |
| P3 低 | 5 | 体验打磨 |
| 误报纠正 | 3 | 子审查给出的错误结论，已核实推翻 |

**核心判断：当前固件即使烧进去，HUD 也拿不到任何真实数据。** 原因是两条最关键的中断链路（CAN 收帧、UART4 IDLE）写好了函数但从未接线。

---

## P0 致命（必须修，否则上板无效）

### P0-1 CAN 接收链路未接线
- 位置: `Core/Src/can_obd.c:47` 定义 `CAN_OBD_RxHandler()`；`Core/Inc/can_obd.h:88` 声明
- 现象: 全工程 grep 无任何调用点；`HAL_CAN_RxFifo0MsgPendingCallback` 在整个工程中不存在
- 根因: `Core/Src/stm32f4xx_it.c` 的 `CAN1_RX0_IRQHandler()` 只调用 `HAL_CAN_IRQHandler(&hcan1)`。HAL 收到帧后会调用 weak 回调 `HAL_CAN_RxFifo0MsgPendingCallback()`，但无人重写 → 帧留在 FIFO0 无人取走
- 后果: `CAN_OBD_SendRequest()` 等不到响应，15 个 PID 全部超时，`obd_valid` 恒为 0，HUD 永远 `NO DATA`
- 修复: 在 `can_obd.c` 增加重写回调（1 个函数，3 行）
  ```c
  void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
      CAN_OBD_RxHandler(hcan);
  }
  ```

### P0-2 UART4 IDLE 中断未处理
- 位置: `Core/Src/uart_ringbuf.c:22` 定义 `UART_RingBuf_OnIDLE()`，无调用点
- 现象: `UART_RingBuf_StartDMA()` 里 `__HAL_UART_ENABLE_IT(rb->huart, UART_IT_IDLE)` 使能了 IDLE 中断，但无人清除标志
- 根因: `UART4_IRQHandler()` 只调 `HAL_UART_IRQHandler(&huart4)`，HAL 不处理 IDLE 标志（它不是错误标志）
- 后果: 双重灾难——① IDLE 标志持续置位，中断反复触发形成中断风暴，CPU 被吃满；② DMA 数据永不搬运进环形缓冲，`UART_RingBuf_Read()` 恒返回 0，`ESP8266_Poll()` 永远读不到字节，WiFi 全废
- 修复: 在 `UART4_IRQHandler()` 里显式判 IDLE 并调用 `UART_RingBuf_OnIDLE(&s_esp_rxbuf)`

### P0-3 HTTP 任务栈溢出
- 位置: `Core/Inc/task_config.h` `TASK_HTTPD_STACK = 2048`
- 调用链峰值（同一栈帧内嵌套）:
  - `ESP8266_Poll()` → `uint8_t buf[1024]`（`Core/Src/wifi.c`）
  - → `HTTPD_HandleRequest()` → `char json[512]` + `VehicleData_t vd`（约 100B，`Core/Src/httpd.c:69-70`）
  - → `HTTPD_SendJSONResponse()` → `char header[128]` + `char full[1024]`（`httpd.c:36,43`）
  - 合计 ≈ 2800 B + 调用开销 + 上下文保存 > 2048 B
- 后果: 栈溢出踩到相邻 TCB/堆，表现为随机 HardFault（`configCHECK_FOR_STACK_OVERFLOW=2` 会先命中 hook 死循环）
- 修复: 栈提到 4096；或把 `full[1024]` 改为直接分两次 `ESP8266_SendData`（省 1KB 栈）

### P0-4 HUD 页面无法通过设备访问
- 位置: `Core/Src/httpd.c:79` — `GET /` 只返回 302 到 `http://192.168.4.1/data`
- 现象: `HTTPD_SendHTMLResponse()`（`httpd.c:51`）已实现但全工程无调用点
- 后果: 手机连上 ESP8266 AP 后打开根路径，看到的是裸 JSON，不是 HUD。目前只能在电脑上本地打开 `WebUI/index.html` 看效果
- 修复方案（二选一）:
  - A. 把 `index.html` 转成 C 常量数组内嵌固件（gzip 后约 8KB，Flash 剩余 960KB 完全够），`GET /` 返回 HTML
  - B. 从 SD 卡读 `index.html` 回传
  - 注意：无论哪种，都必须先修 P1-5（ESP8266 发送时序），否则大文件必丢

---

## P1 高

### P1-1 `obd_valid` 只置 1 从不归零
- 位置: `Core/Src/can_obd.c`（`CAN_OBD_Update()` 内，约 :341）
- 后果: 前端"断线检测"永远显示绿色已连接；实车 CAN 断了、ESP8266 掉线都察觉不到
- 修复: 连续 N 轮（如 3 轮）无有效响应时置 0

### P1-2 前端默认开启 Mock 模式
- 位置: `WebUI/index.html:69` `var mockMode=true`
- 后果: 上板后浏览器一直显示模拟数据，会误判"板子工作正常"——这是最危险的假阳性
- 修复: 默认 `false`；`fetch` 连续失败时在角落显示"无数据"而不是回落到假数据

### P1-3 断线检测依赖永不为 0 的字段
- 位置: `WebUI/index.html:36-39` `updateConn()` 只看 `data.obd_valid`
- 修复: 改为"最近 3 秒内是否成功收到 `/data` 且 JSON 解析成功"

### P1-4 时间戳硬编码
- 位置: `Core/Src/httpd.c:74` `vd.timestamp = HAL_GetTick()/1000 + 1722528000;`
- 后果: HUD 右下角时间恒显示 2024-08-01 附近的时刻，与真实时间无关
- 修复: ESP8266 联网时用 SNTP 对时；否则改为"上电计时"并明确标注，不要伪装成真实时间

### P1-5 ESP8266 发送用固定延时
- 位置: `Core/Src/wifi.c` `ESP8266_SendData()` 用 `HAL_Delay(50)`
- 后果: 不等待 ESP8266 的 `>` 提示或 `SEND OK`，小包侥幸能过，大包（HTML）必然丢
- 修复: 改为等待 `>` 后再发 payload，等待 `SEND OK` 超时才返回

---

## P2 中

| # | 位置 | 问题 | 修复 |
|---|------|------|------|
| P2-1 | `can_obd.c:454` `CAN_OBD_ScanPIDs()` | 无调用点，PID 扫描是孤儿代码，阶段 11.6 无法执行 | 启动时扫描一次并写 SD 卡，或加 HTTP 触发接口 |
| P2-2 | `can_obd.c` `raw = &resp.data[2]; rlen = resp.len - 2;` | 未校验 `resp.len >= 2`，uint8 下溢后越界解析 | 加长度守卫 |
| P2-3 | `httpd.c:10` `HTTPD_BuildJSON` | `snprintf` 截断时返回"应写入长度"，`json_len` 可能大于实际内容，`HTTPD_SendJSONResponse` 会越界读栈 | clamp 到 `buf_len-1` |
| P2-4 | `Core/Src/diskio.c` `disk_initialize()` | 只调 `HAL_SD_Init()`，未调 `HAL_SD_ConfigWideBusOperation(SDIO_BUS_WIDE_4B)` | 补 4 位宽配置，速度提升约 4 倍 |
| P2-5 | `Core/Src/sdlog.c` | `f_open` 返回值未校验；全程无 `f_close`，掉电后 FAT 可能损坏 | 加错误处理与关闭逻辑 |
| P2-6 | `Core/STM32F407ZGTx_FLASH.ld:47` | `_Min_Stack_Size = 0x400`（1KB MSP），中断里跑 HAL_CAN/HAL_UART/HAL_SD | 提到 0x800 |
| P2-7 | `CMakeLists.txt:11-12` | 硬编码 `C:/Users/Administrator/AppData/Local/stm32cube/...` | 改为环境变量或 `find_package`，换机器即可编译 |
| P2-8 | `Docs/pinmap.md`、`Docs/wiring_guide.md`、`TASKS.md:80` | 文档严重过时：仍写 USART1/PA9-PA10、CAN1 PA11/PA12、ST-Link、GPS/IMU 引脚（这些功能已 pass） | 按代码实际重写：UART4 PA0/PA1、CAN1 PB8/PB9、CMSIS-DAP |
| P2-9 | `Scripts/openocd.cfg:8` | `reset_config srst_only srst_nogate`，但 CMSIS-DAP 通常不接 nRESET 线 | 改 `reset_config none`（用 SYSRESETREQ 软复位） |

---

## P3 低（体验打磨）

| # | 位置 | 问题 |
|---|------|------|
| P3-1 | `WebUI/index.html:156-157` | 每帧 `canvas.width=window.innerWidth` 重置画布（即使尺寸未变），且无 `devicePixelRatio` 处理 → 高分屏模糊、性能损耗 |
| P3-2 | `WebUI/index.html:231` | 每个粒子 `shadowBlur=14`（最多 70 个）→ 手机端掉帧 |
| P3-3 | `WebUI/index.html:350` 起 | 0-5km/h 过渡区 `mainAlpha=0.6` + `shyAlpha=0.4` 半透明叠加，Shy 面板与主界面仍会视觉重叠 |
| P3-4 | `Core/Src/energy.c` | `last_throttle` 赋值后未参与急加速判定，评分逻辑不完整 |
| P3-5 | `Core/Src/main.c` | CAN 采样点 85.7%（见下方误报纠正） |

---

## 误报纠正（子审查结论已核实推翻）

1. **"startup 未使能 FPU 而按 hard-float 编译是定时炸弹" — 错误。**
   `Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c:152-155` 在 `SystemInit()` 中已有 `SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2))`，且 `__FPU_USED` 在 `-mfloat-abi=hard -mfpu=fpv4-sp-d16` 下自动为 1。FPU 正常使能，无需修改。

2. **"SDIO ClockDiv=0 超过 SD 卡初始化规范" — 不准确。**
   ① `HAL_SD_InitCard()` 内部会临时切到低速初始化时钟，不依赖用户配置；② STM32 SDIO 分频公式是 `SDIO_CK = PLL48CLK / (ClockDiv + 2)`，`ClockDiv=0` → 24MHz，不是 48MHz。真正的缺陷是 P2-4 未切 4 位宽。

3. **"CAN 采样点算错，应为 87.5%" — 不成立。**
   42MHz(APB1) / 500kbps = 84 个时间份额。采样点 = `(1+BS1)/TQ`。要 87.5% 需 `(1+BS1)/TQ = 7/8`，即 `TQ` 必须是 8 的倍数，而 84 的所有合法分解（Prescaler×TQ=84）都无法让 `(1+BS1)/TQ` 精确等于 7/8。当前 `Prescaler=6, BS1=11, BS2=2` → 采样点 12/14 = 85.7%，是该时钟下最接近且落在 ISO 11898 允许区间内的取值。**保持现状即可。**

---

## 建议修复顺序

1. **P0-1 / P0-2**（两条中断接线，共约 10 行代码）→ 决定 WiFi 和 CAN 能不能活
2. **P0-3 / P0-4 / P1-5**（栈 + 内嵌 HTML + 发送时序）→ 决定 HUD 页面能不能打开
3. **P1-1 / P1-2 / P1-3 / P1-4**（数据可信度 + 断线检测 + 时间）
4. **P2-8 / P2-7 / P2-9**（文档与构建链路，为烧录做准备）
5. P2 其余、P3 打磨

---

## 未覆盖项（需上板或提车后才能验证）

- CAN 实际总线负载与 PID 响应情况（需提车）
- ESP8266 固件版本对 `AT+CIPSERVER` 的支持度（需上板）
- SD 卡实际读写速度与 FatFs 挂载稳定性（需上板）
- 挡风玻璃反射后的实际可读性（需实车）
