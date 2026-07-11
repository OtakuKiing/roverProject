#include <pch.h>

void shake(int reps) {
	static int count = 0;
	static float heading = imu_Accel.heading;

  if (imu_Accel.heading < (heading)) {
		motor0Move(0, 128);
    motor1Move(1, 128);
	}
}