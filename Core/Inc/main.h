/**
  ******************************************************************************
  * @file    main.h
  * @brief   IMT407 Car HUD - Main header
  ******************************************************************************
  */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* Private defines -----------------------------------------------------------*/
#define ESP8266_RST_Pin GPIO_PIN_8
#define ESP8266_RST_GPIO_Port GPIOA

/* External handles ----------------------------------------------------------*/
extern UART_HandleTypeDef huart4;
extern CAN_HandleTypeDef hcan1;
extern SD_HandleTypeDef hsd;

extern DMA_HandleTypeDef hdma_uart4_rx;

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
