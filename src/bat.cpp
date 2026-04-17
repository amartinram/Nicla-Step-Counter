#include "Nicla_System.h"

void setup() {
  Serial.begin(115200);
  nicla::begin();
  nicla::enableCharging(100); 
}

void loop() {
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