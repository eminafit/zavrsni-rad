// Sound and visual alarming related code

// Make sound at frequency of 1000Hz for duration in miliseconds
void buzz(int duration) {
  noTone(BUZZER_PIN);
  tone(BUZZER_PIN, 1000, duration);
}

// Alarm sound is 300 miliseconds
void makeSoundAlarm() {
  buzz(300);
}

// When starting, set LED pins as output and switch them OFF
void setupLED() {
  pinMode(LED_BELT_PIN, OUTPUT);
  digitalWrite(LED_BELT_PIN, LOW);
  pinMode(LED_BATTERY_PIN, OUTPUT);
  digitalWrite(LED_BATTERY_PIN, LOW);
  pinMode(LED_COMM_PIN, OUTPUT);
  digitalWrite(LED_COMM_PIN, LOW);
  pinMode(LED_APP_PIN, OUTPUT);
  digitalWrite(LED_APP_PIN, LOW);
  pinMode(LED_TEMP_PIN, OUTPUT);
  digitalWrite(LED_TEMP_PIN, LOW);
}
