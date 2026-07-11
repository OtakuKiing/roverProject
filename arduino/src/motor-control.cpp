#include "Arduino.h"

/* TODO:
 - Double check the MOTOR_CPR value - 16 or 64?
 - Add simple autonomous functions ie 'forward(distance, speed)' to a separate file
 - Test best value for CALC_RATE_MS
*/

// --- Declarations --- //

// pins to motor driver board (JZK BTS7960B)
const int MOTOR_0_L_PWM = 5; 
const int MOTOR_0_R_PWM = 6; 
const int MOTOR_1_R_PWM = 9; 
const int MOTOR_1_L_PWM = 10; 

// pins to motor encoders
// encoder counts are more significantly reliable when pins are interrupt capable
// UNO R3 only has interrupt pins 2 and 3
const int MOTOR_0_ENCA = 2;  // interrupt
const int MOTOR_0_ENCB = 4; 
const int MOTOR_1_ENCA = 3;  // interrupt
const int MOTOR_1_ENCB = 7; 

// store previous encoder channel for comparison
byte motor0_EncA_Last;
byte motor0_EncB_Last;
byte motor1_EncA_Last;
byte motor1_EncB_Last;

// rpm & distance travelled calculation values
const int CALC_RATE_MS = 100;  // rate at which speeds and distance are calculated in ms
const int MOTOR_CPR = 1400;	// encoder attached to output shaft
const float WHEEL_DIAM = 0.08;  // diameter in metres
const float WHEEL_CIRC = PI * WHEEL_DIAM; // circumference in metres

float motor0_RPM = 0.0;  // speed in revolutions per minute
float motor1_RPM = 0.0;
float motor0_MPS = 0.0;  // speed in metres per second
float motor1_MPS = 0.0;
float motor0_Distance = 0.0;  // cumulative distance travelled
float motor1_Distance = 0.0;

volatile int motor0_Duration;  // the number of the pulses counted
volatile int motor1_Duration;

volatile bool motor0_Direction;  // the rotation direction
volatile bool motor1_Direction;  

uint32_t motors_Before;  // last time motor rpm was checked

// --- Functions --- //

void encoder0Raw() {  // convert encoders signals to counts and direction
    int encA_State = digitalRead(MOTOR_0_ENCA);  // current 'A' value
    int encB_State = digitalRead(MOTOR_0_ENCB);  // current 'B' value

		if(motor0_EncB_Last == encB_State) {  // if 'B' is the same...
				motor0_Direction = (encA_State == HIGH) ? true : false;  // ...then only 'A' must have changed, so direction based off of 'A'...
		} else {
				motor0_Direction = (encB_State == encA_State) ? true : false;  // else direction based off of 'B' relative to 'A'
		}
		(!motor0_Direction) ? ++motor0_Duration : --motor0_Duration; // incrementing pulse counter

    motor0_EncA_Last = encA_State;
    motor0_EncB_Last = encB_State; 
}

void encoder1Raw() {  // convert encoders signals to counts and direction
    int encA_State = digitalRead(MOTOR_1_ENCA);  // current 'A' value
    int encB_State = digitalRead(MOTOR_1_ENCB);  // current 'B' value

    
		if(motor1_EncB_Last == encB_State) {  // if 'B' is the same...
				motor1_Direction = (encA_State == HIGH) ? true : false;  // ...then only 'A' must have changed, so direction based off of 'A'...
		} else {
				motor1_Direction = (encB_State == encA_State) ? true : false;  // else direction based off of 'B' relative to 'A'
		}
		(!motor1_Direction) ? ++motor1_Duration : --motor1_Duration; // incrementing pulse counter


    motor1_EncA_Last = encA_State;
    motor1_EncB_Last = encB_State;
}

void motorsInit() {  // initialize motor pins
	pinMode(MOTOR_0_L_PWM, OUTPUT);
	pinMode(MOTOR_0_R_PWM, OUTPUT);
	pinMode(MOTOR_1_L_PWM, OUTPUT);
	pinMode(MOTOR_1_R_PWM, OUTPUT);
	analogWrite(MOTOR_0_L_PWM, 0); 
	analogWrite(MOTOR_0_R_PWM, 0); 
	analogWrite(MOTOR_1_L_PWM, 0); 
	analogWrite(MOTOR_1_R_PWM, 0); 

	motors_Before = millis();
}

void encodersInit() {  // initialize encoder pins
	pinMode(MOTOR_0_ENCA, INPUT_PULLUP);
	pinMode(MOTOR_1_ENCA, INPUT_PULLUP);
	pinMode(MOTOR_0_ENCB, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(MOTOR_0_ENCA), encoder0Raw, CHANGE);
	pinMode(MOTOR_1_ENCB, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(MOTOR_1_ENCA), encoder1Raw, CHANGE);

	motor0_EncA_Last = digitalRead(MOTOR_0_ENCA);
	motor0_EncB_Last = digitalRead(MOTOR_0_ENCB);
	motor1_EncA_Last = digitalRead(MOTOR_1_ENCA);
	motor1_EncB_Last = digitalRead(MOTOR_1_ENCB);
}

void motorsSpeedDistance() {  // get raw motor values and convert to rpm/mps

	uint32_t motors_Now = millis();
	uint32_t motors_dt = motors_Now - motors_Before;
	
	if(motors_dt >= CALC_RATE_MS) {  // Calculate every >100ms
		noInterrupts();  // stop duration from being changed during calculations
		long dur0 = motor0_Duration;
		long dur1 = motor1_Duration;
		motor0_Duration = 0;
		motor1_Duration = 0;
		interrupts();

		// rpm = (counts / (counts per revolution)) / (time in minutes)
		motor0_RPM = (float(dur0) / MOTOR_CPR) / (motors_dt / 60000.0f);
		motor1_RPM = (float(dur1) / MOTOR_CPR) / (motors_dt / 60000.0f);
		
		motor0_MPS = motor0_RPM * WHEEL_CIRC / 60.0f;
		motor1_MPS = motor1_RPM * WHEEL_CIRC / 60.0f;

		motor0_Distance += motor0_MPS * (motors_dt / 1000.0f);
		motor1_Distance += motor1_MPS * (motors_dt / 1000.0f);

		motors_Before = motors_Now;
	}
}

void motor0Move(bool direction, int speed) {  // simple direction and speed controls
	// kill PWM first 
	analogWrite(MOTOR_0_L_PWM, 0); 
	analogWrite(MOTOR_0_R_PWM, 0); 

	speed = speed * 0.6;  // m1 is broken? half speed to match limits
	if (direction==0) {analogWrite(MOTOR_0_L_PWM, speed);} 
	else {analogWrite(MOTOR_0_R_PWM, speed);}
}

void motor1Move(bool direction, int speed) {  // simple direction and speed controls
	// kill PWM first 
	analogWrite(MOTOR_1_L_PWM, 0); 
	analogWrite(MOTOR_1_R_PWM, 0); 

	//speed = speed / 2;
	if (direction==0) {analogWrite(MOTOR_1_L_PWM, speed);} 
	else {analogWrite(MOTOR_1_R_PWM, speed);}
}

bool motorsKill() {  // completely cuts power to motors
	// set all pins to LOW output
	digitalWrite(MOTOR_0_R_PWM, LOW);
  digitalWrite(MOTOR_1_R_PWM, LOW);
  digitalWrite(MOTOR_0_L_PWM, LOW);
  digitalWrite(MOTOR_1_L_PWM, LOW);
	return(true);
}