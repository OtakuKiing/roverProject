#include "pch.h"

void setup() {
	motor0.begin(motor0ISR);
	motor1.begin(motor1ISR);

	sei();

	Serial.begin(115200);
	timer.set(3000);
	motor0.setRPM(100);
	motor1.setRPM(100);	
}

void loop() {
	motorKinematics();

	if (timer.A_ready) {
		timer.A_ready = false;
		Serial.print(timer.timerCounter);
		Serial.println(" Interrupt! Timer done.");

		switch (timer.timerCounter) {
			case 0:
				break;
			case 1:
				motor0.setRPM(200); 	
				motor1.setRPM(200);
				timer.set(2000);
				break;
			case 2:
				motor0.stop();
				motor1.setRPM(200);
				timer.set(2000);
				break;
			case 3:
				motor0.setRPM(200);
				motor1.setRPM(50);
				timer.set(2000);
				break;
			case 4:
				motor0.stop();
				motor1.stop();
				timer.set(2000);
				break;
		}
	}
}