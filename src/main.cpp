#include <Arduino.h>
#include "Arduino_BHY2.h"
#include <ArduinoBLE.h>
#include "Nicla_System.h" 

constexpr char myUserID[] = "User_01"; 
constexpr int DUMP_DAY = 1440;        
constexpr long MINUTE_INTERVAL = 60000; 

BLEService stepService("00001814-0000-1000-8000-00805f9b34fb"); 
BLEStringCharacteristic logChar("00002a37-0000-1000-8000-00805f9b34fb", BLERead | BLENotify, 512); 
Sensor stepCounter(SENSOR_ID_STC);


void setup() {
  nicla::begin(); 
  BHY2.begin(); 
  stepCounter.begin();
  
  if (!BLE.begin()) {
    while (1); 
  }
  
  BLE.setLocalName("Nicla_Steps"); 
  BLE.setAdvertisedService(stepService);
  stepService.addCharacteristic(logChar);
  BLE.addService(stepService);
  
  BLE.advertise(); 
}


void loop() {
  static unsigned long previousMillis = 0;
  static uint32_t lastTotalSteps = 0;
  static uint16_t dailyLog[DUMP_DAY]; 
  static int currentMinuteIndex = 0;
  static bool hasPendingData = false;


  BHY2.update(); 

  if (millis() - previousMillis >= MINUTE_INTERVAL) {
    previousMillis = millis();
    
    uint32_t currentTotalSteps = stepCounter.value();
    uint16_t stepsThisMinute = (currentTotalSteps >= lastTotalSteps) ? (currentTotalSteps - lastTotalSteps) : 0;
    lastTotalSteps = currentTotalSteps;
    
    if (currentMinuteIndex < DUMP_DAY) {
      dailyLog[currentMinuteIndex] = stepsThisMinute;
      currentMinuteIndex++;
    } else {
      hasPendingData = true;
    }
  }

  if (hasPendingData) {
    BLEDevice central = BLE.central();
    
    if (central && central.connected() && logChar.subscribed()) {
      delay(500); 
      
      logChar.writeValue("START");
      delay(100); 
      
      String packet = "";
      uint32_t totalForDay = 0;

      for (int i = 0; i < currentMinuteIndex; i++) { 
        packet += String(dailyLog[i]) + ","; 
        totalForDay += dailyLog[i];

        if ((i + 1) % 40 == 0 || i == currentMinuteIndex - 1) {
          logChar.writeValue(packet);
          delay(100); 
          packet = "";
        }
      }
      
      delay(200); 
      logChar.writeValue("END:" + String(totalForDay));
      
      currentMinuteIndex = 0;
      hasPendingData = false;
    }
  }
}