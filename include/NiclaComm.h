#ifndef NICLA_COMM_H
#define NICLA_COMM_H

#include <Arduino.h>
#include <ArduinoBLE.h>
#include <mbed.h>

class NiclaComm{
    public:
        NiclaComm();

        void begin();

        bool ackReceived();

        bool centralConnected();

        void sendHeader(uint16_t length, uint32_t totalSteps, int8_t battery);

        bool sendPackets(const uint8_t* dailyLog, uint16_t length);

        bool isSubscribed();

        void bluetoothOn();
        
        void bluetoothOff();
        
    private:
        BLEService _stepService; 
        BLECharacteristic _logCharacteristic;
        uint16_t _offset = 0;
};

#endif