#include <RF24.h>
#include <SPI.h>
#include <math.h>
#include <EEPROM.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"

#define CE_PIN 5
#define CSN_PIN 6
#define BLUEFRUIT_SPI_CS 8
#define BLUEFRUIT_SPI_IRQ 7
#define BLUEFRUIT_SPI_RST 4
#define DEBUG false
#define RF_DEBUG false
#define GPS_DEBUG false
#define GSM_DEBUG false
#define BLE_DEBUG false
#define BATTERY_LOW 0
#define BATTERY_HIGH 1
#define PHONES 5
#define BUZZER_PIN 11
#define LED_APP_PIN 18
#define LED_TEMP_PIN 19
#define LED_COMM_PIN 20
#define LED_BELT_PIN 21
#define LED_BATTERY_PIN 22
#define BATTERY_CHECK_PIN 12
#define DRIVER_CHECK_PIN A5
#define TIME_TO_START 60000 // 300000 // 5 minutes
#define TIME_ALLOWED_WITHOUT_COMM 15000 // 15 seconds
#define BATTERY_LIMIT 3.65
#define TEMP_LIMIT 32.0
#define MAX_DELAYS 3

#define SMS_NO_DEVICE_COMM 0
#define SMS_BATTERY_PROBLEM 1
#define SMS_DRIVER_AWAY 3
#define SMS_DRIVER_AWAY_AGAIN 4
#define SMS_HIGH_TEMPERATURE 5
#define SMS_HIGH_TEMPERATURE_AGAIN 6
#define SMS_NO_APP_COMM 7

struct SeatData {
  bool device0_communicating = false;
  float device0_battery = 0;
  bool present = false;
  bool device1_communicating = false;
  float device1_battery = 0;
  bool belt = false;
  float temperature = 0;
};

struct LastComm {
  unsigned long device0;
  unsigned long device1;
  unsigned long main;
};

struct Location {
  bool valid = false;
  float lat;
  float lon;
  float lastValidLat;
  float lastValidLon;
};

const byte address0[6] = "1XXXX";
const byte address1[6] = "2XXXX";
RF24 radio(CE_PIN, CSN_PIN);
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
float data[4];
bool connectedToPhone = false;
int battery = 1;
SeatData seatData;
LastComm comms;
char phones[PHONES][20];
Location location;
bool driverPresent = false;
bool driverWasPresent = false;
unsigned int nextAlarm = NULL;
unsigned int nrOfDelays = 0;
bool forceComm = false;

// Check battery level on booster every 5 seconds
void checkBattery() {
  static const unsigned long BAT_CHECK_INTERVAL = (5000);
  static unsigned long lastCheckTime = 0;
  if((millis() - lastCheckTime) >= BAT_CHECK_INTERVAL) {
    lastCheckTime += BAT_CHECK_INTERVAL;
    int measured = digitalRead(BATTERY_CHECK_PIN);
    if(DEBUG) { Serial.print("Battery: "); Serial.println(measured == 1 ? "Ok" : "Low"); }
    battery = measured;
  }
}

int batteryLevel(float voltage) {
  return (voltage >= BATTERY_LIMIT ? BATTERY_HIGH : BATTERY_LOW);
}

void setup() {
  if(DEBUG) {
    Serial.begin(9600);
    Serial.println("Starting...");
  }
  pinMode(BATTERY_CHECK_PIN, INPUT_PULLUP);
  pinMode(DRIVER_CHECK_PIN, INPUT);
  setupLED();
  readSettings();
  SPI.begin();
  setupRadio();
  setupBLE();
  setupA9();
  if(DEBUG) { Serial.println("Started..."); }
}

// Read all data from sensors and devices, report that data to the app and then check if there are any alarms
void loop() {
  unsigned long working = millis();
  static const unsigned long CHECK_INTERVAL = 1000; // ms
  static unsigned long lastCheckTime = working;

  // Did we receive anything from mobile? phone numbers or delay instructions  
  checkDataFromBLE();
  // Check BLE connection
  checkConnectionToPhone();
  // Did we receive any data from other devices?
  checkDevices();
  // Do we have GPS updates
  checkA9();
  // Check battery status
  checkBattery();
  // Check if are missing data from any device
  checkComms();
  // Check drivers presence
  readFSR();
  // Check if we have any alarms
  if(working > (CHECK_INTERVAL + lastCheckTime)) { // Do this only once per second
    lastCheckTime = working;
    checkAlarms();
  }
  // Send data to mobile phone
  reportToBLE();
}

// If we did not receive any message from other devices in last 60 seconds,
// mark them as  not communicating
void checkComms() {
  unsigned long now = millis();
  if(comms.device0 + TIME_ALLOWED_WITHOUT_COMM < now) { 
    if(seatData.device0_communicating) { forceComm = true; }
    seatData.device0_communicating = false; 
  }
  if(comms.device1 + TIME_ALLOWED_WITHOUT_COMM < now) { 
    if(seatData.device1_communicating) { forceComm = true; }
    seatData.device1_communicating = false; 
  }
}

// Consider TEMP_LIMIT degrees C and above as high temperature
bool isHighTemperature() {
  return (seatData.temperature >= TEMP_LIMIT);
}

// Read Force Sensitive Resistor value. If above 50, driver is in his seat.
void readFSR() {
  int fsrReading = analogRead(DRIVER_CHECK_PIN);
  bool present = (fsrReading > 50 ? true : false);
  if(present != driverPresent) { forceComm = true; }
  driverPresent = present;
  if(driverPresent && !driverWasPresent) { driverWasPresent = true; }
}

void checkAlarms() {
  static bool beltAlarm = false;
  static bool batteryAlarm = false;
  static bool commAlarm = false;
  static bool appCommAlarm = false;
  static bool tempAlarm = false;
  static bool driverAlarm = false;
  static bool nextAlarmLevel = false;

  bool foundBeltProblem = false;
  bool foundBatteryProblem = false;
  bool foundCommProblem = false;
  bool foundTempProblem = false;
  bool soundAlarm = false;

  // Child taken out of seat, reset some alarms
  if (seatData.device0_communicating && !seatData.present) {
    beltAlarm = tempAlarm = driverAlarm = false;
    nextAlarm = NULL;
    nrOfDelays = 0;
    digitalWrite(LED_TEMP_PIN, LOW);
    digitalWrite(LED_BELT_PIN, LOW);
  }

  if (seatData.device1_communicating && !seatData.belt) { foundBeltProblem = true; }
  if (seatData.device1_communicating && isHighTemperature()) { foundTempProblem = true; }
  if ((seatData.device0_communicating && batteryLevel(seatData.device0_battery) == BATTERY_LOW) || 
      (seatData.device1_communicating && batteryLevel(seatData.device1_battery) == BATTERY_LOW)) { 
    foundBatteryProblem = true; 
  }
  if(battery == BATTERY_LOW) { foundBatteryProblem = true; }
  // Communication with other two devices
  if (!seatData.device0_communicating || !seatData.device1_communicating) { 
    foundCommProblem = true; 
  }
  // Belt (only if communicationg with both devices and child is in the seat)
  if(foundBeltProblem && !beltAlarm && !foundCommProblem && seatData.present) {
    beltAlarm = true;
    soundAlarm = true;
    digitalWrite(LED_BELT_PIN, HIGH);
  } else if(!foundBeltProblem && beltAlarm) {
    beltAlarm = false;
    digitalWrite(LED_BELT_PIN, LOW);
  }
  // Battery
  if(foundBatteryProblem && !batteryAlarm) {
    batteryAlarm = true;
    soundAlarm = true;
    digitalWrite(LED_BATTERY_PIN, HIGH);
    if(!connectedToPhone) { sendAlarmSMS(SMS_BATTERY_PROBLEM); }
  } else if(!foundBatteryProblem && batteryAlarm) {
    batteryAlarm = false;
    digitalWrite(LED_BATTERY_PIN, LOW);
  }
  // Communication with devices
  if(foundCommProblem && !commAlarm && millis() > TIME_TO_START) {
    commAlarm = true;
    soundAlarm = true;
    digitalWrite(LED_COMM_PIN, HIGH);
    if(!connectedToPhone) { sendAlarmSMS(SMS_NO_DEVICE_COMM); }
  } else if(!foundCommProblem && commAlarm) {
    commAlarm = false;
    digitalWrite(LED_COMM_PIN, LOW);
  }
  // Communication with app, also 2 minutes after start
  if(!connectedToPhone && !appCommAlarm && millis() > TIME_TO_START) {
    appCommAlarm = true;
    digitalWrite(LED_APP_PIN, HIGH);
    sendAlarmSMS(SMS_NO_APP_COMM);
    soundAlarm = true;
  } else if(connectedToPhone && appCommAlarm) {
    digitalWrite(LED_APP_PIN, LOW);
    appCommAlarm = false;
  }
  // Driver and temp. alarms
  // - No need to check if we are not communicationg with both devices
  // - No need to check if child is not in the seat
  if(!foundCommProblem && seatData.present) {
    // Driver present or not? (Do not check first TIME_TO_START seconds)
    if(!driverPresent && !driverAlarm && (millis() > TIME_TO_START)) {
      driverAlarm = true;
      soundAlarm = true;
      nextAlarm = (millis() / 1000) + 60; // One minute before next alarm level
      if(!connectedToPhone) { sendAlarmSMS(SMS_DRIVER_AWAY); }
    } else if(driverPresent && driverAlarm) {
      driverAlarm = false;
      nextAlarm = NULL;
      nrOfDelays = 0;
    } else if(!driverPresent && driverAlarm && nextAlarm && (millis() / 1000 > nextAlarm)) {
      sendAlarmSMS(SMS_DRIVER_AWAY_AGAIN);
      nrOfDelays = MAX_DELAYS;
      nextAlarm = (millis() / 1000) + 60; // One minute before next SMS
    }
    // Temp
    if(foundTempProblem && !tempAlarm) {
      tempAlarm = true;
      nextAlarm = (millis() / 1000) + 60;
      digitalWrite(LED_TEMP_PIN, HIGH);
      if(!connectedToPhone) { sendAlarmSMS(SMS_HIGH_TEMPERATURE); }
    } else if(!foundTempProblem && tempAlarm) {
      tempAlarm = false;
      digitalWrite(LED_TEMP_PIN, LOW);
    } else if(foundTempProblem && tempAlarm && nextAlarm && (millis() / 1000 > nextAlarm)) {
      sendAlarmSMS(SMS_HIGH_TEMPERATURE_AGAIN);
      nrOfDelays = MAX_DELAYS;
      nextAlarm = (millis() / 1000) + 60; // One minute before next SMS
    }
  }
  if(soundAlarm || (driverAlarm && ((millis() / 1000) > (nextAlarm - 60))) || tempAlarm) {
    makeSoundAlarm();
  }
}
