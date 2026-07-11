#include <pch.h>
#include <Adafruit_LSM6DSOX.h>
#include <Adafruit_LIS3MDL.h>

Adafruit_LSM6DSOX lsm6ds;
Adafruit_LIS3MDL lis3mdl;

sensors_vec_t imu_Accel;
sensors_vec_t imu_Gyro;
sensors_vec_t imu_Mag;
float imu_Temp;

uint32_t lastIMUOut;

void imuInit() {

  bool lsm6ds_success, lis3mdl_success;

  // hardware I2C mode, can pass in address & alt Wire
  lsm6ds_success = lsm6ds.begin_I2C();
  lis3mdl_success = lis3mdl.begin_I2C();
  
	//if (!(lsm6ds_success && lis3mdl_success)) {while (1) {delay(10);}}  // action to take if IMU fails init

  //IMU.println("LSM6DS and LIS3MDL Found!");  // debug

  // lsm6ds.setAccelRange(LSM6DS_ACCEL_RANGE_2_G);  // default `4`
  // can switch `lsm6ds.getAccelRange()` for cases `LSM6DS_ACCEL_RANGE_X_G` (refer to example code for X=) to find accel_range

  // lsm6ds.setAccelDataRate(LSM6DS_RATE_12_5_HZ);  // default `104`
  // can switch `lsm6ds.getAccelDataRate()` for cases `LSM6DS_RATE_X` (refer to example code for X=) to find data rate

  // lsm6ds.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS );  // default `2000`
	// can switch `lsm6ds.getGyroRange()` for cases `LSM6DS_GYRO_RANGE_X_DPS` (refer to example code for X=) to find angle range

  // lsm6ds.setGyroDataRate(LSM6DS_RATE_12_5_HZ);  // default `104`
	// can switch `lsm6ds.getGyroDataRate()` for cases `LSM6DS_RATE_X` (refer to example code for X=) to find data rate

	//// Magnetometor chip init
  lis3mdl.setDataRate(LIS3MDL_DATARATE_155_HZ);  // can check the datarate by looking at the frequency of the DRDY pin
	// can switch `lis3mdl.getDataRate()` for cases `LIS3MDL_DATARATE_X_HZ` (refer to example code for X=) to find data rate

  lis3mdl.setRange(LIS3MDL_RANGE_4_GAUSS);
	// can switch `lis3mdl.getRange()` for cases `LIS3MDL_RANGE_X_GAUSS` (refer to example code for X=) to find magnetic field range

  lis3mdl.setPerformanceMode(LIS3MDL_ULTRAHIGHMODE);
  // can switch `lis3mdl.getPerformanceMode()` for cases `LIS3MDL_X` (refer to example code for X=) to find performance mode

  lis3mdl.setOperationMode(LIS3MDL_CONTINUOUSMODE);
  // can switch `lis3mdl.getOperationMode()` for cases `LIS3MDL_X` (refer to example code for X=) to find operation mode

  lis3mdl.setIntThreshold(500);
  lis3mdl.configInterrupt(false, false, true, // enable z axis
                          true, // polarity
                          false, // don't latch
                          true); // enabled!

}

void imuGetData() {  // get accel, gyro, mag, and temp data
	
  sensors_event_t accel, gyro, mag, temp;

  // get new normalized sensor events
  lsm6ds.getEvent(&accel, &gyro, &temp);
  lis3mdl.getEvent(&mag);

	imu_Accel = accel.acceleration;
	imu_Gyro = gyro.gyro;
	imu_Mag = mag.magnetic;
	imu_Temp = temp.temperature;

  // acceleration is measured in m/s^2
  // rotation is measured in rad/s
  // magnetic field is measured in uTesla
	// temperature is measured in C
}
