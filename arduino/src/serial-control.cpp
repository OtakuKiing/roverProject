/* TODO:
- add specific error codes
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
uint8_t target_Id;
uint8_t target_Led_Pwm;
uint8_t target_Dir; 
uint8_t target_Speed;
uint8_t prev_Target_Speed;

bool header = 0;	// 
int heartbeatStart;  // timer since last valid command
int heartbeatInterval = 1000;  //how long to wait before executing 'no comms' procedure

void serialInit() {
  Serial.begin(115200);
}

void serialRead() {
  if(Serial.available() > 0) {
    uint8_t next_Byte = Serial.peek();
    if((next_Byte == 0xFF) && (header == 0)) {  // check if header is valid
      header = 1;
      Serial.read();  // eat header byte
      heartbeatStart = millis();  // reset heartbeat timer
    }

  	if((header == 1) && (Serial.available())){  // check for valid header and waiting data
      next_Byte = Serial.peek();  // store packet id
      switch (next_Byte) {  // check id
				case 0x00:  // emergency motor kill
					Packet cmd;
					Serial.read();
					header = 0;
					
					target_Id = cmd.id;
					break;
					
				case 0x03:
					if(Serial.available() >= 2) {  // wait for full packet
					Packet cmd;
					Serial.readBytes((uint8_t*)&cmd, 2);
					header = 0;

					target_Id = cmd.id;
					target_Led_Pwm = cmd.byte1;
					}
					break;

				case 0x04:
				case 0x05:
					if(Serial.available() >= 3) {  // wait for full packet
					Packet cmd;
					Serial.readBytes((uint8_t*)&cmd, 3);
					header = 0;

					target_Id = cmd.id;
					target_Dir = cmd.byte1;
					target_Speed = cmd.byte2;
					}
					break;

				case 0xFE:  // serial heartbeat, only received when silent for 500ms
					Serial.read();
					if(Serial.availableForWrite()) {  // respond with ACK message
						Serial.write(0xFE);
					}
					header = 0;
					break;

				default:
					Serial.read();  // eat invalid data
					header = 0;
					break;
      }
      heartbeatStart = millis();  // reset heartbeat timer if receiving packet, including invalid data
    } else {
      Serial.read();  // eat invalid header
    }
  } else {
    int now = millis();
    if(now - heartbeatStart > heartbeatInterval) {
      // action to take if heartbeat stops
			motorsKill();
			digitalWrite(13, HIGH);
    }
  }
}

void serialWrite(Packet cmd_Reply) {
	switch (cmd_Reply.id) {
		case 0x00:  // ACK motor kill
			if (Serial.availableForWrite()) {
				Serial.write(0xFF); Serial.write(cmd_Reply.id);}
			break;
		case 0x06:  // return encoder values
		case 0x07:
			if(Serial.availableForWrite() > 6) {
				Serial.write(0xFF); Serial.write((uint8_t*)&cmd_Reply, sizeof(cmd_Reply));}  // convert reply packet to raw bytes
			break;
	}
}