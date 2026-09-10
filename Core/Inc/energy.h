#ifndef ENERGY_H
#define ENERGY_H

#include "shared_data.h"

typedef struct {
    float power_kw;
    float energy_kwh;
    float regen_kwh;
    float distance_km;
    float energy_per_km;

    /* 驾驶评分 */
    float drive_score;
    uint8_t drive_grade;
    uint8_t hard_accel_count;
    uint8_t hard_brake_count;
    uint8_t regen_count;
    uint8_t smooth_count;

    /* 评分用的历史数据 */
    float last_speed;
    float last_throttle;
    uint32_t last_eval_time;
} EnergyState;

extern EnergyState g_energy;

void Energy_Init(void);
void Energy_Update(float voltage, float current, float speed_kmh);
void Energy_GetStats(VehicleData_t *vd);

#endif
