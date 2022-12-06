#include <M5StickC.h>
#include <TinyGPS++.h>
#include <QubitroMqttClient.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
TinyGPSPlus gps;
HardwareSerial ss(2);
float Lat, Lng, Alt;
JSONVar temperature, pressure, humidity, windspeed;
String  lat_str , lng_str, alt_str, pressure_str, humidity_str, windspeed_str;
int sat, battery = 0;
float b, c = 0;
JSONVar myObject;
//Weather API configuration
String temperature_str = "0";

// Weather API Domain name with URL path or IP address with path
String openWeatherMapApiKey = "a246f96f326a55e6c57782ec160ff8bc";
// Example:
//String openWeatherMapApiKey = "bd939aa3d23ff33d3c8f5dd1dd435";
// Replace with your country code and city
String city = "Austin";
String countryCode = "USA";
String jsonBuffer;
unsigned long lastTime = 0;
// Set timer to 10 seconds (10000 milliseconds)
//Timer set to 1 hour (3600000 milliseconds) - API call limit max 2000 calls per day
unsigned long timerDelay = 3600000;
// WiFi Client
WiFiClient wifiClient;
// Qubitro Client
QubitroMqttClient mqttClient(wifiClient);
// Device Parameters
char deviceID[] = "3a299bc3-2739-4f9a-b24a-ec639a22c21f";
char deviceToken[] = "5ITaXE22MVwWFDpE9Sa5Gc8HN0rnhF9GGHccX1rr";
// WiFi Parameters
const char* ssid = "ATTB47FZWa";
const char* password = "=5vf4z5#w#hd";
void setup()
{
M5.begin();
M5.Lcd.setRotation(3);
M5.Lcd.fillScreen(BLACK);
M5.Lcd.setSwapBytes(true);
M5.Lcd.setTextSize(1);
M5.Lcd.setCursor(7, 20, 2);
M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
ss.begin(9600, SERIAL_8N1, 33, 32);
wifi_init();
qubitro_init();
}
void loop()
{

if (((millis() - lastTime) > timerDelay) || (temperature_str == "0")) {
  delay(10000);
  String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
  jsonBuffer = httpGETRequest(serverPath.c_str());
  //Serial.println(jsonBuffer);
   myObject = JSON.parse(jsonBuffer);
   if (JSON.typeof(myObject) == "undefined") {
     temperature_str = "0";
     pressure_str = "0";
     humidity_str = "0";
     windspeed_str = "0";
   }
   else {
     temperature = myObject["main"]["temp"];
     pressure = myObject["main"]["pressure"];
     humidity = myObject["main"]["humidity"];
     windspeed = myObject["wind"]["speed"];

     temperature_str = JSON.stringify(temperature);
     pressure_str = JSON.stringify(pressure);
     humidity_str = JSON.stringify(humidity);
     windspeed_str = JSON.stringify(windspeed);
     
   }
   lastTime = millis();
}

while (ss.available() > 0)
if (gps.encode(ss.read()))
{
if (gps.location.isValid())
{
M5.Lcd.setTextColor(RED);
M5.Lcd.setCursor(0, 0);
M5.Lcd.printf("</Qubitro Uplink Done/>");
Lat = gps.location.lat();
lat_str = String(Lat , 6);
Lng = gps.location.lng();
lng_str = String(Lng , 6);
Alt = gps.altitude.feet();
alt_str = String(Alt);
sat = gps.satellites.value();
Serial.print("satellites found:  ");
Serial.println(sat);
Serial.print("latitude :  ");
Serial.println(lat_str);
M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
M5.Lcd.setCursor(0, 20);
M5.Lcd.printf("latitude : %s", lat_str);
Serial.print("longitude: ");
Serial.println(lng_str);
//*Just for example
Serial.print("Alt: ");
Serial.println(gps.altitude.feet());
Serial.print("Course: ");
Serial.println(gps.course.deg());
Serial.print("Speed: ");
Serial.println(gps.speed.mph());
Serial.print("Date: "); printDate();
Serial.print("Time: "); printTime();
M5.Lcd.setCursor(0, 40);
M5.Lcd.printf("longitude : %s", lng_str);
String payload = "{\"latitude\":" + String(lat_str) + ",\"longitude\":" + String(lng_str) +  ",\"altitude\":" + String(alt_str) +
                    ",\"satellites\":" + String(sat) + ",\"Battery\":" + String(battery) +
                    ",\"temperature\":" + String(temperature_str) + ",\"pressure\":" + String(pressure_str) + ",\"humidity\":" + String(humidity_str) +
                    ",\"windspeed\":" + String(windspeed_str) + "}";
                    
mqttClient.poll();
mqttClient.beginMessage(deviceID);
mqttClient.print(payload);
mqttClient.endMessage();
M5.Lcd.setCursor(0, 60);
M5.Lcd.printf("satellites : %d", sat);
M5.Lcd.print("||");
batteryLevel();
M5.Lcd.setCursor(95, 60);
M5.Lcd.printf("Bat : %d", battery);
M5.Lcd.print("%");
delay(5000);
M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
}
}
}
void wifi_init() {
// Set WiFi mode
WiFi.mode(WIFI_STA);
// Disconnect WiFi
WiFi.disconnect();
delay(100);
// Initiate WiFi connection
WiFi.begin(ssid, password);
// Print connectivity status to the terminal
Serial.print("Connecting to WiFi...");
while (true)
{
delay(1000);
Serial.print(".");
if (WiFi.status() == WL_CONNECTED)
{
Serial.println("");
Serial.println("WiFi Connected.");
Serial.print("Local IP: ");
Serial.println(WiFi.localIP());
Serial.print("RSSI: ");
Serial.println(WiFi.RSSI());
M5.Lcd.setTextColor(RED);
M5.Lcd.setCursor(0, 8);
M5.Lcd.printf("Network Connected");
break;
}
}
}
void qubitro_init() {
char host[] = "broker.qubitro.com";
int port = 1883;
mqttClient.setId(deviceID);
mqttClient.setDeviceIdToken(deviceID, deviceToken);
Serial.println("Connecting to Qubitro...");
if (!mqttClient.connect(host, port))
{
Serial.print("Connection failed. Error code: ");
Serial.println(mqttClient.connectError());
Serial.println("Visit docs.qubitro.com or create a new issue on github.com/qubitro");
}
Serial.println("Connected to Qubitro.");
M5.Lcd.setTextColor(RED);
M5.Lcd.setCursor(0, 25);
M5.Lcd.printf("Uplink Established");
mqttClient.subscribe(deviceID);
delay(2000);
//M5.Lcd.fillScreen(BLACK);
}
void batteryLevel() {
c = M5.Axp.GetVapsData() * 1.4 / 1000;
b = M5.Axp.GetVbatData() * 1.1 / 1000;
//  M5.Lcd.print(b);
battery = ((b - 3.0) / 1.2) * 100;
if (battery > 100)
battery = 100;
else if (battery < 100 && battery > 9)
M5.Lcd.print(" ");
else if (battery < 9)
M5.Lcd.print("  ");
if (battery < 10)
M5.Axp.DeepSleep();
Serial.print("battery: ");
Serial.println(battery);
}
void printDate()
{
Serial.print(gps.date.day());
Serial.print("/");
Serial.print(gps.date.month());
Serial.print("/");
Serial.println(gps.date.year());
}
// printTime() formats the time into "hh:mm:ss", and prints leading 0's
// where they're called for.
void printTime()
{
Serial.print(gps.time.hour());
Serial.print(":");
if (gps.time.minute() < 10) Serial.print('0');
Serial.print(gps.time.minute());
Serial.print(":");
if (gps.time.second() < 10) Serial.print('0');
Serial.println(gps.time.second());
}

//API call
String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);

  // Send HTTP GET request
  int httpResponseCode = http.GET();

  String payload = "{}";

  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
  return payload;
}
