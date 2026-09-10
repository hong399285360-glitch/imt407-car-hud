#ifndef CAN_OBD_H
#define CAN_OBD_H

#include "stm32f4xx_hal.h"
#include "shared_data.h"

/* ==================== CAN OBD 配置 ==================== */

/* CAN ID 定义 (ISO 15765-4) */
#define OBD_REQ_ID_FUNC     0x7DF    /* 功能寻址(广播) */
#define OBD_REQ_ID_PHY      0x7E0    /* 物理寻址(ECU #1) */
#define OBD_RESP_ID         0x7E8    /* ECU 响应ID */

/* OBD Service 定义 */
#define SID_SERVICE_01      0x01     /* 标准PID (SAE J1979) */
#define SID_SERVICE_05      0x05     /* 中国新能源车数据流 */
#define SID_SERVICE_22      0x22     /* UDS增强诊断 */

/* 响应SID (请求SID + 0x40) */
#define SID_RESP_01         0x41
#define SID_RESP_05         0x45
#define SID_RESP_22         0x62

/* ==================== PID 定义 ==================== */

/* Service 01 标准PID */
#define PID_S01_SPEED       0x0D     /* 车速 */
#define PID_S01_THROTTLE    0x11     /* 节气门位置 */
#define PID_S01_RUNTIME     0x1F     /* 启动后运行时间 */

/* Service 05 国标新能源PID */
#define PID_S05_SOC         0x03     /* 电池SOC: 0.392×raw, % */
#define PID_S05_SPEED       0x05     /* 车速: 1×raw, km/h */
#define PID_S05_AMBIENT     0x06     /* 环境温度: raw-40, °C */
#define PID_S05_MOTOR_TEMP  0x07     /* 电机温度: raw-40, °C */
#define PID_S05_IGBT_TEMP   0x08     /* IGBT温度: raw-40, °C */
#define PID_S05_THROTTLE    0x09     /* 加速踏板: 0.392×raw, % */
#define PID_S05_BRAKE       0x0A     /* 制动踏板: 0.392×raw, % */
#define PID_S05_CELL_VMAX   0x0B     /* 单体电压最高: 0.06867×raw, mV (16bit) */
#define PID_S05_CELL_VMIN   0x0E     /* 单体电压最低: 0.06867×raw, mV (16bit) */
#define PID_S05_BATT_TMAX   0x11     /* 电池温度最高: raw-40, °C */
#define PID_S05_BATT_TMIN   0x14     /* 电池温度最低: raw-40, °C */
#define PID_S05_MOTOR_RPM   0x1A     /* 电机转速: 0.02441×raw-8000, rpm (16bit) */
#define PID_S05_BUS_VOLT    0x1B     /* 母线电压: 0.01526×raw, V (16bit) */
#define PID_S05_BUS_CURR    0x1C     /* 母线电流: 0.01526×raw-500, A (16bit) */
#define PID_S05_MOTOR_PWR   0x21     /* 电机功率: 0.03052×raw-1000, kW (16bit) */
#define PID_S05_MC_VOLT     0x22     /* 电机控制器输入电压: 0.01526×raw, V (16bit) */

/* ==================== ISO 15765-2 帧类型 ==================== */

#define FRAME_TYPE_SF       0x00     /* 单帧: 高4位=0 */
#define FRAME_TYPE_FF       0x10     /* 首帧: 高4位=1 */
#define FRAME_TYPE_CF       0x20     /* 连续帧: 高4位=2 */
#define FRAME_TYPE_FC       0x30     /* 流控帧: 高4位=3 */

#define FC_STATUS_CONTINUE  0x00     /* ContinueToSend */
#define FC_STATUS_WAIT      0x01     /* Wait */
#define FC_STATUS_OVERFLOW  0x02     /* Overflow */

#define MAX_OBD_DATA_LEN    64       /* 最大OBD数据长度 */
#define OBD_RESP_TIMEOUT    100      /* 响应超时 ms */
#define OBD_FC_TIMEOUT      50       /* 流控等待超时 ms */

/* ==================== 数据结构 ==================== */

typedef struct {
    uint8_t data[MAX_OBD_DATA_LEN];
    uint16_t len;
} OBD_Response_t;

typedef struct {
    CAN_HandleTypeDef *hcan;
    uint8_t initialized;
    uint8_t rx_buf[MAX_OBD_DATA_LEN];
    uint16_t rx_len;
    uint16_t rx_expected;
    uint8_t multi_frame_active;
    uint8_t next_seq;
    uint32_t last_rx_time;
    uint32_t ok_count;      /* 累计收到配对正确响应的次数, 用于判断链路有效性 */
} CAN_OBD_t;

extern CAN_OBD_t g_can_obd;

/* ==================== 接口函数 ==================== */

void CAN_OBD_Init(CAN_HandleTypeDef *hcan);
void CAN_OBD_Update(VehicleData_t *vd);
void CAN_OBD_RxHandler(CAN_HandleTypeDef *hcan);

int8_t CAN_OBD_SendRequest(uint8_t service, uint8_t pid, OBD_Response_t *resp);
int8_t CAN_OBD_SendRequestExt(uint8_t service, uint16_t pid, OBD_Response_t *resp);

/* PID扫描模式 */
void CAN_OBD_ScanPIDs(void);

/* PID解析函数 */
float CAN_OBD_ParseS01(uint8_t pid, uint8_t *raw, uint8_t len);
float CAN_OBD_ParseS05(uint8_t pid, uint8_t *raw, uint8_t len);

#endif
