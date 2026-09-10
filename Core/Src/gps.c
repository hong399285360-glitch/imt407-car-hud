#include "gps.h"
#include <string.h>
#include <stdlib.h>
#include "cmsis_os2.h"

GPS_t g_gps;

static float parse_nmea_coord(const char *str, uint8_t is_lon) {
    if (!str || !*str) return 0;
    int deg_digits = is_lon ? 3 : 2;
    float deg = 0;
    int i;
    for (i = 0; i < deg_digits && str[i] && str[i] != '.'; i++) {
        deg = deg * 10 + (str[i] - '0');
    }
    float min_val = 0;
    if (str[i]) {
        min_val = strtof(&str[i], NULL);
    }
    return deg + min_val / 60.0f;
}

static char *get_field(char *str, uint8_t index) {
    if (!str) return NULL;
    char *p = str;
    uint8_t cur = 0;
    while (cur < index && p) {
        p = strchr(p, ',');
        if (!p) return NULL;
        p++;
        cur++;
    }
    return p;
}

void GPS_Init(UART_HandleTypeDef *huart, UART_RingBuf_t *rxbuf) {
    g_gps.huart = huart;
    g_gps.rxbuf = rxbuf;
    g_gps.line_len = 0;

    const char *pmtk = "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n";
    HAL_UART_Transmit(huart, (uint8_t *)pmtk, strlen(pmtk), 200);
    HAL_Delay(100);

    const char *rate = "$PMTK220,1000*1F\r\n";
    HAL_UART_Transmit(huart, (uint8_t *)rate, strlen(rate), 200);
}

void GPS_ParseLine(char *line) {
    char *p;
    p = strchr(line, '*');
    if (p) *p = '\0';

    if (strncmp(line, "$GPRMC", 6) == 0 || strncmp(line, "$GNRMC", 6) == 0) {
        char *status = get_field(line, 2);
        if (!status || status[0] != 'A') return;

        char *lat = get_field(line, 3);
        char *lat_dir = get_field(line, 4);
        char *lon = get_field(line, 5);
        char *lon_dir = get_field(line, 6);
        char *speed = get_field(line, 7);

        if (!lat || !*lat || !lon || !*lon) return;

        float lat_deg = parse_nmea_coord(lat, 0);
        if (lat_dir && lat_dir[0] == 'S') lat_deg = -lat_deg;

        float lon_deg = parse_nmea_coord(lon, 1);
        if (lon_dir && lon_dir[0] == 'W') lon_deg = -lon_deg;

        float spd = 0;
        if (speed && *speed) spd = strtof(speed, NULL);

        SharedData_Lock();
        g_vehicle_data.lat = lat_deg;
        g_vehicle_data.lon = lon_deg;
        g_vehicle_data.gps_speed = spd * 1.852f;
        g_vehicle_data.gps_valid = 1;
        SharedData_Unlock();
    }

    if (strncmp(line, "$GPGGA", 6) == 0 || strncmp(line, "$GNGGA", 6) == 0) {
        char *alt = get_field(line, 9);
        if (alt && *alt) {
            float altitude = strtof(alt, NULL);
            SharedData_Lock();
            g_vehicle_data.altitude = altitude;
            SharedData_Unlock();
        }
    }
}

void GPS_Update(VehicleData_t *vd) {
    uint8_t ch;
    while (UART_RingBuf_Read(g_gps.rxbuf, &ch, 1) > 0) {
        if (ch == '\n') {
            g_gps.line[g_gps.line_len] = '\0';
            GPS_ParseLine(g_gps.line);
            g_gps.line_len = 0;
        } else if (ch != '\r') {
            if (g_gps.line_len < sizeof(g_gps.line) - 1) {
                g_gps.line[g_gps.line_len++] = ch;
            }
        }
    }

    vd->lat = g_vehicle_data.lat;
    vd->lon = g_vehicle_data.lon;
    vd->altitude = g_vehicle_data.altitude;
    vd->gps_speed = g_vehicle_data.gps_speed;
    vd->gps_valid = g_vehicle_data.gps_valid;
}
