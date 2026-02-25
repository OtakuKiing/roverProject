/* TODO:
- add specific error codes
*/

#include "serial-control.h"

struct __attribute__((packed)) PacketIn{  // message id, then data. packed means no extra bytes added for padding
  uint8_t id;
  uint8_t byte1;
  uint8_t byte2;
  uint8_t byte3;
  uint8_t byte4;
};

const int LED_PIN = 13;

// globalised motor values from packets
uint8_t target_Id;
uint8_t target_Led_Pwm;
uint8_t target_Dir; 
uint8_t target_Speed;

bool header = 0;	// 
int heartbeatStart;  // timer since last valid command
int heartbeatInterval = 1000;  //how long to wait before executing 'no comms' procedure

void serialInit() {
  Serial.begin(115200);
}

void serialRead() {
  target_Id = 0;
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
					PacketIn cmd;
					Serial.read();
					header = 0;
					
					target_Id = cmd.id;
					break;
					
				case 0x03:
					if(Serial.available() >= 2) {  // wait for full packet
					PacketIn cmd;
					Serial.readBytes((uint8_t*)&cmd, 2);
					header = 0;

					target_Id = cmd.id;
					target_Led_Pwm = cmd.byte1;
					}
					break;

				case 0x04:
				case 0x05:
					if(Serial.available() >= 3) {  // wait for full packet
					PacketIn cmd;
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
			digitalWrite(13, HIGH);
    }
  }
}