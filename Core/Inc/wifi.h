#ifndef WIFI_H
#define WIFI_H

#include "stm32f4xx_hal.h"
#include "uart_ringbuf.h"

#define ESP_MAX_CONN 4
#define ESP_RX_BUF 1024
/* ESP8266 AT 固件单次 AT+CIPSEND 的数据上限约 2048 字节,
   这里按 1024 分块: 既留足余量, 又便于 16KB 级网页分多次下发 */
#define ESP_TX_CHUNK 1024

typedef enum {
    ESP_IDLE = 0,
    ESP_WAIT_OK,
    ESP_READY,
    ESP_ERROR           /* 初始化某一步失败: 模组未供电/波特率不对/固件卡死 */
} ESP_State_t;

typedef struct {
    UART_RingBuf_t *rxbuf;
    UART_HandleTypeDef *huart;
    ESP_State_t state;
    /* 初始化失败时停在哪条 AT 命令 (从 1 开始, 0 = 未失败)。
       模组没接好时这是唯一能拿到的线索 —— 没有它只能看到"连不上热点"。 */
    uint8_t init_fail_step;
    uint8_t connections[ESP_MAX_CONN];
    char rx_line[ESP_RX_BUF];
    uint16_t rx_line_len;
    uint32_t cmd_timeout;
} ESP8266_t;

extern ESP8266_t g_esp;

void ESP8266_Init(UART_HandleTypeDef *huart, UART_RingBuf_t *rxbuf);
uint8_t ESP8266_SendAT(const char *cmd, uint16_t timeout_ms);
void ESP8266_Poll(void);
/* 发送并关闭连接, 返回 0 成功 / -1 失败 (失败也已关闭连接) */
int8_t ESP8266_SendData(uint8_t conn_id, const char *data, uint16_t len);
int8_t ESP8266_SendJSON(uint8_t conn_id, const char *json, uint16_t len);
/* 分块发送但不关闭连接 (同一 HTTP 响应可分多次调用), 返回 0 成功 */
int8_t ESP8266_SendChunk(uint8_t conn_id, const char *data, uint16_t len);
void ESP8266_CloseConn(uint8_t conn_id);
uint8_t ESP8266_IsConnected(uint8_t conn_id);

#endif
