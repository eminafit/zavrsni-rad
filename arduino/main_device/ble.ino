// Bluetooth related code

// Start Bluetooth modul and go to `data` mode
void setupBLE() {
  if (!ble.begin(BLE_DEBUG)) {
    if(DEBUG && BLE_DEBUG) { Serial.println("Couldn't find Bluetooth, make sure it's in command mode"); }
    while(1) { }
  }
  if(DEBUG && BLE_DEBUG) ble.info();
  ble.setMode(BLUEFRUIT_MODE_DATA);
  checkConnectionToPhone();
}

void checkConnectionToPhone() {
  // `ble.isConnected()` will:
  // 1.- Switch to command mode
  // 2.- Run AT command "AT+GAPGETCONN" and parse response (1: connected, 0: not)
  // 3.- Switch back to data mode
  // 4.- Return true or false
  connectedToPhone = ble.isConnected();
}

// Every 30 seconds check BLE connection
void phoneConnectionCheck() {
  static const unsigned long BLE_CHECK_INTERVAL = (30000); // ms
  static unsigned long lastCheckTime = 0;
  if((millis() - lastCheckTime) >= BLE_CHECK_INTERVAL) {
    lastCheckTime += BLE_CHECK_INTERVAL;
    checkConnectionToPhone();
  }
}

// Send data we have to connected phone
// Data looks like this: "1;1;1;1;1;23.6;1;1;0;25\n"
// point-comma separated values:
// - Device 0 (presence monitor) communicating: 0 or 1
// - Device 0 battery has enough charge: 0 or 1
// - Child present in the seat: 0 or 1
// - Device 1 (belt and temperature monitor) communicating: 0 or 1
// - Device 1 battery has enough charge: 0 or 1
// - Temperature - String, example 23.6
// - Belt on: 0 or 1
// - Main device battery has enough charge left: 0 or 1
// - Driver present in the seat: 0 or 1
// - Number of seconds before SMS alarm is sent because driver is not present. 
//   This gives oportunity to driver to extend (up to three times) this period. 
//   Can be empty if driver is in the seat or if in the firt two minutes after
//   devices starts.

void reportToBLE() {
  static const unsigned long BLE_COMM_INTERVAL = (10000); // ms
  static unsigned long lastCommTime = 0;
  // Do it every 10 seconds or if we have newer data from devices
  if(((millis() - lastCommTime) >= BLE_COMM_INTERVAL) || (comms.main < comms.device0) || (comms.main < comms.device1) || forceComm) {
    comms.main = millis();
    lastCommTime = millis();
    char to_send[30];     
    char *p = to_send;
    to_send[0] = '\0';
    p = mystrcat(p, seatData.device0_communicating ? "1" : "0");
    p = mystrcat(p,";");
    p = mystrcat(p, batteryLevel(seatData.device0_battery) == BATTERY_HIGH ? "1" : "0");
    p = mystrcat(p,";");
    p = mystrcat(p, seatData.present ? "1" : "0");
    p = mystrcat(p,";");
    p = mystrcat(p, seatData.device1_communicating ? "1" : "0");
    p = mystrcat(p,";");
    p = mystrcat(p, batteryLevel(seatData.device1_battery) == BATTERY_HIGH ? "1" : "0");
    p = mystrcat(p,";");
    char data[5];
    dtostrf(seatData.temperature, 4, 1, data);
    p = mystrcat(p,data);
    p = mystrcat(p,";");
    p = mystrcat(p, seatData.belt ? "1" : "0");
    p = mystrcat(p,";");
    p = mystrcat(p, battery ? "1" : "0");
    p = mystrcat(p,";");
    p = mystrcat(p, driverPresent ? "1" : "0");
    p = mystrcat(p,";");
    if(nextAlarm) {
      signed int seconds = (nrOfDelays == MAX_DELAYS ? 0 :(int)(nextAlarm - (millis() / 1000)));
      if (seconds < 0) { seconds = 0; }
      itoa(seconds, data, 10);
      p = mystrcat(p,data);
    }
    p = mystrcat(p, "\n");
    if(DEBUG && BLE_DEBUG) { Serial.print(to_send); }
    ble.print(to_send);
    forceComm = false;
  }
}

// Optimized string concatenation, without using Arduino "String"
// library that adds 1500+ bytes to program storage.
char* mystrcat(char* dest, char* src) {
  while (*dest) dest++;
  while (*dest++ = *src++);
  return --dest;
}

// Read data that phone may send us. Like contact phone numbers or delay interval
void checkDataFromBLE() {
  char response[50];
  int i = 0;

  while ( ble.available()  && i < (sizeof(response) - 1)) {
    int c = ble.read();
    // Messages from phone are new line terminated
    if (c == '\n') {
      response[i] = 0x00;
      // if we receive only letter `D`, then we change delay alarming for driver absence to 5 minutes ...
      if (response[0] == 'D') {
        if(nrOfDelays < MAX_DELAYS) {
          nrOfDelays++;
          nextAlarm = (int)(millis() / 1000) + 360;
        }
      } else { // ...otherwise we received a phone number
        // Phone number format: `N:+3876XXXXXXX` whene N is phone number order from 0 to 4
        int phone_number = (int)(response[0] - '0');
        strcpy(phones[phone_number], &response[2]);
        if(DEBUG && BLE_DEBUG) {
          Serial.println();
          Serial.print("Phone: ");
          Serial.println(phone_number);
          Serial.print("Value: ");
          Serial.println(&response[2]);
          Serial.println(phones[phone_number]);
        }
        // We receive 5 numbers from phone, one after another. After the last one we store settings in EEPROM
        // We will also force sending data to phone
        if(phone_number == (PHONES-1)) { 
          writeSettings(); 
          forceComm = true; 
        }
      }
      i = 0;
    } else {
      response[i] = c;
      if(DEBUG && BLE_DEBUG) Serial.print((char)c); 
      i++; 
    }
  }
}
