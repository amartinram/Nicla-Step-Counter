#include <Arduino.h>
#include "Arduino_BHY2.h"
#include <ArduinoBLE.h>
#include "Nicla_System.h" 

// --- CONFIGURATION ---
constexpr int DUMP_DAY = 30; 
constexpr int MAX_DAYS_TO_STORE = 3; 
constexpr int MAX_BUFFER = DUMP_DAY * MAX_DAYS_TO_STORE; 
constexpr long MINUTE_INTERVAL = 1000; 

// --- BLE & SENSOR OBJECTS ---
BLEService stepService("00001814-0000-1000-8000-00805f9b34fb"); 
BLECharacteristic logChar("00002a37-0000-1000-8000-00805f9b34fb", BLERead | BLENotify, 512);
Sensor stepCounter(SENSOR_ID_STC);

// --- GLOBAL VARIABLES FOR SENSOR & MEMORY ---
static unsigned long previousMillis = 0;
static uint32_t lastTotalSteps = 0;
static uint8_t dailyLog[MAX_BUFFER]; 
static int currentMinuteIndex = 0;
static bool hasPendingData = false;

// --- GLOBAL VARIABLES FOR BLE STATE MACHINE ---
enum SyncState { IDLE, SEND_START, SEND_DATA, SEND_END, SEND_BATT };
SyncState syncState = IDLE;
unsigned long lastBleTime = 0;
int currentSendIndex = 0;
int minutesToSend = 0;
uint32_t totalForBatch = 0;

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
  unsigned long now = millis();

  BHY2.update(); 

  if (now - previousMillis >= MINUTE_INTERVAL) {
    previousMillis += MINUTE_INTERVAL; 
    
    uint32_t currentTotalSteps = stepCounter.value();
    uint32_t stepsDiff = (currentTotalSteps >= lastTotalSteps) ? (currentTotalSteps - lastTotalSteps) : 0;
    
    uint8_t stepsThisMinute = (stepsDiff > 255) ? 255 : (uint8_t)stepsDiff; 
    lastTotalSteps = currentTotalSteps;
    
    if (currentMinuteIndex < MAX_BUFFER) {
      dailyLog[currentMinuteIndex++] = stepsThisMinute;
    } else {
      memmove(dailyLog, dailyLog + 1, MAX_BUFFER - 1);
      dailyLog[MAX_BUFFER - 1] = stepsThisMinute;
    }

    if (currentMinuteIndex >= DUMP_DAY) {
      hasPendingData = true;
    }
  }

  BLEDevice central = BLE.central();
  
  if (hasPendingData && syncState == IDLE && central && central.connected() && logChar.subscribed()) {
    syncState = SEND_START;
    lastBleTime = now;
    currentSendIndex = 0;
    totalForBatch = 0;
    minutesToSend = currentMinuteIndex; 
  }

  if (syncState != IDLE) {
    
    if (!central || !central.connected() || !logChar.subscribed()) {
      syncState = IDLE; 
    }
    else if (now - lastBleTime >= 40) {
      lastBleTime = now;

      switch (syncState) {
        
        case SEND_START:
          logChar.writeValue((const uint8_t*)"START", 5);
          syncState = SEND_DATA;
          break;

        case SEND_DATA: {
          int bytesRemaining = minutesToSend - currentSendIndex;
          if (bytesRemaining > 0) {
            int chunkSize = (bytesRemaining > 80) ? 80 : bytesRemaining;
            uint8_t buffer[80];
            
            for (int i = 0; i < chunkSize; i++) { 
              buffer[i] = dailyLog[currentSendIndex + i];
              totalForBatch += buffer[i];
            }
            
            logChar.writeValue(buffer, chunkSize);
            currentSendIndex += chunkSize;
          }else {
            syncState = SEND_END;
          }
          break;
        }

        case SEND_END: {
          char endMsg[32]; 
          snprintf(endMsg, sizeof(endMsg), "END:%lu", totalForBatch);
          logChar.writeValue((const uint8_t*)endMsg, strlen(endMsg));
          syncState = SEND_BATT;

          /*
          memmove(dailyLog, dailyLog + minutesToSend, currentMinuteIndex - minutesToSend);
          currentMinuteIndex -= minutesToSend;
          
          hasPendingData = false;
          syncState = IDLE; 
          */
          break;
        }

        case SEND_BATT: {
          char battMsg[16];
          snprintf(battMsg, sizeof(battMsg), "BATT:%d", nicla::getBatteryVoltagePercentage());
          logChar.writeValue((const uint8_t*)battMsg, strlen(battMsg));

          memmove(dailyLog, dailyLog + minutesToSend, currentMinuteIndex - minutesToSend);
          currentMinuteIndex -= minutesToSend;
          
          hasPendingData = false;
          syncState = IDLE; 
          break;
        }
        
        case IDLE:
          break;
        
        default:
          break;
      }
    }
  }
}