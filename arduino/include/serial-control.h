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
#include <pch.h> 

#define PAYLOAD_SIZE 24

typedef enum DataId_t {
	EMG 		= 0x01,  	// modify emergency states
	DEBUG 	= 0x02,  	// verbose/debug data
	CONFIG 	= 0x03,  	// export settings, eg motor CPR, IMU precision
	MOT_IN 	= 0x10,  	// motor commands
	ERR 		= 0x80,  	// error codes
	MOT_OUT = 0x90,  	// motor rpm data
	BATT_IV = 0xA0,  	// voltage and current across the entire system
	TEMP 		= 0xA1,  	// temperature across the entire system
	IMU 		= 0xB0,   // imu data
	MAG 		= 0xB1,		// magnetometer data
	BARO 		= 0xB2,		// barometer data
} DataId_t;

typedef enum ConfigId_t {
	PARAM_MOTOR_KP,
	PARAM_MOTOR_KI,
	PARAM_MOTOR_KD,

	// add more params for different components

	PARAM_SAVE  // save certain variables to EEPROM
} ConfigId_t;


typedef struct __attribute__((packed)) Debug_t {  // how to structure this... whats it gonna be used for?
  uint8_t b0;  // 2 bytes...
	uint8_t b1;  // 2 bytes...
} Debug_t;  // = 4 bytes

typedef struct __attribute__((packed)) Config_t {
  ConfigId_t param;
	float value;
} Config_t;  // = 4 bytes

typedef struct __attribute__((packed)) MotIn_t {
  int16_t rpm0;  // 2 bytes...
	int16_t rpm1;  // 2 bytes...
} MotIn_t;  // = 4 bytes

typedef struct __attribute__((packed)) Error_t {
	uint32_t timestamp;  // 4 bytes...
	uint8_t code;  // 1 bytes...
} Error_t;  // = 5 bytes

typedef struct __attribute__((packed)) Emg_t {
	uint32_t timestamp;  // 4 bytes...
	uint8_t code;  // 1 byte...
} Emg_t;  // = 5 bytes

typedef struct __attribute__((packed)) MotOut_t {
	uint32_t timestamp;  // 4 bytes...
	int16_t rpm0;  // 2 bytes...
	int16_t rpm1;  // 2 bytes...
} MotOut_t;  // = 8 bytes

typedef struct __attribute__((packed)) IMU_t {
	uint32_t timestamp;  // 4 bytes...
	int16_t accel[3];  // 2 * 3 bytes...
	int16_t gyro[3];  // 2 * 3 bytes...
} IMU_t;  // = 16 bytes

typedef struct __attribute__((packed)) Mag_t {
	uint32_t timestamp;  // 4 bytes...
	int16_t mag[3];  // 2 * 3 bytes...
} Mag_t;  // = 10 bytes

typedef struct __attribute__((packed)) Baro_t {
	uint32_t timestamp;  // 4 bytes...
	int32_t pressure;  // 4 bytes...
	int16_t temperature;  // 2 bytes...
} Baro_t;  // = 10 bytes

typedef struct __attribute__((packed)) Battery_t {
	uint16_t voltage;  // 2 bytes...
	uint16_t volt12;  // 2 bytes...
	uint16_t volt5;  // 2 bytes...
	uint16_t current;  // 2 bytes...
	uint8_t percentage; // 1 byte...
} Battery_t;  // = 9 bytes

typedef struct __attribute__((packed)) Temp_t {
	uint8_t mcu;  // 1 byte...
	uint8_t battery;  // 1 byte...
	uint8_t chassis0;  // 1 byte...
	uint8_t chassis1;  // 1 bytes
} Temp_t;  // = 4 bytes


typedef struct __attribute__((packed)) PacketFields_t {

	DataId_t id;  // 1 byte
	uint8_t sequence;  // 1 byte

	union {
		IMU_t imu;
		Mag_t mag;
		Baro_t baro;
		Battery_t battery;
		Temp_t temp;
		MotIn_t mot_in;
		MotOut_t mot_out;
		Emg_t emg;
		Error_t err;
		Config_t config;
		Debug_t debug;
		uint8_t raw[PAYLOAD_SIZE];  

	} data;  // = 24 bytes

	uint8_t crc8;  // 1 byte
} PacketFields_t;

typedef union __attribute__((packed)) Packet_t {
	PacketFields_t fields;
	uint8_t raw[sizeof(PacketFields_t)];
} Packet_t;


// handlers

void handler_EMG(Emg_t cmd);

void handler_DEBUG(Debug_t cmd);

void handler_CONFIG(Config_t cmd);

void handler_MOT_IN(MotIn_t cmd);

void handler_ERR(Error_t cmd);

void handler_MOT_RPM(MotOut_t cmd);

void handler_BATT_IV(Battery_t cmd);

void handler_TEMP(Temp_t cmd);

void handler_IMU(IMU_t cmd);

void handler_MAG(Mag_t cmd);

void handler_BARO(Baro_t cmd);

#endif