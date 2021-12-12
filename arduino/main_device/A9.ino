// A9 GSM/GPS modem

void setupA9() {
  delay(5000); // Modem start-up time
  Serial1.begin(9600);
  delay(100);

  Serial1.println("AT+CREG=1"); // Connect to network
  delay(1000);
  clearA9SerialBuffer();

  Serial1.println("AT+GPS=1"); // Set GPS on
  delay(100);
  clearA9SerialBuffer();

  Serial1.println("AT+GPSLP=2"); // Low-power GPS mode
  delay(100);
  clearA9SerialBuffer();

  Serial1.println("AT+GPSRD=30"); // Instruct to write GPS data every 30 seconds on Serial1
  delay(100);
  clearA9SerialBuffer();
}

// Check Serial1 for GPS data from A9
void checkA9() {
  char response[60];
  int i = 0;
  char *token;
  double north, east, lat, lon;
  bool valid = true;

  while(Serial1.available() != 0 && i < (sizeof(response) - 1)) {
    response[i] = (char)Serial1.read();
    i++;
    delay(10);
  }
  if(i > 0) {
    response[i] = '\0';
    if(DEBUG && GPS_DEBUG) Serial.println(response);
    if(response[0] == '+') {
      int x = 0;
      token = strtok(response, ",");
      while(x <= 6 && token != NULL) {
        if(x == 2) {
          north = atof(token);
          lat = ((long)north / 100) + (fmod(north, 100.0) / 60);
          if(DEBUG && GPS_DEBUG) {
            Serial.print("North: ");
            Serial.println(north, 4);
            Serial.print("Latitude: ");
            Serial.println(lat, 4);
          }
        }
        if(x == 4) {
          east = atof(token);
          lon = ((long)east / 100) + (fmod(east, 100.0) / 60);
          if(DEBUG && GPS_DEBUG) {
            Serial.print("East: ");
            Serial.println(east);
            Serial.print("Longtitude: ");
            Serial.println(lon, 4);
          }
        }
        if(x == 6) {
          if(token[0] == '0') valid = false;
          if(DEBUG && GPS_DEBUG) {
            Serial.print("Valid: ");
            Serial.println(valid);
          }
          // Save data
          location.valid = valid;
          if(valid) {
            location.lat = lat;
            location.lon = lon;
            location.lastValidLat = lat;
            location.lastValidLon = lon;
          }
        }
        token = strtok(NULL, ",");
        x++;
      }
    }
  }
}

// Clear Serial1 buffer
void clearA9SerialBuffer() {
  while (Serial1.available()) {
    char c = (char)Serial1.read();
    if(DEBUG && (GSM_DEBUG || GPS_DEBUG)) {
      Serial.print(c);
    }
  }
}

void sendAlarmSMS(int smsType) {
  // Off receiving GPS updates, so that GPS data is not sent to us during sending of SMS
  Serial1.println("AT+GPSRD=0");
  delay(200);
  // Clear buffer
  clearA9SerialBuffer();

  Serial1.println("AT+CMGF=1"); // Set messaging mode
  delay(100);
  clearA9SerialBuffer();
  char atCommand[25];
  char message[160];
  char latS[10];
  char lonS[10];
  float lat = location.valid ? location.lat : location.lastValidLat;
  float lon = location.valid ? location.lon : location.lastValidLon;
  dtostrf(lat, 7, 4, latS);
  dtostrf(lon, 7, 4, lonS);
  // Configure message
  if(smsType == SMS_NO_DEVICE_COMM) { sprintf(message, "One of the pheripheral devices is not communicationg with Car Seat Monitor. Please correct this ASAP!"); }
  if(smsType == SMS_NO_APP_COMM) { sprintf(message, "Car Seat Monitor is unable to communicate with phone app. Please correct this to get better user experience."); }
  if(smsType == SMS_BATTERY_PROBLEM) { sprintf(message, "One of the batteries in Car Seat Monitoring system is in need of charging"); }
  if(smsType == SMS_DRIVER_AWAY) { sprintf(message, "You are away from your car seat. Please return to your car seat or take the child out of the car too."); }
  if(smsType == SMS_DRIVER_AWAY_AGAIN) { 
    if(lat != 0.0) {
      sprintf(message, "Child left in the car seat at location %sN %sW! https://maps.google.com/maps?q=%s,%s", latS, lonS, latS, lonS); 
    } else {
      sprintf(message, "Child left in the car seat alone!"); 
    }
  }
  if(smsType == SMS_HIGH_TEMPERATURE) { sprintf(message, "Temperature in the car is too high! Please return to the vehicle and take child out of car seat!"); }
  if(smsType == SMS_HIGH_TEMPERATURE_AGAIN) { 
    if(lat != 0.0) {
      sprintf(message, "Child left in the car seat at location %sN %sW! Temperature in the car is too high! https://maps.google.com/maps?q=%s,%s", latS, lonS, latS, lonS); 
    } else {
      sprintf(message, "Child left in the car seat alone!"); 
    }
  }
  // Some messages go only to driver, other go to all saved contacts
  int phonesToMessage = (smsType == SMS_DRIVER_AWAY_AGAIN || smsType == SMS_HIGH_TEMPERATURE_AGAIN) ? PHONES : 1;
  for(int x=0;x<phonesToMessage;x++) {
    if (phones[x] == "") break;
    // Set messaging for specific phone
    sprintf(atCommand, "AT+CMGS=\"%s\"\r", phones[x]);
    Serial1.println(atCommand);
    delay(500);
    clearA9SerialBuffer();
    // Send SMS message text to A9
    Serial1.println(message);
    delay(100);
    // Finish message by sending CTRL+Z
    Serial1.println((char)26);// ASCII code of CTRL+Z
    delay(1000);
    clearA9SerialBuffer();
  }
  // Set receiving GPS updates again
  Serial1.println("AT+GPSRD=30");
  delay(100);
  clearA9SerialBuffer();
}
