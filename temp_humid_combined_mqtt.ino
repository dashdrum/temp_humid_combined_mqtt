// ADC_MODE(ADC_VCC);  

#include "config.h"

#include <ESP8266WiFi.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h> // OTA Updates
#include <ArduinoJson.h>
#include <PubSubClient.h>  
#include "DHT.h"                  // DHT library
#include <WEMOS_SHT3X.h>          // SHT30 Library
#include <ArduinoJson.h>          // JSON Library
#include <Seeed_BME280.h>         // BMP/E Library
#include <Wire.h>   


//------------------------------------------------------------------------------------------------------- //

boolean disable_deep_sleep = false;
long lastReport = 0;

//Setup the web server for http OTA updates. 
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

// Intialize WiFi libraries
WiFiClient espClient;
WiFiManager wifiManager;

//Initialize MQTT
PubSubClient client(espClient);


#define DHTPIN CONFIG_DHTPIN
#define DHTTYPE DHT22

BME280 bme280;
DHT dht(DHTPIN, DHTTYPE);
SHT3X sht30(0x45);


// ------------------------------------------------------------------------------------------------------ //
void callback(char* topic, byte* payload, unsigned int length) {
  
  String topicStr = topic;
//  Serial.print("Topic received: ");
//  Serial.println(topicStr);
  
  payload[length] = '\0';
  char* c_payload = (char*)payload;
  String payloadStr = (String)c_payload;
//  Serial.print("Payload received: ");
//  Serial.println(c_payload);
  
  if (topicStr == CONFIG_MQTT_DISABLE_DEEP_SLEEP_TOPIC){
    if(payloadStr == CONFIG_MQTT_DISABLE_DEEP_SLEEP){
      disable_deep_sleep = true;
      Serial.println("Deep Sleep Disabled");
    } else if(payloadStr == CONFIG_MQTT_ENABLE_DEEP_SLEEP){
      disable_deep_sleep = false;
      Serial.println("Deep Sleep Enabled");
    }
  }
  if(topicStr == CONFIG_MQTT_RESTART_COMMAND_TOPIC){
    if(payloadStr == CONFIG_MQTT_RESTART_DEVICE){
      Serial.println("Restarting Device");
      ESP.restart();
    }
  }
}

void send_mqtt_config(){
  DynamicJsonDocument doc(1024);
  char message[512];
  char chipID[8];
  char friendly_name[128] = CONFIG_FRIENDLY_NAME;
  char mqtt_name[128];

  snprintf(chipID, sizeof chipID, "%08X", ESP.getChipId());
  Serial.print("Chip ID:");
  Serial.println(chipID);
  
// Restart Switch config 

  mqtt_name[0] = '\0';
  strcpy(mqtt_name, friendly_name);
  strcat(mqtt_name, " Restart");
  doc["name"] = mqtt_name;
  doc["state_topic"] = CONFIG_MQTT_RESTART_STATE_TOPIC;
  doc["command_topic"] = CONFIG_MQTT_RESTART_COMMAND_TOPIC;
//  doc["availability_topic"] = CONFIG_MQTT_AVAILABILITY_TOPIC;
  doc["unique_id"] = CONFIG_MQTT_RESTART_STATE_TOPIC;
  doc["icon"] = "mdi:restart";
  JsonObject device  = doc.createNestedObject("device");
  device["name"] = CONFIG_HOST;
  device["identifiers"] = chipID;
//  device["model"] = CONFIG_DEVICE_MODEL;
  
//  Serial.print("Config topic: ");
//  Serial.println(CONFIG_MQTT_RESTART_CONFIG_TOPIC);
//  serializeJsonPretty(doc, Serial);
//  Serial.println("");
  serializeJson(doc, message, 1024);
//  Serial.print("Sending configuration: ");
//  Serial.println(message);
//  Serial.println(strlen(message));
  client.publish_P(CONFIG_MQTT_RESTART_CONFIG_TOPIC, message, true);

  doc.remove("command_topic");
  
// Temperature config  
  
  mqtt_name[0] = '\0';
  strcpy(mqtt_name, friendly_name);
  strcat(mqtt_name, "  Temperature");
  doc["name"] = mqtt_name;
  doc["state_topic"] = CONFIG_MQTT_TEMP_TOPIC;
//  doc["expire_after"] = 3600;
//  doc["availability_topic"] = CONFIG_MQTT_AVAILABILITY_TOPIC;
  doc["unique_id"] = CONFIG_MQTT_TEMP_TOPIC;
  doc["unit_of_measurement"] = "°C";
  doc["icon"] = "mdi:thermometer";
  
  serializeJson(doc, message, 1024);
  client.publish_P(CONFIG_MQTT_TEMP_CONFIG_TOPIC, message, true);
  
// Humidity config  
  
  mqtt_name[0] = '\0';
  strcpy(mqtt_name, friendly_name);
  strcat(mqtt_name, " Humidity");
  doc["name"] = mqtt_name;
  doc["state_topic"] = CONFIG_MQTT_HUMID_TOPIC;
  doc["expire_after"] = 3600;
//  doc["availability_topic"] = CONFIG_MQTT_AVAILABILITY_TOPIC;
  doc["unique_id"] = CONFIG_MQTT_HUMID_TOPIC;
  doc["unit_of_measurement"] = "%";
  doc["icon"] = "mdi:water-percent";
    
    serializeJson(doc, message);
    client.publish_P(CONFIG_MQTT_HUMID_CONFIG_TOPIC, message, true);
  
// Pressure config  

  if (CONFIG_SENSOR_TYPE == BME_280){
    mqtt_name[0] = '\0';
    strcpy(mqtt_name, friendly_name);
    strcat(mqtt_name, " Pressure");
    doc["name"] = mqtt_name;
    doc["state_topic"] = CONFIG_MQTT_PRESS_TOPIC;
    doc["expire_after"] = 3600;
//    doc["availability_topic"] = CONFIG_MQTT_AVAILABILITY_TOPIC;
    doc["unique_id"] = CONFIG_MQTT_PRESS_TOPIC;
    doc["unit_of_measurement"] = "mBar";
    doc["icon"] = "mdi:gauge";
    
    serializeJson(doc, message);
    client.publish_P(CONFIG_MQTT_PRESS_CONFIG_TOPIC, message, true);
  }
  
// Battery config  
  
  mqtt_name[0] = '\0';
  strcpy(mqtt_name, friendly_name);
  strcat(mqtt_name, " Battery");
  doc["name"] = mqtt_name;
  doc["state_topic"] = CONFIG_MQTT_BATT_TOPIC;
  doc["expire_after"] = 3600;
//  doc["availability_topic"] = CONFIG_MQTT_AVAILABILITY_TOPIC;
  doc["unique_id"] = CONFIG_MQTT_BATT_TOPIC;
  doc["unit_of_measurement"] = "%";
  doc["icon"] = "mdi:battery";
  
  serializeJson(doc, message);
  client.publish_P(CONFIG_MQTT_BATT_CONFIG_TOPIC, message, true);
  
// WiFi config  
  
  mqtt_name[0] = '\0';
  strcpy(mqtt_name, friendly_name);
  strcat(mqtt_name, " WiFi Signal");
  doc["name"] = mqtt_name;
  doc["state_topic"] = CONFIG_MQTT_WIFI_TOPIC;
  doc["expire_after"] = 3600;
//  doc["availability_topic"] = CONFIG_MQTT_AVAILABILITY_TOPIC;
  doc["unique_id"] = CONFIG_MQTT_WIFI_TOPIC;
  doc["unit_of_measurement"] = "dB";
  doc["icon"] = "mdi:wifi";
  
  serializeJson(doc, message);
  client.publish_P(CONFIG_MQTT_WIFI_CONFIG_TOPIC, message, true);
  
}

void reconnect() {
  //Reconnect to Wifi and to MQTT.
  
  //If Wifi is already connected, then autoconnect doesn't do anything.
  
  wifiManager.autoConnect(CONFIG_HOST);

  Serial.print("Attempting MQTT connection...");

  // Availability and Deep Sleep do not mix, so I've commented the Last Will and Testament portion of the 
  // connect statement below, as well as the Birth Message. In addition, the "availability_topic" entries are commented
  // commented in the MQTT discovery config above.
  
  if (client.connect(CONFIG_HOST, CONFIG_MQTT_USER, CONFIG_MQTT_PASS)){  // , CONFIG_MQTT_AVAILABILITY_TOPIC, 0, 1,"offline")) {
    Serial.println("connected");
  } else {
    
    Serial.print("failed, rc=");
    Serial.println(client.state());
    return;
  }

//  // Send birth message
//  Serial.println("Sending Birth message");
//  client.publish(CONFIG_MQTT_AVAILABILITY_TOPIC, "online", true);

  // Set restart switch status
  Serial.println("Set restart switch status");
  client.publish(CONFIG_MQTT_RESTART_STATE_TOPIC, CONFIG_MQTT_NO_RESTART_DEVICE, true);
  
  // Subscribe
  Serial.print("Subscribing: ");
  Serial.println(CONFIG_MQTT_DISABLE_DEEP_SLEEP_TOPIC);
  client.subscribe(CONFIG_MQTT_DISABLE_DEEP_SLEEP_TOPIC);
  
  // Subscribe
  Serial.print("Subscribing: ");
  Serial.println(CONFIG_MQTT_RESTART_COMMAND_TOPIC);
  client.subscribe(CONFIG_MQTT_RESTART_COMMAND_TOPIC);

  // Send MQTT Discovery config
  send_mqtt_config();
}

float getBatteryLevel(){
  float batt;
  float divisor;

  batt = analogRead(A0);
  Serial.print("Battery: ");
  Serial.println( batt);

  divisor = 1024.0f;
  
  if (CONFIG_POWER_SOURCE == EX){
    return 100.0;
  } else if (CONFIG_POWER_SOURCE == AAB){
    divisor = 1024.0f;
  } else if (CONFIG_POWER_SOURCE == AAA){
    divisor = 1024.0f;
  } else if (CONFIG_POWER_SOURCE == LI){
    divisor = 1008.01701f;
  } else {
    return batt;
  }
  return batt/divisor * 100.0f;
}

long getWiFiSignal(){
//  Serial.println(WiFi.RSSI());
  return WiFi.RSSI();
}

void setup() {
  // init the serial
  Serial.begin(115200);
  delay(100);
  Serial.println();

  // Set the wifi config portal to only show for 3 minutes, then continue
  wifiManager.setConfigPortalTimeout(180);
  // Connect WiFi
  wifiManager.autoConnect(CONFIG_HOST);

  if (WiFi.status() != WL_CONNECTED){  // if not connected then restart ESP
    ESP.restart();
  }
  // init the MQTT connection
  client.setServer(CONFIG_MQTT_HOST, CONFIG_MQTT_PORT);
  client.setCallback(callback);
  
  //setup http firmware update page.
  httpUpdater.setup(&httpServer, CONFIG_OTA_PATH, CONFIG_OTA_USER, CONFIG_OTA_PASS);
  httpServer.begin();

  //  mDNS Setup
  if(MDNS.begin(CONFIG_HOST)){
    if(MDNS.addService("http", "tcp", 80)){
      Serial.printf("HTTPUpdateServer ready! Open http://%s.local%s in your browser and login with username '%s' and your password\n", CONFIG_HOST, CONFIG_OTA_PATH, CONFIG_OTA_USER);
    } else {
       Serial.println('mDNS addService has failed');
    }
  } else {
    Serial.println('mDNS begin has failed');
  }

  if(CONFIG_SENSOR_TYPE == DHT_22){
    dht.begin();
  }

  if (CONFIG_SENSOR_TYPE == BME_280){
    // Setup BME280 
    Serial.println("setup bme280");
      //hangs without i2c device
    if(!bme280.init()){
       Serial.println("Device error!");
    }  
    Serial.println("done");
  }

}

void publishData(float t, float h, float p, float b, long w){

  char c_temp[64];
  char c_humid[64];
  char c_press[64];
  char c_batt[64];
  char c_wifi[64];
  
  if(!isnan(t) && t > -40.0){
    snprintf(c_temp, sizeof c_temp, "%.2f", t);
    client.publish(CONFIG_MQTT_TEMP_TOPIC, c_temp, true);
  }

  if(!isnan(h) && h > 0.0){
    snprintf(c_humid, sizeof c_humid, "%.2f", h);
    client.publish(CONFIG_MQTT_HUMID_TOPIC, c_humid, true);
  }

  if(CONFIG_SENSOR_TYPE == BME_280){
    if(!isnan(p)){
      snprintf(c_press, sizeof c_press, "%.2f", p);
      client.publish(CONFIG_MQTT_PRESS_TOPIC, c_press, true);
    }
  }
  
  snprintf(c_batt, sizeof c_batt, "%.2f", b);
  client.publish(CONFIG_MQTT_BATT_TOPIC, c_batt, true);

  snprintf(c_wifi, sizeof c_wifi, "%+d", w);
  client.publish(CONFIG_MQTT_WIFI_TOPIC, c_wifi, true);
  
}

void handleSensors(){

  float h, t, p, b;
  long w;

  if (CONFIG_SENSOR_TYPE == SHT_3X){
    
    // Read from the SHT30
    sht30.get();
    // Get humidity value
    h = sht30.humidity;
    // Get temperature as Celsius (the default)
    t = sht30.cTemp;  
    
  } else if (CONFIG_SENSOR_TYPE == DHT_22){

    Serial.println("Reading from DHT22");
  
    // Reading DHT temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    t = dht.readTemperature();
    
  } else if (CONFIG_SENSOR_TYPE == BME_280){
    
    t = bme280.getTemperature();
    h = bme280.getHumidity();
    // The 280 has a pressure sensor
    p = bme280.getPressure()/100.0;
  }

  b = getBatteryLevel();

  w = getWiFiSignal();
      
  publishData(t, h, p, b, w);
  
}

void loop() {

  static int loop_counter = 0;
  
  // Make sure we can connect to the MQTT server
  if (!client.connected()) {
    reconnect();
  }

  MDNS.update();
  
  httpServer.handleClient(); //handles requests for the firmware update page

  // Process MQTT
  client.loop();

  long now = millis();
  
  if(now < lastReport) lastReport = now;

  //  Report every 60s.  This is an issue only when deep sleep is disabled.
  if ((now - lastReport) > 60000 || lastReport == 0){
    lastReport = now;
    handleSensors();
  }
    
  delay(500); // wait for things to finish

  if(disable_deep_sleep == false){

    Serial.print("Loop Counter: ");
    Serial.println( ++loop_counter );

    // Wait for the 3rd time through the loop so that we receive the DISABLE_SLEEP_STATE from MQTT
    if(loop_counter > 2){
    
      Serial.println("INFO: Closing the MQTT connection");
      client.disconnect();
    
      Serial.println("Good night!");
      ESP.deepSleep(CONFIG_SLEEPING_TIME_IN_SECONDS * 1000000, WAKE_RF_DEFAULT);
      delay(500); // wait for deep sleep to happen
    }
  }

}
