#include "pch.h"

float motor0_Target_RPM;
float motor1_Target_RPM;
Packet p0 = {};

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
	switch (target_Id) {
		case 0x01:
			p0.id = {0x01};  // test, unsigned data
			memcpy(&p0.byte1, &motor0_Target_RPM, sizeof(float));
			serialWrite(p0);
			break;
		case 0x02:
			p0.id = {0x02};  // test, unsigned data
			memcpy(&p0.byte1, &motor1_Target_RPM, sizeof(float));
			serialWrite(p0);
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

	serialControlInput();  // read serial
	serialControlOutput();  // write to serial

}