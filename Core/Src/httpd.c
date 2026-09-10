#include "httpd.h"
#include "wifi.h"
#include "web_page.h"
#include "can_obd.h"
#include "sdlog.h"
#include <string.h>
#include <stdio.h>

#define HTTP_OK_HEADER "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n"

uint16_t HTTPD_BuildJSON(VehicleData_t *vd, char *buf, uint16_t buf_len) {
    int n = snprintf(buf, buf_len,
        "{\"speed\":%.0f,\"voltage\":%.1f,\"current\":%.1f,"
        "\"power\":%.1f,\"motor_rpm\":%.0f,\"throttle\":%.0f,"
        "\"brake\":%.0f,\"soc\":%.0f,"
        "\"motor_temp\":%.0f,\"igbt_temp\":%.0f,\"batt_temp\":%.0f,"
        "\"cell_vmax\":%.0f,\"cell_vmin\":%.0f,\"cell_dv\":%.0f,"
        "\"batt_tmax\":%.0f,\"batt_tmin\":%.0f,\"batt_dt\":%.0f,"
        "\"energy_km\":%.1f,\"regen_pct\":%.0f,"
        "\"total_energy\":%.2f,\"total_regen\":%.2f,\"total_distance\":%.1f,"
        "\"drive_score\":%.0f,\"drive_grade\":%d,"
        "\"ts\":%lu,\"obd_valid\":%d}",
        vd->speed, vd->voltage, vd->current,
        vd->power, vd->motor_rpm, vd->throttle,
        vd->brake, vd->soc,
        vd->motor_temp, vd->igbt_temp, vd->temp_batt,
        vd->cell_vmax, vd->cell_vmin, vd->cell_delta_v,
        vd->batt_tmax, vd->batt_tmin, vd->batt_delta_t,
        vd->energy_km, vd->regen_pct,
        vd->total_energy, vd->total_regen, vd->total_distance,
        vd->drive_score, vd->drive_grade,
        (unsigned long)vd->timestamp,
        vd->obd_valid
    );

    /* snprintf 返回的是"本应写入的长度": 缓冲区不够时它会大于 buf_len。
       直接强转成 uint16_t 交给下游 memcpy 会越界读, 这里 clamp 到实际写入长度。 */
    if (n < 0 || buf_len == 0) return 0;
    if (n >= (int)buf_len) return (uint16_t)(buf_len - 1);
    return (uint16_t)n;
}

void HTTPD_SendJSONResponse(uint8_t conn_id, const char *json, uint16_t json_len) {
    char header[128];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n"
        "Content-Length: %d\r\nConnection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n\r\n",
        json_len);

    char full[HTTP_BUF_SIZE];
    if (hlen + json_len < HTTP_BUF_SIZE) {
        memcpy(full, header, hlen);
        memcpy(full + hlen, json, json_len);
        /* 发失败时 ESP8266_SendData 内部已经关了连接, 这里只记日志。
           原实现不看返回值: JSON 发丢了浏览器拿到的是空响应, 无从判断。 */
        if (ESP8266_SendData(conn_id, full, (uint16_t)(hlen + json_len)) != 0)
            SDLog_Log("[WARN] /data response send failed");
    } else {
        /* 装不下就一条也发不出去。必须主动关连接, 否则浏览器一直等到 ESP 超时。 */
        SDLog_Log("[WARN] /data response too large, dropped");
        ESP8266_CloseConn(conn_id);
    }
}

/* 下发内嵌 HUD 页面 (约 16.5KB)。
   不能用 malloc 拼一个大缓冲: 单次 AT+CIPSEND 装不下, 且 32KB 堆会被吃掉一半。
   改为 "HTTP 头 + 正文" 分块连续发送, 全部发完再关闭连接。 */
void HTTPD_SendHTMLResponse(uint8_t conn_id, const char *html, uint16_t html_len) {
    char header[128];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %u\r\nConnection: close\r\n"
        "Cache-Control: no-store\r\n\r\n",
        (unsigned)html_len);

    if (ESP8266_SendChunk(conn_id, header, (uint16_t)hlen) != 0) {
        ESP8266_CloseConn(conn_id);
        return;
    }
    /* 正文 16.5KB 会分成 17 个 AT+CIPSEND 块, 中途任一块失败都返回 -1。
       原实现忽略返回值: 页面只发了半截就关连接, 浏览器白屏, 日志里毫无线索。 */
    if (ESP8266_SendChunk(conn_id, html, html_len) != 0)
        SDLog_Log("[WARN] HTML body send failed");
    ESP8266_CloseConn(conn_id);
}

void HTTPD_HandleRequest(uint8_t conn_id, const char *request) {
    if (strncmp(request, "GET /data", 9) == 0) {
        char json[512];
        VehicleData_t vd;

        SharedData_Lock();
        memcpy(&vd, &g_vehicle_data, sizeof(VehicleData_t));
        /* 本机没有 RTC/GPS: 这里只上报"上电秒数"(用于诊断与数据新鲜度判断),
           界面上的绝对时间由浏览器本地时间渲染, 不再伪造 1970/2024 时间戳 */
        vd.timestamp = HAL_GetTick() / 1000;
        SharedData_Unlock();

        uint16_t json_len = HTTPD_BuildJSON(&vd, json, sizeof(json));
        HTTPD_SendJSONResponse(conn_id, json, json_len);
    } else if (strncmp(request, "GET /scan", 9) == 0) {
        /* 调试入口: 触发一次 OBD PID 扫描, 结果写入 SD:/obd_scan.log。
           注意扫描会阻塞约 6 秒 (30 个 PID × 200ms), 期间不响应 ESP8266,
           只用于提车后核对 PID 定义, 日常不要点。 */
        const char *body = "PID scan started. Result -> SD:/obd_scan.log\n";
        char hdr[160];
        int hlen = snprintf(hdr, sizeof(hdr),
            "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n"
            "Content-Length: %u\r\nConnection: close\r\n\r\n",
            (unsigned)strlen(body));
        if (ESP8266_SendChunk(conn_id, hdr, (uint16_t)hlen) != 0 ||
            ESP8266_SendChunk(conn_id, body, (uint16_t)strlen(body)) != 0)
            SDLog_Log("[WARN] /scan response send failed");
        ESP8266_CloseConn(conn_id);
        CAN_OBD_ScanPIDs();
    } else if (strncmp(request, "GET / ", 6) == 0 ||
               strncmp(request, "GET /HTTP", 8) == 0 ||
               strncmp(request, "GET /index", 10) == 0) {
        HTTPD_SendHTMLResponse(conn_id, web_page_html, (uint16_t)web_page_html_len);
    } else {
        const char *not_found = "HTTP/1.1 404 Not Found\r\n\r\n";
        ESP8266_SendData(conn_id, not_found, strlen(not_found));
    }
}
