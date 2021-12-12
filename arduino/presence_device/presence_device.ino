#include <SPI.h>
#include <RF24.h>
#include <EEPROM.h>

#define CE_PIN 5
#define CSN_PIN 6
#define FSR_PIN A1
#define VBAT_PIN A9
#define BUZZER_PIN 10
#define SET_BUTTON 11
#define ONBOARD_LED_PIN 13
#define DEVICE_TYPE 0
#define DEBUG true

const byte address[6] = "2XXXX";
RF24 radio(CE_PIN, CSN_PIN);

bool present = false;
int noWeight = 0;
float battery = 3.7;
float data[4];
bool forceComm = false;

void buzz(int duration) {
  tone(BUZZER_PIN, 1000, duration);
}

void alarm() {
  buzz(500);
}

int readFSR() {
  int fsrReading = analogRead(FSR_PIN);
  if(DEBUG) {
    Serial.print("FSR reading = ");
    Serial.println(fsrReading);
  }
  return fsrReading;
}

// https://learn.adafruit.com/adafruit-feather-32u4-basic-proto/power-management#measuring-battery-1753817-9
void checkBattery() {
  float measured = analogRead(VBAT_PIN);
  measured *= 2; 
  measured *= 3.3;
  measured /= 1024;
  battery = measured;
  if(DEBUG) {
    Serial.print("VBat: " ); 
    Serial.println(measured);
  }
}

void checkFSR() {
  int fsr = readFSR();
  if (fsr <= noWeight) { // Empty seat
    if (present) { // Child not in the seat
      forceComm = true;  
      present = false;
    }
  } else { // Child in the seat
    if(!present) { // Child was not in the seat
      forceComm = true;  
      present = true;
    }
  }
}

void readSettings() {
  EEPROM_readAnything(0x00, noWeight);
  if(DEBUG) {
    Serial.print("No weight: " ); 
    Serial.println(noWeight);
  }
}

void setNoWeight() {
  for (int x = 0; x < 3; x++) {
    buzz(300);
    delay(700);
  }
  noWeight = readFSR() + 10;
  EEPROM_writeAnything(0x00, noWeight);
  if(DEBUG) {
    Serial.print("New no weight: " ); 
    Serial.println(noWeight);
  }
}

void checkButton() {
  if(digitalRead(SET_BUTTON) == HIGH) {
    if(DEBUG) {
      Serial.println("Setting zero weight");
    }
    setNoWeight();
  }
}

void check() {
  checkButton();
  checkFSR();
  checkBattery();
}

bool communicate() {
  digitalWrite(ONBOARD_LED_PIN, HIGH);
  data[0] = DEVICE_TYPE;
  data[1] = battery;
  data[2] = present ? 1 : 0;

  bool ok = radio.write( data, sizeof(data) );
  if(DEBUG) {
    Serial.println(present);
    Serial.println(ok ? "Data sent" : "Failed sending data");
  }
  digitalWrite(ONBOARD_LED_PIN, LOW);
  return ok;
}

void setupRadio() {
  SPI.begin();
  if (!radio.begin()) {
    if(DEBUG) { Serial.println(F("Radio hardware not responding!")); }
    while (1) {
      // hold in infinite loop
    }
  }
  radio.setPayloadSize(sizeof(data));
  radio.stopListening();
  radio.openWritingPipe(address);
}

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(ONBOARD_LED_PIN, OUTPUT);
  pinMode(SET_BUTTON, INPUT);
  if(DEBUG) {
    Serial.begin(9600);
    Serial.println("Starting");
  }
  setupRadio();
  readSettings();
}

void loop() {
  unsigned long now = millis(); // Miliseconds after device started
  static const unsigned long CHECK_INTERVAL = 1000; // ms
  static unsigned long lastCheckTime = 0;
  static const unsigned long COMM_INTERVAL = 10000; // ms
  static unsigned long lastCommTime = 0;

  if(now > (CHECK_INTERVAL + lastCheckTime)) { // Second passed after last check?
    lastCheckTime = now;
    check();
    // 10 seconds passed after last communication or we forced sending new message
    if(forceComm || (now > (COMM_INTERVAL + lastCommTime))) {
      if (communicate()) {
        lastCommTime = now;
        forceComm = false;
      }
    }
  }
}

// https://playground.arduino.cc/Code/EEPROMWriteAnything/
template <class T> int EEPROM_writeAnything(int ee, const T& value) {
  const byte* p = (const byte*)(const void*)&value;
  unsigned int i;
  for (i = 0; i < sizeof(value); i++) EEPROM.write(ee++, *p++);
  return i;
}

template <class T> int EEPROM_readAnything(int ee, T& value) {
  byte* p = (byte*)(void*)&value;
  unsigned int i;
  for (i = 0; i < sizeof(value); i++) *p++ = EEPROM.read(ee++);
  return i;
}