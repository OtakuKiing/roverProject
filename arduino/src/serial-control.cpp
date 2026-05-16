/* TODO:
- add specific error codes
*/

/* ID codes
0x00 = e-stop i/o
0x01 = motor0 i/o
0x02 = motor1 i/o
0xFE = heartbeat i/o
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