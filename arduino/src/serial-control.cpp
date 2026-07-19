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

	bool crc8_decoder(uint8_t crc_rx, uint8_t *data, uint8_t length) {
		uint8_t crc_calc = 0x00  // crc starts as 0x00
		for (uint8_t i = 0; i < length; i++) { // for each byte in data array
			crc_calc ^= data[i] // 0x00 XOR data byte
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

*/

#include "serial-control.h"

Packet_t packet;
Packet_t* packetPtr = nullptr;

uint8_t packetCounter = 0;
bool packetReady = false;

void serialInit() {
  Serial.begin(115200);
}

// handlers defined in serial-control.h

void incomingRead() {  // retrieves raw serial data, converts to packets, cleaning, COBS, & validation
	if (packetCounter == 23) {packetCounter = 0; packetReady = true;}
	if (Serial.available()) {packet.raw[packetCounter] = Serial.read(); packetCounter++;}
	packetPtr = &packet;  // create a pointer to the entire packet
}

void incomingDispatch() {  // assigns meaning to packets
	if (packetReady) {
		switch(packet.fields.id){
			case MOT_IN:
				handler_MOT_IN(packetPtr->fields.data.mot_in);  // passes the value of ...mot_in
				break;
			case ERR:
				handler_ERR(packetPtr->fields.data.err);
				break;
			case EMG:
				handler_EMG(packetPtr->fields.data.emg);
				break;
			case DEBUG:
				handler_DEBUG(packetPtr->fields.data.debug);  // passes the value of ...mot_in
				break;
			case CONFIG:
				handler_CONFIG(packetPtr->fields.data.config);
				break;
			case MOT_OUT:
				handler_MOT_RPM(packetPtr->fields.data.mot_out);
				break;
			case BATT_IV:
				handler_BATT_IV(packetPtr->fields.data.battery);  // passes the value of ...mot_in
				break;
			case TEMP:
				handler_TEMP(packetPtr->fields.data.temp);
				break;
			case IMU:
				handler_IMU(packetPtr->fields.data.imu);
				break;
			case MAG:
				handler_MAG(packetPtr->fields.data.mag);  // passes the value of ...mot_in
				break;
			case BARO:
				handler_BARO(packetPtr->fields.data.baro);
				break;
		}
	packetReady = !packetReady;
	}
}

// handlers

void handler_EMG(Emg_t cmd) {

}

void handler_DEBUG(Debug_t cmd) {

}

void handler_CONFIG(Config_t cmd) {

}

void handler_MOT_IN(MotIn_t cmd) {
	if ((cmd.rpm0 = 0) | (cmd.rpm1 = 0)) {  // explicitly pulls pins to low
		motor0.stop();
		motor1.stop();
	} else {
		motor0.setRPM(cmd.rpm0);
		motor1.setRPM(cmd.rpm1);
	}
};

void handler_ERR(Error_t cmd) {

}

void handler_MOT_RPM(MotOut_t cmd) {

}

void handler_BATT_IV(Battery_t cmd) {

}

void handler_TEMP(Temp_t cmd) {

}

void handler_IMU(IMU_t cmd) {

}

void handler_MAG(Mag_t cmd) {

}

void handler_BARO(Baro_t cmd) {

}


/*
void serialReadDep() {
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

void serialWriteDep(Packet cmd_Out) {  // write data to the serial bus
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
	*/