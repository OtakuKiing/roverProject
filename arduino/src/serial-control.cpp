/* TODO:
- add crc8 and COBS
*/

/* ID code table
| Direction     | Category  | Message ID | Content                                           |
| ------------- | --------- | ---------- | ------------------------------------------------- |
| Pi –> Arduino | Admin     | 0x00       | Emergency state X - run routine and await command |
|               |           | 0x01       | Exit emergency state                              |
|               |           | 0x02       | Toggle verbose telemetry                          |
|               |           | 0x03       | Toggle audio alerts                               |
|               |           | 0x04       | Debug 1 (eg live PID)                             |
|               |           | 0x05       | Debug 2 (eg live PID)                             |
|               | Command   | 0x10       | Move motors                                       |
|---------------|-----------|------------|---------------------------------------------------|
| Arduino –> Pi | Admin     | 0x80       | Error (eg missing expected component)             |
|               | Telemetry | 0x90       | Motors RPM                                        |
|               |           | 0x91       | Motors voltage                                    |
|               |           | 0x92       | Motors current                                    |
|               |           | 0x93       | Encoder counts (verbose)                          |
|               |           | 0xA0       | Battery voltage and current                       |
|               |           | 0xA1       | Battery temperature                               |
|               |           | 0xB0       | Raw IMU                                           |
|               |           | 0xB1       | Raw GNSS                                          |
|---------------|-----------|------------|---------------------------------------------------|
| Bidirectional | Admin     | 0xFE       | Heartbeat                                         |
*/

/*
0x00 = kill motors i/o
0x01 = 
0x02
0x03 = 
0x04 = motor0 i
0x05 = motor1 i
0x06 = motor0 o
0x07 = motor1 o
0x08
0x09
0x0A
0xFE = heartbeat i/0
0xFF = header i/o
*/

#include "serial-control.h"

const int LED_PIN = 13;

// globalised motor values from packets
uint8_t next_Byte;
uint8_t target_Id;
uint8_t target_Dir; 
uint8_t target_Speed;
uint8_t prev_Target_Speed;
Packet prev_Cmd_Out;

bool header = 0;	// 
int heartbeatIn;  // timer since last valid command
int heartbeatInterval = 1000;  //how long to wait before executing 'no comms' procedure

void serialInit() {
  Serial.begin(115200);
	heartbeatIn = 0;
}

void serialRead() {
  if((header == 0) && (Serial.available())) {
    next_Byte = Serial.peek();
    if((next_Byte == 0xFF)) {  // check if header is valid
      header = 1;
      Serial.read();  // eat header byte
      heartbeatIn = millis();  // reset heartbeat timer
    }
	}

	if((header == 1) && (Serial.available())) {  // check for valid header and waiting data
		heartbeatIn = millis();  // reset heartbeat timer if receiving packet, including invalid data
		next_Byte = Serial.peek();  // store packet id
		switch (next_Byte) {  // check id
			case 0x00:  // insert emergency action here
				target_Id = Serial.read();
				header = 0;
				break;

			case 0x01:  // motor0 command
			case 0x02:  // motor1 command
				if(Serial.available() >= 5) {  // wait for full packet
					Packet cmd;
					Serial.readBytes((uint8_t*)&cmd, sizeof(cmd));  // read packet and assign to the variable cmd 
					header = 0;  // reset header

					// globalise serial data
					target_Id = cmd.id;
					target_Dir = cmd.byte1;
					target_Speed = cmd.byte2;
				}
				break;

			case 0xFE:  // serial heartbeat, only received when silent for 500ms
				Serial.read();
				if(Serial.availableForWrite()) {  // respond with ACK message
					Serial.write(0xFF);
					Serial.write(0xFE);
				}
				header = 0;
				break;

			default:
				Serial.read();  // eat invalid data
				header = 0;
				break;
		}
	}

	int now = millis();
	if(now - heartbeatIn > heartbeatInterval) {  // action to take if heartbeat stops
		motorsKill();
		digitalWrite(13, HIGH);
	}
}

void serialWrite(Packet cmd_Out) {  // write data to the serial bus
	if ((prev_Cmd_Out.id != cmd_Out.id) || (prev_Cmd_Out.byte1 != cmd_Out.byte1) || (prev_Cmd_Out.byte2 != cmd_Out.byte2) || (prev_Cmd_Out.byte3 != cmd_Out.byte3) || (prev_Cmd_Out.byte4 != cmd_Out.byte4)) {  // only update on change
		switch (cmd_Out.id) {
			case 0x00:  // ACK motor kill
				if (Serial.availableForWrite()) {
					Serial.write(0xFF); Serial.write(cmd_Out.id);}
				break;

			case 0x01:  // motor0 encoder values
			case 0x02:  // motor1 encoder values
				if(Serial.availableForWrite() >= 6) {
					Serial.write(0xFF); Serial.write((uint8_t*)&cmd_Out, sizeof(cmd_Out));}  // convert Out packet to raw bytes
				break; 
			}
		prev_Cmd_Out = cmd_Out;
	}
}