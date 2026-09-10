#ifndef TASK_CONFIG_H
#define TASK_CONFIG_H

#include "cmsis_os2.h"

#define TASK_OBD_STACK    1024
#define TASK_ENERGY_STACK 512
#define TASK_HTTPD_STACK  4096
#define TASK_SDLOG_STACK  1024

#define TASK_OBD_PRIO     osPriorityAboveNormal
#define TASK_ENERGY_PRIO  osPriorityBelowNormal
#define TASK_HTTPD_PRIO   osPriorityNormal
#define TASK_SDLOG_PRIO   osPriorityLow

void TaskConfig_Init(void);

/* UART4 空闲线中断处理入口, 由 stm32f4xx_it.c 的 UART4_IRQHandler 调用 */
void TaskConfig_OnUART4IDLE(void);

void TaskOBD_Entry(void *arg);
void TaskEnergy_Entry(void *arg);
void TaskHTTPD_Entry(void *arg);
void TaskSDLog_Entry(void *arg);

#endif
