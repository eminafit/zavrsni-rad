// Settings in EEPROM

// Read settings, each phone can take max of 20 bytes
void readSettings() {
  int address = 0x00;
  for(int x=0;x<PHONES;x++) {
    address += (x * 20);
    EEPROM_readAnything(address, phones[x]);
  }
}

// Write settings
void writeSettings() {
  if(DEBUG) Serial.println("Saving contacts");
  int address = 0x00;
  for(int x=0;x<PHONES;x++) {
    address += (x * 20);
    EEPROM_writeAnything(address, phones[x]);
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