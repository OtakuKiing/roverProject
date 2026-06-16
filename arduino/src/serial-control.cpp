/* TODO:
- add crc8 and COBS

Packet Structure:
	current - header, id, data1, data2, data3, data4
	target - id, data1, data2, ..., dataX, flags, crc8, cobs terminator

Packet parser breakdown:
	read bytes and place in buffer until 0x00 is read
	decode cobs
	check crc8, discard if bad and update packet loss
	read id
	read data bytes (length implicit from id, use lookup table)
	decode flags bitmap
	execute command

CRC8 breakdown:
	(crc) xor (next byte)
	for the next 8 bits (ie loop 8 times), perform bitwise long division w/ no carries:
		if leading bit of (crc) is 1, shift towards MSB [trim MSB] and then xor with (poly)
		else just shift
	return crc

	bool crc8_decoder(uint8_t crc_rx, uint8_t *data_as_array, uint8_t length) {
		uint8_t crc_calc = 0x00
		for (uint8_t i = 0; i < length; i++) { // for each byte in data array
			crc_calc ^= data_as_array[i] // crc_calc = 0x00 xor data byte
			for (uint8_t j = 0; j < 8; j++) { // for each bit in the byte
				if (crc_calc & 0x80) { // if binary starts with 1
					crc_calc = (crc_calc << 1) ^ 0x07 // shift by 1 place then xor with poly 00
				} else {
					crc_calc = crc_calc << 1 // else just shift
				}
			}
		}
		if (crc_rx == crc_calc) {return true} else {return false}
	}

Flags bitmap
emergency, verbose, debug, reserved1-5
e.g. 208 = 11010000 = emergency mode, verbose telemetry, !debug, motor0 forwards, motor1 backwards, spare 1-3

Packet length lookup table

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

uint8_t header = 0;	// 
uint32_t heartbeatIn;  // timer since last valid command
uint32_t heartbeatInterval = 5000;  //how long to wait before executing 'no comms' procedure

uint32_t lastSerialOut = 0;
const uint32_t SERIAL_OUT_RATE_MS = 50; // 20Hz, adjust as needed

void serialInit() {
  Serial.begin(115200);
	heartbeatIn = millis();
}

void serialRead() {
  if((header == 0) && (Serial.available())) {
    next_Byte = Serial.peek();
    if((next_Byte == 0xFF)) {  // check if header is valid
      header = 1;
      Serial.read();  // eat header byte
      heartbeatIn = millis();  // reset heartbeat timer
    } else { Serial.read();} // eat invalid byte
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

	uint32_t now = millis();
	if(now - heartbeatIn > heartbeatInterval) {  // action to take if heartbeat stops
		motorsKill();
		digitalWrite(13, HIGH);
	}
}

void serialWrite(Packet cmd_Out) {  // write data to the serial bus
	uint32_t now = millis();
  if (now - lastSerialOut <= SERIAL_OUT_RATE_MS) return;
	lastSerialOut = now;
	if (now - heartbeatIn >= heartbeatInterval) return;  // action to take if heartbeat stops

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