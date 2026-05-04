#include "pch.h"

int motor0_Pid_Speed = 0;
int motor1_Pid_Speed = 0;
float motor0_Target_RPM;
float motor1_Target_RPM;

// read and use serial data
void serialInControls() {
	serialRead();

	switch (target_Id) {
		case 0x00:
			motorsKill();
			target_Id = 0x00;  // integral must be reset after kill
			break;
			
		case 0x01:
			motor0_Target_RPM = ((float)target_Speed / 255.0f) * 251.0f; // PWM to RPM limits
			if (target_Dir = 1) {motor0_Target_RPM = -motor0_Target_RPM;} // convert to signed RPM for PID
			break;

		case 0x02:
			motor1_Target_RPM = ((float)target_Speed / 255.0f) * 251.0f; // PWM to RPM limits
			if (target_Dir = 1) {motor1_Target_RPM = -motor1_Target_RPM;} // convert to signed RPM for PID
			break;
	}
}

// write motor data to serial
void serialOutMotors() {
	static unsigned long last_send = 0;
  if (millis() - last_send < 20) return; // rate limited to 50Hz
  last_send = millis();

	Packet p0 = {0x01};  // motor0 data
	memcpy(&p0.byte1, &motor0_RPM, sizeof(float));
	serialWrite(p0);

	Packet p1 = {0x02};  // motor1 data
	memcpy(&p1.byte1, &motor1_RPM, sizeof(float));
	serialWrite(p1);
}

// control motors using commands from serial
void moveMotors() {
	switch (target_Id) {
		case 0x01:
			motor0_Pid_Speed = (pidController(motor0_Target_RPM, motor0_RPM)); // pidController returns unsigned PWM
			motor0Move(target_Dir, motor0_Pid_Speed);
			break;

		case 0x02:
			motor1_Pid_Speed = (pidController(motor1_Target_RPM, motor1_RPM)); // pidController returns unsigned PWM
			motor1Move(target_Dir, motor1_Pid_Speed);
			break;
	}
}


void setup() {
  motorsInit();
  encodersInit();
  serialInit();
  pinMode(LED_PIN, OUTPUT);  // onboard LED has no pwm
}

void loop() {
	motorsSpeedDistance();	// update all motor values
	serialInControls();  // read serial
	moveMotors();
	serialOutMotors();  // write to motor data to serial
}