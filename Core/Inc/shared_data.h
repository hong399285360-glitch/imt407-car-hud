#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"

typedef struct {
    /* OBD - 驾驶数据 */
    float speed;
    float soc;
    float voltage;
    float current;
    float power;
    float motor_rpm;
    float throttle;
    float brake;

    /* OBD - 温度三件套 */
    float motor_temp;
    float igbt_temp;
    float temp_batt;

    /* OBD - 电池健康 */
    float cell_vmax;
    float cell_vmin;
    float cell_delta_v;
    float batt_tmax;
    float batt_tmin;
    float batt_delta_t;

    /* GPS (保留字段, 暂不使用) */
    float lat, lon;
    float altitude;
    float gps_speed;

    /* IMU (保留字段, 暂不使用) */
    float accel_x, accel_y, accel_z;
    float gyro_x, gyro_y, gyro_z;
    float grade;

    /* 能耗 + 行程统计 */
    float energy_km;
    float regen_pct;
    float total_energy;
    float total_regen;
    float total_distance;

    /* 驾驶效率评分 */
    float drive_score;
    uint8_t drive_grade;

    uint32_t timestamp;
    uint8_t obd_valid;
    uint8_t gps_valid;
} VehicleData_t;

extern VehicleData_t g_vehicle_data;
extern osMutexId_t g_data_mutex;

void SharedData_Init(void);
void SharedData_Lock(void);
void SharedData_Unlock(void);

/* 把 OBD 任务算好的数据合并回全局共享区。
   dst 中只有 CAN 拥有的字段会被覆盖, 能耗/评分等字段保持不变 ——
   否则会把本任务两次加锁之间其它任务(Energy 等)的更新冲掉。 */
void SharedData_MergeOBD(VehicleData_t *dst, const VehicleData_t *src);

#endif
