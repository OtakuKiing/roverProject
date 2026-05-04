#include "pid-controller.h"

const float Kp = 2.0f, Ki = 0.5f, Kd = 1.0f; // proportional, integral, derivative gains
const float out_max = 251.0f; const int MIN_PWM = 10; // output limits
int pidController(float setpoint, float measured) {
	// static means variable persists only within this function
	static unsigned long previous_time = 0;
	static float previous_error = 0.0f;
	static float integral = 0;
  
	unsigned long time = micros(); // time now in usec
	float dt = (time - previous_time) / 1e6f; // change in time in sec
	if (dt <= 0.0f) return (int)0; // if no time has passed, no output

	float error = setpoint - measured; // how far it is from the target RPM

	integral += error * dt; // scale error to time
	if (Ki != 0.0f) integral = constrain(integral, -out_max / Ki, out_max / Ki); // clamp integral to prevent windup if Ki is valid

	float derivative = (error - previous_error) / dt; // TODO: should this be derivative of error or measurements?

	float output = (Kp * error) + (Ki * integral) + (Kd * derivative);
	output = constrain(output, -out_max, out_max); // clamp signed output

	int pwm_out = (int)((output / out_max) * 255); // convert RPM to PWM
	if (abs(pwm_out) < MIN_PWM) pwm_out = 0; // PWM deadzone from -10 to 10

	previous_error = error;
	previous_time = time;

	return abs(pwm_out); // return unsigned PWM
}