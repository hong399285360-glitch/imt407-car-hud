/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   IMT407 Car HUD - Interrupt handlers
  ******************************************************************************
  */

#include "main.h"
#include "stm32f4xx_it.h"
#include "FreeRTOS.h"
#include "task.h"
#include "task_config.h"

/* FreeRTOS 中断处理函数声明（GCC 端口在 port.c 中定义） */
extern void xPortSysTickHandler(void);
extern void vPortSVCHandler(void);
extern void xPortPendSVHandler(void);

/* External handlers --------------------------------------------------------*/
extern UART_HandleTypeDef huart4;
extern CAN_HandleTypeDef hcan1;
extern SD_HandleTypeDef hsd;

extern DMA_HandleTypeDef hdma_uart4_rx;

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/

/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  HAL_IncTick();
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
  {
    xPortSysTickHandler();
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  vPortSVCHandler();
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  xPortPendSVHandler();
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/******************************************************************************/

/**
  * @brief This function handles DMA1 stream2 global interrupt (UART4 RX).
  */
void DMA1_Stream2_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_uart4_rx);
}

/**
  * @brief This function handles UART4 global interrupt (ESP8266).
  */
void UART4_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart4);

  /* HAL_UART_IRQHandler() 不处理 IDLE 空闲线中断, 必须自行判断并清除标志。
     否则 UART_IT_IDLE 会反复触发造成中断风暴, 且 DMA 收到的 ESP8266 数据
     永远不会进入环形缓冲区 (ESP8266_Init 的 AT 应答也就永远读不到)。 */
  if (__HAL_UART_GET_FLAG(&huart4, UART_FLAG_IDLE) != RESET &&
      __HAL_UART_GET_IT_SOURCE(&huart4, UART_IT_IDLE) != RESET)
  {
    TaskConfig_OnUART4IDLE();
  }
}

/**
  * @brief This function handles CAN1 RX0 interrupt (OBD response).
  */
void CAN1_RX0_IRQHandler(void)
{
  HAL_CAN_IRQHandler(&hcan1);
}

/**
  * @brief This function handles SDIO global interrupt.
  */
void SDIO_IRQHandler(void)
{
  HAL_SD_IRQHandler(&hsd);
}
