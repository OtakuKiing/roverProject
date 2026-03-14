#ifndef SERIAL_CTRL
#define SERIAL_CTRL

#include "Arduino.h"
#include "motor-control.h"  // for motorsKill() function

struct __attribute__((packed)) Packet{  // message id, then data. packed means no extra bytes added for padding
  uint8_t id;
  uint8_t byte1;
  uint8_t byte2;
  uint8_t byte3;
  uint8_t byte4;
};

void serialInit();
void serialRead();
void serialWrite(Packet cmd_Out);

extern uint8_t target_Id; 
extern uint8_t target_Led_Pwm; 
extern uint8_t target_Dir;
extern uint8_t target_Speed;
extern const int LED_PIN;

#endif