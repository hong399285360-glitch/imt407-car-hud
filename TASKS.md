# IMT407 车载HUD — 任务清单

> 每完成一条就勾选。上下文压缩后从这里恢复进度。
> 最后更新: 2026-09-09

## ⚡ 当前状态速览 (压缩后先读这里)

**项目**: 极狐T1 车载HUD — STM32F407ZGT6 + ESP8266 WiFi AP → 平板浏览器 → 挡风玻璃反射
**数据源**: CAN 直读 OBD-II (ISO 15765, Service 01 + 国标 Service 05), 已彻底去掉 ELM327; GPS/IMU 已 pass
**当前固件**: 阶段14 静态审计修复完成, 54 编译单元, text=73536 / data=480 / bss=41376, **0 错误 0 警告**
**代码状态**: P0/P1 修复全部落地 (CAN 配对校验/三态 obd_valid、锁与阻塞 I/O 解耦、FatFs 串行化、RTOS 对象失败处理、-u _printf_float、SDIO 优先级 5、IWDG 心跳兜底、ESP8266 初始化校验/硬复位/环缓告警/发送返回值检查)
**硬件状态**: 烧录线(CMSIS-DAP + SWD 杜邦线)未到货 → 阶段11 全部阻塞中; 实车未提
**下一步动作** (烧录线到货后):
1. `Scripts/build.ps1` 编译 → `Scripts/flash.ps1` 烧录 (SWD: PA13/PA14/GND/3V3)
2. 板载 CAN1(PB8/PB9) ↔ CAN2 回环自测 (500kbps + ISO 15765 收发链路)
3. 验证 UART4/ESP8266、SDIO/FatFs、WiFi/HTTP、`GET /scan` PID 扫描
4. 提车后: TJA1050 接 OBD (6=CAN-H, 14=CAN-L, 16=+12V, 4/5=GND), PID 扫描确认极狐T1 实际响应, 再调 can_obd.c

**关键文档**: [PROGRESS.md](./PROGRESS.md) 完整日志 | [Docs/pinmap.md](./Docs/pinmap.md) 引脚 | [Docs/board_reference.md](./Docs/board_reference.md) 板子资料 | [Docs/wiring_guide.md](./Docs/wiring_guide.md) 接线 | [Docs/self_audit_2026-09-09.md](./Docs/self_audit_2026-09-09.md) 阶段13 自查 | [OBD_PID_TABLE.md](./OBD_PID_TABLE.md) PID 表
**重要修正**: CAN1 引脚=**PB8/PB9** (非 PA11/PA12, 被 USB OTG 占用); FatFs 码页=**932**; `/data` 用 **'/data' 端点** (非 data.json)

**GitHub 同步** (完成): 仓库 `hong399285360-glitch/imt407-car-hud`(公开, main 分支)。本机无 git, 通过 GitHub App connector 的 push_files 分批推送: README+构建配置 / 文档 / Scripts / Core 固件。不推: Drivers/(38MB ST HAL/FatFs 第三方库)、build/、生成物 Core/Src/web_page.c + Core/Inc/web_page.h(README 已注明用 gen_web_page.py 生成)。

---

## 阶段1: 工具链安装 ✅
- [x] 1.1 安装 VS Code + STM32CubeIDE扩展包 v3.10.0
- [x] 1.2 安装 GNU Arm Embedded Toolchain v14.3.1 (STM32 bundle)
- [x] 1.3 安装 CMake v4.3.1 + Ninja v1.13.2 (STM32 bundle)
- [x] 1.4 安装 OpenOCD v0.12.0 + GDB v14.3.1
- [x] 1.5 获取 STM32F4 HAL库源码 (从Gitee镜像)
- [x] 1.6 获取 FreeRTOS源码 (从STM32Cube pack)
- [x] 1.7 获取 FatFs R0.16 (从elm-chan.org官网)
- [x] 1.8 工具路径:
  - GCC/Ninja/CMake/GDB: `C:\Users\Administrator\AppData\Local\stm32cube\bundles\`
  - OpenOCD: WinGet xpack v0.12.0

## 阶段2: 项目骨架 ✅
- [x] 2.1 创建目录结构 (Core/Inc, Core/Src, WebUI, Scripts, Drivers, Docs)
- [x] 2.2 创建 .gitignore
- [x] 2.3 TASKS.md + PROGRESS.md 就位

## 阶段3: HAL配置 + 初始化代码 ✅
- [x] 3.1 编写 stm32f4xx_hal_conf.h (HAL配置, 含CAN模块启用)
- [x] 3.2 编写 main.c (时钟配置168MHz + 外设初始化 + FreeRTOS启动)
- [x] 3.3 编写 stm32f4xx_hal_msp.c (引脚/外设MSP配置, 含CAN1 MSP)
- [x] 3.4 编写 stm32f4xx_it.c/h (中断处理: SysTick/SVC/PendSV/CAN1_RX0)
- [x] 3.5 编写 FreeRTOSConfig.h + cmsis_os2.h + cmsis_os2.c (RTOS适配层)
- [x] 3.6 编写 CMakeLists.txt + startup_stm32f407xx.s + 链接脚本
- [x] 3.7 编写 syscalls.c (newlib-nano系统调用存根)
- [x] 3.8 编写 diskio.c (FatFs SDIO磁盘IO对接)

## 阶段4: 业务逻辑模块 ✅
- [x] 4.1 uart_ringbuf.c/h — UART环形缓冲区 + DMA空闲中断
- [x] 4.2 shared_data.h/c — 共享数据结构 + Mutex (含新增字段)
- [x] 4.3 wifi.c/h — ESP8266 AT命令控制
- [x] 4.4 httpd.c/h — HTTP服务器 + JSON打包 (含新增字段)
- [x] 4.5 can_obd.c/h — CAN直读OBD (替代旧obd.c/h) ✨重写
- [x] 4.6 gps.c/h — NMEA解析
- [x] 4.7 imu.c/h — MPU6050读取
- [x] 4.8 energy.c/h — 能耗计算 (修复正向计算)
- [x] 4.9 sdlog.c/h — SD卡日志 (新增SDLog_Log函数)
- [x] 4.10 task_config.c/h — FreeRTOS任务调度

## 阶段5: HTML5 HUD仪表盘 ✅
- [x] 5.1 WebUI/index.html v1.0 基础版
- [x] 5.2 HUD v2.0 性能版: G值球/功率弧/指南针/电机转速
- [x] 5.3 HUD v2.1 字号放大版: 驾驶易读性优化
- [x] 5.4 HUD v3.0 数据无效状态处理: 断线标灰/连接状态指示器
- [x] 5.5 HUD v3.0 新增显示项: SOC/电机温度/制动踏板
- [x] 5.6 HUD mockData对齐新JSON字段 (含obd_valid/模拟断连)
- [x] 5.7 本地服务器预览 (python http.server @8080)

## 阶段6: 构建脚本 ✅
- [x] 6.1 CMakeLists.txt + arm-toolchain.cmake
- [x] 6.2 Scripts/build.ps1 (自动定位STM32 bundle工具链)
- [x] 6.3 Scripts/flash.ps1 + openocd.cfg

## 阶段7: 文档 ✅
- [x] 7.1 Docs/wiring_guide.md (更新: TJA1050替代ELM327)
- [x] 7.2 Docs/build_flash.md
- [x] 7.3 Docs/pinmap.md (更新: CAN1引脚/CAN波特率计算)

## 阶段8: 编译验证 ✅
- [x] 8.1 首次编译通过 (50个源文件, Flash 60KB, RAM 42KB)
- [x] 8.2 修复FatFs集成/UART DMA/FreeRTOS中断/SDIO驱动/系统调用/链接顺序
- [x] 8.3 生成 .elf/.bin/.hex
- [x] 8.4 CAN直读方案编译验证 (51个源文件, Flash 62KB, RAM 41KB)

---

## 阶段9: CAN直读方案 — 代码完成 ✅
> ELM327 AT命令 → CAN直读 + ISO 15765协议栈 + 国标Service 05

- [x] 9.1 can_obd.c/h: ISO 15765-4协议栈 (单帧/首帧/连续帧/流控帧)
- [x] 9.2 ISO 15765-2多帧重组逻辑
- [x] 9.3 PID扫描模式 (29个PID自动扫描, SD卡记录)
- [x] 9.4 Service 01 + Service 05双协议PID请求
- [x] 9.5 国标S05 PID解析公式全部实现 (17个PID)
- [x] 9.6 main.c/hal_msp.c/it.c: CAN1初始化 (500kbps, PB8/PB9, TJA1050) ※引脚已于自查后修正
- [x] 9.7 energy.c修复: V×I正向计算 + HAL_GetTick真实时间间隔
- [x] 9.8 shared_data.h: 新增motor_temp/igbt_temp/cell_vmax/cell_vmin/brake
- [x] 9.9 httpd.c: JSON新增motor_temp/soc/brake/obd_valid
- [x] 9.10 HUD v3.0: 数据无效状态处理 + 新增显示项 + mockData对齐
- [x] 9.11 删除旧obd.c/h, 清理所有ELM327/USART2痕迹
- [x] 9.12 文档更新: wiring_guide.md + pinmap.md

## 阶段10: 功能升级 — 5大新功能 ✅
> 去掉GPS/IMU, 纯OBD数据驱动的5大新功能

- [x] 10.1 shared_data.h: 新增电池健康/温度/评分字段
- [x] 10.2 can_obd.c: 新增IGBT/制动/单体电压/电池温度PID请求
- [x] 10.3 energy.c: 驾驶评分体系 (急加速-2/急制动-3/回收+1/匀速+0.5)
- [x] 10.4 sdlog.c: CSV 25字段导出
- [x] 10.5 httpd.c: JSON新增全部新字段
- [x] 10.6 HUD v4.0: 能量流粒子动画/电池健康面板/温度三件套/评分+评分条/行程统计
- [x] 10.7 编译验证通过 (51源文件, Flash 63KB, RAM 41KB)
- [x] 10.8 TASKS.md + PROGRESS.md 更新

## 阶段10.5: HUD v5.0 极简重设计 ✅
> 基于行业HUD标准研究, 彻底重构设计理念

- [x] 10.5.1 研究行业HUD案例 (BMW Shy Tech / 奔驰AR-HUD / Polestar)
- [x] 10.5.2 核心改为能量流可视化 (功率大字+粒子动画)
- [x] 10.5.3 车速降为底部进度条 (不抢视觉中心)
- [x] 10.5.4 SOC环形图标+温度角落图标
- [x] 10.5.5 Shy Tech状态机 (行驶/蠕行/停车3态)
- [x] 10.5.6 颜色减到4种语义 (白/青/绿/红)
- [x] 10.5.7 PROGRESS.md + TASKS.md 更新

## 阶段10.4: HUD易读性修复 v4.3 ✅
> v4.0 → v4.3, 解决所有文字重叠问题

- [x] 10.5.1 所有文字改用middle baseline (不再依赖top baseline猜高度)
- [x] 10.5.2 行高=字号×1.6, 相邻行间隙=字号×0.6
- [x] 10.5.3 面板紧凑布局: 标签数字同行左右对齐 (温度6行→4行, 电池5行→3行)
- [x] 10.5.4 自适应缩放: 先算总高度, 超出屏幕按比例缩小base
- [x] 10.5.5 流式Y坐标: 从上到下累加, 不再用h×百分比
- [x] 10.5.6 车轮节点去掉重复速度, 改双圈图形
- [x] 10.5.7 PROGRESS.md 更新

## 阶段10.6: HUD v5.0~v7.2 设计演进 ✅
> 从信息看板 → Shy Tech极简 → 弧+环+大字混合视觉

- [x] 10.6.1 v5.0 极简重设计 (行业HUD标准研究, Shy Tech状态机)
- [x] 10.6.2 v6.0 完全重做 (无框文字链, 粒子能量流)
- [x] 10.6.3 v7.0 无框设计 (废除所有方框, 超大字填满屏幕)
- [x] 10.6.4 v7.1 混合视觉 (功率弧+SOC环+温度环, 动态参数用环/框体现)
- [x] 10.6.5 v7.2 修复弧环与文字重叠:
  - 功率弧半径从字号×0.62→×1.5, 文字完全在弧内
  - SOC/温度环放大 (0.13H→0.17H / 0.105H→0.14H)
  - 环内文字用alphabetic baseline手动居中
  - 粒子椭圆形羽化遮罩 (径向渐变0.7→0.5→0)
  - 车速进度条两端避让SOC/温度环
- [x] 10.6.6 编译验证通过 (text=63196, bss=40872)
- [x] 10.6.7 TASKS.md + PROGRESS.md 更新

## 阶段11: 硬件验证 ⬜ (11.1~11.5 等烧录线; 11.6~11.10 需提车)
> 烧录线(CMSIS-DAP + SWD 杜邦线)未到货, 全部阻塞中。到货后**照 [Docs/board_self_test.md](./Docs/board_self_test.md) 执行**（每步有判据和排查表）。

### 等烧录线到货即可做 (烧板自测, 不接车)
- [ ] 11.1 烧录固件到STM32F407开发板 (SWD: PA13/PA14/GND/3V3, `Scripts/build.ps1` → `Scripts/flash.ps1`)
- [ ] 11.2 板载 CAN1(PB8/PB9) ↔ CAN2 回环自测: 500kbps + ISO 15765 收发链路 (先不接车)
- [ ] 11.3 验证UART通信 (ESP8266, UART4 PA0/PA1)
- [ ] 11.4 验证SDIO通信 (MicroSD + FatFs)
- [ ] 11.5 验证WiFi + HTTP服务器 + `GET /scan` PID 扫描 (结果写 SD 卡)

### 提车后才能做 (接 OBD)
- [ ] 11.6 **PID扫描**: 上电后扫描29个PID, SD卡记录结果 (TJA1050: 6=CAN-H, 14=CAN-L, 16=+12V, 4/5=GND)
- [ ] 11.7 确认极狐T1响应哪些Service 05 PID
- [ ] 11.8 根据扫描结果调整can_obd.c (禁用无响应PID)
- [ ] 11.9 HUD界面实车联调
- [ ] 11.10 实测字号/布局在挡风玻璃反射后的可读性

## 阶段12: 体验优化 ⬜ (需实车后)
- [ ] 12.1 日间/夜间模式切换
- [ ] 12.2 数据刷新率优化 (根据实车CAN总线负载)
- [ ] 12.3 功率弧动态范围调整 (根据极狐T1实际功率)
- [ ] 12.4 评分参数调优 (根据实际驾驶数据)

## 阶段13: 代码自查修复 ✅ (2026-09-09 自查产出)
> 详见 Docs/self_audit_2026-09-09.md
> 结论: 固件即使烧进去也拿不到真实数据 —— 两条关键中断链路未接线
> 全部代码类问题(P0/P1/P2/P3)已修复并重新编译通过, 0 错误 0 警告

- [x] 13.1 [P0] CAN接收接线: 补 HAL_CAN_RxFifo0MsgPendingCallback → CAN_OBD_RxHandler
- [x] 13.2 [P0] UART4 IDLE处理: UART4_IRQHandler 中调用 UART_RingBuf_OnIDLE
- [x] 13.3 [P0] HTTP任务栈 2048→4096 (或 full[1024] 改分两次发送)
- [x] 13.4 [P0] HUD页面内嵌固件: index.html → C常量, GET / 返回HTML
- [x] 13.5 [P1] obd_valid 超时归零
- [x] 13.6 [P1] 前端 mockMode 默认关 + 断线检测改为"3秒内收到有效数据"
- [x] 13.7 [P1] timestamp 去掉硬编码 (SNTP对时或标注为运行时长)
- [x] 13.8 [P1] ESP8266_SendData 改为等待 '>' / 'SEND OK'
- [x] 13.9 [P2] CAN_OBD_ScanPIDs 接线 (启动扫描或HTTP触发)
- [x] 13.10 [P2] resp.len>=2 长度守卫 + JSON snprintf 截断 clamp
- [x] 13.11 [P2] SDIO 切4位宽 + sdlog f_open校验/f_close
- [x] 13.12 [P2] MSP栈 0x400→0x800
- [x] 13.13 [P2] CMakeLists 去掉硬编码绝对路径
- [x] 13.14 [P2] 重写 pinmap.md / wiring_guide.md (UART4 PA0/PA1, CAN1 PB8/PB9, CMSIS-DAP)
- [x] 13.15 [P2] openocd.cfg reset_config 改为 none
- [x] 13.16 [P3] 前端 canvas 尺寸/DPR 优化 + 粒子 shadowBlur 降载 + Shy面板过渡区重叠
- [x] 13.17 修复后重新编译验证 (52单元, text=62672, data=104, bss=41024, 0错误0警告)

## 阶段14: 第二轮深度 debug — 静态审计修复 ✅ (2026-09-09)
> 无烧录线/无实车条件下能做的全部代码级修复, 每批改完立即编译自查
> 最终: 54 编译单元, text=73536, data=480, bss=41376, dec=115392, ELF=508656, 0 错误 0 警告

### 14.1 [P0] CAN 数据可信性 (p0-can)
- [x] 请求/响应配对校验: 响应必须回显本次请求的 SID+PID, 不匹配则丢弃重等
  (否则上一轮迟到的响应会被当成本轮结果, 数值错乱)
- [x] 连续帧 SN 连续性校验: SN 不连续立即丢弃, 防字节错位拼出乱码
- [x] 负响应 0x7F 立即失败, 不白等满 100ms
- [x] obd_valid 改三态: 本轮有有效响应→1 / 连续 3 轮全失败→0 / 未达阈值→保持

### 14.2 [P0] 锁与阻塞 I/O 解耦 (p0-lock)
- [x] TaskOBD: 锁内只 memcpy 快照, CAN 15 个 PID 串行请求(~1.5s)移到锁外, 回写走 SharedData_MergeOBD
- [x] TaskSDLog: 锁内取快照, 阻塞式 f_write 移到锁外
- [x] SharedData_MergeOBD 只覆盖 OBD 自有字段, 不再把 Energy 任务刚写的值冲回旧快照
- [x] /data 路由: 锁内 memcpy + 补 timestamp, 锁外拼 JSON

### 14.3 [P0] FatFs 并发串行化 (p0-fs)
- [x] sdlog.c 新增 s_sd_mutex, 覆盖全部 FatFs 调用 (FF_FS_REENTRANT=0, FatFs 自身无互斥)
- [x] SDLog_Write / SDLog_Sync / SDLog_Log 全部加锁; 锁建不出来则不挂载

### 14.4 [P0] RTOS 对象创建失败处理 (p0-mutex)
- [x] osMutexNew / osThreadNew 返回值检查, 失败进安全态 (Lock/Unlock 跳过 NULL) 而非裸跑

### 14.5 [P1] newlib-nano 浮点格式化 (p1-build)
- [x] CMakeLists 补 `-u _printf_float`; nm 确认 _dtoa_r 进镜像 (此前 %.1f 输出空)
- [x] syscalls.c `_write` 改空实现 (去掉悬空 huart1 引用)

### 14.6 [P1] 中断优先级 + 看门狗兜底 (p1-irq)
- [x] SDIO_IRQn 优先级 4→5 (5 即 configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 4 违规)
- [x] 启用 IWDG: HAL_IWDG_MODULE_ENABLED + stm32f4xx_hal_conf.h 补 #include + CMake 加源文件
- [x] 新建 watchdog.c/h: OBD/Energy/HTTPD 三任务心跳, 全部报活才 Refresh
- [x] 20s 启动宽限期 + __HAL_DBGMCU_FREEZE_IWDG; SDLog 任务故意不埋心跳 (SD 阻塞属正常)
- [x] nm 确认 HAL_IWDG_Refresh / Watchdog_Init / TaskWatchdog_Entry 进镜像

### 14.7 [P1] ESP8266 链路健壮性 (p1-esp)
- [x] 初始化逐条校验: 5 条 AT 命令各自等应答 + 重试, 失败置 ESP_ERROR 并记 init_fail_step (落 SD 日志)
- [x] PA8 硬复位替代 AT+RST (模组固件卡死时软复位无效)
- [x] 环形缓冲 overflow 计数: 留一空槽判满, 满则丢新字节并累计; ESP8266_Poll 发现新增溢出写 SD 日志
- [x] HTTP 各响应路径检查发送返回值: /data 缓冲装不下主动关连接, HTML/scan/JSON 失败记日志
- [x] ESP8266_SendData/SendJSON 返回 int8_t (0/-1), 不再静默失败

### 14.8 编译验证 ✅
- [x] 54 编译单元, text=73536, data=480, bss=41376, dec=115392, ELF=508656, 0 错误 0 警告
- [x] nm 确认 UART_RingBuf_Overflow / RB_Push / ESP_InitStep / ESP8266_Init/Poll 进镜像
- [x] 修正文档不实记录: PROGRESS.md obd_valid 描述、FatFs 码页 936→932、board_reference.md CMakeLists 行、OBD_PID_TABLE.md CAN 引脚 PA11/PA12→PB8/PB9
