# IMT407 车载HUD — 任务与路线图

> 本文档记录项目从零到上车的完整任务清单与当前状态。
> 项目介绍与快速开始见 [README.md](README.md)，完整开发日志见 [PROGRESS.md](PROGRESS.md)。

## 当前状态

**项目**：极狐T1 车载HUD — STM32F407 直读 OBD-II → ESP8266 WiFi → 平板浏览器渲染 → 挡风玻璃反射

| 维度 | 状态 |
|------|------|
| 固件代码（CAN 协议栈/任务/日志/HTTP） | ✅ 完成，0 错误 0 警告 |
| HUD 前端（v7.2，Shy Tech / 能量流 / 环形仪表） | ✅ 完成 |
| 静态代码审计（两轮深度修复） | ✅ 完成 |
| 上板硬件验证（烧录/SD/WiFi/CAN 回环） | ⏳ 等烧录线（CMSIS-DAP）到货 |
| 实车 OBD 联调 | ⏳ 等提车 |

**下一步动作**（烧录线到货后）：
1. `Scripts/build.ps1` 编译 → `Scripts/flash.ps1` 烧录（SWD: PA13/PA14/GND/3V3）
2. 板载 CAN1(PB8/PB9) ↔ CAN2 回环自测（500kbps + ISO 15765 收发链路）
3. 验证 UART4/ESP8266、SDIO/FatFs、WiFi/HTTP、`GET /scan` PID 扫描
4. 提车后：TJA1050 接 OBD（6=CAN-H, 14=CAN-L, 16=+12V, 4/5=GND），PID 扫描确认极狐T1 实际响应，再调 `can_obd.c`

> 自测全程按 [Docs/board_self_test.md](Docs/board_self_test.md) 执行（每步有判据和排查表）。

## 里程碑总览

| # | 阶段 | 状态 |
|---|------|------|
| 1 | 工具链安装 | ✅ |
| 2 | 项目骨架 | ✅ |
| 3 | HAL 配置 + 初始化代码 | ✅ |
| 4 | 业务逻辑模块 | ✅ |
| 5 | HUD 界面迭代（v1.0 → v7.2） | ✅ |
| 6 | 构建/烧录脚本 | ✅ |
| 7 | 文档 | ✅ |
| 8 | 编译验证 | ✅ |
| 9 | CAN 直读方案落地 | ✅ |
| 10 | 功能升级 + 界面重设计 | ✅ |
| 11 | 硬件验证 | ⏳ 等烧录线 + 提车 |
| 12 | 体验优化 | ⏳ 需实车 |
| 13 | 代码自查修复（P0~P3） | ✅ |
| 14 | 第二轮深度静态审计 | ✅ |

## 详细任务

### 阶段 1-8：基础设施 ✅

- 工具链：GCC 14.3.1 / CMake 4.3.1 / Ninja 1.13.2 / OpenOCD 0.12.0（STM32Cube bundle）
- 项目骨架：目录结构、`.gitignore`、CMakeLists、arm-toolchain.cmake、链接脚本
- HAL 配置：`stm32f4xx_hal_conf.h`（含 CAN/IWDG 模块）、`main.c`（168MHz + FreeRTOS）、MSP/中断
- 业务模块：UART 环形缓冲、共享数据 + Mutex、ESP8266 控制、HTTP 服务器、CAN 直读 OBD、能耗计算、SD 日志、任务调度
- 构建脚本：`build.ps1` 自动定位工具链、`flash.ps1` + `openocd.cfg`（CMSIS-DAP）
- 首次编译通过：50 源文件，Flash 60KB / RAM 42KB

### 阶段 9：CAN 直读方案 ✅

- ISO 15765-4 协议栈：单帧/首帧/连续帧/流控帧 + 多帧重组
- PID 扫描模式（29 个 PID 自动扫描，SD 卡记录）
- Service 01 + 国标 Service 05 双协议请求，S05 解析公式全部实现（17 个 PID）
- CAN1 初始化：500kbps，PB8/PB9（非 PA11/PA12，被 USB OTG 占用）
- 删除 ELM327 相关旧代码（obd.c），清理全部 USART2 痕迹

### 阶段 10：功能升级 + HUD 界面迭代 ✅

**功能升级**：
- 新增电池健康/温度/驾驶评分字段
- 驾驶评分体系：急加速 -2 / 急制动 -3 / 能量回收 +1 / 匀速 +0.5
- SD 卡 CSV 导出 25 字段
- 编译验证：51 源文件，Flash 63KB / RAM 41KB

**HUD 界面演进**（v1.0 → v7.2）：
- v1.0 基础仪表 → v2.x 性能版（G 值球/功率弧/指南针/电机转速）+ 字号放大
- v3.0 数据无效状态处理（断线标灰/连接指示器）+ SOC/电机温度/制动踏板
- v4.0 能量流粒子动画/电池健康面板/温度三件套/评分条/行程统计
- v4.3 易读性修复：middle baseline、行高=字号×1.6、流式 Y 坐标、自适应缩放
- v5.0 极简重设计：研究 BMW Shy Tech / 奔驰 AR-HUD / Polestar，能量流为核心，车速降为底部进度条
- v6.0 无框文字链 + 粒子能量流
- v7.x 混合视觉：功率弧 + SOC 环 + 温度环；修复弧环与文字重叠（弧半径 ×1.5、环放大、羽化遮罩、进度条避让）

### 阶段 11：硬件验证 ⏳

**等烧录线到货即可做**（烧板自测，不接车）：
- [ ] 11.1 烧录固件（SWD: PA13/PA14/GND/3V3）
- [ ] 11.2 板载 CAN1(PB8/PB9) ↔ CAN2 回环自测：500kbps + ISO 15765 收发链路
- [ ] 11.3 验证 UART4 通信（ESP8266）
- [ ] 11.4 验证 SDIO（MicroSD + FatFs）
- [ ] 11.5 验证 WiFi + HTTP + `GET /scan` PID 扫描（结果写 SD 卡）

**提车后才能做**（接 OBD）：
- [ ] 11.6 PID 扫描：上电后扫描 29 个 PID，SD 卡记录结果
- [ ] 11.7 确认极狐T1 响应哪些 Service 05 PID
- [ ] 11.8 根据扫描结果调整 `can_obd.c`（禁用无响应 PID）
- [ ] 11.9 HUD 界面实车联调
- [ ] 11.10 实测字号/布局在挡风玻璃反射后的可读性

### 阶段 12：体验优化 ⏳（需实车后）

- [ ] 12.1 日间/夜间模式切换
- [ ] 12.2 数据刷新率优化（根据实车 CAN 总线负载）
- [ ] 12.3 功率弧动态范围调整（根据极狐T1 实际功率）
- [ ] 12.4 评分参数调优（根据实际驾驶数据）

### 阶段 13：代码自查修复 ✅（详见 [Docs/self_audit_2026-09-09.md](Docs/self_audit_2026-09-09.md)）

自查结论：固件即使烧进去也拿不到真实数据——两条关键中断链路未接线（CAN 接收回调、UART4 IDLE 处理）。全部问题已修复并重新编译通过：

- **P0**：CAN 接收接线（`HAL_CAN_RxFifo0MsgPendingCallback` → `CAN_OBD_RxHandler`）；UART4 IDLE 处理；HTTP 任务栈 2048→4096；HUD 页面内嵌固件（`GET /` 返回 HTML）
- **P1**：obd_valid 超时归零；前端 mockMode 默认关 + 3 秒断线检测；timestamp 去硬编码；ESP8266 发送等待 `>`/`SEND OK`
- **P2**：PID 扫描接线；长度守卫 + JSON 截断 clamp；SDIO 4 位宽 + 文件校验；MSP 栈扩大；CMakeLists 去绝对路径；文档重写
- **P3**：canvas DPR/性能优化、粒子 shadowBlur 降载、Shy 面板重叠修复

### 阶段 14：第二轮深度静态审计 ✅（2026-09-09）

无硬件条件下能做的全部代码级修复，每批改完立即编译自查：

- **P0 CAN 数据可信性**：请求/响应配对校验（响应必须回显 SID+PID）、连续帧 SN 连续性校验、负响应 0x7F 立即失败、obd_valid 三态
- **P0 锁与阻塞 I/O 解耦**：TaskOBD/TaskSDLog 锁内只做快照 memcpy，阻塞 I/O 移出临界区；`/data` 路由锁内拷贝锁外拼 JSON
- **P0 FatFs 并发串行化**：`s_sd_mutex` 覆盖全部 FatFs 调用（FF_FS_REENTRANT=0）
- **P0 RTOS 对象失败处理**：Mutex/Thread 创建失败进安全态而非裸跑
- **P1 浮点格式化**：CMakeLists 补 `-u _printf_float`（此前 JSON 中 %.1f 输出为空）
- **P1 中断优先级 + 看门狗**：SDIO 优先级 4→5；启用 IWDG（LSI 32kHz，约 4s 超时）；OBD/Energy/HTTPD 三任务心跳，全部报活才刷新；20s 启动宽限期 + 调试冻结
- **P1 ESP8266 链路健壮性**：初始化 5 条 AT 逐条校验 + 重试；PA8 硬复位替代 AT+RST；环形缓冲溢出计数落日志；发送返回值检查

**最终编译验证**：54 编译单元，text=73,536 / data=480 / bss=41,376，0 错误 0 警告。

## 关键设计决策

| 决策 | 内容 |
|------|------|
| CAN 引脚 | CAN1 = **PB8/PB9**（本板 PA11/PA12 被 USB OTG 占用） |
| ESP8266 挂载 | UART4（PA0/PA1），AT 固件 AP 模式 |
| 数据接口 | HTTP `/data` 端点（非 data.json），轮询刷新 |
| FatFs 码页 | **932**（日文 SJIS，适配中文字符） |
| 生成文件 | `Core/Src/web_page.c` + `Core/Inc/web_page.h` 由 `gen_web_page.py` 生成，不入库 |
