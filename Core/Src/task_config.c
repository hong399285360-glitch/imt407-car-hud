#include "task_config.h"
#include "shared_data.h"
#include "wifi.h"
#include "can_obd.h"
#include "energy.h"
#include "httpd.h"
#include "sdlog.h"
#include "uart_ringbuf.h"
#include "watchdog.h"
#include <string.h>

extern UART_HandleTypeDef huart4;
extern CAN_HandleTypeDef hcan1;

/* main.c 中的致命错误处理: 关中断死循环 (IWDG 启用后由看门狗复位) */
extern void Error_Handler(void);

static UART_RingBuf_t s_esp_rxbuf;

/* 创建任务并检查返回值。
   osThreadNew 失败的唯一现实原因是 FreeRTOS heap 耗尽, 此时系统状态已不可信:
   少了 OBD 任务 HUD 永远显示 0, 少了 HTTPD 任务界面直接打不开。
   带病运行只会产生更难排查的现象, 故直接进致命态。 */
static void CreateTask(void (*entry)(void *), uint32_t stack, osPriority_t prio) {
    osThreadAttr_t attr;

    memset(&attr, 0, sizeof(attr));
    attr.stack_size = stack;
    attr.priority = prio;

    if (osThreadNew(entry, NULL, &attr) == NULL)
        Error_Handler();
}

void TaskConfig_Init(void) {
    SharedData_Init();

    UART_RingBuf_Init(&s_esp_rxbuf, &huart4);
    UART_RingBuf_StartDMA(&s_esp_rxbuf);

    /* 注意: ESP8266_Init() 不能在这里调用 —— 它内部会 osDelay(1) 等待模组应答,
       而本函数在 osKernelStart() 之前执行, 调度器还没起来, osDelay 会空指针崩溃。
       因此把 ESP8266 初始化放到 TaskHTTPD_Entry 里做。 */
    CAN_OBD_Init(&hcan1);
    Energy_Init();
    SDLog_Init();

    CreateTask(TaskOBD_Entry,    TASK_OBD_STACK,    TASK_OBD_PRIO);
    CreateTask(TaskEnergy_Entry, TASK_ENERGY_STACK, TASK_ENERGY_PRIO);
    CreateTask(TaskHTTPD_Entry,  TASK_HTTPD_STACK,  TASK_HTTPD_PRIO);
    CreateTask(TaskSDLog_Entry,  TASK_SDLOG_STACK,  TASK_SDLOG_PRIO);

    /* 放最后启动 IWDG: 前面 SDLog_Init() 里无卡时 f_mount 会等 SDIO 超时,
       若狗先跑起来, 这段启动耗时可能被误判成卡死。 */
    Watchdog_Init();
}

/* 由 UART4_IRQHandler 在检测到 IDLE 空闲线中断时调用,
   把 DMA 已收到的数据搬进环形缓冲区 (中断上下文, 不要加锁) */
void TaskConfig_OnUART4IDLE(void) {
    UART_RingBuf_OnIDLE(&s_esp_rxbuf);
}

void TaskOBD_Entry(void *arg) {
    (void)arg;
    /* 快照放 static: VehicleData_t 约 200 字节, 本任务栈只有 1KB,
       放栈上会挤掉 CAN 解析余量。该变量仅本任务访问, 无需加锁。 */
    static VehicleData_t s_obd_snap;

    for (;;) {
        Watchdog_Heartbeat(WD_TASK_OBD);

        /* 1) 锁内只做一次 memcpy(微秒级)。
           原写法在锁内直接跑 CAN_OBD_Update(), 而它内部是 15 个 PID 的
           串行请求-等待, 最坏约 1.5s —— 期间 HTTPD 的 /data、SDLog 取数、
           Energy 统计全部被堵死, 浏览器侧表现为界面卡住不刷新。 */
        SharedData_Lock();
        memcpy(&s_obd_snap, &g_vehicle_data, sizeof(s_obd_snap));
        SharedData_Unlock();

        /* 2) 锁外做长耗时 CAN 请求 */
        CAN_OBD_Update(&s_obd_snap);

        /* 3) 锁内只回写 OBD 拥有的字段 */
        SharedData_Lock();
        SharedData_MergeOBD(&g_vehicle_data, &s_obd_snap);
        SharedData_Unlock();

        osDelay(100);
    }
}

void TaskEnergy_Entry(void *arg) {
    (void)arg;
    for (;;) {
        Watchdog_Heartbeat(WD_TASK_ENERGY);

        SharedData_Lock();
        float v = g_vehicle_data.voltage;
        float i = g_vehicle_data.current;
        float spd = g_vehicle_data.speed;
        SharedData_Unlock();

        Energy_Update(v, i, spd);

        SharedData_Lock();
        Energy_GetStats(&g_vehicle_data);
        SharedData_Unlock();

        osDelay(100);
    }
}

void TaskHTTPD_Entry(void *arg) {
    (void)arg;
    /* ESP8266 初始化放这里: 需要 osDelay 等待模组应答, 必须等调度器启动后执行 */
    ESP8266_Init(&huart4, &s_esp_rxbuf);
    for (;;) {
        /* 首轮心跳要等 ESP8266_Init 跑完才会到, 模组没接好时可能几秒,
           watchdog 的 20s 启动宽限期已覆盖这种情况。 */
        Watchdog_Heartbeat(WD_TASK_HTTPD);

        ESP8266_Poll();
        osDelay(10);
    }
}

void TaskSDLog_Entry(void *arg) {
    (void)arg;
    static VehicleData_t s_sdlog_snap;   /* 同上: 避免占用 1KB 任务栈 */

    for (;;) {
        /* 锁内取快照, 锁外做阻塞式 SD 写。
           原写法在锁内写 SD: f_write 单条记录虽短, 但卡或格式化时
           可能阻塞数百毫秒, 期间整个共享数据区被锁死。 */
        SharedData_Lock();
        memcpy(&s_sdlog_snap, &g_vehicle_data, sizeof(s_sdlog_snap));
        SharedData_Unlock();

        SDLog_Write(&s_sdlog_snap);

        osDelay(1000);
    }
}
