#include "energy.h"
#include "stm32f4xx_hal.h"

EnergyState g_energy;

static uint32_t s_last_tick = 0;

void Energy_Init(void) {
    g_energy.power_kw = 0;
    g_energy.energy_kwh = 0;
    g_energy.regen_kwh = 0;
    g_energy.distance_km = 0;
    g_energy.energy_per_km = 0;

    g_energy.drive_score = 100.0f;
    g_energy.drive_grade = 1;
    g_energy.hard_accel_count = 0;
    g_energy.hard_brake_count = 0;
    g_energy.regen_count = 0;
    g_energy.smooth_count = 0;
    g_energy.last_speed = 0;
    g_energy.last_throttle = 0;
    g_energy.last_eval_time = 0;

    s_last_tick = HAL_GetTick();
}

void Energy_Update(float voltage, float current, float speed_kmh) {
    uint32_t now = HAL_GetTick();
    float dt = (float)(now - s_last_tick) / 1000.0f;
    s_last_tick = now;

    if (dt <= 0 || dt > 5.0f) dt = 0.1f;

    g_energy.power_kw = voltage * current / 1000.0f;

    float dE = g_energy.power_kw * dt / 3600.0f;

    if (g_energy.power_kw > 0)
        g_energy.energy_kwh += dE;
    else
        g_energy.regen_kwh += -dE;

    g_energy.distance_km += speed_kmh * dt / 3600.0f;

    if (g_energy.distance_km > 0.01f) {
        g_energy.energy_per_km =
            (g_energy.energy_kwh - g_energy.regen_kwh) / g_energy.distance_km * 100.0f;
    }
}

void Energy_GetStats(VehicleData_t *vd) {
    /* 功率: 优先用CAN OBD PID直接读到的值, 没读到才用V×I计算 */
    if (vd->power == 0 && g_energy.power_kw != 0)
        vd->power = g_energy.power_kw;
    vd->energy_km = g_energy.energy_per_km;
    vd->total_energy = g_energy.energy_kwh;
    vd->total_regen = g_energy.regen_kwh;
    vd->total_distance = g_energy.distance_km;

    float total = g_energy.energy_kwh + g_energy.regen_kwh;
    if (total > 0.001f)
        vd->regen_pct = g_energy.regen_kwh / total * 100.0f;
    else
        vd->regen_pct = 0;

    /* === 驾驶效率评分 — 每5秒评估一次 === */
    uint32_t now = HAL_GetTick();

    if (g_energy.last_eval_time == 0) {
        g_energy.last_eval_time = now;
        g_energy.last_speed = vd->speed;
        g_energy.last_throttle = vd->throttle;
    }

    if (now - g_energy.last_eval_time >= 5000) {
        float dSpeed = vd->speed - g_energy.last_speed;

        /* 急加速: 油门>80% 且 功率>50kW */
        if (vd->throttle > 80.0f && vd->power > 50.0f) {
            g_energy.drive_score -= 2.0f;
            g_energy.hard_accel_count++;
        }

        /* 急制动: 制动>50% 且 速度>60 */
        if (vd->brake > 50.0f && vd->speed > 60.0f) {
            g_energy.drive_score -= 3.0f;
            g_energy.hard_brake_count++;
        }

        /* 回收奖励: 功率<0(回充) 且 制动<30%(滑行) */
        if (vd->power < 0.0f && vd->brake < 30.0f) {
            g_energy.drive_score += 1.0f;
            g_energy.regen_count++;
        }

        /* 匀速奖励: 5秒内速度变化<2km/h 且 速度>20(非停车) */
        if (dSpeed > -2.0f && dSpeed < 2.0f && vd->speed > 20.0f) {
            g_energy.drive_score += 0.5f;
            g_energy.smooth_count++;
        }

        if (g_energy.drive_score < 0) g_energy.drive_score = 0;
        if (g_energy.drive_score > 100) g_energy.drive_score = 100;

        if (g_energy.drive_score >= 90) g_energy.drive_grade = 1;      /* A */
        else if (g_energy.drive_score >= 80) g_energy.drive_grade = 2; /* B */
        else if (g_energy.drive_score >= 70) g_energy.drive_grade = 3; /* C */
        else if (g_energy.drive_score >= 60) g_energy.drive_grade = 4; /* D */
        else g_energy.drive_grade = 5;                                  /* E */

        g_energy.last_speed = vd->speed;
        g_energy.last_throttle = vd->throttle;
        g_energy.last_eval_time = now;
    }

    vd->drive_score = g_energy.drive_score;
    vd->drive_grade = g_energy.drive_grade;
}
