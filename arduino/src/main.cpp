#include "pch.h"

int motor0_Pid_Speed = 0;
int motor1_Pid_Speed = 0;
int motor0_Target_RPM;
int motor1_Target_RPM;

void serialControlInput() {
  serialRead();

	switch (target_Id) {
		case 0x00:
			motorsKill();
			break;

		case 0x04:
			motor0_Target_RPM = target_Speed * 251 / 255;
			motor0Move(target_Dir, target_Speed);
			break;

		case 0x05:
			motor1_Target_RPM = target_Speed * 251 / 255;
			motor1Move(target_Dir, target_Speed);
			break;
	}
}

void serialControlOutput() {
	Packet packet_Out = {0x06};  // test, motor0 control only
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

	serialControlInput();  // read serial
	serialControlOutput();  // write to serial

}