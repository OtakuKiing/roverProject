#ifndef IMU_H
#define IMU_H

#include <Adafruit_Sensor.h>

extern sensors_vec_t imu_Accel, imu_Gyro, imu_Mag;
extern float imu_Temp;

void imuGetData();
void imuInit();

#endif