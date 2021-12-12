// NFR24L01 related code

void setupRadio() {
  if (!radio.begin()) {
    if(DEBUG) { Serial.println("Radio hardware not responding!"); }
    while (1) { }
  }
  radio.setPayloadSize(sizeof(data));
  radio.openReadingPipe(1, address0);
  radio.openReadingPipe(2, address1);
  radio.startListening();
}

void checkDevices() {
  if ( radio.available() ) {
    radio.read( data, sizeof(data) );
    int device = (int)data[0];
    if(DEBUG && RF_DEBUG) {
      Serial.print(" - Device ");
      Serial.print(device);
      Serial.print(" - Battery: ");
      Serial.print(data[1]);
      Serial.print("V");
    }
    if(device == 0) {
      seatData.device0_communicating = true;
      comms.device0 = millis();
      seatData.device0_battery = data[1];
      seatData.present = (data[2] ? true : false);
      if(DEBUG && RF_DEBUG) {
        Serial.print(" - Child in seat: ");
        Serial.println(data[2] ? "Yes" : "No");
      }
    } else if(device == 1) {
      seatData.device1_communicating = true;
      comms.device1 = millis();
      seatData.device1_battery = data[1];
      seatData.belt = (data[3] ? true : false);
      seatData.temperature = data[2];
      if(DEBUG && RF_DEBUG) {
        Serial.print(" - Temperature: ");
        Serial.print(data[2]);
        Serial.print("°C");
        Serial.print(" - Belt connected: ");
        Serial.println(data[3] ? "Yes" : "No");
      }
    }
  }
}
