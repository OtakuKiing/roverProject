/*
Target ID code table
| Direction     | Category  | Message ID | Content                                           |
| ------------- | --------- | ---------- | ------------------------------------------------- |
| Pi –> Arduino | Admin     | 0x00       | Emergency state X - run routine and await command |
|               |           | 0x01       | Exit emergency state                              |
|               |           | 0x02       | Toggle verbose telemetry                          |
|               |           | 0x03       | Toggle audio alerts                               |
|               |           | 0x04       | Debug 1 (eg live PID tuning)                             |
|               |           | 0x05       | Debug 2 (eg live PID)                             |
|               | Command   | 0x10       | Move motors                                       |
|---------------|-----------|------------|---------------------------------------------------|
| Arduino –> Pi | Admin     | 0x80       | Error (eg missing expected component)             |
|               | Telemetry | 0x90       | Motors RPM                                        |
|               |           | 0x91       | Encoder counts (verbose)                          |
|               |           | 0x92       | Motors electronics                                |
|               |           | 0xA0       | Battery voltage and current                       |
|               |           | 0xA1       | Battery temperature                               |
|               |           | 0xB0       | Raw IMU                                           |
|               |           | 0xB1       | Raw GNSS                                          |
|---------------|-----------|------------|---------------------------------------------------|
| Bidirectional | Admin     | 0xFE       | Heartbeat                                         |
*/

// TODO: Update Packet structure to the following:
// Message ID - message target (ie system, motorX)
// Flags - bitmap structure containing extra data (ie motor directions, 
// D0-3 - data bytes
// CRC8 - checksum for data validation
// Terminator - end byte for COBS framing using 0x00

#ifndef SERIAL_CTRL
#define SERIAL_CTRL

#include "Arduino.h"
#include "motor-control.h"  // for motorsKill() function

struct __attribute__((packed)) Packet{  // message id, then data. packed means no extra bytes added for padding, so message can be read from raw byte stream
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