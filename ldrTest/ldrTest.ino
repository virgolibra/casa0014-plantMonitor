// test LDR sensor

int LDRPin = 2;
int lightStatus = 0;

void setup() {
   // open serial connection
  Serial.begin(115200);
  delay(100);

  // start LDR
  pinMode(LDRPin, INPUT);
}

// blink the LED
void loop() {
  // turn on LED
  lightStatus = digitalRead(LDRPin);
  delay(1000);
  Serial.print("Light Status: "); 
  Serial.println(lightStatus);
}
