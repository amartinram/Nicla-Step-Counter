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

enum StateMachine {IDLE, SEND_HEADER, SEND_DATA};
StateMachine stateMachine = IDLE;
unsigned long lastBleTime = 0;
int currentSendIndex = 0;
int minutesToSend = 0;
uint32_t totalStepsTaken = 0;

constexpr unsigned long STATE_MACHINE_TIMER = 10000;
BLEDevice central;

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
  BLE.poll();

  if (now - previousMillis >= MINUTE_INTERVAL) {
    previousMillis += MINUTE_INTERVAL; 
    
    uint32_t currentTotalSteps = stepCounter.value();
    uint32_t stepsDiff = (currentTotalSteps >= lastTotalSteps) ? (currentTotalSteps - lastTotalSteps) : 0;
    
    uint8_t stepsThisMinute = (stepsDiff > 255) ? 255 : (uint8_t)stepsDiff; 
    lastTotalSteps = currentTotalSteps;
    
    if (currentMinuteIndex < MAX_BUFFER) {
      dailyLog[currentMinuteIndex++] = stepsThisMinute;
    } /*else {
      memmove(dailyLog, dailyLog + 1, MAX_BUFFER - 1);
      dailyLog[MAX_BUFFER - 1] = stepsThisMinute;
    }*/

    if (currentMinuteIndex >= DUMP_DAY) {
      hasPendingData = true;
    }
  }

  if (!central || !central.connected()) {
      central = BLE.central();
  }
  
  if (hasPendingData && stateMachine == IDLE && central && central.connected() && logCharacteristic.subscribed()) {
    stateMachine = SEND_HEADER;
    lastBleTime = now;
    currentSendIndex = 0;
    minutesToSend = currentMinuteIndex; 
  }

  if (stateMachine != IDLE) {
    
    if (now - lastBleTime > STATE_MACHINE_TIMER) {
      stateMachine = IDLE;
    }
    else if (!central || !central.connected() || !logCharacteristic.subscribed()) {
      stateMachine = IDLE; 
    }
    else if (now - lastBleTime >= 40) {
      lastBleTime = now;

      switch (stateMachine) {
        
        case SEND_HEADER: {
          totalStepsTaken = 0;
          for (int i = 0; i < minutesToSend; i++) { 
            totalStepsTaken += dailyLog[i]; 
          }

          uint8_t header[9];
          //First two bytes preamble to mark the start of a transaction
          header[0] = 0xAA; 
          header[1] = 0xBB; 
          header[2] = (minutesToSend >> 8) & 0xFF;
          header[3] = minutesToSend & 0xFF;
          header[4] = (totalStepsTaken >> 24) & 0xFF;
          header[5] = (totalStepsTaken >> 16) & 0xFF;
          header[6] = (totalStepsTaken >> 8) & 0xFF;
          header[7] = totalStepsTaken & 0xFF;
          header[8] = nicla::getBatteryVoltagePercentage();

          logCharacteristic.writeValue(header, 9);
          stateMachine = SEND_DATA;
          break;
        }

        case SEND_DATA: {
          int bytesRemaining = minutesToSend - currentSendIndex;
          if (bytesRemaining > 0) {
            int chunkSize = (bytesRemaining > 244) ? 244 : bytesRemaining;
            uint8_t buffer[244];
            
            memcpy(buffer, dailyLog + currentSendIndex, chunkSize);
            
            logCharacteristic.writeValue(buffer, chunkSize);
            currentSendIndex += chunkSize;
          } else {
            memmove(dailyLog, dailyLog + minutesToSend, currentMinuteIndex - minutesToSend);
            currentMinuteIndex -= minutesToSend;
            
            hasPendingData = false;
            stateMachine = IDLE; 
          }
          break;
        }
        
        case IDLE:
        default:
          break;
      }
    }
  }
}