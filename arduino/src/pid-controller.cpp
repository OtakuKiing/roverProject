#include "pid-controller.h"

int pidController(float input, float target) {
    static float integral = 0;
    static float previous_error = 0;

    const float Kp = 0.5f, Ki = 0.1f, Kd = 0.1f;
    const float out_min = 0.0f, out_max = 255.0f;

    float error = target - input;

    integral += error;
    // Clamp integral to prevent windup
    if (integral > out_max / Ki) integral = out_max / Ki;
    if (integral < out_min / Ki) integral = out_min / Ki;

    float derivative = error - previous_error;
    previous_error = error;

    int output = Kp * error + Ki * integral + Kd * derivative;
    if (output > out_max) output = out_max;
    if (output < out_min) output = out_min;

    return output;
}