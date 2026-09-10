#include "shared_data.h"
#include <string.h>

VehicleData_t g_vehicle_data;
osMutexId_t g_data_mutex;

void SharedData_Init(void) {
    memset(&g_vehicle_data, 0, sizeof(VehicleData_t));
    /* 失败时保持 NULL: 下面 Lock/Unlock 会跳过, 不会拿 NULL 去调用
       osMutexAcquire (那是未定义行为)。真正的兜底在 TaskConfig_Init():
       同一块 heap 里后续创建任务也会失败, 那里会进 Error_Handler。 */
    g_data_mutex = osMutexNew(NULL);
}

void SharedData_Lock(void) {
    if (g_data_mutex == NULL) return;
    osMutexAcquire(g_data_mutex, osWaitForever);
}

void SharedData_Unlock(void) {
    if (g_data_mutex == NULL) return;
    osMutexRelease(g_data_mutex);
}

void SharedData_MergeOBD(VehicleData_t *dst, const VehicleData_t *src) {
    /* 只覆盖 CAN_OBD_Update() 会写的字段。
       energy_* / total_* / regen_pct / drive_score / drive_grade 归 Energy 任务所有,
       GPS/IMU 保留字段归各自模块所有 —— 合并时一律不动, 否则会把
       本任务两次加锁之间其它任务刚写的值冲回旧快照。

       注: power 同时被 Energy_GetStats() 兜底写过, 这里以 OBD 为准
       (OBD 内部已做 V×I 兜底), 最坏情况是丢失一次 Energy 的兜底值,
       下一轮 100ms 内即恢复。 */
    dst->speed      = src->speed;
    dst->soc        = src->soc;
    dst->voltage    = src->voltage;
    dst->current    = src->current;
    dst->power      = src->power;
    dst->motor_rpm  = src->motor_rpm;
    dst->throttle   = src->throttle;
    dst->brake      = src->brake;

    dst->motor_temp = src->motor_temp;
    dst->igbt_temp  = src->igbt_temp;
    dst->temp_batt  = src->temp_batt;

    dst->cell_vmax    = src->cell_vmax;
    dst->cell_vmin    = src->cell_vmin;
    dst->cell_delta_v = src->cell_delta_v;
    dst->batt_tmax    = src->batt_tmax;
    dst->batt_tmin    = src->batt_tmin;
    dst->batt_delta_t = src->batt_delta_t;

    dst->obd_valid = src->obd_valid;
    dst->timestamp = src->timestamp;
}
