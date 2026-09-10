#include "sdlog.h"
#include "ff.h"
#include <stdio.h>

static FATFS s_fs;
static FIL s_file;
static FIL s_log_file;
static uint8_t s_mounted = 0;
static uint8_t s_file_ok = 0;
static uint8_t s_log_ok = 0;
static uint32_t s_write_count = 0;

/* FatFs 串行化锁。
   本工程 FatFs 配置为 FF_FS_REENTRANT=0(见 ffconf.h), 即 FatFs 自身
   不做任何互斥。而 SDLog_Write() 跑在 SDLog 任务, SDLog_Log() 由 HTTPD
   任务的 GET /scan 调用 —— 两个任务共用同一个 FATFS 卷对象 s_fs
   (win[]、winsect 等卷内状态), 同时进入 f_write/f_sync 会破坏文件系统。
   本锁覆盖 sdlog.c 内全部 FatFs 调用(diskio.c 只是被 FatFs 调用的底层块驱动)。 */
static osMutexId_t s_sd_mutex = NULL;

void SDLog_Init(void) {
    /* 先建锁再挂载: 建不出来就干脆不挂载, 宁可不记录也不让两个任务
       裸奔并发写 SD。本函数在 osKernelStart() 之前执行, 此时无并发,
       故下面这段初始化不需要加锁。 */
    s_sd_mutex = osMutexNew(NULL);
    if (s_sd_mutex == NULL)
        return;

    if (f_mount(&s_fs, "0:", 1) != FR_OK)
        return;
    s_mounted = 1;

    if (f_open(&s_file, "0:/hud_log.csv", FA_OPEN_ALWAYS | FA_WRITE) != FR_OK)
        return;                       /* 主日志打不开: 保持 s_file_ok=0, 后续写入直接跳过 */
    s_file_ok = 1;

    f_lseek(&s_file, f_size(&s_file));

    if (f_size(&s_file) == 0) {
        f_printf(&s_file,
            "ts,speed,soc,voltage,current,power,motor_rpm,throttle,brake,"
            "motor_temp,igbt_temp,batt_temp,"
            "cell_vmax,cell_vmin,cell_dv,batt_tmax,batt_tmin,batt_dt,"
            "energy_km,regen_pct,total_energy,total_regen,total_distance,"
            "drive_score,drive_grade\n");
    }
    f_sync(&s_file);

    /* 扫描日志是可选文件: 打开失败不影响主日志 */
    if (f_open(&s_log_file, "0:/obd_scan.log", FA_OPEN_ALWAYS | FA_WRITE) == FR_OK) {
        s_log_ok = 1;
        f_lseek(&s_log_file, f_size(&s_log_file));
        f_sync(&s_log_file);
    }
}

void SDLog_Write(VehicleData_t *vd) {
    if (!s_mounted || !s_file_ok || s_sd_mutex == NULL) return;
    if (osMutexAcquire(s_sd_mutex, osWaitForever) != osOK) return;

    if (f_printf(&s_file,
        "%lu,%.1f,%.0f,%.1f,%.1f,%.1f,%.0f,%.0f,%.0f,"
        "%.0f,%.0f,%.0f,"
        "%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,"
        "%.1f,%.0f,%.2f,%.2f,%.1f,"
        "%.0f,%d\n",
        (unsigned long)vd->timestamp,
        vd->speed, vd->soc, vd->voltage, vd->current,
        vd->power, vd->motor_rpm, vd->throttle, vd->brake,
        vd->motor_temp, vd->igbt_temp, vd->temp_batt,
        vd->cell_vmax, vd->cell_vmin, vd->cell_delta_v,
        vd->batt_tmax, vd->batt_tmin, vd->batt_delta_t,
        vd->energy_km, vd->regen_pct,
        vd->total_energy, vd->total_regen, vd->total_distance,
        vd->drive_score, vd->drive_grade) < 0) {
        /* f_printf 成功返回写入字符数, 失败返回 -1 (见 ff.c putc_flush)。
           注意不能写成 != FR_OK: FR_OK==0, 成功时返回值恒 != 0, 会把日志永久关掉。
           卡被拔出 / 写失败时关闭写入, 否则每 1s 都要等一次 SDIO 超时, 拖慢 OBD 任务。 */
        s_file_ok = 0;
    } else {
        s_write_count++;
        if (s_write_count % 10 == 0)
            f_sync(&s_file);
    }

    osMutexRelease(s_sd_mutex);
}

void SDLog_Sync(void) {
    if (!s_mounted || !s_file_ok || s_sd_mutex == NULL) return;
    if (osMutexAcquire(s_sd_mutex, osWaitForever) != osOK) return;
    f_sync(&s_file);
    osMutexRelease(s_sd_mutex);
}

void SDLog_Log(const char *msg) {
    if (!s_mounted || !s_log_ok || s_sd_mutex == NULL) return;
    if (osMutexAcquire(s_sd_mutex, osWaitForever) != osOK) return;
    f_printf(&s_log_file, "%s\n", msg);
    f_sync(&s_log_file);
    osMutexRelease(s_sd_mutex);
}
