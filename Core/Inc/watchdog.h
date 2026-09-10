/**
  ******************************************************************************
  * @file   watchdog.h
  * @brief  独立看门狗 + 任务心跳监视
  *
  * 装车后没有调试器, 一旦某个任务卡死, 界面会永久黑屏。
  * 本模块用 IWDG 做整机兜底: 只有所有被监视的任务都在窗口内报活才喂狗,
  * 任何一个任务停摆就让 IWDG 复位整机, 由重启代替黑屏。
  ******************************************************************************
  */

#ifndef __WATCHDOG_H
#define __WATCHDOG_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 参与心跳监视的任务。
 * SDLog 故意不在其中: 它允许因 SD 卡拔出/接触不良而长时间阻塞,
 * 把开车中的偶发卡顿也算成故障会导致反复复位, 得不偿失。
 */
typedef enum {
    WD_TASK_OBD = 0,
    WD_TASK_ENERGY,
    WD_TASK_HTTPD,
    WD_TASK_COUNT
} WatchdogTask_t;

/* 启动 IWDG 并创建看门狗任务。
   必须在 osKernelStart() 之前、其它硬件初始化之后调用:
   放太早的话, SDLog_Init() 里无卡时的 SDIO 超时等待会把狗喂不上。 */
void Watchdog_Init(void);

/* 各任务在每轮循环里调用, 上报自己还活着 */
void Watchdog_Heartbeat(WatchdogTask_t task);

#ifdef __cplusplus
}
#endif

#endif /* __WATCHDOG_H */
