// blinkTest.ino
// A simple blink test to test Adafruit Feather HUZZAH ESP8266 board.

// set pin mode
void setup() {
  pinMode(0, OUTPUT);
}

// blink the LED
void loop() {
  // turn on LED
  digitalWrite(0, HIGH);
  delay(1000);

  // turn off LED
  digitalWrite(0, LOW);
  delay(1000);
}
