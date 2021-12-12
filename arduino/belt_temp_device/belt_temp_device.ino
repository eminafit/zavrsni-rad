#include <SPI.h>
#include <RF24.h>
#include "DHT.h"

#define CE_PIN 5
#define CSN_PIN 6
#define VBAT_PIN A9
#define DHT_PIN 11
#define CONTACT_PIN 10
#define ONBOARD_LED_PIN 13
#define DHT_TYPE DHT22
#define DEVICE_TYPE 1
#define DEBUG true

const byte address[6] = "1XXXX";
RF24 radio(CE_PIN, CSN_PIN);
DHT dht(DHT_PIN, DHT_TYPE);

float battery = 3.7;
float temperature;
bool belt = false;
float data[4];
bool forceComm = true;

// https://learn.adafruit.com/adafruit-feather-32u4-basic-proto/power-management#measuring-battery-1753817-9
void checkBattery() {
  float measured = analogRead(VBAT_PIN);
  measured *= 2;
  measured *= 3.3;
  measured /= 1024;
  battery = measured;
  if (DEBUG) {
    Serial.print("VBat: " ); 
    Serial.println(measured);
  }
}

// Belt check
void checkBelt() {
  bool beltStatus = (digitalRead(CONTACT_PIN) == LOW);
  if(DEBUG) {
    Serial.print("Belt contact: ");
    Serial.println(beltStatus);
  }
  if(beltStatus != belt) { forceComm = true; }
  belt = beltStatus;
}

void checkDHT() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Celsius (the default)
  temperature = dht.readTemperature();
  if(DEBUG) {  
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println("°C");
  }
}

bool communicate() {
  digitalWrite(ONBOARD_LED_PIN, HIGH);
  data[0] = DEVICE_TYPE;
  data[1] = battery;
  data[2] = temperature;
  data[3] = belt;
  radio.stopListening();
  bool ok = radio.write( data, sizeof(data) );
  if(DEBUG) { Serial.println(ok ? "Data sent" : "Failed sending data"); }
  digitalWrite(ONBOARD_LED_PIN, LOW);
  return ok;
}

void check() {
  checkBattery();
  checkDHT();
  checkBelt();
}

void setupRadio() {
  SPI.begin();
  if (!radio.begin()) {
    if(DEBUG) { Serial.println(F("Radio hardware not responding!")); }
    while (1) {
      // Infinite loop
    }
  }
  radio.setPayloadSize(sizeof(data));
  radio.stopListening(); // Only transmitting
  radio.openWritingPipe(address);
}

void setup() {
  if(DEBUG) { 
    Serial.begin(9600);
    Serial.println("Starting");
  }
  pinMode(CONTACT_PIN, INPUT);
  pinMode(ONBOARD_LED_PIN, OUTPUT);
  dht.begin();
  setupRadio();
}

void loop() {
  unsigned long now = millis(); 
  static const unsigned long CHECK_INTERVAL = 1000; // ms
  static unsigned long lastCheckTime = 0;
  static const unsigned long COMM_INTERVAL = 10000; // ms
  static unsigned long lastCommTime = 0;

  if(now > (CHECK_INTERVAL + lastCheckTime)) { 
    lastCheckTime = now;
    check();
    //da li je proslo 10 sek od zadnje komunikacije
    // ili je komunikacija forsirana
    if(forceComm || (now > (COMM_INTERVAL + lastCommTime))) {
      if (communicate()) {
        lastCommTime = now;
        forceComm = false;
      }
    }
  }
}
