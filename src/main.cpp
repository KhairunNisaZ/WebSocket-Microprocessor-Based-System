#include <Arduino.h>
// Import required libraries
#include <Arduino_JSON.h>
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <AsyncTCP.h>
#include "SPIFFS.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "DHTesp.h"

// Replace with your network credentials
const char* ssid = "iPhone Khairun Nisa";  // Enter SSID here
const char* password = "nisa4nov";  //Enter Password here

#define DHTPIN 27     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22     // DHT 22 (AM2302)

const int potpin = 34;
int value = 0;

DHT dht(DHTPIN, DHTTYPE);
DHTesp dhtSensor;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

// Json Variable to Hold Sensor Readings
JSONVar readings;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 2000;

// Get Sensor Readings and return JSON object
String getSensorReadings(){
  TempAndHumidity load = dhtSensor.getTempAndHumidity();
  readings["temperature"] = String(load.temperature);
  readings["humidity"] =  String(load.humidity);
  readings["potentiometer"] =  String(value);
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  initSPIFFS();
  // SPIFFS.begin();
  dht.begin();
  dhtSensor.setup(DHTPIN, DHTesp::DHT22);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.serveStatic("/", SPIFFS, "/");

  // Request for the latest sensor readings
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getSensorReadings();
    request->send(200, "application/json", json);
    json = String();
  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  // Start server
  server.begin();
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    // Send Events to the client with the Sensor Readings Every 1/2 seconds
    value = analogRead(potpin);
    // Serial.println(value);
    events.send("ping",NULL,millis());
    events.send(getSensorReadings().c_str(),"new_readings" ,millis());
    lastTime = millis();
  }
}