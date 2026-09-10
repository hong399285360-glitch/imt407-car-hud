#include "uart_ringbuf.h"
#include <string.h>

void UART_RingBuf_Init(UART_RingBuf_t *rb, UART_HandleTypeDef *huart) {
    memset(rb, 0, sizeof(UART_RingBuf_t));
    rb->huart = huart;
    rb->head = 0;
    rb->tail = 0;
    rb->last_dma_pos = 0;
}

void UART_RingBuf_StartDMA(UART_RingBuf_t *rb) {
    /* 使用标准 HAL_UART_Receive_DMA 启动 DMA 接收 */
    HAL_UART_Receive_DMA(rb->huart, rb->dma_buf, RX_BUF_SIZE);

    /* 使能空闲线中断（用于检测一帧数据结束） */
    __HAL_UART_ENABLE_IT(rb->huart, UART_IT_IDLE);

    rb->last_dma_pos = 0;
}

/* 向环形缓冲写入一个字节 (仅中断上下文调用)。
   采用"留一空槽"判满: 容量为 RX_BUF_SIZE-1, 满时丢弃新字节并累加 overflow。
   不覆盖旧数据的理由: 覆盖会让 head 越过 tail, 之后 tail != head 恒成立,
   读出的内容整体错位 —— 表现为 HTTP 请求被截断成乱七八糟的片段, 极难定位。 */
static inline void RB_Push(UART_RingBuf_t *rb, uint8_t byte) {
    uint16_t next = (uint16_t)((rb->head + 1) % RX_BUF_SIZE);
    if (next == rb->tail) {
        rb->overflow++;
        return;
    }
    rb->buf[rb->head] = byte;
    rb->head = next;
}

void UART_RingBuf_OnIDLE(UART_RingBuf_t *rb) {
    uint16_t dma_remaining;
    uint16_t dma_pos;
    uint16_t i;

    /* 清除空闲中断标志 */
    __HAL_UART_CLEAR_IDLEFLAG(rb->huart);

    /* 获取 DMA 当前剩余传输字节数 */
    dma_remaining = __HAL_DMA_GET_COUNTER(rb->huart->hdmarx);
    dma_pos = RX_BUF_SIZE - dma_remaining;

    /* 将 DMA 缓冲区数据搬移到环形缓冲区 */
    if (dma_pos >= rb->last_dma_pos) {
        for (i = rb->last_dma_pos; i < dma_pos; i++) {
            RB_Push(rb, rb->dma_buf[i]);
        }
    } else {
        /* DMA 缓冲区回绕了 */
        for (i = rb->last_dma_pos; i < RX_BUF_SIZE; i++) {
            RB_Push(rb, rb->dma_buf[i]);
        }
        for (i = 0; i < dma_pos; i++) {
            RB_Push(rb, rb->dma_buf[i]);
        }
    }

    rb->last_dma_pos = dma_pos;

    /* 如果 DMA 缓冲区已满，重置 DMA 以继续接收 */
    if (dma_remaining == 0) {
        HAL_UART_Receive_DMA(rb->huart, rb->dma_buf, RX_BUF_SIZE);
        rb->last_dma_pos = 0;
    }
}

uint16_t UART_RingBuf_Read(UART_RingBuf_t *rb, uint8_t *out, uint16_t max_len) {
    uint16_t count = 0;
    while (count < max_len && rb->tail != rb->head) {
        out[count++] = rb->buf[rb->tail];
        rb->tail = (rb->tail + 1) % RX_BUF_SIZE;
    }
    return count;
}

uint16_t UART_RingBuf_Available(UART_RingBuf_t *rb) {
    uint16_t head = rb->head;
    uint16_t tail = rb->tail;
    if (head >= tail)
        return head - tail;
    else
        return RX_BUF_SIZE - tail + head;
}

uint32_t UART_RingBuf_Overflow(UART_RingBuf_t *rb) {
    return rb->overflow;
}
