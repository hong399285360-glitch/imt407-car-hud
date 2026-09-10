#ifndef UART_RINGBUF_H
#define UART_RINGBUF_H

#include "stm32f4xx_hal.h"

#define RX_BUF_SIZE 512

typedef struct {
    uint8_t buf[RX_BUF_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
    /* 因环形缓冲写满而丢弃的字节数。中断里累加, 只增不减。
       没有这个计数的话, 溢出现场是"数据莫名其妙缺了一段", 无从判断。 */
    volatile uint32_t overflow;
    UART_HandleTypeDef *huart;
    uint8_t dma_buf[RX_BUF_SIZE];
    uint16_t last_dma_pos;
} UART_RingBuf_t;

void UART_RingBuf_Init(UART_RingBuf_t *rb, UART_HandleTypeDef *huart);
void UART_RingBuf_StartDMA(UART_RingBuf_t *rb);
void UART_RingBuf_OnIDLE(UART_RingBuf_t *rb);
uint16_t UART_RingBuf_Read(UART_RingBuf_t *rb, uint8_t *out, uint16_t max_len);
uint16_t UART_RingBuf_Available(UART_RingBuf_t *rb);
/* 返回累计溢出丢弃的字节数 (0 表示从未溢出) */
uint32_t UART_RingBuf_Overflow(UART_RingBuf_t *rb);

#endif
