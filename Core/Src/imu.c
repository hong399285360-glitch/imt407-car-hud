#include "imu.h"
#include <math.h>

static I2C_HandleTypeDef *s_hi2c;
static const float ACC_LSB = 16384.0f;
static const float GYRO_LSB = 131.0f;

static HAL_StatusTypeDef IMU_WriteReg(uint8_t reg, uint8_t val) {
    return HAL_I2C_Mem_Write(s_hi2c, MPU6050_ADDR, reg, 1, &val, 1, 100);
}

static HAL_StatusTypeDef IMU_ReadRegs(uint8_t reg, uint8_t *buf, uint8_t len) {
    return HAL_I2C_Mem_Read(s_hi2c, MPU6050_ADDR, reg, 1, buf, len, 200);
}

void IMU_Init(I2C_HandleTypeDef *hi2c) {
    s_hi2c = hi2c;
    HAL_Delay(100);

    IMU_WriteReg(0x6B, 0x00);
    HAL_Delay(10);

    IMU_WriteReg(0x19, 0x07);
    IMU_WriteReg(0x1A, 0x03);
    IMU_WriteReg(0x1B, 0x00);
    IMU_WriteReg(0x1C, 0x00);
    HAL_Delay(10);
}

void IMU_ReadAll(I2C_HandleTypeDef *hi2c, float *ax, float *ay, float *az,
                 float *gx, float *gy, float *gz, float *temp) {
    uint8_t buf[14];
    if (IMU_ReadRegs(0x3B, buf, 14) != HAL_OK) {
        *ax = *ay = *az = 0;
        *gx = *gy = *gz = 0;
        *temp = 0;
        return;
    }

    int16_t raw_ax = (buf[0] << 8) | buf[1];
    int16_t raw_ay = (buf[2] << 8) | buf[3];
    int16_t raw_az = (buf[4] << 8) | buf[5];
    int16_t raw_temp = (buf[6] << 8) | buf[7];
    int16_t raw_gx = (buf[8] << 8) | buf[9];
    int16_t raw_gy = (buf[10] << 8) | buf[11];
    int16_t raw_gz = (buf[12] << 8) | buf[13];

    *ax = raw_ax / ACC_LSB;
    *ay = raw_ay / ACC_LSB;
    *az = raw_az / ACC_LSB;
    *gx = raw_gx / GYRO_LSB;
    *gy = raw_gy / GYRO_LSB;
    *gz = raw_gz / GYRO_LSB;
    *temp = raw_temp / 340.0f + 36.53f;
}

float IMU_CalcGrade(float ax, float ay, float az) {
    float mag = sqrtf(ax * ax + ay * ay + az * az);
    if (mag < 0.01f) return 0;
    float angle_rad = asinf(ay / mag);
    return tanf(angle_rad) * 100.0f;
}

void IMU_Update(VehicleData_t *vd) {
    float ax, ay, az, gx, gy, gz, temp;
    IMU_ReadAll(s_hi2c, &ax, &ay, &az, &gx, &gy, &gz, &temp);

    vd->accel_x = ax;
    vd->accel_y = ay;
    vd->accel_z = az;
    vd->gyro_x = gx;
    vd->gyro_y = gy;
    vd->gyro_z = gz;
    vd->grade = IMU_CalcGrade(ax, ay, az);
}
