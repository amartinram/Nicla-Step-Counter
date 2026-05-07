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

        void sendHeader(int length, uint32_t totalSteps, int8_t battery);

        bool sendPackets(const uint8_t* dailyLog, int length);

        int getOffset();

        bool isSubscribed();

        void bluetoothOn();
        
        void bluetoothOff();
        
    private:
        BLEService _stepService; 
        BLECharacteristic _logCharacteristic;
        int _offset = 0;
};

#endif