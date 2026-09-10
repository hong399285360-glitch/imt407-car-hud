/**
  ******************************************************************************
  * @file    stm32f4xx_hal_msp.c
  * @brief   HAL MSP initialization for IMT407 Car HUD
  ******************************************************************************
  */

#include "main.h"

/**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

  /* System interrupt init*/
  /* PendSV_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);
}

/**
  * @brief UART MSP Initialization
  */
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (huart->Instance == UART4)
  {
    /* UART4 clock enable (启明欣欣F407: ESP8266接在UART4上, 不是USART1) */
    __HAL_RCC_UART4_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* UART4 GPIO Configuration
       PA0 ------> UART4_TX  (注意: PA0也是KEY_UP, 不能同时用)
       PA1 ------> UART4_RX */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* UART4 DMA Init - RX: DMA1 Stream2 Ch4 */
    hdma_uart4_rx.Instance = DMA1_Stream2;
    hdma_uart4_rx.Init.Channel = DMA_CHANNEL_4;
    hdma_uart4_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_uart4_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_uart4_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_uart4_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_uart4_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_uart4_rx.Init.Mode = DMA_CIRCULAR;
    hdma_uart4_rx.Init.Priority = DMA_PRIORITY_MEDIUM;
    hdma_uart4_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_uart4_rx);

    __HAL_LINKDMA(huart, hdmarx, hdma_uart4_rx);

    /* UART4 interrupt Init */
    HAL_NVIC_SetPriority(UART4_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(UART4_IRQn);
  }
}

/**
  * @brief UART MSP De-Initialization
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
  if (huart->Instance == UART4)
  {
    __HAL_RCC_UART4_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0|GPIO_PIN_1);
    HAL_DMA_DeInit(huart->hdmarx);
    HAL_NVIC_DisableIRQ(UART4_IRQn);
  }
}

/**
  * @brief SD MSP Initialization
  */
void HAL_SD_MspInit(SD_HandleTypeDef* hsd)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (hsd->Instance == SDIO)
  {
    /* Peripheral clock enable */
    __HAL_RCC_SDIO_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /* SDIO GPIO Configuration
       PC8  ------> SDIO_D0
       PC9  ------> SDIO_D1
       PC10 ------> SDIO_D2
       PC11 ------> SDIO_D3
       PC12 ------> SDIO_CK
       PD2  ------> SDIO_CMD */
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDIO;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDIO;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* SDIO DMA Init */
    /* Note: SDIO uses DMA2 Stream3 or Stream6 Ch4 (handled by HAL SD driver) */

    /* SDIO interrupt Init
       优先级必须是 5 或更大数值(=更低优先级): FreeRTOS 规定能调用
       FromISR API 的中断优先级不得高于 configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY(5),
       否则会破坏内核临界区, 表现为偶发死锁。原来写成 4 是违规的
       (当前 SD 走轮询模式, 该中断实际不触发, 所以一直没暴露;
        将来若改成 DMA 模式就会立刻踩到)。 */
    HAL_NVIC_SetPriority(SDIO_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(SDIO_IRQn);
  }
}

/**
  * @brief SD MSP De-Initialization
  */
void HAL_SD_MspDeInit(SD_HandleTypeDef* hsd)
{
  if (hsd->Instance == SDIO)
  {
    __HAL_RCC_SDIO_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12);
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_2);
    HAL_NVIC_DisableIRQ(SDIO_IRQn);
  }
}

/**
 * @brief CAN MSP Initialization (TJA1050)
 *   PB8 -> CAN1_RX
 *   PB9 -> CAN1_TX
 * (启明欣欣F407轻奢版V3.1: CAN1在PB8/PB9, 不是PA11/PA12)
 */
void HAL_CAN_MspInit(CAN_HandleTypeDef* hcan)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (hcan->Instance == CAN1)
  {
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* CAN1 GPIO Configuration
       PB8 ------> CAN1_RX
       PB9 ------> CAN1_TX */
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* CAN1 RX0 interrupt Init */
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
  }
}

/**
 * @brief CAN MSP De-Initialization
 */
void HAL_CAN_MspDeInit(CAN_HandleTypeDef* hcan)
{
  if (hcan->Instance == CAN1)
  {
    __HAL_RCC_CAN1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8|GPIO_PIN_9);
    HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);
  }
}
