/**
  ******************************************************************************
  * @file    stm32f4xx_it.h
  * @brief   IMT407 Car HUD - Interrupt handlers header
  ******************************************************************************
  */

#ifndef __STM32F4xx_IT_H
#define __STM32F4xx_IT_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Exported functions prototypes ---------------------------------------------*/
void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void DebugMon_Handler(void);
void SVC_Handler(void);
void PendSV_Handler(void);
void DMA1_Stream2_IRQHandler(void);
void UART4_IRQHandler(void);
void CAN1_RX0_IRQHandler(void);
void SDIO_IRQHandler(void);
void SysTick_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_IT_H */
