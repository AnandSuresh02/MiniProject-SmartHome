#ifdef ENABLE_DEBUG
       #define DEBUG_ESP_PORT Serial
       #define NODEBUG_WEBSOCKETS
       #define NDEBUG
#endif 

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "SinricPro.h"
#include "SinricProSwitch.h"
#include <WiFiManager.h>

#include <map>

WiFiManager wifiManager;

#define APP_KEY           ""
#define APP_SECRET        ""   

#define device_ID_1   ""
#define device_ID_2   "" 

#define RelayPin1 5  
#define RelayPin2 4


#define SwitchPin1 10  
#define SwitchPin2 0

#define wifiLed   16   

#define BAUD_RATE   9600

#define DEBOUNCE_TIME 250

typedef struct {
  int relayPIN;
  int flipSwitchPIN;
} deviceConfig_t;

std::map<String, deviceConfig_t> devices = {
    //{deviceId, {relayPIN,  flipSwitchPIN}}
    {device_ID_1, {  RelayPin1, SwitchPin1 }},
    {device_ID_2, {  RelayPin2, SwitchPin2 }}
};

typedef struct {      
  String deviceId;
  bool lastFlipSwitchState;
  unsigned long lastFlipSwitchChange;
} flipSwitchConfig_t;

std::map<int, flipSwitchConfig_t> flipSwitches;    

void setupRelays() { 
  for (auto &device : devices) {           
    int relayPIN = device.second.relayPIN;
    pinMode(relayPIN, OUTPUT);             
    digitalWrite(relayPIN, LOW);
  }
}

void setupFlipSwitches() {
  for (auto &device : devices)  {                    
    flipSwitchConfig_t flipSwitchConfig;             
    flipSwitchConfig.deviceId = device.first;        
    flipSwitchConfig.lastFlipSwitchChange = 0;        
    flipSwitchConfig.lastFlipSwitchState = true;     

    int flipSwitchPIN = device.second.flipSwitchPIN;  

    flipSwitches[flipSwitchPIN] = flipSwitchConfig;   
    pinMode(flipSwitchPIN, INPUT_PULLUP);                  
  }
}

bool onPowerState(String deviceId, bool &state)
{
  Serial.printf("%s: %s\r\n", deviceId.c_str(), state ? "on" : "off");
  int relayPIN = devices[deviceId].relayPIN; 
  digitalWrite(relayPIN, state);             
  return true;
}

void handleFlipSwitches() {
  unsigned long actualMillis = millis();                            
  for (auto &flipSwitch : flipSwitches) {                                       
    unsigned long lastFlipSwitchChange = flipSwitch.second.lastFlipSwitchChange;  

    if (actualMillis - lastFlipSwitchChange > DEBOUNCE_TIME) {               

      int flipSwitchPIN = flipSwitch.first;                                     
      bool lastFlipSwitchState = flipSwitch.second.lastFlipSwitchState;       
      bool flipSwitchState = digitalRead(flipSwitchPIN);                        
      if (flipSwitchState != lastFlipSwitchState) {                             
#ifdef TACTILE_BUTTON
        if (flipSwitchState) {                                                 
#endif      
          flipSwitch.second.lastFlipSwitchChange = actualMillis;            
          String deviceId = flipSwitch.second.deviceId;                    
          int relayPIN = devices[deviceId].relayPIN;                         
          bool newRelayState = !digitalRead(relayPIN);
          digitalWrite(relayPIN, newRelayState); 

          SinricProSwitch &mySwitch = SinricPro[deviceId];
          mySwitch.sendPowerStateEvent(!newRelayState);
#ifdef TACTILE_BUTTON
        }
#endif      
        flipSwitch.second.lastFlipSwitchState = flipSwitchState;
      }
    }
  }
}

void setupSinricPro()
{
  for (auto &device : devices)
  {
    const char *deviceId = device.first.c_str();
    SinricProSwitch &mySwitch = SinricPro[deviceId];
    mySwitch.onPowerState(onPowerState);
  }

  SinricPro.begin(APP_KEY, APP_SECRET);
  SinricPro.restoreDeviceStates(true);
}

void setup()
{
  Serial.begin(BAUD_RATE);

  wifiManager.autoConnect();

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.printf(".");
    delay(250);
  }
  digitalWrite(wifiLed, LOW);
  Serial.printf("connected!\r\n[WiFi]: IP-Address is %s\r\n", WiFi.localIP().toString().c_str());
  pinMode(wifiLed, OUTPUT);
  digitalWrite(wifiLed, HIGH);

  setupRelays();
  setupFlipSwitches();
  setupSinricPro();
}

void loop()
{
  SinricPro.handle();
  handleFlipSwitches();
}
