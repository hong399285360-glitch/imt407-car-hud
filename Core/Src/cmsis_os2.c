/**
  ******************************************************************************
  * @file    cmsis_os2.c
  * @brief   Minimal CMSIS-RTOS v2 API implementation on FreeRTOS
  ******************************************************************************
  */

#include "cmsis_os2.h"
#include <string.h>

/* Helper: convert CMSIS priority (0-56) to FreeRTOS priority (0-configMAX_PRIORITIES-1) */
static UBaseType_t cmsis_to_freertos_prio(osPriority_t prio)
{
  /* CMSIS: idle=1 ... normal=24 ... isr=56 (56 levels)
     FreeRTOS: 0 = idle, higher number = higher priority
     Map: CMSIS prio / 8 roughly */
  UBaseType_t fr_prio;
  if (prio <= osPriorityIdle)          fr_prio = 0;
  else if (prio <= osPriorityLow)       fr_prio = 1;
  else if (prio <= osPriorityBelowNormal) fr_prio = 2;
  else if (prio <= osPriorityNormal)    fr_prio = 3;
  else if (prio <= osPriorityAboveNormal) fr_prio = 4;
  else if (prio <= osPriorityHigh)      fr_prio = 5;
  else                                  fr_prio = 6;

  if (fr_prio >= configMAX_PRIORITIES)
    fr_prio = configMAX_PRIORITIES - 1;
  return fr_prio;
}

/* ===== Thread ===== */

osThreadId_t osThreadNew(void (*func)(void *), void *arg, const osThreadAttr_t *attr)
{
  TaskHandle_t handle = NULL;
  const char *name = (attr && attr->name) ? attr->name : "task";
  uint16_t stack_depth = (attr && attr->stack_size > 0) ? (uint16_t)(attr->stack_size / 4) : 128;
  UBaseType_t prio = (attr) ? cmsis_to_freertos_prio(attr->priority) : tskIDLE_PRIORITY + 1;

  BaseType_t ret = xTaskCreate((TaskFunction_t)func, name, stack_depth, arg, prio, &handle);
  if (ret != pdPASS) {
    return NULL;
  }
  return handle;
}

osStatus_t osThreadTerminate(osThreadId_t thread_id)
{
  if (thread_id == NULL) return osErrorParameter;
  vTaskDelete(thread_id);
  return osOK;
}

osThreadId_t osThreadGetId(void)
{
  return xTaskGetCurrentTaskHandle();
}

osStatus_t osThreadYield(void)
{
  taskYIELD();
  return osOK;
}

/* ===== Delay ===== */

osStatus_t osDelay(uint32_t ticks)
{
  vTaskDelay((TickType_t)ticks);
  return osOK;
}

osStatus_t osDelayUntil(uint32_t *wakeup_time, uint32_t ticks)
{
  TickType_t t = (TickType_t)*wakeup_time;
  vTaskDelayUntil(&t, (TickType_t)ticks);
  *wakeup_time = (uint32_t)t;
  return osOK;
}

/* ===== Mutex ===== */

osMutexId_t osMutexNew(const osMutexAttr_t *attr)
{
  (void)attr;
  SemaphoreHandle_t mtx = xSemaphoreCreateMutex();
  return mtx;
}

osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout)
{
  if (mutex_id == NULL) return osErrorParameter;
  BaseType_t ret = xSemaphoreTake(mutex_id, (TickType_t)timeout);
  return (ret == pdPASS) ? osOK : osErrorTimeout;
}

osStatus_t osMutexRelease(osMutexId_t mutex_id)
{
  if (mutex_id == NULL) return osErrorParameter;
  BaseType_t ret = xSemaphoreGive(mutex_id);
  return (ret == pdPASS) ? osOK : osErrorResource;
}

osStatus_t osMutexDelete(osMutexId_t mutex_id)
{
  if (mutex_id == NULL) return osErrorParameter;
  vSemaphoreDelete(mutex_id);
  return osOK;
}

/* ===== Kernel ===== */

osStatus_t osKernelInitialize(void)
{
  /* FreeRTOS doesn't need explicit init before task creation */
  return osOK;
}

osStatus_t osKernelStart(void)
{
  vTaskStartScheduler();
  return osOK; /* Never reached */
}

uint32_t osKernelGetTickCount(void)
{
  return (uint32_t)xTaskGetTickCount();
}

uint32_t osKernelGetTickFreq(void)
{
  return configTICK_RATE_HZ;
}

/* ===== FreeRTOS Hooks (weak defaults) ===== */

__attribute__((weak)) void vApplicationMallocFailedHook(void)
{
  taskDISABLE_INTERRUPTS();
  for (;;);
}

__attribute__((weak)) void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  (void)xTask;
  (void)pcTaskName;
  taskDISABLE_INTERRUPTS();
  for (;;);
}

__attribute__((weak)) void vApplicationTickHook(void)
{
  /* HAL tick increment is done in SysTick_Handler before FreeRTOS tick */
}
