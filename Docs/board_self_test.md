# 上板自测执行清单

> 烧录线(CMSIS-DAP + SWD 杜邦线)一到，照此清单逐步执行，每步有明确判据。
> 配套文档: [build_flash.md](./build_flash.md) 烧录 | [wiring_guide.md](./wiring_guide.md) 接线 | [pinmap.md](./pinmap.md) 引脚
> 最后更新: 2026-09-09 (阶段14 固件)

## 0. 物料清单（动手前清点）

| 物料 | 用途 | 备注 |
|------|------|------|
| 启明欣欣 F407 轻奢版 V3.1 | 主控 | 已确认引脚 PB8/PB9 CAN1、PA0/PA1 UART4 |
| CMSIS-DAP 仿真器 | 烧录/调试 | 本板标配，驱动 `stmcdc.inf` |
| SWD 杜邦线 ×4 | 烧录 | SWDIO(PA13)/SWCLK(PA14)/3V3/GND |
| ESP8266 ESP-01S | WiFi | 外接，需 AMS1117-3.3 独立供电（峰值 300mA） |
| TJA1050 模块 | CAN 收发器 | 外接 5V 供电（实车/回环自测用） |
| MicroSD 卡 | 日志 | 板载卡槽，FAT32 |
| USB 线（数据线） | 供电 | 板上 Mini USB 或 DC 6-24V 均可 |
| 杜邦线若干 | 接线 | ESP8266/TJA1050 接线用 |
| 万用表 | 通电前检查 | VCC/GND 不短路 |

> **观测手段提醒**：本固件**没有驱动板载 LED（PG13/14/15），也没有空闲 TTL 串口**（UART4 被 ESP8266 占用）。
> 固件"活没活着"只能通过 ①WiFi 热点出现 ②HTTP 页面/JSON ③SD 卡日志文件 三种方式观测，见各阶段判据。

---

## 阶段A: 编译 + 烧录（不需要任何外设，只要板子 + DAP）

### A1. 编译
```powershell
cd d:\HONG\Doc\TRAE\imt407-car-hud
.\Scripts\build.ps1
```
- **通过判据**：输出 `=== Build Complete ===`，size 行显示 **54 编译单元，text=73536 / 0 错误 0 警告**
- **失败排查**：
  - 找不到 gcc/cmake → `%LOCALAPPDATA%\stm32cube\bundles` 下工具链被删
  - 改了 `WebUI/index.html` 没重新生成 → 先跑 `python Scripts/gen_web_page.py` 再编译

### A2. 烧录
```powershell
.\Scripts\flash.ps1
```
- **接线**：DAP SWDIO→PA13、SWCLK→PA14、3V3→3V3、GND→GND（共地必须接）
- **通过判据**：输出依次出现 `Programming Started → Programming Finished → Verify OK → Reset → shutdown command invoked`，最后 `=== Flash Complete ===`（绿色）
- **失败排查**（按顺序）：
  1. `Error: open failed` / `could not connect` → 四根线是否接对、GND 是否共地、DAP 驱动是否装了 `stmcdc.inf`
  2. `Error: init mode failed` → 把 `openocd.cfg` 里 `adapter speed 4000` 改成 `1000`
  3. `Error: verification failed` → 芯片被锁/供电不稳，断电重插 USB 再试
  4. 板子完全无反应 → 确认 USB 是**数据线**不是充电线（此前 DFU 失败的教训：J3 口用充电线会识别为 VID_0000）

### A3. 烧录后确认固件在跑
烧录完 `reset` 已自动复位运行。此时**没有 LED 可看**，直接进入阶段B/C 用外设观测。

---

## 阶段B: SD 卡 + 任务调度验证（只加 SD 卡，不接 ESP8266）

> 目的：验证 SDIO/FatFs 挂载、四个任务调度、SDLog 写文件。
> 这步在 ESP8266 未接时做，日志链路是独立的，不受 WiFi 影响。

### B1. 操作
1. MicroSD 卡（FAT32）插入板载卡槽
2. 上电
3. 等 10 秒（含 IWDG 20s 宽限内 SDLog_Init 挂载耗时），断电拔卡
4. 电脑上读 SD 卡

### B2. 通过判据
- 卡里出现 **`hud_log.csv`**，且**有内容**（首行是表头，之后每 1 秒一行数据，`ts` 是上电秒数）
- `obd_scan.log` 可不存在（只有 /scan 或 ESP 报错才写）
- 数据行里 `voltage/current/power` 都是 0、`obd_valid` 是 0 → **正常**（没接 CAN，无响应，预期如此）

### B3. 失败排查
| 现象 | 原因 | 处理 |
|------|------|------|
| 卡里没有 `hud_log.csv` | SD 卡不是 FAT32 / 卡槽接触不良 | 换卡、重新插紧 |
| 有文件但只有表头、没数据行 | SDLog_Write 没跑（任务没起来？） | 进阶段C 看 HTTPD 是否正常；若 HTTPD 正常而 SDLog 不写 → 用 SWD 调试看 `s_file_ok` |
| 文件乱码/损坏 | 之前没正常卸载 | 格式化 FAT32 重试 |

---

## 阶段C: ESP8266 + WiFi + HTTP 验证（核心，需要 ESP8266）

### C1. 接线（照 wiring_guide.md 第二节）
| ESP-01S | F407 | 说明 |
|---------|------|------|
| VCC | AMS1117-3.3 输出 | **不要直接吃 F407 板载 3.3V**（峰值 300mA 可能带不动） |
| GND | GND | 与 F407 共地 |
| TX | PA1 (UART4 RX) | 交叉 |
| RX | PA0 (UART4 TX) | 交叉 |
| EN/CH_PD | 3.3V | 必须拉高 |
| RST | PA8 | 固件会硬复位它 |
| GPIO0 / GPIO2 | 3.3V | 运行模式 |

### C2. 操作与判据
1. **上电，等 8~10 秒**（PA8 硬复位 + 5 条 AT 初始化，含 1.5s 等待）
2. 手机/电脑搜 WiFi → **出现热点 `F407-HUD`** → 密码 `12345678` 连接
   - **通过**：热点出现 = UART4 收发 + ESP8266 供电 + AT 初始化 5 步全过
   - **失败排查**（这是最容易卡的一步）：
     | 现象 | 原因 | 处理 |
     |------|------|------|
     | 搜不到热点 | ESP 没供电/供电不足 | 检查 AMS1117、EN 是否 3.3V |
     | 搜不到热点 | 接线 RX/TX 接反 | 确认 TX→PA1、RX→PA0 交叉 |
     | 搜不到热点 | 波特率不对 | ESP-01S 出厂 115200，确认固件是原厂 AT 固件 |
     | 搜不到热点 | 初始化失败 | 拔 SD 卡读 `obd_scan.log`，应有 `[ERROR] ESP8266 init failed at step N`，N 指明卡在哪条 AT |
     | 热点出现但连不上 | 密码错/距离近 | 重输 `12345678`，天线附近再试 |
3. **浏览器访问 `http://192.168.4.1`**（ESP8266 AP 模式默认网关）
   - **通过**：HUD 页面渲染出来（内嵌 16.5KB HTML，由固件下发）
   - **失败排查**：
     | 现象 | 原因 | 处理 |
     |------|------|------|
     | 页面白屏/打不开 | 连的是 AP 但没拿到 IP | 等 DHCP 分配（通常 192.168.4.2），或手动设平板 IP 192.168.4.2/24 |
     | 页面加载一半 | HTML 发送分块中断 | SD 日志应有 `[WARN] HTML body send failed`；重试刷新 |
     | 一直转圈 | ESP 固件 AT 版本旧 | 需要支持 CIPSEND 带 `>` 提示的新 AT 固件（`SEND OK` 应答） |
4. **访问 `http://192.168.4.1/data`**（HUD 页面的数据端点）
   - **通过**：返回 JSON，含 `speed/voltage/current/power/soc/obd_valid` 等字段，`obd_valid` 为 **0**（没接 CAN，预期无数据）
   - 页面连接指示应显示"已连接"，数据区显示 **NO DATA / 灰色**（mockMode 默认 false，不会伪造数据）
   - **失败排查**：返回 404 → 确认路径是 `/data` 不是 `data.json`（固件只认 `/data`）

---

## 阶段D: `GET /scan` 链路验证（不接车也能做，预期全 FAIL 也算通过）

> 目的：验证 HTTP 路由 + CAN 请求函数 + SD 日志写入三件事。不接车时 29 个 PID 全部无响应，
> 但**每条 `[FAIL]` 都写进 SD 卡**，这本身就是"链路通了"的证据。

### D1. 操作
1. 浏览器访问 `http://192.168.4.1/scan`
2. 页面**立即**显示 `PID scan started. Result -> SD:/obd_scan.log`（响应先发，扫描后跑）
3. 之后约 6 秒（29 PID × 200ms）内**页面/新请求不响应**——HTTPD 任务被扫描阻塞，属设计行为，不要刷新
4. 断电拔 SD 卡，读 `obd_scan.log`

### D2. 通过判据
- 日志包含 `=== PID Scan Start ===` 和 `=== PID Scan Complete ===` 两行
- 中间 29 行，**不接车时全部是 `[FAIL] ... no response`** → 通过！
  （每行都证明：HTTP 路由通 → CAN_OBD_ScanPIDs 执行 → FatFs 写入正常）
- 有任何一行 `[OK]` 都说明有 CAN 节点响应（板载回环/干扰），值得记录

### D3. 失败排查
| 现象 | 原因 | 处理 |
|------|------|------|
| 日志文件不存在 | SD 未挂载 / /scan 没触发 | 先过阶段B 确认 SD 正常 |
| 只有 Start 没有 Complete | 扫描中途卡死（CAN 请求锁未释放） | SWD 调试看 `s_can_req_mutex` |
| 页面转圈不返回 | 请求落在扫描阻塞窗口内 | 等扫描结束（约 6s）后重新访问即可 |

---

## 阶段E: CAN1 ↔ CAN2 回环自测 ⚠️ 当前不可执行

> **现状**：固件只初始化了 CAN1（main.c 只有 `MX_CAN1_Init`），**没有 CAN2 支持**。
> 板上 CAN2 绿色端子（PB12 RX / PB13 TX，自带收发器）存在，但直接接线没有响应方，测不出任何东西。
> 这一步当前**跑不通**，两个出路：
> 1. **加自测固件**（推荐，烧录线没到正好有时间）：条件编译 `CAN_SELFTEST` 模式——初始化 CAN2，
>    收到 0x7DF 请求帧自动回 0x7E8 模拟响应 → CAN1 走完整 ISO 15765 栈拿到"假数据" → 全链路验证。
>    实现后本阶段可直接执行。
> 2. **跳过，等实车**：直接接 OBD 测真 ECU（阶段F）。
>
> 若自测固件已就绪，本阶段操作：
> 1. CAN1 绿色端子 H ↔ CAN2 绿色端子 H，L ↔ L（杜邦线两根）
> 2. 编译烧录自测固件，上电
> 3. 访问 `/data` → `obd_valid` 应为 **1**，`speed/soc/voltage` 出现模拟值
> 4. 判据：模拟数据持续刷新 = 请求/响应配对 + 多帧重组 + 解析全部通过
> 5. 失败排查：`obd_valid` 仍为 0 → 检查两根 H/L 杜邦线、CAN 总线终端电阻（两端各 120Ω，确认板载是否已带）

---

## 阶段F: 实车 OBD 验证（提车后，最终目标）

1. TJA1050 接 OBD-II：6=CAN-H（蓝）、14=CAN-L（黄）；TJA1050 的 VCC 接**板子 5V**（不取 OBD 的 +12V）
   - 注意：OBD 16 脚 +12V **不接**（wiring_guide: 不为 HUD 供电），只借 6/14 两根信号线
2. 上电 → 访问 `/data` → `obd_valid` 变 **1**、速度/SOC/电压有真实值 → HUD 主链路通
3. 访问 `/scan` → 拔卡看哪些 PID `[OK]` → 对照 [OBD_PID_TABLE.md](../OBD_PID_TABLE.md) 调整 can_obd.c（禁用无响应 PID）
4. 按 TASKS.md 阶段11.9~11.10 做实车联调

---

## 附录: 速查

| 项 | 值 |
|----|-----|
| WiFi AP | `F407-HUD` / `12345678`（网关 192.168.4.1） |
| 数据端点 | `http://192.168.4.1/data` |
| 页面 | `http://192.168.4.1/` (内嵌 HTML) |
| 扫描 | `http://192.168.4.1/scan` (阻塞 ~6s) |
| SD 主日志 | `hud_log.csv` (1s/行) |
| SD 文本日志 | `obd_scan.log` (扫描结果 + ESP/HTTP 告警) |
| 无车预期 | `/data` 的 `obd_valid=0`，页面 NO DATA |
| 烧录 | `Scripts\build.ps1` → `Scripts\flash.ps1` |
| 改 WebUI 后 | `python Scripts/gen_web_page.py` → 重新编译 |
| IWDG | 20s 启动宽限，之后需三任务心跳，否则 4s 复位 |
