#include "can_obd.h"
#include "cmsis_os2.h"
#include <string.h>
#include <stdio.h>
#include "sdlog.h"

CAN_OBD_t g_can_obd;

/* CAN 请求串行化锁。
   CAN_OBD_Update()  (OBD 任务, 100ms 周期) 与
   CAN_OBD_ScanPIDs() (HTTPD 任务, GET /scan) 共用 g_can_obd 的
   rx_buf / rx_len / multi_frame_active。两者并发时, 一方发请求、
   另一方收到响应, 会把响应错配给错误的 PID —— 数据张冠李戴。
   本锁把"发请求 → 等响应 → 取数据"整段串行化, 粗粒度但正确。
   锁顺序: 只有 Update 会先持 data 锁再持本锁, 不存在反向获取, 无死锁。 */
static osMutexId_t s_can_req_mutex = NULL;

/* ==================== CAN 初始化 ==================== */

static void CAN_FilterConfig(CAN_HandleTypeDef *hcan) {
    CAN_FilterTypeDef filter;

    /* 只接收 ID=0x7E8 的标准帧 (ECU响应) */
    filter.FilterIdHigh = (OBD_RESP_ID << 5) & 0xFFFF;
    filter.FilterIdLow = 0x0000;
    filter.FilterMaskIdHigh = 0x07FF << 5;
    filter.FilterMaskIdLow = 0x0000;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterBank = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterActivation = CAN_FILTER_ENABLE;

    HAL_CAN_ConfigFilter(hcan, &filter);
}

void CAN_OBD_Init(CAN_HandleTypeDef *hcan) {
    g_can_obd.hcan = hcan;
    g_can_obd.initialized = 0;
    g_can_obd.multi_frame_active = 0;
    g_can_obd.rx_len = 0;
    g_can_obd.rx_expected = 0;
    g_can_obd.next_seq = 1;
    g_can_obd.ok_count = 0;

    /* 请求锁必须在使能 CAN 中断之前就绪, 否则中断已开始收帧,
       Update/ScanPIDs 却拿不到锁, 串行化形同虚设。 */
    s_can_req_mutex = osMutexNew(NULL);
    if (s_can_req_mutex == NULL) {
        /* 创建失败 → 不置 initialized, Update/ScanPIDs 直接返回。
           宁可 OBD 数据不刷新, 也不让两路请求互相踩内存。 */
        return;
    }

    CAN_FilterConfig(hcan);

    HAL_CAN_Start(hcan);

    HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING);

    g_can_obd.initialized = 1;
}

/* ==================== ISO 15765-2 帧处理 ==================== */

void CAN_OBD_RxHandler(CAN_HandleTypeDef *hcan) {
    CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];
    uint16_t dl;

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data) != HAL_OK)
        return;

    /* 只处理标准帧, ID=0x7E8 */
    if (rx_header.StdId != OBD_RESP_ID)
        return;

    g_can_obd.last_rx_time = HAL_GetTick();

    uint8_t frame_type = rx_data[0] & 0xF0;

    switch (frame_type) {
        case FRAME_TYPE_SF:
            /* 单帧: [长度(4bit), 数据(7byte)] */
            dl = rx_data[0] & 0x0F;
            if (dl > 7) dl = 7;

            memcpy(g_can_obd.rx_buf, &rx_data[1], dl);
            g_can_obd.rx_len = dl;
            g_can_obd.multi_frame_active = 0;
            break;

        case FRAME_TYPE_FF:
            /* 首帧: [0x1X, 长度高4bit, 长度低8bit, 数据(6byte)] */
            g_can_obd.rx_expected = ((uint16_t)(rx_data[0] & 0x0F) << 8) | rx_data[1];
            if (g_can_obd.rx_expected > MAX_OBD_DATA_LEN)
                g_can_obd.rx_expected = MAX_OBD_DATA_LEN;

            dl = 6;
            memcpy(g_can_obd.rx_buf, &rx_data[2], dl);
            g_can_obd.rx_len = dl;
            g_can_obd.multi_frame_active = 1;
            g_can_obd.next_seq = 1;

            /* 发送流控帧: ContinueToSend, BS=0, STmin=0 */
            {
                uint8_t fc_frame[8] = {0};
                fc_frame[0] = FRAME_TYPE_FC | FC_STATUS_CONTINUE;
                /* fc_frame[1] = Block Size (0=无限) */
                /* fc_frame[2] = STmin (0=无延迟) */

                CAN_TxHeaderTypeDef tx_header;
                tx_header.StdId = OBD_REQ_ID_PHY;
                tx_header.RTR = CAN_RTR_DATA;
                tx_header.IDE = CAN_ID_STD;
                tx_header.DLC = 8;
                tx_header.TransmitGlobalTime = 0;

                uint32_t tx_mailbox;
                HAL_CAN_AddTxMessage(hcan, &tx_header, fc_frame, &tx_mailbox);
            }
            break;

        case FRAME_TYPE_CF:
            /* 连续帧: [序列号(4bit), 数据(7byte)] */
            if (!g_can_obd.multi_frame_active)
                break;

            uint8_t seq = rx_data[0] & 0x0F;

            /* 已收满 → 本次多帧接收结束, 多余 CF 直接丢弃 */
            if (g_can_obd.rx_len >= g_can_obd.rx_expected) {
                g_can_obd.multi_frame_active = 0;
                break;
            }

            /* ISO 15765-2: SN 从 1 开始递增, 15 之后回绕到 0。
               SN 不连续说明中间丢了帧, 后续字节位置全部错位, 拼出来的是
               "看起来完整、实则错位"的假数据 —— 必须整条丢弃。 */
            if (seq != g_can_obd.next_seq) {
                g_can_obd.multi_frame_active = 0;
                g_can_obd.rx_len = 0;
                break;
            }
            g_can_obd.next_seq = (g_can_obd.next_seq + 1) & 0x0F;

            dl = 7;
            uint16_t remaining = g_can_obd.rx_expected - g_can_obd.rx_len;
            if (dl > remaining) dl = remaining;

            memcpy(&g_can_obd.rx_buf[g_can_obd.rx_len], &rx_data[1], dl);
            g_can_obd.rx_len += dl;

            if (g_can_obd.rx_len >= g_can_obd.rx_expected) {
                g_can_obd.multi_frame_active = 0;
            }
            break;

        default:
            break;
    }
}

/* ==================== 请求发送 ==================== */

static int8_t CAN_OBD_SendRaw(uint8_t *data, uint8_t len) {
    if (len > 8) len = 8;

    CAN_TxHeaderTypeDef tx_header;
    tx_header.StdId = OBD_REQ_ID_FUNC;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.IDE = CAN_ID_STD;
    tx_header.DLC = 8;
    tx_header.TransmitGlobalTime = 0;

    uint8_t frame[8] = {0};
    memcpy(frame, data, len);

    uint32_t tx_mailbox;
    uint32_t start = HAL_GetTick();

    /* 等待空邮箱改为 osDelay 让出 CPU: 原忙等会让本任务独占 CPU 达 50ms,
       期间低优先级任务 (SDLog) 完全得不到运行。
       本函数只在任务上下文被调用 (Update / ScanPIDs), 可安全阻塞。 */
    while (HAL_CAN_GetTxMailboxesFreeLevel(g_can_obd.hcan) == 0) {
        if (HAL_GetTick() - start > 50) return -1;
        osDelay(1);
    }

    if (HAL_CAN_AddTxMessage(g_can_obd.hcan, &tx_header, frame, &tx_mailbox) != HAL_OK)
        return -1;

    return 0;
}

int8_t CAN_OBD_SendRequest(uint8_t service, uint8_t pid, OBD_Response_t *resp) {
    if (!g_can_obd.initialized) return -1;

    /* 清空接收缓冲 */
    g_can_obd.rx_len = 0;
    g_can_obd.multi_frame_active = 0;

    /* 构建单帧请求: [长度, SID, PID, ...] */
    uint8_t req[8] = {0};
    req[0] = 0x02;             /* 单帧, 2字节数据 */
    req[1] = service;
    req[2] = pid;

    if (CAN_OBD_SendRaw(req, 8) != 0)
        return -1;

    /* 等待响应 */
    const uint8_t exp_sid = (uint8_t)(service + 0x40);
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < OBD_RESP_TIMEOUT) {
        /* rx_len >= 2 才算有效: 响应至少要有 [SID, PID] 两个字节,
           否则调用方的 resp.len - 2 会下溢 (uint8_t 回绕成 254) */
        if (!g_can_obd.multi_frame_active && g_can_obd.rx_len >= 2) {
            /* 负响应 (ISO 14229): 7F <SID> <NRC> —— ECU 明确拒绝该 PID,
               立即失败, 不必白等满 100ms (15 个 PID 一轮能省下大量时间) */
            if (g_can_obd.rx_buf[0] == 0x7F)
                return -1;

            /* 请求/响应配对校验: 必须回显本次请求的 SID 与 PID。
               否则总线上的迟到响应 (上一轮的) 或其它 ECU 的帧会被当成
               这次的结果 —— 例如把 SOC 的 4 字节当母线电压解析, 数值错乱。 */
            if (g_can_obd.rx_buf[0] != exp_sid || g_can_obd.rx_buf[1] != pid) {
                g_can_obd.rx_len = 0;   /* 丢弃不匹配帧, 继续等本次响应 */
                osDelay(1);
                continue;
            }

            /* 收到完整且配对正确的响应 */
            uint16_t copy_len = g_can_obd.rx_len;
            if (copy_len > MAX_OBD_DATA_LEN) copy_len = MAX_OBD_DATA_LEN;
            memcpy(resp->data, g_can_obd.rx_buf, copy_len);
            resp->len = copy_len;
            g_can_obd.ok_count++;
            return 0;
        }
        osDelay(1);
    }

    return -1;
}

int8_t CAN_OBD_SendRequestExt(uint8_t service, uint16_t pid, OBD_Response_t *resp) {
    if (!g_can_obd.initialized) return -1;

    g_can_obd.rx_len = 0;
    g_can_obd.multi_frame_active = 0;

    /* 扩展PID (2字节): [长度, SID, PID_H, PID_L, ...] */
    uint8_t req[8] = {0};
    req[0] = 0x03;
    req[1] = service;
    req[2] = (uint8_t)(pid >> 8);
    req[3] = (uint8_t)(pid & 0xFF);

    if (CAN_OBD_SendRaw(req, 8) != 0)
        return -1;

    /* 等待响应 */
    const uint8_t exp_sid = (uint8_t)(service + 0x40);
    const uint8_t pid_h = (uint8_t)(pid >> 8);
    const uint8_t pid_l = (uint8_t)(pid & 0xFF);
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < OBD_RESP_TIMEOUT) {
        /* 扩展 PID 的响应必须回显 [SID, PID_H, PID_L] 三字节, 故要求 >= 3 */
        if (!g_can_obd.multi_frame_active && g_can_obd.rx_len >= 3) {
            /* 负响应 (7F <SID> <NRC>) → ECU 明确拒绝, 立即失败 */
            if (g_can_obd.rx_buf[0] == 0x7F)
                return -1;

            /* 请求/响应配对校验 (同 SendRequest, 但校验两字节 PID) */
            if (g_can_obd.rx_buf[0] != exp_sid ||
                g_can_obd.rx_buf[1] != pid_h ||
                g_can_obd.rx_buf[2] != pid_l) {
                g_can_obd.rx_len = 0;
                osDelay(1);
                continue;
            }

            uint16_t copy_len = g_can_obd.rx_len;
            if (copy_len > MAX_OBD_DATA_LEN) copy_len = MAX_OBD_DATA_LEN;
            memcpy(resp->data, g_can_obd.rx_buf, copy_len);
            resp->len = copy_len;
            g_can_obd.ok_count++;
            return 0;
        }
        osDelay(1);
    }

    return -1;
}

/* ==================== PID 解析 ==================== */

float CAN_OBD_ParseS01(uint8_t pid, uint8_t *raw, uint8_t len) {
    if (len < 1) return 0;

    switch (pid) {
        case PID_S01_SPEED:
            return (float)raw[0];                    /* km/h */

        case PID_S01_THROTTLE:
            return raw[0] * 100.0f / 255.0f;         /* % */

        case PID_S01_RUNTIME:
            if (len >= 2)
                return (float)((raw[0] << 8) | raw[1]);  /* 秒 */
            return 0;

        default:
            return 0;
    }
}

float CAN_OBD_ParseS05(uint8_t pid, uint8_t *raw, uint8_t len) {
    switch (pid) {
        case PID_S05_SOC:
            if (len >= 4) {
                uint32_t val = ((uint32_t)raw[0] << 24) | ((uint32_t)raw[1] << 16) |
                               ((uint32_t)raw[2] << 8) | raw[3];
                return val * 0.392f;                  /* % */
            }
            return 0;

        case PID_S05_SPEED:
            return (len >= 1) ? (float)raw[0] : 0;   /* km/h */

        case PID_S05_AMBIENT:
            return (len >= 1) ? (float)raw[0] - 40.0f : 0;  /* °C */

        case PID_S05_MOTOR_TEMP:
            return (len >= 1) ? (float)raw[0] - 40.0f : 0;  /* °C */

        case PID_S05_IGBT_TEMP:
            return (len >= 1) ? (float)raw[0] - 40.0f : 0;  /* °C */

        case PID_S05_THROTTLE:
            return (len >= 1) ? raw[0] * 0.392f : 0;        /* % */

        case PID_S05_BRAKE:
            return (len >= 1) ? raw[0] * 0.392f : 0;        /* % */

        case PID_S05_CELL_VMAX:
        case PID_S05_CELL_VMIN:
            if (len >= 2) {
                uint16_t val = ((uint16_t)raw[0] << 8) | raw[1];
                return val * 0.06867f;                /* mV */
            }
            return 0;

        case PID_S05_BATT_TMAX:
        case PID_S05_BATT_TMIN:
            return (len >= 1) ? (float)raw[0] - 40.0f : 0;  /* °C */

        case PID_S05_MOTOR_RPM:
            if (len >= 2) {
                uint16_t val = ((uint16_t)raw[0] << 8) | raw[1];
                return val * 0.02441f - 8000.0f;     /* rpm */
            }
            return 0;

        case PID_S05_BUS_VOLT:
            if (len >= 2) {
                uint16_t val = ((uint16_t)raw[0] << 8) | raw[1];
                return val * 0.01526f;                /* V */
            }
            return 0;

        case PID_S05_BUS_CURR:
            if (len >= 2) {
                uint16_t val = ((uint16_t)raw[0] << 8) | raw[1];
                return val * 0.01526f - 500.0f;       /* A */
            }
            return 0;

        case PID_S05_MOTOR_PWR:
            if (len >= 2) {
                uint16_t val = ((uint16_t)raw[0] << 8) | raw[1];
                return val * 0.03052f - 1000.0f;      /* kW */
            }
            return 0;

        case PID_S05_MC_VOLT:
            if (len >= 2) {
                uint16_t val = ((uint16_t)raw[0] << 8) | raw[1];
                return val * 0.01526f;                /* V */
            }
            return 0;

        default:
            return 0;
    }
}

/* ==================== 数据更新 ==================== */

/* 连续多少轮完全没有有效响应, 才判定 OBD 链路失效。
   单次超时多为总线抖动或 ECU 瞬时忙, 立即归零会让 HUD 频繁闪烁"数据失效"。 */
#define OBD_FAIL_STREAK_MAX  3

void CAN_OBD_Update(VehicleData_t *vd) {
    static uint8_t s_fail_streak = 0;   /* 连续无有效响应的轮数 */

    if (!g_can_obd.initialized || s_can_req_mutex == NULL) return;

    /* 与 CAN_OBD_ScanPIDs() 串行化, 防止两路请求共用 rx_buf 互相错配。
       本函数只有开头这一个 return, 锁在末尾统一释放。 */
    osMutexAcquire(s_can_req_mutex, osWaitForever);

    OBD_Response_t resp;
    uint8_t *raw;
    uint8_t rlen;
    uint32_t ok0 = g_can_obd.ok_count;  /* 本轮开始时的成功计数快照 */

    /* === Service 05 国标PID === */

    /* 母线电压 */
    if (CAN_OBD_SendRequest(SID_SERVICE_05, PID_S05_BUS_VOLT, &resp) == 0) {
        raw = &resp.data[2];  /* 跳过 [SID, PID] */
        rlen = resp.len - 2;
        vd->voltage = CAN_OBD_ParseS05(PID_S05_BUS_VOLT, raw, rlen);
        vd->obd_valid = 1;
    }

    /* 母线电流 */
    if (CAN_OBD_SendRequest(SID_SERVICE_05, PID_S05_BUS_CURR, &resp) == 0) {
        raw = &resp.data[2];
        rlen = resp.len - 2;
        vd->current = CAN_OBD_ParseS05(PID_S05_BUS_CURR, raw, rlen);
    }

    /* 电机功率 */
    if (CAN_OBD_SendRequest(SID_SERVICE_05, PID_S05_MOTOR_PWR, &resp) == 0) {
        raw = &resp.data[2];
        rlen = resp.len - 2;
        vd->power = CAN_OBD_ParseS05(PID_S05_MOTOR_PWR, raw, rlen);
    }

    /* 如果功率读不到, 用V×I计算 */
    if (vd->power == 0 && vd->voltage > 0) {
        vd->power = vd->voltage * vd->current / 1000.0f;
    }

    /* SOC */
    if (CAN_OBD_SendRequest(SID_SERVICE_05, PID_S05_SOC, &resp) == 0) {
        raw = &resp.data[2];
        rlen = resp.len - 2;
        vd->soc = CAN_OBD_ParseS05(PID_S05_SOC, raw, rlen);
    }

    /* 电机转速 */
    if (CAN_OBD_SendRequest(SID_SERVICE_05, PID_S05_MOTOR_RPM, &resp) == 0) {
        raw = &resp.data[2];
        rlen = resp.len - 2;
        vd->motor_rpm = CAN_OBD_ParseS05(PID_S05_MOTOR_RPM, raw, rlen);
    }

    /* 电机温度 */
    if (CAN_OBD_SendRequest(SID_SERVICE_05, PID_S05_MOTOR_TEMP, &resp) == 0) {
        raw = &resp.data[2];
        rlen = resp.len - 2;
        vd->motor_temp = CAN_OBD_ParseS05(PID_S05_MOTOR_TEMP, raw, rlen);
    }

    /* IGBT温度 */
    if (CAN_OBD_SendRequest(SID_SERVICE_05, PID_S05_IGBT_TEMP, &resp) == 0) {
        raw = &resp.data[2];
        rlen = resp.len - 2;
        vd->igbt_temp = CAN_OBD_ParseS05(PID_S05_IGBT_TEMP, raw, rlen);
    }

    /* 加速踏板 */
    if (CAN_OBD_SendRequest(SID_SERVICE_05, PID_S05_THROTTLE, &resp) == 0) {
        raw = &resp.data[2];
        rlen = resp.len - 2;
        vd->throttle = CAN_OBD_ParseS05(PID_S05_THROTTLE, raw, rlen);
    }

    /* 制动踏板 */
    if (CAN_OBD_SendRequest(SID_SERVICE_05, PID_S05_BRAKE, &resp) == 0) {
        raw = &resp.data[2];
        rlen = resp.len - 2;
        vd->brake = CAN_OBD_ParseS05(PID_S05_BRAKE, raw, rlen);
    }

    /* 单体电压最高 */
    if (CAN_OBD_SendRequest(SID_SERVICE_05, PID_S05_CELL_VMAX, &resp) == 0) {
        raw = &resp.data[2];
        rlen = resp.len - 2;
        vd->cell_vmax = CAN_OBD_ParseS05(PID_S05_CELL_VMAX, raw, rlen);
    }

    /* 单体电压最低 */
    if (CAN_OBD_SendRequest(SID_SERVICE_05, PID_S05_CELL_VMIN, &resp) == 0) {
        raw = &resp.data[2];
        rlen = resp.len - 2;
        vd->cell_vmin = CAN_OBD_ParseS05(PID_S05_CELL_VMIN, raw, rlen);
    }

    /* 电池温度最高 */
    if (CAN_OBD_SendRequest(SID_SERVICE_05, PID_S05_BATT_TMAX, &resp) == 0) {
        raw = &resp.data[2];
        rlen = resp.len - 2;
        vd->batt_tmax = CAN_OBD_ParseS05(PID_S05_BATT_TMAX, raw, rlen);
        vd->temp_batt = vd->batt_tmax;
    }

    /* 电池温度最低 */
    if (CAN_OBD_SendRequest(SID_SERVICE_05, PID_S05_BATT_TMIN, &resp) == 0) {
        raw = &resp.data[2];
        rlen = resp.len - 2;
        vd->batt_tmin = CAN_OBD_ParseS05(PID_S05_BATT_TMIN, raw, rlen);
    }

    /* 计算压差和温差 */
    vd->cell_delta_v = vd->cell_vmax - vd->cell_vmin;
    vd->batt_delta_t = vd->batt_tmax - vd->batt_tmin;

    /* 车速 (Service 05优先, 读不到试Service 01) */
    if (CAN_OBD_SendRequest(SID_SERVICE_05, PID_S05_SPEED, &resp) == 0) {
        raw = &resp.data[2];
        rlen = resp.len - 2;
        vd->speed = CAN_OBD_ParseS05(PID_S05_SPEED, raw, rlen);
    } else if (CAN_OBD_SendRequest(SID_SERVICE_01, PID_S01_SPEED, &resp) == 0) {
        raw = &resp.data[2];
        rlen = resp.len - 2;
        vd->speed = CAN_OBD_ParseS01(PID_S01_SPEED, raw, rlen);
    }

    /* 链路有效性: 本轮只要有任意一个 PID 收到"配对正确"的响应, 即认为链路正常。
       注意不能用 vd->voltage > 0 判断 —— 那是上一轮残留的值, 本轮可能已读取失败。 */
    if (g_can_obd.ok_count != ok0) {
        s_fail_streak = 0;
        vd->obd_valid = 1;
    } else {
        /* 连续 OBD_FAIL_STREAK_MAX 轮全无有效响应才判失效, 容忍偶发丢帧 */
        if (s_fail_streak < 255) s_fail_streak++;
        if (s_fail_streak >= OBD_FAIL_STREAK_MAX) vd->obd_valid = 0;
        /* 未达阈值: 保持上一轮的 obd_valid 不变 */
    }

    vd->timestamp = HAL_GetTick();
    osMutexRelease(s_can_req_mutex);
}

/* ==================== PID 扫描模式 ==================== */

void CAN_OBD_ScanPIDs(void) {
    if (s_can_req_mutex == NULL) return;

    /* 与 CAN_OBD_Update() 串行化 (见 s_can_req_mutex 注释)。
       本函数无提前 return, 锁在末尾释放。扫描期间 Update 会阻塞等待,
       这是有意的: 扫描是用户主动触发的诊断动作, 宁可 OBD 刷新暂停几秒。 */
    osMutexAcquire(s_can_req_mutex, osWaitForever);

    OBD_Response_t resp;
    char log_line[256];

    const struct {
        uint8_t sid;
        uint8_t pid;
        const char *name;
    } scan_list[] = {
        /* Service 01 标准PID */
        { SID_SERVICE_01, 0x00, "S01 PID Support [01-20]" },
        { SID_SERVICE_01, 0x04, "S01 Engine Load" },
        { SID_SERVICE_01, 0x05, "S01 Coolant Temp" },
        { SID_SERVICE_01, 0x0C, "S01 Engine RPM" },
        { SID_SERVICE_01, PID_S01_SPEED, "S01 Vehicle Speed" },
        { SID_SERVICE_01, PID_S01_THROTTLE, "S01 Throttle Position" },
        { SID_SERVICE_01, 0x1C, "S01 OBD Compliance" },
        { SID_SERVICE_01, 0x1F, "S01 Runtime" },
        { SID_SERVICE_01, 0x42, "S01 Control Module Voltage" },
        { SID_SERVICE_01, 0x46, "S01 Ambient Temp" },
        { SID_SERVICE_01, 0x51, "S01 Fuel Type" },
        { SID_SERVICE_01, 0x5B, "S01 Hybrid SOC" },

        /* Service 05 国标PID */
        { SID_SERVICE_05, 0x00, "S05 PID Support" },
        { SID_SERVICE_05, PID_S05_SOC, "S05 Battery SOC" },
        { SID_SERVICE_05, PID_S05_SPEED, "S05 Vehicle Speed" },
        { SID_SERVICE_05, PID_S05_AMBIENT, "S05 Ambient Temp" },
        { SID_SERVICE_05, PID_S05_MOTOR_TEMP, "S05 Motor Temp" },
        { SID_SERVICE_05, PID_S05_IGBT_TEMP, "S05 IGBT Temp" },
        { SID_SERVICE_05, PID_S05_THROTTLE, "S05 Throttle" },
        { SID_SERVICE_05, PID_S05_BRAKE, "S05 Brake" },
        { SID_SERVICE_05, PID_S05_CELL_VMAX, "S05 Cell V Max" },
        { SID_SERVICE_05, PID_S05_CELL_VMIN, "S05 Cell V Min" },
        { SID_SERVICE_05, PID_S05_BATT_TMAX, "S05 Batt Temp Max" },
        { SID_SERVICE_05, PID_S05_BATT_TMIN, "S05 Batt Temp Min" },
        { SID_SERVICE_05, PID_S05_MOTOR_RPM, "S05 Motor RPM" },
        { SID_SERVICE_05, PID_S05_BUS_VOLT, "S05 Bus Voltage" },
        { SID_SERVICE_05, PID_S05_BUS_CURR, "S05 Bus Current" },
        { SID_SERVICE_05, PID_S05_MOTOR_PWR, "S05 Motor Power" },
        { SID_SERVICE_05, PID_S05_MC_VOLT, "S05 MC Input Voltage" },
    };

    SDLog_Log("=== PID Scan Start ===");

    for (int i = 0; i < (int)(sizeof(scan_list) / sizeof(scan_list[0])); i++) {
        int8_t ret = CAN_OBD_SendRequest(scan_list[i].sid, scan_list[i].pid, &resp);

        if (ret == 0 && resp.len >= 2) {
            int n = snprintf(log_line, sizeof(log_line),
                "[OK] %s (SID=%02X PID=%02X): len=%d data=",
                scan_list[i].name, scan_list[i].sid, scan_list[i].pid, resp.len);

            for (int j = 0; j < resp.len && n < (int)sizeof(log_line) - 4; j++)
                n += snprintf(log_line + n, sizeof(log_line) - n, "%02X ", resp.data[j]);

            SDLog_Log(log_line);
        } else {
            snprintf(log_line, sizeof(log_line),
                "[FAIL] %s (SID=%02X PID=%02X): no response",
                scan_list[i].name, scan_list[i].sid, scan_list[i].pid);
            SDLog_Log(log_line);
        }

        osDelay(200);  /* 每个PID间隔200ms, 避免总线过载 */
    }

    SDLog_Log("=== PID Scan Complete ===");
    osMutexRelease(s_can_req_mutex);
}

/* ==================== HAL CAN 中断回调 ==================== */

/**
  * @brief  CAN RX FIFO0 挂起中断回调 (重写 HAL 弱函数)
  * @note   CAN_OBD_Init() 已通过 HAL_CAN_ActivateNotification() 使能
  *         CAN_IT_RX_FIFO0_MSG_PENDING。HAL 的默认弱函数为空实现,
  *         若不在本文件重写, 0x7E8 响应帧永远不会被读取,
  *         整个 OBD 数据链路会一直停在"无响应"状态。
  */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    CAN_OBD_RxHandler(hcan);
}
