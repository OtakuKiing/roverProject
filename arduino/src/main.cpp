#include "pch.h"

int motor0_Pid_Speed = 0;
int motor1_Pid_Speed = 0;
float motor0_Target_RPM;
float motor1_Target_RPM;

void serialControlInput() {
  serialRead();

	switch (target_Id) {
		case 0x00:
			motorsKill();
			break;

		case 0x01:
			motor0_Target_RPM = (float)target_Speed * 251.0f / 255.0f;
			motor0Move(target_Dir, target_Speed);
			break;

		case 0x02:
			motor1_Target_RPM = (float)target_Speed * 251.0f / 255.0f;
			motor1Move(target_Dir, target_Speed);
			break;
	}
}

void serialControlOutput() {
	Packet packet_Out = {0x01};  // test, motor0 only
	memcpy(&packet_Out.byte1, &motor0_Target_RPM, sizeof(float));
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