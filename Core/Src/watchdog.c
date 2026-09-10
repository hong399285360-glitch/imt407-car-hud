/**
  ******************************************************************************
  * @file   watchdog.c
  * @brief  独立看门狗 + 任务心跳监视
  ******************************************************************************
  */

#include "watchdog.h"
#include "cmsis_os2.h"
#include "stm32f4xx_hal.h"
#include <string.h>

/* IWDG 超时: LSI 标称 32kHz, 分频 32 -> 1000 计数/s, 重装载 4000 -> 约 4s。
   LSI 实际落在 17~47kHz(见 F407 数据手册), 所以真实超时约 2.7~9.4s。
   故意取得宽松: 正常任务周期 100ms, 3s 窗口已经有 30 倍余量。 */
#define WD_IWDG_PRESCALER   IWDG_PRESCALER_32
#define WD_IWDG_RELOAD      4000

/* 看门狗任务自身检查周期 */
#define WD_CHECK_PERIOD_MS  1000

/* 单个任务允许的最长不上报时间。
   最慢的被监视任务是 HTTPD(10ms 轮询) 与 Energy/OBD(100ms),
   3s 窗口足够吸收 ESP8266 一次长应答或 CAN 一次重传风暴。 */
#define WD_STALL_MS         3000

/* 启动宽限期: 这段时间内无条件喂狗。
   ESP8266_Init() 在 HTTPD 任务里跑, 模组没接好时要等一串 AT 应答超时,
   可能几秒才出第一轮心跳 —— 那是"启动慢", 不是"卡死", 不该复位整机。 */
#define WD_GRACE_MS         20000

#define WD_TASK_STACK       512
#define WD_TASK_PRIO        osPriorityLow

static IWDG_HandleTypeDef s_hiwdg;

/* 各任务最后一次上报心跳的时刻(HAL_GetTick 计数)。
   0 表示该任务还没上报过第一次心跳。volatile: 多任务写、看门狗任务读。 */
static volatile uint32_t s_hb_tick[WD_TASK_COUNT];

static uint32_t s_start_tick;

void Watchdog_Heartbeat(WatchdogTask_t task) {
    if ((unsigned)task < WD_TASK_COUNT)
        s_hb_tick[task] = HAL_GetTick();
}

static void TaskWatchdog_Entry(void *arg) {
    (void)arg;

    for (;;) {
        osDelay(WD_CHECK_PERIOD_MS);

        uint32_t now = HAL_GetTick();

        /* 宽限期内无条件喂狗 */
        if ((now - s_start_tick) < WD_GRACE_MS) {
            HAL_IWDG_Refresh(&s_hiwdg);
            continue;
        }

        int alive = 1;
        for (unsigned i = 0; i < WD_TASK_COUNT; i++) {
            uint32_t t = s_hb_tick[i];
            /* 还没上报过第一次心跳, 或超过窗口没再上报 -> 判定卡死 */
            if (t == 0 || (now - t) > WD_STALL_MS) {
                alive = 0;
                break;
            }
        }

        /* 只有全部任务都在窗口内报活才喂狗。
           任一任务停摆 -> 停止喂狗 -> 约 4s 后 IWDG 复位整机。 */
        if (alive)
            HAL_IWDG_Refresh(&s_hiwdg);
    }
}

void Watchdog_Init(void) {
    /* 调试时冻结 IWDG: 断点停住时不被看门狗复位。
       只在调试器连接时生效, 不影响正常运行时的兜底能力。 */
    __HAL_DBGMCU_FREEZE_IWDG();

    s_hiwdg.Instance = IWDG;
    s_hiwdg.Init.Prescaler = WD_IWDG_PRESCALER;
    s_hiwdg.Init.Reload = WD_IWDG_RELOAD;

    /* IWDG 一旦启动就无法关闭。初始化失败只可能是参数越界(编译期常量, 不会发生),
       此时不喂狗也不复位, 直接放弃看门狗功能, 但不能因此拖住系统启动。 */
    if (HAL_IWDG_Init(&s_hiwdg) != HAL_OK)
        return;

    s_start_tick = HAL_GetTick();

    osThreadAttr_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.stack_size = WD_TASK_STACK;
    attr.priority = WD_TASK_PRIO;

    /* 创建失败时没有人喂狗: 约 4s 后 IWDG 会复位整机。
       这正是期望行为 —— heap 耗尽时系统已不可信, 重启比带病运行好。 */
    (void)osThreadNew(TaskWatchdog_Entry, NULL, &attr);
}
