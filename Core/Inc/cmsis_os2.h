/**
  ******************************************************************************
  * @file    cmsis_os2.h
  * @brief   Minimal CMSIS-RTOS v2 API wrapper for FreeRTOS
  *          (Implements only the APIs used by this project)
  ******************************************************************************
  */

#ifndef CMSIS_OS2_H
#define CMSIS_OS2_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

/* ===== Status codes ===== */
typedef enum {
  osOK                    = 0,
  osError                 = -1,
  osErrorTimeout          = -2,
  osErrorResource         = -3,
  osErrorParameter        = -4,
  osErrorNoMemory         = -5,
  osErrorISR              = -6,
  osStatusReserved        = 0x7FFFFFFF
} osStatus_t;

/* ===== Priority levels ===== */
typedef enum {
  osPriorityNone          = 0,
  osPriorityIdle          = 1,
  osPriorityLow           = 8,
  osPriorityBelowNormal   = 16,
  osPriorityNormal        = 24,
  osPriorityAboveNormal   = 32,
  osPriorityHigh          = 40,
  osPriorityRealtime      = 48,
  osPriorityRealtime1     = 49,
  osPriorityRealtime2     = 50,
  osPriorityRealtime3     = 51,
  osPriorityRealtime4     = 52,
  osPriorityRealtime5     = 53,
  osPriorityRealtime6     = 54,
  osPriorityRealtime7     = 55,
  osPriorityISR           = 56,
  osPriorityReserved      = 0x7FFFFFFF
} osPriority_t;

/* ===== Timeout ===== */
#define osWaitForever     0xFFFFFFFFU
#define osNoTimeout       0U

/* ===== Thread ===== */
typedef TaskHandle_t            osThreadId_t;

typedef struct {
  const char  *name;
  uint32_t     attr_bits;
  void        *cb_mem;
  uint32_t     cb_size;
  void        *stack_mem;
  uint32_t     stack_size;
  osPriority_t priority;
  uint32_t     tz_module;
  uint32_t     reserved;
} osThreadAttr_t;

osThreadId_t osThreadNew(void (*func)(void *), void *arg, const osThreadAttr_t *attr);
osStatus_t   osThreadTerminate(osThreadId_t thread_id);
osThreadId_t osThreadGetId(void);
osStatus_t   osThreadYield(void);

/* ===== Delay ===== */
osStatus_t osDelay(uint32_t ticks);
osStatus_t osDelayUntil(uint32_t *wakeup_time, uint32_t ticks);

/* ===== Mutex ===== */
typedef SemaphoreHandle_t       osMutexId_t;

typedef struct {
  const char *name;
  uint32_t    attr_bits;
  void       *cb_mem;
  uint32_t    cb_size;
} osMutexAttr_t;

osMutexId_t osMutexNew(const osMutexAttr_t *attr);
osStatus_t  osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout);
osStatus_t  osMutexRelease(osMutexId_t mutex_id);
osStatus_t  osMutexDelete(osMutexId_t mutex_id);

/* ===== Kernel ===== */
osStatus_t osKernelInitialize(void);
osStatus_t osKernelStart(void);
uint32_t   osKernelGetTickCount(void);
uint32_t   osKernelGetTickFreq(void);

/* ===== Timer (stub, not used) ===== */
typedef TimerHandle_t osTimerId_t;

#ifdef __cplusplus
}
#endif

#endif /* CMSIS_OS2_H */
