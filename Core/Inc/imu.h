#ifndef IMU_H
#define IMU_H

#include "stm32f4xx_hal.h"
#include "shared_data.h"

#define MPU6050_ADDR (0x68 << 1)

void IMU_Init(I2C_HandleTypeDef *hi2c);
void IMU_Update(VehicleData_t *vd);
void IMU_ReadAll(I2C_HandleTypeDef *hi2c, float *ax, float *ay, float *az,
                 float *gx, float *gy, float *gz, float *temp);
float IMU_CalcGrade(float ax, float ay, float az);

#endif
