#ifndef HTTPD_H
#define HTTPD_H

#include "shared_data.h"
#include "wifi.h"

#define HTTP_BUF_SIZE 1024

void HTTPD_HandleRequest(uint8_t conn_id, const char *request);
uint16_t HTTPD_BuildJSON(VehicleData_t *vd, char *buf, uint16_t buf_len);
void HTTPD_SendJSONResponse(uint8_t conn_id, const char *json, uint16_t json_len);
void HTTPD_SendHTMLResponse(uint8_t conn_id, const char *html, uint16_t html_len);

#endif
