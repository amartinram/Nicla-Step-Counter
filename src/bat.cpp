#include "Nicla_System.h"
#include <Arduino.h>
#include "Arduino_BHY2.h"
#include <ArduinoBLE.h>
#include "Nicla_System.h" 

BLEService stepService("00001814-0000-1000-8000-00805f9b34fb"); 
BLECharacteristic logCharacteristic("00002a37-0000-1000-8000-00805f9b34fb", BLERead | BLENotify | BLEWrite, 512);
Sensor stepCounter(SENSOR_ID_STC);

void setup() {
  Serial.begin(115200);
  nicla::begin();
  nicla::enableCharging(100); 
  if (!BLE.begin()) {
    while (1);
  }
  
  BLE.setLocalName("Nicla_Steps"); 
  BLE.setAdvertisedService(stepService);
  stepService.addCharacteristic(logCharacteristic);
  BLE.addService(stepService);
  
  BLE.setAdvertisingInterval(1600);
  BLE.advertise();
}

void loop() {
  BLE.poll();
  auto status = nicla::getOperatingStatus();
  int pct = nicla::getBatteryVoltagePercentage();

  Serial.print("Battery Percentage: ");
  Serial.println(pct);

  if (status == OperatingStatus::Charging) {
    Serial.println("Status: Charging normally");
  } else if (status == OperatingStatus::ChargingComplete) {
    Serial.println("Status: Fully Charged");
  } else if (status == OperatingStatus::Ready) {
    Serial.println("Status: Running on battery (not charging)");
  } else if (status == OperatingStatus::Error) {
    Serial.println("Status: FAULT! Check wiring or temperature.");
  }

  delay(2000);
}