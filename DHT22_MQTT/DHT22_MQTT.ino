/*
    Get date and time - uses the ezTime library at https://github.com/ropg/ezTime -
    and then show data from a DHT22 on a web page served by the Huzzah and
    push data to an MQTT server - uses library from https://pubsubclient.knolleary.net

    Duncan Wilson
    CASA0014 - 2 - Plant Monitor Workshop
    May 2020


    This is the updated version 03 Nov 2021
    mqtt_server = "mqtt.cetools.org";
    client.setServer(mqtt_server, 1884); 
    client publish topic: student/CASA0014/plant/ucfnmz0/
*/


#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ezTime.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <DHT_U.h>

#include <LiquidCrystal.h>


// Liquid Crystal Display
// lcd(RS,EN,d4,d5,d6,d7);
LiquidCrystal lcd(4,5,14,15,16,2);

#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// Sensors - DHT22 and Nails
uint8_t DHTPin = 12;        // on Pin 2 of the Huzzah
float Temperature;
float Humidity;

int redLED = 1;
bool redFlag = true;
//int blueLED = 2;
DHT dht(DHTPin, DHTTYPE);   // Initialize DHT sensor.

uint8_t soilPin = 0;    // we read a value between 0-1023 from analog pin
int Moisture = 1; // initial value just in case web page is loaded before readMoisture called

int sensorVCC = 13;
int counter = 0;


// Wifi and MQTT
// The sensitive data is in the arduino_secrets.h
#include "arduino_secrets.h" 
/*
**** please enter your sensitive data in the Secret tab/arduino_secrets.h
**** using format below
#define SECRET_SSID "ssid name"
#define SECRET_PASS "ssid password"
#define SECRET_MQTTUSER "user name - eg student"
#define SECRET_MQTTPASS "password";
 */

const char* ssid     = SECRET_SSID;
const char* password = SECRET_PASS;
const char* mqttuser = SECRET_MQTTUSER;
const char* mqttpass = SECRET_MQTTPASS;

const char* mqtt_server = "mqtt.cetools.org";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

ESP8266WebServer server(80);

// Date and time
Timezone GB;


void setup() {
  // declare the built in LED and turn it off
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  pinMode(sensorVCC, OUTPUT); 
  digitalWrite(sensorVCC, HIGH);
//  pinMode(blueLED, OUTPUT); 
//  digitalWrite(blueLED, HIGH);
  pinMode(redLED, OUTPUT); 
  digitalWrite(redLED, HIGH);

  // open serial connection
  Serial.begin(115200);
  delay(100);

  // start DHT sensor
  pinMode(DHTPin, INPUT);
  dht.begin();

  // start LCD
  lcd.begin(16,2);
  lcd.setCursor(0,0);
  lcd.print("Hello!");
  delay(1000);
  lcd.setCursor(0,1);
  lcd.print("Plant Monitor!");
  delay(1000);

  // Initiate the connecting to wifi routine
  startWifi();
  startWebserver();
  syncDate();

  // start MQTT server
  // Once connected to wifi establish connection to mqtt broker
  client.setServer(mqtt_server, 1884);  
  client.setCallback(callback);



}




void loop() {
   digitalWrite(redLED, HIGH);
   server.handleClient();
// minuteChanged() function check the min change. Read the data per minute and send MQTT
// Replace minuteChanged() by secondChanged, which read and update the data per second.
//if (secondChanged()) {
     if (minuteChanged()) {
    readMoisture();
    sendMQTT();
    lcdDisplay();
    Serial.println(GB.dateTime("H:i:s")); // UTC.dateTime("l, d-M-y H:i:s.v T")
//
//    if (redFlag == true){
//      digitalWrite(redLED, LOW);
//      redFlag == false;
//    } else{
//      digitalWrite(redLED, HIGH);
//      redFlag == true;
//    }
    
  }
  client.loop();
}

// The function to display and update LCD data
void lcdDisplay(){
  // lcd clear
  lcd.clear();

  // display temperature
  lcd.setCursor(0,0);
  lcd.print("T:");
  lcd.setCursor(2,0);
  lcd.print(Temperature);

  // display humidity
  lcd.setCursor(8,0);
  lcd.print("H:");
  lcd.setCursor(10,0);
  lcd.print(Humidity);

  // display moisture
  lcd.setCursor(0,1);
  lcd.print("M:");
  lcd.setCursor(2,1);
  lcd.print(Moisture);

  // display time
  lcd.setCursor(5,1);
  lcd.print("fnmz0");
  lcd.setCursor(11,1);
  lcd.print(hour());
  lcd.setCursor(13,1);
  lcd.print(":");
  lcd.setCursor(14,1);
  lcd.print(minute());


  
}

// The function to read moisture data.
void readMoisture(){
  
  // power the sensor
  digitalWrite(sensorVCC, HIGH);
//  digitalWrite(blueLED, LOW);
  delay(100);
  // read the value from the sensor:
  Moisture = analogRead(soilPin);         
  //Moisture = map(analogRead(soilPin), 0,320, 0, 100);    // note: if mapping work out max value by dipping in water     
  //stop power 
  digitalWrite(sensorVCC, LOW);  
//  digitalWrite(blueLED, HIGH);
  delay(100);
  Serial.print("Wet ");
  Serial.println(Moisture);   // read the value from the nails
}


// This function is used to set-up the connection to the wifi
// using the user and passwords defined in the secrets file
// It then prints the connection status to the serial monitor

void startWifi(){
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // lcd 
  lcd.clear();
  lcd.print("Connecting to:");
  lcd.setCursor(0,1);
  lcd.print(ssid);
  lcd.setCursor(0,0);
  
  WiFi.begin(ssid, password);

  // check to see if connected and wait until you are
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0,0);  
  lcd.print("WiFi connected");  
  lcd.setCursor(0,1);
  lcd.print("IP:");
  lcd.setCursor(3,1);
  lcd.print(WiFi.localIP());
  delay(1000);
}


void syncDate() {
  // get real date and time
  waitForSync();
  Serial.println("UTC: " + UTC.dateTime());
  GB.setLocation("Europe/London");
  Serial.println("London time: " + GB.dateTime());

}

void startWebserver() {
  // when connected and IP address obtained start HTTP server
  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("HTTP server started");
}


// This function sends (publishes) a message to a MQTT topic
// once a connection is established with the broker. It sends
// an incrementing variable called value to the topic:
// "student/CASA0014/plant/ucfnmz0"

void sendMQTT() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // client publish topic: student/CASA0014/plant/ucfnmz0/

  Temperature = dht.readTemperature(); // Gets the values of the temperature
  snprintf (msg, 50, "%.1f", Temperature);
  Serial.print("Publish message for t: ");
  Serial.println(msg);
  client.publish("student/CASA0014/plant/ucfnmz0/temperature", msg);

  Humidity = dht.readHumidity(); // Gets the values of the humidity
  snprintf (msg, 50, "%.0f", Humidity);
  Serial.print("Publish message for h: ");
  Serial.println(msg);
  client.publish("student/CASA0014/plant/ucfnmz0/humidity", msg);

  //Moisture = analogRead(soilPin);   // moisture read by readMoisture function
  snprintf (msg, 50, "%.0i", Moisture);
  Serial.print("Publish message for m: ");
  Serial.println(msg);
  client.publish("student/CASA0014/plant/ucfnmz0/moisture", msg);
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

// This function is used to make sure the arduino is connected
// to an MQTT broker before it tries to send a message and to 
// keep alive subscriptions on the broker (ie listens for inTopic)

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    lcd.clear();
    lcd.print("Attempt MQTT");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqttuser, mqttpass)) {
      Serial.println("connected");
      lcd.setCursor(0,1);
      lcd.print("Connected");
      delay(1000);
      // Once connected, publish an announcement...
      client.publish("student/CASA0014/plant/ucfnmz0/outTopic", "hello PLANT");
      // ... and resubscribe
      client.subscribe("student/CASA0014/plant/ucfnmz0/inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      lcd.setCursor(0,1);
      lcd.print("Try again in 5s");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



void handle_OnConnect() {
  Temperature = dht.readTemperature(); // Gets the values of the temperature
  Humidity = dht.readHumidity(); // Gets the values of the humidity
  server.send(200, "text/html", SendHTML(Temperature, Humidity, Moisture));
}

void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}

String SendHTML(float Temperaturestat, float Humiditystat, int Moisturestat) {
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>ESP8266 DHT22 Report</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  ptr += "p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<div id=\"webpage\">\n";
  ptr += "<h1>ESP8266 Huzzah DHT22 Report</h1>\n";

  ptr += "<p>Temperature: ";
  ptr += (int)Temperaturestat;
  ptr += " C</p>";
  ptr += "<p>Humidity: ";
  ptr += (int)Humiditystat;
  ptr += "%</p>";
  ptr += "<p>Moisture: ";
  ptr += Moisturestat;
  ptr += "</p>";
  ptr += "<p>Sampled on: ";
  ptr += GB.dateTime("l,");
  ptr += "<br>";
  ptr += GB.dateTime("d-M-y H:i:s T");
  ptr += "</p>";

  ptr += "</div>\n";
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}
