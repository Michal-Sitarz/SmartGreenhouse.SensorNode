// ------------------------------------- //
// Smart Greenhouse System - Sensor Node //
// ------------------------------------- //

// Sensor info
const char sensorId[] = "arduino1";

// WiFi setup
#include <WiFiNINA.h>
#include "arduino_secrets.h"
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

// MQTT setup
#include <ArduinoMqttClient.h>
WiFiClient wifiClient;
MqttClient mqtt(wifiClient);

const char broker[] = "192.168.1.98";
int port = 1883;
const char topic[]= "greenhouse/conditions";

// Real-Time Clock (RTC)
#include <RTCZero.h>
RTCZero rtc;
const byte year_prefix = 20; // for the 21st century, i.e. 2077

// DHT22 sensor
#include <DHT.h>
#define DHTPIN 2
#define DHTTYPE DHT22
#define INTERVAL 2000 // for 5 minutes put 300000 (5 x 60,000 miliseconds)

DHT dht(DHTPIN, DHTTYPE);

// conditions for sensor readings
float t; 
float h;

void setup() {
  Serial.begin(9600);
  //while (!Serial) {;} // this checks for native USB support connectivity
  Serial.println("Initializing setup...");
  
  // WiFi connection
  Serial.print("Attempting to connect to the WiFi network: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    Serial.print(".");
    delay(5000);
  }
  Serial.println();
  Serial.println("Connected to the WiFi network succesfully!");
  displayWifiInfo();

  // MQTT broker connection
  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  if (!mqtt.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqtt.connectError());
    while (1);
  }
  Serial.println("Connected to the MQTT broker succesfully!");
  Serial.println();

  // real-time clock setup
  rtc.begin();
  setCurrentDateTime();

  // sensors array
  dht.begin();
}

void loop() {
  delay(INTERVAL);
  
  // Read conditions from DHT sensor
  t = dht.readTemperature();
  h = dht.readHumidity();

  // Check if readings failed and exit early if they did to try again.
  if(isnan(t) || isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Prepare the string with readings
  String msgJson = "{ ";
  msgJson.concat("\"AirTemperature\": ");
  msgJson.concat(trimZero(t));
  msgJson.concat(", \"AirHumidity\": ");
  msgJson.concat(trimZero(h));
  msgJson.concat(", \"SoilMoisture\": ");
  msgJson.concat(0);
  msgJson.concat(", \"LightIntensity\": ");
  msgJson.concat(0);
  msgJson.concat(", \"SensorNodeId\": ");
  msgJson.concat("\"");
  msgJson.concat(sensorId);
  msgJson.concat("\"");
  msgJson.concat(" }");


  // Display reading in the serial console
  Serial.println(msgJson);

  // Send message to the MQTT broker
  mqtt.beginMessage(topic);
  mqtt.print(msgJson);
  mqtt.endMessage();

  mqtt.poll(); // <- keep the connection alive and prevent from disconnection
}

// helper method to trim trailing zeros from the DHT22 temp & humidity readouts
String trimZero(float x){
  String s = "";
  s.concat(x);
  s.remove(s.length()-1);
  return s;
}

// helper method to display at the serial console the WiFi info after connecting
void displayWifiInfo(){
  // Arduino board info
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // WiFi network info
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // signal strength
  long rssi = WiFi.RSSI();  
  Serial.print("signal strength (RSSI): ");
  Serial.println(rssi);
  
  Serial.println("---------------------------");
  Serial.println();
}

// helper method to setup current date and time for the RTC
void setCurrentDateTime(){

  //const char compile_time[] = __TIME__;
  //const char compile_date[] = __DATE__;

  const byte hours = 12;
  const byte minutes = 14;
  const byte seconds = 0;

  const byte day = 21;
  const byte month = 7;
  // year prefix set in global constants
  const byte year = 21;

  rtc.setHours(hours);
  rtc.setMinutes(minutes);
  rtc.setSeconds(seconds);

  rtc.setDay(day);
  rtc.setMonth(month);
  rtc.setYear(year);
}

// helper method to return current date & time as a string
String getDateTime(){
  String s = "";

  if (rtc.getDay() < 10){s.concat("0");}
  s.concat(rtc.getDay());
  s.concat("/");
  if (rtc.getMonth() < 10){s.concat("0");}
  s.concat(rtc.getMonth());
  s.concat("/");
  s.concat(year_prefix);
  if (rtc.getYear() < 10){s.concat("0");}
  s.concat(rtc.getYear());
  s.concat(" ");

  if (rtc.getHours() < 10){s.concat("0");}
  s.concat(rtc.getHours());
  s.concat(":");
  if (rtc.getMinutes() < 10){s.concat("0");}
  s.concat(rtc.getMinutes());
  s.concat(":");
  if (rtc.getSeconds() < 10){s.concat("0");}
  s.concat(rtc.getSeconds());

  return s;
}
