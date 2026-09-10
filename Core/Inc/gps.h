#ifndef GPS_H
#define GPS_H

#include "stm32f4xx_hal.h"
#include "uart_ringbuf.h"
#include "shared_data.h"

typedef struct {
    UART_RingBuf_t *rxbuf;
    UART_HandleTypeDef *huart;
    char line[128];
    uint16_t line_len;
} GPS_t;

extern GPS_t g_gps;

void GPS_Init(UART_HandleTypeDef *huart, UART_RingBuf_t *rxbuf);
void GPS_Update(VehicleData_t *vd);
void GPS_ParseLine(char *line);

#endif
