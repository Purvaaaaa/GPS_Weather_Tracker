/**************************************************************************************************
* Project: Realtime GPS location Tracker and weather monitoring Application using M5stack
* Author: Purva Tiwari (EID: pt8734)
* Algorithm:
*   1. Include all header files and import the Tiny GPS library into Arduino
*   2. Configure Device keys and Device Token for Qubitro portal connection. The device must be registered on Qubitro
*   3. Define API calling key for openweathermap.net free APIs. (API key must be generated from the portal openweathermap.net)
*   4. Define wifi access SSID and Password
*   5. Setup function to initialize all connections and load parameters
*   6. Start of Loop: Loop function will be invoked iteratively and collect data from the below components:
*        - read GPS Sttalite location data from M5stack mini GPS device
*        - call Weather API to get local weather condition data
*        - calculate M5stick current Baterry % level.
*   7. Craft a custom JSON payload to combine GPS Satellite data and Weather data and M5stick Battery level data into one JSON payload
*   8. call MQTT client (Qubitro service) to publish the JSON payload
*   9. End of loop
****************************************************************************************************/

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
String temperature_str = "0";

// Weather API Access Key. NOTE This key will be generated from api.openweathermap.net portal after user account creation
String openWeatherMapApiKey = "a246f96f326a55e6c57782ec160ff8bc";
// openWeatherMapApiKey API call requirement: we need to pass City and COuntry code to to make an API call
String city = "Austin";
String countryCode = "USA";
String jsonBuffer;
unsigned long lastTime = 0;
// Set timer to 20 seconds (20000 milliseconds) - Free API call has limit and only 2000 calls per day are allowed on free account
// This project will call weather API in every 20 seconds and refresh the weather data
unsigned long timerDelay = 20000;
// WiFi Client
WiFiClient wifiClient;
// Qubitro Client
QubitroMqttClient mqttClient(wifiClient);
// Device Parameters
char deviceID[] = "3a299bc3-2739-4f9a-b24a-ec639a22c21f";
char deviceToken[] = "5ITaXE22MVwWFDpE9Sa5Gc8HN0rnhF9GGHccX1rr";
// WiFi Parameters - replace below * with wifi user name and passowrd
const char* ssid = "*******";
const char* password = "********";

//Setup function will be invoked one time in the beginning of flow to establish all connections
void setup()
{
M5.begin();
M5.Lcd.setRotation(3);
M5.Lcd.fillScreen(BLACK);
M5.Lcd.setSwapBytes(true);
M5.Lcd.setTextSize(1);
M5.Lcd.setCursor(7, 20, 2);
M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
//below are mendatory GPS device PIN configuration to establish a connection
ss.begin(9600, SERIAL_8N1, 33, 32);
wifi_init();
qubitro_init();
}
//Loop function will be involved iteratively and refresh GPS data and Weather data
void loop()
{
  //Below if condition will make a weather API call. refresh weather API data in every 20 seconds or refresh if previous API call was failed.
  if (((millis() - lastTime) > timerDelay) || (temperature_str == "0")) {
    delay(10000);
    //craft API URL and associate City, country code and API access key
    String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
    //make an API call using HTTP protocol
    jsonBuffer = httpGETRequest(serverPath.c_str());
    myObject = JSON.parse(jsonBuffer);
    //If there is an error in API call then initiatize weather data to zero and retry API call on next iteration
    if (JSON.typeof(myObject) == "undefined") {
      temperature_str = "0";
      pressure_str = "0";
      humidity_str = "0";
      windspeed_str = "0";
   }
   else {
     // Weather API will return big Json response - select only few required attributes
     temperature = myObject["main"]["temp"];  //get Temperature
     pressure = myObject["main"]["pressure"]; //get Air pressure
     humidity = myObject["main"]["humidity"]; //get humidity
     windspeed = myObject["wind"]["speed"];   //get Wind Speed
     //Cast Json object to String
     temperature_str = JSON.stringify(temperature);
     pressure_str = JSON.stringify(pressure);
     humidity_str = JSON.stringify(humidity);
     windspeed_str = JSON.stringify(windspeed);
   }
   //read the current time in milliseconds
   lastTime = millis();
}
//Below block of code will check on GPS device availability and read various GPS statalite attrubutes
while (ss.available() > 0)
if (gps.encode(ss.read()))
{
if (gps.location.isValid())
{
M5.Lcd.setTextColor(RED);
M5.Lcd.setCursor(0, 0);
M5.Lcd.printf("</Qubitro Uplink Done/>");
Lat = gps.location.lat(); //get Latitude from current GPS device location
lat_str = String(Lat , 6);
Lng = gps.location.lng(); //get Logitude from current GPS device location
lng_str = String(Lng , 6);
Alt = gps.altitude.feet();
alt_str = String(Alt);  //get Altitude from current GPS device location
sat = gps.satellites.value(); //get GPS Satatlite code from current GPS connection
Serial.print("satellites found:  ");
Serial.println(sat);
Serial.print("latitude :  ");
Serial.println(lat_str);
M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
M5.Lcd.setCursor(0, 20);
// Print GPS data on serial port
Serial.print("longitude: ");
Serial.println(lng_str);
Serial.print("Alt: ");
Serial.println(gps.altitude.feet());
Serial.print("Course: ");
Serial.println(gps.course.deg());
Serial.print("Speed: ");
Serial.println(gps.speed.mph());
//get current date and time from GPS 
Serial.print("Date: "); printDate();
Serial.print("Time: "); printTime();
//Print GPS data on M5stick screen
M5.Lcd.setCursor(0, 40);
M5.Lcd.printf("latitude : %s", lat_str);
M5.Lcd.printf("longitude : %s", lng_str);

//craft JSON payload and combine GPS data, Weather Data and current Battery status
String payload = "{\"latitude\":" + String(lat_str) + ",\"longitude\":" + String(lng_str) +  ",\"altitude\":" + String(alt_str) +
                    ",\"satellites\":" + String(sat) + ",\"Battery\":" + String(battery) +
                    ",\"temperature\":" + String(temperature_str) + ",\"pressure\":" + String(pressure_str) + ",\"humidity\":" + String(humidity_str) +
                    ",\"windspeed\":" + String(windspeed_str) + "}";

//call MQTT Brocker service to publish data to Qubitro platform
mqttClient.poll();
mqttClient.beginMessage(deviceID);
mqttClient.print(payload);
mqttClient.endMessage();
M5.Lcd.setCursor(0, 60);
M5.Lcd.printf("satellites : %d", sat);
M5.Lcd.print("||");
batteryLevel();   //generate current Battery level in %
M5.Lcd.setCursor(95, 60);
M5.Lcd.printf("Bat : %d", battery);
M5.Lcd.print("%");
delay(5000);
M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
}
}
}

//Function to connect WIFI and retry in case of failure.
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

//Function to establish Qubitro broker connection
void qubitro_init() {
char host[] = "broker.qubitro.com";
int port = 1883;
//Pass Qubitry registered Device ID and Device Token
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

//function to make Weather API calls using HTTP protocol
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
