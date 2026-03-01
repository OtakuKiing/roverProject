#ifndef MOTOR_CONTROL  
#define MOTOR_CONTROL  

void motorsInit();
void encodersInit();
void encoder0Raw();
void encoder1Raw();
void motorsSpeedDistance();
void motor0Move(bool direction, int speed);
void motor1Move(bool direction, int speed);
bool motorsKill();

extern float motor0_RPM;
extern float motor1_RPM;
extern float motor0_Distance;

#endif