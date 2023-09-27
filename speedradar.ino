//APPENDIX B: Program Codes
#include <Arduino.h>
#include <SPI.h>
#include <DMD2.h>
#include <fonts/SystemFont5x7.h>
#include <espnow.h> 
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
//#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <Firebase_ESP_Client.h>
// Provide the token generation process info.
#include <addons/TokenHelper.h>
// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

#include "time.h"
/* 1. Define the WiFi credentials */
#define WIFI_SSID "Lorra"
#define WIFI_PASSWORD "hellooooo"
/* 2. Define the API Key */
#define API_KEY "AIzaSyBlMuxqSNO-GdK6u8kQf7gDLr-NP-nqVu0"
/* 3. Define the RTDB URL */
#define DATABASE_URL "https://terrestrial-speed-radar-default-rtdb.europewest1.firebasedatabase.app/" //<databaseName>.firebaseio.com or 
<databaseName>.<region>.firebasedatabase.app
/* 4. Define the user Email and password that alreadey registerd or added in your 
project */
#define USER_EMAIL "ookannumber1@gmail.com"
#define USER_PASSWORD "speedRADAR"
// Define NTP Client to get time
WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 3600;

NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
// Set web server port number to 80
WiFiServer server(80);
SPIDMD dmd(1,1); // DMD controls the entire display
// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
// Structure to keep the timePassed and carSpeed data
// Is also required in the client to be able to save the data directly
typedef struct speed_time {
 float timePassed;
 double carSpeed;
};
// Create a struct_message called myData
speed_time speedData;
String months[12]={"January", "February", "March", "April", "May", "June", 
"July", "August", "September", "October", "November", "December"};
String uid;
// Database main path 
String databasePath;
//Updated in every loop
String parentPath;
String speedpath = "/speed";
String timePath = "/timestamp";
String datePath = "/datePath";
//send new readings every 1 minutes
unsigned long previous_time = 0;
unsigned long Delay = 6000;
// callback function executed when data is received
void OnRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
 memcpy(&speedData, incomingData, sizeof(speedData));
 Serial.print("Bytes received: ");

 sendDataToCloud();
 Serial.println(len);
 Serial.print("timePassed: ");
 Serial.println(speedData.timePassed);
 Serial.print("carSpeed: ");
 Serial.println(speedData.carSpeed);
}
void setup()
{
 Serial.begin(9600);
 dmd.setBrightness(255);
 dmd.selectFont(SystemFont5x7);
 dmd.begin();
 WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 Serial.print("Connecting to Wi-Fi");
 unsigned long ms = millis();
 while (WiFi.status() != WL_CONNECTED)
 {
 Serial.print(".");
 delay(300);
 timeClient.begin();
 }
 Serial.println();
 Serial.print("Connected with IP: ");
 Serial.println(WiFi.localIP());
 Serial.println();
 // Printout MAC address
 Serial.println();
 Serial.print("ESP Board MAC Address: ");
 Serial.println(WiFi.macAddress());
 // Init ESP-NOW
 if (esp_now_init() != ERR_OK) {
 Serial.println("There was an error initializing ESP-NOW");
 return;
 }
 // Once the ESP-Now protocol is initialized, we will register the callback function
 // to be able to react when a package arrives in near to real time without pooling 
every loop.
 esp_now_register_recv_cb(OnRecv);
 /* Assign the api key (required) */
 config.api_key = API_KEY;
 /* Assign the user sign in credentials */
 auth.user.email = USER_EMAIL;
 auth.user.password = USER_PASSWORD;
 /* Assign the RTDB URL (required) */
 config.database_url = DATABASE_URL;
 /* Assign the callback function for the long running token generation task */
 config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
 Firebase.begin(&config, &auth);
 Firebase.reconnectWiFi(true);
// Getting the user UID might take a few seconds
 Serial.println("Getting User UID");
 while ((auth.token.uid) == "") {
 Serial.print('.');
 delay(1000);
 }
 // Print user UID
 uid = auth.token.uid.c_str();
 Serial.print("User UID: ");
 Serial.println(uid);
 // Update database path
 databasePath = "/UsersData/" + uid + "/readings";
}
void loop(){
 dmd.drawString(2,2,String(speedData.carSpeed));
}
void sendDataToCloud(){
 timeClient.update();
time_t epochTime = timeClient.getEpochTime();
 //Get a time structure
struct tm *ptm = gmtime ((time_t *)&epochTime); 
//Month day
int monthDay = ptm->tm_mday;
//Current month
int currentMonth = ptm->tm_mon+1;
//Current Year
int currentYear = ptm->tm_year+1900;
//Print complete date Time:
String currentDate = String(currentYear) + "-" + String(currentMonth) + "-" + 
String(monthDay);
String currentTime = String(timeClient.getHours()) + ":" + 
String(timeClient.getMinutes()) + ":" + String(timeClient.getSeconds());
String currentStamp = String(currentDate) + ":" + String(currentTime);
if (Firebase.ready() && (millis() - previous_time > Delay || previous_time == 0))
{
 previous_time = millis();
 Serial.print("CurrentStamp: ");
 Serial.println(currentStamp);
 FirebaseJson json;
 parentPath= databasePath + "/" + String(currentTime);
 json.set(speedpath.c_str(), String(speedData.carSpeed));
 json.set(timePath, String(currentTime));
 json.set(datePath, String(currentDate));
 Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, 
parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());}}
