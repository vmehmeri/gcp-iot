
/*******************************************************************
This code implements the Quantified Desk project, which consists of
an Arduino/Genuino MKR1000 hooked up to an ultrasonic sensor to measure
the distance to the floor (height) of a standing desk.

It keeps track of the time duration (in minutes) in each state (Sitting/Standing)
which is defined by a distance threshold above which the desk is considered to be
in standing position, and below which it is considered in sitting position.

A time counter for each state increases if the corresponding state is the current state.
This data is periodically sent to GCP IoT Core along with a timestamp. 

Complete Project with Dashboard visualization
https://github.com/vmehmeri/gcp-iot/QuantifiedDesk

PRE-REQUISITE
Setup GCP IoT Core and upload SSL certificate to device. 

  - Instructions for setting up GCP IoT Core:
  https://cloud.google.com/iot/docs/quickstart
  
  - Instructions for updating Firmware and adding SSL certificate to device:
  https://www.arduino.cc/en/Tutorial/FirmwareUpdater

 *******************************************************************/

#include "arduino_secrets.h"
#include <Ultrasonic.h>
#include <WiFi101.h>
#include <RTCZero.h>
#include <WiFiUdp.h>

#include <MQTT.h>
#include <CloudIoTCore.h>
#include <CloudIoTCoreMqtt.h>
#include <jwt.h>


// SECRETS CONFIG -- PLEASE SET THESE IN arduino_secrets.h FILE
//WiFi creds -------------------------------------------------------------------------------------------
char ssid[] = SECRET_SSID; //  your network SSID (name)
char pass[] = SECRET_WIFIPASSWD;    // your network password (use for WPA, or use as key for WEP)

// Google Cloud IoT details.
const char* project_id = SECRET_GCP_PROJECTID;
const char* location = SECRET_GCP_LOCATION; 
const char* registry_id = SECRET_GCP_REGISTRYID; 
const char* device_id = SECRET_GCP_DEVICEID; 

// To get the private key run (where private-key.pem is the ec private key
// used to create the certificate uploaded to google cloud iot):
// openssl ec -in <private-key.pem> -noout -text
// and copy priv: part.
// The key length should be exactly the same as the key length bellow (32 pairs
// of hex digits). If it's bigger and it starts with "00:" delete the "00:". If
// it's smaller add "00:" to the start. If it's too big or too small something
// is probably wrong with your key.
const char* private_key_str = SECRET_GCP_PRIVATEKEY;

// Time (seconds) to expire token += 20 minutes for drift
const int jwt_exp_secs = 3600*24; // Maximum 24H (3600*24)

// In case we ever need extra topics
const int ex_num_topics = 0;
const char* ex_topics[ex_num_topics];

//------------------------------------------------------------------------------------------------------

#define slotNumber 1 //This will vary for multi slot devices 

// Google Cloud IoT configuration that you don't need to change
Client *netClient;
CloudIoTCoreDevice *device;
CloudIoTCoreMqtt *mqtt;
MQTTClient *mqttClient;
unsigned long iss = 0;
String jwt;


// Project Config variables
unsigned int distanceThreshold = 82; // Define here the distance (in cm) that marks the threshold between sitting and standing
const int GMT = 2; //change this to adapt it to your time zone
const int TrigPin = 4; //number of the Trigger pin
const int EchoPin = 5; //number of the Echo pin

RTCZero rtc;
WiFiSSLClient client;
Ultrasonic ultrasonic(TrigPin, EchoPin);

unsigned int count = 0;
unsigned long distanceSum = 0;
unsigned int timeStanding = 0;
unsigned int timeSitting = 0;
unsigned long startMillis; 
unsigned long distanceAvg;
unsigned long distance;

int status = WL_IDLE_STATUS;

void setup() {
  Serial.begin(9600);
  
  while (!Serial) {
      ; // wait for serial port to connect. Needed for native USB port only
    }
    
   startMillis = millis();
   rtc.begin();
   
   // check for the presence of the shield:
   if (WiFi.status() == WL_NO_SHIELD) {
     Serial.println("WiFi shield not present");
     // don't continue:
     while (true);
   }

   // attempt to connect to Wifi network:
   while ( status != WL_CONNECTED) {
     Serial.print("Attempting to connect to SSID: ");
     Serial.println(ssid);
     status = WiFi.begin(ssid, pass);

     // wait 10 seconds for connection:
     delay(10000);
   }
  
  Serial.println("Connected to Wi-Fi");
  
  // Get Real-Time from NTP using built-in RTC module
  unsigned long epoch;
  int numberOfTries = 0, maxTries = 6;
  do {
    epoch = WiFi.getTime();
    numberOfTries++;
  }
  while ((epoch == 0) && (numberOfTries < maxTries));

  if (numberOfTries == maxTries) {
    Serial.print("NTP unreachable!!");
    while (1);
  }
  else {
    Serial.print("Epoch received: ");
    Serial.println(epoch);
    rtc.setEpoch(epoch);

    Serial.println();
  }
  
  setupCloudIoT();
  
}

void loop() {

  delay(5000); // wait 5 seconds

  ultrasonic.measure();
  distance = ultrasonic.get_cm();
  
  Serial.print("Sensor(cm): ");
  Serial.println(distance);
  
  distanceSum = distanceSum + distance;
  count = count + 1;
  
  /* Takes the average of the last 12 measurements (the number 12 is arbitrary, but
   *  with a 5-second delay between measurements and 12 measurements, that means 
   *  data is aggregated and sent to Azure every 60 seconds, which seems reasonable for
   *  this project.
   */
  if (count == 12) {
    distanceAvg = distanceSum / count;
    count = 0;
    distanceSum = 0;

    if (distanceAvg < distanceThreshold) {
      // Add elapsed time since last measurement to sitting time
      timeSitting = timeSitting + ((millis()-startMillis)/1000);  
    } else {
      // Add elapsed time since last measurement to standing time
      timeStanding = timeStanding + ((millis()-startMillis)/1000);  
    }
    
    startMillis = millis();
    
    // Show current aggregate numbers
    printRTCDate();
    printRTCTime();
    Serial.println();
    Serial.print("Time sitting: ");
    Serial.print(timeSitting/60); 
    Serial.println("min");
    Serial.print("Time standing: ");
    Serial.print(timeStanding/60);
    Serial.println("min");
    Serial.println("");

    // Creates a string to send to Azure IoT HUB. 
    // It's simply comma-separated string of values for sitting and standing, followed by date and time (for timestamping)
    String data_string = (String(timeSitting/60) + "," + String(timeStanding/60) + "," + getRTCDate() + "," + getRTCTime());
    //Serial.println(data_string);
    
    // Send to GCP
    sendToGcp(data_string);
    
    String response = "";
    char c;
    while (client.available()) {
      c = client.read();
      response.concat(c);
    }
  
  }

}

void printRTCTime()
{
  print2digits(rtc.getHours() + GMT);
  Serial.print(":");
  print2digits(rtc.getMinutes());
  Serial.print(":");
  print2digits(rtc.getSeconds());
  Serial.println();
}

void printRTCDate()
{
  Serial.print(rtc.getDay());
  Serial.print("/");
  Serial.print(rtc.getMonth());
  Serial.print("/");
  Serial.print(rtc.getYear());

  Serial.print(" ");
}

void print2digits(int number) {
  if (number < 10) {
    Serial.print("0");
  }
  Serial.print(number);
}

String getRTCDate()
{
  String date_str = String(rtc.getDay()) + "/" + String(rtc.getMonth()) + "/" + String(rtc.getYear());
  return date_str;
}

String getRTCTime()
{
  String time_str = get2digits(rtc.getHours() + GMT) + ":" + get2digits(rtc.getMinutes()) + ":" + get2digits(rtc.getSeconds());
  return time_str;
}

String get2digits(int number) {
  if (number < 10) {
    return "0" + String(number);
  }
  return String(number);
}

// ------------------------ GCP Helper functions ------------------------
String getJwt() {
  // Disable software watchdog as these operations can take a while.
  Serial.println("Refreshing JWT...");
  iss = WiFi.getTime();
  jwt = device->createJWT(iss, jwt_exp_secs);
  return jwt;
}

bool publishTelemetry(String data) {
  return mqtt->publishTelemetry(data);
}

bool publishTelemetry(const char* data, int length) {
  return mqtt->publishTelemetry(data, length);
}

bool publishTelemetry(String subfolder, String data) {
  return mqtt->publishTelemetry(subfolder, data);
}

bool publishTelemetry(String subfolder, const char* data, int length) {
  return mqtt->publishTelemetry(subfolder, data, length);
}

void connect() {
  //connectWifi();
  mqtt->mqttConnect();
}

void setupCloudIoT() {
  device = new CloudIoTCoreDevice(
      project_id, location, registry_id, device_id,
      private_key_str);

  //setupWifi();
  netClient = new WiFiSSLClient();

  mqttClient = new MQTTClient(512);
  mqttClient->setOptions(3600*8, true, 5000); // keepAlive, cleanSession, timeout
  mqtt = new CloudIoTCoreMqtt(mqttClient, netClient, device);
  mqtt->setUseLts(false);
  mqtt->startMQTT();
}

void sendToGcp(String data) {
  mqtt->loop();
  delay(10);  // <- fixes some issues with WiFi stability

  if (!mqttClient->connected()) {
    connect();
  }

  publishTelemetry(data);
  
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
}