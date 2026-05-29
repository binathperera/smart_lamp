#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>

const char* ssid = "SLT_FIBRE";
const char* password = "316WIFI2B";

const int LED_BUILTIN = 2;
const int BULB = 4;
float s;

#define AWS_IOT_PUBLISH_TOPIC   "sdk/test/java"
#define AWS_IOT_SUBSCRIBE_TOPIC "sdk/test/java"

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

void messageHandler(char* topic, byte* payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);
 
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    Serial.print("JSON Parsing failed: ");
    Serial.println(error.c_str());
    return;
  }
 
  const char* message = doc["state"];
  
  // Print the state to verify it parsed correctly
  Serial.print("Parsed state value: ");
  Serial.println(message);
  
  if (message != nullptr) {
    // FIX 2: Use strcmp() to correctly compare the values of text strings
    if (strcmp(message, "ON") == 0) {
      digitalWrite(LED_BUILTIN, HIGH);
      s = 1;
      Serial.println("Action: LED turned HIGH");
      digitalWrite(BULB,HIGH);
    }
    else if (strcmp(message, "OFF") == 0) {
      digitalWrite(LED_BUILTIN, LOW);
      digitalWrite(BULB, LOW);
      s = 0;
      Serial.println("Action: LED turned LOW");
    }
  }
}

void syncTime() {
  // Configure NTP server. Adjust offsets if you need local time, 
  // but UTC (0, 0) works perfectly for certificate validation.
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  
  Serial.print("Waiting for NTP time sync");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) { // Wait until time is updated
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("\nTime synchronized!");
  
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current UTC time: ");
  Serial.print(asctime(&timeinfo));
}

void connectAWS(){
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
 
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
 
  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.setServer(AWS_IOT_ENDPOINT, 8883);
 
  // Create a message handler
  client.setCallback(messageHandler);
 
  Serial.println("Connecting to AWS IOT");
 
  while (!client.connect(THINGNAME))
  {
    // Check if the secure client has an internal error code
    char error_buf[100];
    net.lastError(error_buf, 100);
    if (strlen(error_buf) > 0) {
      Serial.print("TLS Error: ");
      Serial.println(error_buf);
    } else {
      // If no text string is available, print the raw code
      Serial.print("MQTT State Code: ");
      Serial.println(client.state());
    }
    
    delay(2000);
  }
 
  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }
 
  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
 
  Serial.println("AWS IoT Connected!");
}

void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["status"] = s;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BULB, OUTPUT);
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  syncTime();
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
  });
  // ... (other OTA callbacks)
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  connectAWS();
  
  s=0;
}

void loop() {
  ArduinoOTA.handle();
  // put your main code here, to run repeatedly:
  //publishMessage();
  if (!client.connected()) {
    Serial.println("MQTT connection lost. Retrying...");
    connectAWS();
    delay(5000); // Wait 5 seconds before retrying to prevent spamming
  }
  client.loop(); 
}