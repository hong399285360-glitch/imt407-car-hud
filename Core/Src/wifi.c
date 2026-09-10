#include "wifi.h"
#include <string.h>
#include <stdio.h>
#include "cmsis_os2.h"
#include "sdlog.h"

ESP8266_t g_esp;

static void ESP_Send(const char *data, uint16_t len) {
    HAL_UART_Transmit(g_esp.huart, (uint8_t *)data, len, 1000);
}

static void ESP_SendStr(const char *str) {
    ESP_Send(str, strlen(str));
}

/* 按行等待 OK/ready, 返回 0 = 命中, -1 = 命中 ERROR/FAIL 或超时。
   原实现无返回值, 调用方只能看 g_esp.state, 无法区分"这条命令失败"与
   "上一条命令留下的状态" —— 初始化因此变成"发完就算成功"。 */
static int8_t ESP_WaitResponse(uint16_t timeout_ms) {
    uint32_t start = HAL_GetTick();
    g_esp.rx_line_len = 0;
    while ((HAL_GetTick() - start) < timeout_ms) {
        uint8_t ch;
        uint16_t n = UART_RingBuf_Read(g_esp.rxbuf, &ch, 1);
        if (n > 0) {
            if (ch == '\n' || ch == '\r') {
                if (g_esp.rx_line_len > 0) {
                    g_esp.rx_line[g_esp.rx_line_len] = '\0';
                    if (strstr(g_esp.rx_line, "OK") || strstr(g_esp.rx_line, "ready")) {
                        g_esp.state = ESP_READY;
                        return 0;
                    }
                    if (strstr(g_esp.rx_line, "ERROR") || strstr(g_esp.rx_line, "FAIL")) {
                        return -1;
                    }
                    g_esp.rx_line_len = 0;
                }
            } else {
                if (g_esp.rx_line_len < ESP_RX_BUF - 1) {
                    g_esp.rx_line[g_esp.rx_line_len++] = ch;
                }
            }
        } else {
            osDelay(1);
        }
    }
    return -1;
}

/* 模组硬复位引脚 (MX_GPIO_Init 已配成推挽输出) */
#define ESP_RST_PORT  GPIOA
#define ESP_RST_PIN   GPIO_PIN_8

/* 发一条 AT 命令并校验应答, 失败按 retries 次重试。
   step 只用于记录失败位置 (g_esp.init_fail_step)。返回 0 = 成功。 */
static int8_t ESP_InitStep(uint8_t step, const char *cmd, uint16_t timeout_ms, uint8_t retries) {
    for (uint8_t i = 0; i <= retries; i++) {
        g_esp.state = ESP_WAIT_OK;
        ESP_SendStr(cmd);
        if (ESP_WaitResponse(timeout_ms) == 0)
            return 0;
        osDelay(200);                   /* 给模组一点喘息时间再重试 */
    }
    g_esp.init_fail_step = step;
    return -1;
}

void ESP8266_Init(UART_HandleTypeDef *huart, UART_RingBuf_t *rxbuf) {
    g_esp.huart = huart;
    g_esp.rxbuf = rxbuf;
    g_esp.state = ESP_IDLE;
    g_esp.init_fail_step = 0;
    memset(g_esp.connections, 0, ESP_MAX_CONN);

    /* 硬复位: PA8 拉低再放开。AT+RST 是软件复位, 模组固件卡死时它连命令
       都收不进去, 只有硬复位能救回来。 */
    HAL_GPIO_WritePin(ESP_RST_PORT, ESP_RST_PIN, GPIO_PIN_RESET);
    osDelay(200);
    HAL_GPIO_WritePin(ESP_RST_PORT, ESP_RST_PIN, GPIO_PIN_SET);
    osDelay(1500);                      /* 等模组启动并打印完 ready */

    /* 每条命令都校验应答。原写法发完就走、最后无条件置 ESP_READY:
       模组没供电/波特率不对/固件卡死时, 系统照样认为"就绪",
       现象只是浏览器连不上热点, 完全无法定位。 */
    if (ESP_InitStep(1, "AT\r\n",                                   1000, 3) != 0) goto fail;
    if (ESP_InitStep(2, "AT+CWMODE=2\r\n",                          1000, 2) != 0) goto fail;
    if (ESP_InitStep(3, "AT+CWSAP=\"F407-HUD\",\"12345678\",5,3\r\n", 3000, 2) != 0) goto fail;
    if (ESP_InitStep(4, "AT+CIPMUX=1\r\n",                          1000, 2) != 0) goto fail;
    if (ESP_InitStep(5, "AT+CIPSERVER=1,80\r\n",                    2000, 2) != 0) goto fail;

    g_esp.state = ESP_READY;
    return;

fail:
    /* 保留 ESP_ERROR 与 init_fail_step 供诊断, HTTPD 任务继续轮询, 不阻塞系统 */
    g_esp.state = ESP_ERROR;
    {
        /* 模组没插/供电异常时, 界面上只表现为"连不上热点", 现场无从下手。
           把失败的 AT 步骤写进 SD 日志, 事后至少能知道卡在哪一步。 */
        char esp_err_msg[80];
        snprintf(esp_err_msg, sizeof(esp_err_msg),
                 "[ERROR] ESP8266 init failed at step %u",
                 (unsigned)g_esp.init_fail_step);
        SDLog_Log(esp_err_msg);
    }
}

uint8_t ESP8266_SendAT(const char *cmd, uint16_t timeout_ms) {
    g_esp.state = ESP_WAIT_OK;
    ESP_SendStr(cmd);
    return (ESP_WaitResponse(timeout_ms) == 0) ? 1 : 0;
}

void ESP8266_Poll(void) {
    /* 环形缓冲溢出是静默丢数据: 一条 HTTP 请求被截断后表现为 404 或乱码,
       界面上只会觉得"偶尔点不动", 现场什么都看不到。这里发现新增溢出就
       写一条 SD 日志, 至少事后能确认"是不是丢过数据"。 */
    static uint32_t s_ovf_seen = 0;
    uint32_t ovf = UART_RingBuf_Overflow(g_esp.rxbuf);
    if (ovf != s_ovf_seen) {
        char ovf_msg[64];
        snprintf(ovf_msg, sizeof(ovf_msg),
                 "[WARN] ESP RX ring overflow: %lu bytes lost",
                 (unsigned long)(ovf - s_ovf_seen));
        SDLog_Log(ovf_msg);
        s_ovf_seen = ovf;
    }

    uint8_t buf[ESP_RX_BUF];
    uint16_t n = UART_RingBuf_Read(g_esp.rxbuf, buf, ESP_RX_BUF - 1);
    if (n == 0) return;

    buf[n] = '\0';

    if (strstr((char *)buf, ",CONNECT")) {
        for (uint8_t i = 0; i < ESP_MAX_CONN; i++) {
            char pattern[8];
            snprintf(pattern, sizeof(pattern), "%d,CONNECT", i);
            if (strstr((char *)buf, pattern)) {
                g_esp.connections[i] = 1;
            }
        }
    }

    if (strstr((char *)buf, ",CLOSED")) {
        for (uint8_t i = 0; i < ESP_MAX_CONN; i++) {
            char pattern[8];
            snprintf(pattern, sizeof(pattern), "%d,CLOSED", i);
            if (strstr((char *)buf, pattern)) {
                g_esp.connections[i] = 0;
            }
        }
    }

    char *ipd = strstr((char *)buf, "+IPD,");
    if (ipd) {
        int conn_id, data_len;
        if (sscanf(ipd, "+IPD,%d,%d:", &conn_id, &data_len) == 2) {
            if (conn_id >= 0 && conn_id < ESP_MAX_CONN) {
                g_esp.connections[conn_id] = 1;
                char *http_start = strchr(ipd, ':');
                if (http_start) {
                    http_start++;
                    extern void HTTPD_HandleRequest(uint8_t conn_id, const char *request);
                    HTTPD_HandleRequest((uint8_t)conn_id, http_start);
                }
            }
        }
    }
}

/* 逐字节扫描接收缓冲, 等待 ok / err 两个 token 之一出现。
   返回 0 = 命中 ok, -1 = 命中 err 或超时。
   说明: ESP8266 的 '>' 提示符后面不带换行, 无法用按行解析的 ESP_WaitResponse。 */
static int8_t ESP_WaitToken(const char *ok, const char *err, uint16_t timeout_ms) {
    char win[24];
    uint8_t wlen = 0;
    uint32_t start = HAL_GetTick();

    win[0] = '\0';
    while ((HAL_GetTick() - start) < timeout_ms) {
        uint8_t ch;
        if (UART_RingBuf_Read(g_esp.rxbuf, &ch, 1) > 0) {
            if (wlen >= sizeof(win) - 1) {
                memmove(win, win + 1, sizeof(win) - 2);
                wlen = (uint8_t)(sizeof(win) - 2);
            }
            win[wlen++] = (char)ch;
            win[wlen] = '\0';
            if (err && strstr(win, err)) return -1;
            if (ok && strstr(win, ok)) return 0;
        } else {
            osDelay(1);
        }
    }
    return -1;
}

/* 单次 AT+CIPSEND: 必须等到 '>' 提示符才能发数据, 发完等 "SEND OK" 确认。
   原来用固定 HAL_Delay(50) 是发不出去的 (ESP8266 未就绪时数据被丢弃)。 */
static int8_t ESP_SendOneChunk(uint8_t conn_id, const char *data, uint16_t len) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%u,%u\r\n", (unsigned)conn_id, (unsigned)len);
    ESP_SendStr(cmd);

    if (ESP_WaitToken(">", "ERROR", 1000) != 0) {
        return -1;                  /* 没等到 '>' 提示符, 数据不能发 */
    }
    ESP_Send(data, len);
    if (ESP_WaitToken("SEND OK", "SEND FAIL", 5000) != 0) {
        return -1;                  /* ESP8266 未确认发出 */
    }
    return 0;
}

int8_t ESP8266_SendChunk(uint8_t conn_id, const char *data, uint16_t len) {
    if (conn_id >= ESP_MAX_CONN) return -1;
    while (len > 0) {
        uint16_t n = (len > ESP_TX_CHUNK) ? ESP_TX_CHUNK : len;
        if (ESP_SendOneChunk(conn_id, data, n) != 0) return -1;
        data += n;
        len = (uint16_t)(len - n);
    }
    return 0;
}

void ESP8266_CloseConn(uint8_t conn_id) {
    char cmd[24];
    if (conn_id >= ESP_MAX_CONN) return;
    snprintf(cmd, sizeof(cmd), "AT+CIPCLOSE=%u\r\n", (unsigned)conn_id);
    ESP_SendStr(cmd);
    g_esp.connections[conn_id] = 0;
}

int8_t ESP8266_SendData(uint8_t conn_id, const char *data, uint16_t len) {
    int8_t rc = ESP8266_SendChunk(conn_id, data, len);
    ESP8266_CloseConn(conn_id);
    return rc;
}

int8_t ESP8266_SendJSON(uint8_t conn_id, const char *json, uint16_t len) {
    return ESP8266_SendData(conn_id, json, len);
}

uint8_t ESP8266_IsConnected(uint8_t conn_id) {
    if (conn_id < ESP_MAX_CONN)
        return g_esp.connections[conn_id];
    return 0;
}
