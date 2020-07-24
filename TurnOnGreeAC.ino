#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Gree.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
// Wifi: SSID and password
const char* WIFI_SSID = "";
const char* WIFI_PASSWORD = "";

// MQTT: ID, server IP, port
const PROGMEM char* MQTT_CLIENT_ID = "esp8266_ir";
const PROGMEM char* MQTT_SERVER_IP = "192.168.2.50";
const PROGMEM uint16_t MQTT_SERVER_PORT = 1883;

// MQTT: topics 
const char* MQTT_TEMP_COMMAND_TOPIC = "bedroomair/ac/temperature/set";
const char* MQTT_AIR_MODE_TOPIC = "bedroomair/ac/mode/set";
const char* MQTT_FAN_MODE_TOPIC = "bedroomair/ac/fan/set";
const char* MQTT_SWING_TOPIC = "bedroomair/ac/swing/set";
const char* MQTT_POWER_TOPIC = "home/bedroom/airbutton/set";
const char* MQTT_POWER_STATE_TOPIC = "home/bedroom/airbutton";
//current infomation
bool ison=false;
String current_air_mode="";
String current_fan_mode="";
String current_temp="";
String current_swing="";
// payloads by default (on/off)
const char* DEVICE_ON = "ON";
const char* DEVICE_OFF = "OFF";


//air mode
const char* MODE_HEAT = "heat";
const char* MODE_COOL = "cool";
const char* MODE_AUTO = "auto";
const char* MODE_FAN_ONLY = "fan_only";
//swing button
const char* SWING_ON = "on";
const char* SWING_OFF = "off";
//fan speed
const char* FAN_SPEED_MEDIUM = "medium";
const char* FAN_SPEED_LOW = "low";
const char* FAN_SPEED_HIGH = "high";
// ESP8266 GPIO pin to use. Recommended: 4 (D2).
const uint16_t kIrLed = 14; 
IRGreeAC ac(kIrLed);  // Set the GPIO to be used for sending messages.

//wifi mqtt init
WiFiClient wifiClient;
PubSubClient client(wifiClient);

void printState() {
  // Display the settings.
  Serial.println("GREE A/C remote is in the following state:");
  Serial.printf("  %s\n", ac.toString().c_str());
  // Display the encoded IR sequence.
  unsigned char* ir_code = ac.getRaw();
  Serial.print("IR Code: 0x");
  for (uint8_t i = 0; i < kGreeStateLength; i++)
    Serial.printf("%02X", ir_code[i]);
  Serial.println();
}

void setup() {
  ac.begin();
  Serial.begin(9600);
  delay(200);

  // Set up what we want to send. See ir_Gree.cpp for all the options.
  // Most things default to off.
  Serial.println("Default state of the remote.");
  printState();
  Serial.println("Setting desired state for A/C.");
  ac.on();
  ac.setFan(1);
  // kGreeAuto, kGreeDry, kGreeCool, kGreeFan, kGreeHeat
  ac.setMode(kGreeCool);
  ac.setTemp(20);  // 16-30C
  ac.setSwingVertical(true, kGreeSwingAuto);
  ac.setXFan(false);
  ac.setLight(true);
  ac.setSleep(false);
  ac.setTurbo(false);
    // init the WiFi connection
  Serial.println();
  Serial.println();
  Serial.print("INFO: Connecting to ");
  WiFi.mode(WIFI_STA);
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("INFO: WiFi connected");
  Serial.print("INFO: IP address: ");
  Serial.println(WiFi.localIP());

  // init the MQTT connection
  client.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
  client.setCallback(callback);


  
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  //delay(5000);
  
}


void push_air_power_state(bool state){
    if (state) {
        client.publish(MQTT_POWER_STATE_TOPIC, DEVICE_ON, true);
    } else {
         client.publish(MQTT_POWER_STATE_TOPIC, DEVICE_OFF, true);
    }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("INFO: Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_CLIENT_ID)) {
      Serial.println("INFO: connected");
      // ... and resubscribe
      client.subscribe(MQTT_TEMP_COMMAND_TOPIC);
      client.subscribe(MQTT_AIR_MODE_TOPIC);
      client.subscribe(MQTT_FAN_MODE_TOPIC);
      client.subscribe(MQTT_SWING_TOPIC);
      client.subscribe(MQTT_POWER_TOPIC);
    } else {
      Serial.print("ERROR: failed, rc=");
      Serial.print(client.state());
      Serial.println("DEBUG: try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void callback(char* p_topic, byte* p_payload, unsigned int p_length) {
  String payload;
    Serial.println("INFO:callback...");
    for (uint8_t i = 0; i < p_length; i++) {
    payload.concat((char)p_payload[i]);
    }
    // handle message topic
  if (String(MQTT_TEMP_COMMAND_TOPIC).equals(p_topic)) {
    ac.setTemp(atoi(payload.c_str()));
    Serial.print(atoi(payload.c_str()));
    if(!current_temp.equals(payload)){
    ac.send();}
    current_temp=payload;
  }
  if (String(MQTT_AIR_MODE_TOPIC).equals(p_topic)) {
    if (payload.equals(String(MODE_HEAT))) {
      // kGreeAuto, kGreeDry, kGreeCool, kGreeFan, kGreeHeat
      ac.setMode(kGreeHeat);
      
    } else if (payload.equals(String(MODE_COOL))) {
         ac.setMode(kGreeCool);
    }else if (payload.equals(String(MODE_AUTO))) {
        ac.setMode(kGreeAuto);
    }else if (payload.equals(String(MODE_FAN_ONLY))) {
        ac.setMode(kGreeFan);
    }
    if(!current_air_mode.equals(payload)){
    ac.send();}
    current_air_mode=payload;
    
  }
  if (String(MQTT_FAN_MODE_TOPIC).equals(p_topic)) {
    if (payload.equals(String(FAN_SPEED_MEDIUM))) {
      // kGreeAuto, kGreeDry, kGreeCool, kGreeFan, kGreeHeat
       ac.setFan(3);
        
    } else if (payload.equals(String(FAN_SPEED_LOW))) {
         ac.setFan(2);
    }else if (payload.equals(String(FAN_SPEED_HIGH))) {
        ac.setFan(5);
    }
    if(!current_fan_mode.equals(payload)){
    ac.send();}
    current_fan_mode=payload;
  }
  if (String(MQTT_SWING_TOPIC).equals(p_topic)) {
    if (payload.equals(String(SWING_ON))) {
      ac.setSwingVertical(true, kGreeSwingAuto);
        
    } else if (payload.equals(String(SWING_OFF))) {
         ac.setSwingVertical(false, kGreeSwingAuto);
    }
    if(!current_swing.equals(payload)){
    ac.send();}
    current_swing=payload;
  }
  if (String(MQTT_POWER_TOPIC).equals(p_topic)) {
    if (payload.equals(String(DEVICE_ON))) {
        if(!ison){
        ison=true;
        ac.on();
        ac.send();}
        push_air_power_state(true);
    } else if (payload.equals(String(DEVICE_OFF))) {
        if(ison){
         ison=false;
         ac.off();
         ac.send();
         push_air_power_state(false);}
   }
  }
  
}
