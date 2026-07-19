// Functional! setRPM() works

#ifndef MOTOR_CONTROL  
#define MOTOR_CONTROL  

#include <Arduino.h>

class Motor {
	public:
		Motor(
			uint8_t l_pwm_pin, 
			uint8_t r_pwm_pin, 
			uint8_t enc_a_pin, 
			uint8_t enc_b_pin, 
			int counts_per_rev = 1400, 
			float wheel_diam = 0.08f );

		void begin(void (*isr)());

		void setRPM(int16_t rpm);

		void encoderInterrupt();

		void stop();

		void updateKinematics();

		int16_t getRPM() const {return currentRPM_;}
		int16_t getMPS() const {return currentMPS_;}
		int getEncA() const {return ENC_A_PIN_;}
		int getEncB() const {return ENC_B_PIN_;}

	private:
		// motor constants
		uint8_t L_PWM_PIN_;
		uint8_t R_PWM_PIN_;
		uint8_t ENC_A_PIN_;
		uint8_t ENC_B_PIN_;

		float WHEEL_DIAM_;  // diameter in metres
		float WHEEL_CIRC_; // circumference in metres
		const uint8_t CALC_RATE_MS_ = 100;  // 10 Hz
		int CPR_;  // counts per revolution of encoder

		byte encA_Last_;  // previous value of encA
		byte encB_Last_;  // previous value of encB

		volatile bool currentDirection_; 
		volatile int encoderCounts_;  // encoder counts
		uint32_t motor_Last_;  // last time RPM was calculated

		int16_t targetRPM_;
		int16_t currentRPM_;
		float currentMPS_;
		float distance_;
		
};

extern Motor motor0;
extern Motor motor1;

void motorKinematics();

void motor0ISR();
void motor1ISR();

#endif