/* TODO:
- add specific error codes
*/

/* ID codes
0x00 = e-stop i/o
0x04 = motor0 i
0x05 = motor1 i
0x06 = motor0 o
0x07 = motor1 o
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
Packet prev_Cmd_Out;

bool header = 0;	// 
unsigned long heartbeatIn;  // timer since last valid command
int heartbeatInterval = 1000;  // how long to wait before executing 'no comms' procedure

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
			case 0x00:  // emergency motor kill
				target_Id = Serial.read();
				header = 0;
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

	unsigned long now = millis();
	if(now - heartbeatIn > heartbeatInterval) {  // action to take if heartbeat stops
		motorsKill();
		digitalWrite(LED_PIN, HIGH);
	}
}

void serialWrite(Packet cmd_Out) {
	if ((prev_Cmd_Out.id != cmd_Out.id) || (prev_Cmd_Out.byte1 != cmd_Out.byte1) || (prev_Cmd_Out.byte2 != cmd_Out.byte2)) {  // only update on change
		switch (cmd_Out.id) {
			case 0x00:  // ACK motor kill
				if (Serial.availableForWrite() > 1) {
					Serial.write(0xFF); Serial.write(cmd_Out.id);}
				break;

			case 0x06:  // return motor encoder values
			case 0x07:
				if(Serial.availableForWrite() > 6) {
					Serial.write(0xFF); Serial.write((uint8_t*)&cmd_Out, sizeof(cmd_Out));}  // convert outbound packet to raw bytes
				break; 
			}
		prev_Cmd_Out = cmd_Out;
	}
}