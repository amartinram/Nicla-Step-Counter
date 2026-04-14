#include <Arduino.h>
#include "Arduino_BHY2.h"
#include <ArduinoBLE.h>
#include "Nicla_System.h" 

constexpr int DUMP_DAY = 1440; 
constexpr int MAX_DAYS_TO_STORE = 2; 
constexpr int MAX_BUFFER = DUMP_DAY * MAX_DAYS_TO_STORE; 
constexpr long MINUTE_INTERVAL = 60000; 

BLEService stepService("00001814-0000-1000-8000-00805f9b34fb"); 
BLECharacteristic logCharacteristic("00002a37-0000-1000-8000-00805f9b34fb", BLERead | BLENotify, 512);
Sensor stepCounter(SENSOR_ID_STC);

static unsigned long previousMillis = 0;
static uint32_t lastTotalSteps = 0;
static uint8_t dailyLog[MAX_BUFFER]; 
static int currentMinuteIndex = 0;
static bool hasPendingData = false;

enum StateMachine {IDLE, SEND_START, SEND_DATA, SEND_END, SEND_BATT};
StateMachine stateMachine = IDLE;
unsigned long lastBleTime = 0;
int currentSendIndex = 0;
int minutesToSend = 0;
uint32_t totalStepsTaken = 0;

void setup() {
  nicla::begin(); 
  BHY2.begin(); 
  stepCounter.begin();
  
  if (!BLE.begin()) {
    while (1); 
  }
  
  BLE.setLocalName("Nicla_Steps"); 
  BLE.setAdvertisedService(stepService);
  stepService.addCharacteristic(logCharacteristic);
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
  
  if (hasPendingData && stateMachine == IDLE && central && central.connected() && logCharacteristic.subscribed()) {
    stateMachine = SEND_START;
    lastBleTime = now;
    currentSendIndex = 0;
    totalStepsTaken = 0;
    minutesToSend = currentMinuteIndex; 
  }

  if (stateMachine != IDLE) {
    
    if (!central || !central.connected() || !logCharacteristic.subscribed()) {
      stateMachine = IDLE; 
    }
    else if (now - lastBleTime >= 40) {
      lastBleTime = now;

      switch (stateMachine) {
        
        case SEND_START:
          logCharacteristic.writeValue((const uint8_t*)"START", 5);
          stateMachine = SEND_DATA;
          break;

        case SEND_DATA: {
          int bytesRemaining = minutesToSend - currentSendIndex;
          if (bytesRemaining > 0) {
            int chunkSize = (bytesRemaining > 80) ? 80 : bytesRemaining;
            uint8_t buffer[80];
            
            for (int i = 0; i < chunkSize; i++) { 
              buffer[i] = dailyLog[currentSendIndex + i];
              totalStepsTaken += buffer[i];
            }
            
            logCharacteristic.writeValue(buffer, chunkSize);
            currentSendIndex += chunkSize;
          }else {
            stateMachine = SEND_END;
          }
          break;
        }

        case SEND_END: {
          char endMsg[32]; 
          snprintf(endMsg, sizeof(endMsg), "END:%lu", totalStepsTaken);
          logCharacteristic.writeValue((const uint8_t*)endMsg, strlen(endMsg));
          stateMachine = SEND_BATT; //IDLE

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
          logCharacteristic.writeValue((const uint8_t*)battMsg, strlen(battMsg));

          memmove(dailyLog, dailyLog + minutesToSend, currentMinuteIndex - minutesToSend);
          currentMinuteIndex -= minutesToSend;
          
          hasPendingData = false;
          stateMachine = IDLE; 
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