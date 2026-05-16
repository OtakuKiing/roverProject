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

		case 0x01:
			motor0_Target_RPM = target_Speed * 251 / 255;
			motor0Move(target_Dir, target_Speed);
			break;

		case 0x02:
			motor1_Target_RPM = target_Speed * 251 / 255;
			motor1Move(target_Dir, target_Speed);
			break;
	}
}

void serialControlOutput() {
	Packet packet_Out = {0x01};  // test, LED control only
	float target_Speed_f = (float)target_Speed;  // convert to float, which takes up byte1-4 in the packet
	memcpy(&packet_Out.byte1, &target_Speed_f, sizeof(float));
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