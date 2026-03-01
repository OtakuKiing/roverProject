#include "pch.h"

int motor0_Pid_Speed = 0;
int motor1_Pid_Speed = 0;
int test0;

void serialControlInput() {
  serialRead();

	switch (target_Id) {
		case 0x00:
			motorsKill();
			break;

		case 0x04:
			test0 = target_Speed * 251 / 255;
			motor0_Pid_Speed = pidController(int(motor0_RPM), test0);
			motor0Move(target_Dir, motor0_Pid_Speed);
			break;

		case 0x05:
			motor1_Pid_Speed = pidController(int(motor1_RPM), target_Speed);
			motor1Move(target_Dir, motor1_Pid_Speed);
			break;
	}
}

void serialControlOutput() {
	Packet packet_Out = {0x06};
	memcpy(&packet_Out.byte1, &motor0_RPM, sizeof(float));
	serialWrite(packet_Out);
}

void setup() {
  motorsInit();
  encodersInit();
  serialInit();
  pinMode(LED_PIN, OUTPUT);  // onboard LED has no pwm
}

void loop() {

	motorsSpeedDistance();	// update all motor values

	serialControlInput();
	serialControlOutput();

}