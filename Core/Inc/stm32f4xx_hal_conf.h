/**
  ******************************************************************************
  * @file    stm32f4xx_hal_conf.h
  * @brief   HAL configuration file for IMT407 Car HUD
  ******************************************************************************
  */

#ifndef __STM32F4xx_HAL_CONF_H
#define __STM32F4xx_HAL_CONF_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/* ########################## Module Selection ############################## */
#define HAL_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_SD_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_EXTI_MODULE_ENABLED

/* #define HAL_ADC_MODULE_ENABLED       */
#define HAL_CAN_MODULE_ENABLED
/* #define HAL_CEC_MODULE_ENABLED       */
/* #define HAL_CRC_MODULE_ENABLED       */
/* #define HAL_CRYP_MODULE_ENABLED      */
/* #define HAL_DAC_MODULE_ENABLED       */
/* #define HAL_DCMI_MODULE_ENABLED      */
/* #define HAL_DMA2D_MODULE_ENABLED     */
/* #define HAL_ETH_MODULE_ENABLED       */
/* #define HAL_NAND_MODULE_ENABLED      */
/* #define HAL_NOR_MODULE_ENABLED       */
/* #define HAL_PCCARD_MODULE_ENABLED    */
/* #define HAL_SRAM_MODULE_ENABLED      */
/* #define HAL_SDRAM_MODULE_ENABLED     */
/* #define HAL_HASH_MODULE_ENABLED      */
/* #define HAL_I2S_MODULE_ENABLED       */
#define HAL_IWDG_MODULE_ENABLED
/* #define HAL_LTDC_MODULE_ENABLED      */
/* #define HAL_DSI_MODULE_ENABLED       */
/* #define HAL_SPI_MODULE_ENABLED       */
/* #define HAL_RTC_MODULE_ENABLED       */
/* #define HAL_SAI_MODULE_ENABLED       */
/* #define HAL_SMBUS_MODULE_ENABLED     */
/* #define HAL_USART_MODULE_ENABLED     */
/* #define HAL_IRDA_MODULE_ENABLED      */
/* #define HAL_SMARTCARD_MODULE_ENABLED */
/* #define HAL_WWDG_MODULE_ENABLED      */
/* #define HAL_PCD_MODULE_ENABLED       */
/* #define HAL_HCD_MODULE_ENABLED       */
/* #define HAL_QSPI_MODULE_ENABLED      */
/* #define HAL_CEC_MODULE_ENABLED       */
/* #define HAL_FMPI2C_MODULE_ENABLED    */
/* #define HAL_SPDIFRX_MODULE_ENABLED   */
/* #define HAL_DFSDM_MODULE_ENABLED     */
/* #define HAL_LPTIM_MODULE_ENABLED     */
/* #define HAL_MMC_MODULE_ENABLED       */

/* ########################## HSE/HSI Values adaptation ##################### */
#define HSE_VALUE    ((uint32_t)8000000U)  /* 8 MHz */
#define HSE_STARTUP_TIMEOUT    ((uint32_t)100U)
#define LSE_STARTUP_TIMEOUT    ((uint32_t)5000U)
#define HSI_VALUE    ((uint32_t)16000000U) /* 16 MHz */
#define HSI_STARTUP_TIMEOUT    ((uint32_t)100U)
#define LSI_VALUE    ((uint32_t)32000U)    /* 32 kHz */
#define LSE_VALUE    ((uint32_t)32768U)    /* 32.768 kHz */
#define EXTERNAL_CLOCK_VALUE    ((uint32_t)12288000U) /* 12.288 MHz */

/* ########################### System Configuration ######################### */
#define VDD_VALUE                    ((uint32_t)3300U) /* 3.3V */
#define TICK_INT_PRIORITY            ((uint32_t)0x0FU)
#define USE_RTOS                     0U
#define PREFETCH_ENABLE              1U
#define INSTRUCTION_CACHE_ENABLE     1U
#define DATA_CACHE_ENABLE            1U

#define USE_HAL_ADC_REGISTER_CALLBACKS        0U
#define USE_HAL_CAN_REGISTER_CALLBACKS        0U
#define USE_HAL_CEC_REGISTER_CALLBACKS        0U
#define USE_HAL_DAC_REGISTER_CALLBACKS        0U
#define USE_HAL_I2C_REGISTER_CALLBACKS        0U
#define USE_HAL_I2S_REGISTER_CALLBACKS        0U
#define USE_HAL_SPI_REGISTER_CALLBACKS        0U
#define USE_HAL_TIM_REGISTER_CALLBACKS        0U
#define USE_HAL_UART_REGISTER_CALLBACKS       0U
#define USE_HAL_USART_REGISTER_CALLBACKS      0U
#define USE_HAL_SMARTCARD_REGISTER_CALLBACKS  0U
#define USE_HAL_IRDA_REGISTER_CALLBACKS       0U
#define USE_HAL_SMBUS_REGISTER_CALLBACKS      0U
#define USE_HAL_PCD_REGISTER_CALLBACKS        0U
#define USE_HAL_HCD_REGISTER_CALLBACKS        0U
#define USE_HAL_SAI_REGISTER_CALLBACKS        0U
#define USE_HAL_SD_REGISTER_CALLBACKS         0U
#define USE_HAL_SRAM_REGISTER_CALLBACKS       0U
#define USE_HAL_NAND_REGISTER_CALLBACKS       0U
#define USE_HAL_NOR_REGISTER_CALLBACKS        0U
#define USE_HAL_PCCARD_REGISTER_CALLBACKS     0U
#define USE_HAL_SDRAM_REGISTER_CALLBACKS      0U
#define USE_HAL_MMC_REGISTER_CALLBACKS        0U
#define USE_HAL_RTC_REGISTER_CALLBACKS        0U
#define USE_HAL_WWDG_REGISTER_CALLBACKS       0U

/* ########################## Assert Selection ############################## */
/* #define USE_FULL_ASSERT    1U */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_dma.h"
#include "stm32f4xx_hal_flash.h"
#include "stm32f4xx_hal_pwr.h"
#include "stm32f4xx_hal_cortex.h"
#include "stm32f4xx_hal_uart.h"
#include "stm32f4xx_hal_i2c.h"
#include "stm32f4xx_hal_sd.h"
#include "stm32f4xx_hal_tim.h"
#include "stm32f4xx_hal_exti.h"
#include "stm32f4xx_hal_can.h"
#include "stm32f4xx_hal_iwdg.h"

/* Exported macro ------------------------------------------------------------*/
#ifdef USE_FULL_ASSERT
  #define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
  void assert_failed(uint8_t* file, uint32_t line);
#else
  #define assert_param(expr) ((void)0U)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_HAL_CONF_H */
