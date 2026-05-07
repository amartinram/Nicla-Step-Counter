#ifndef NICLA_COMM_H
#define NICLA_COMM_H

#include <Arduino.h>
#include <ArduinoBLE.h>
#include <mbed.h>

class NiclaComm{
    public:
        NiclaComm();

        bool begin();

        bool ackReceived();

        bool centralConnected();

        void sendHeader(int length, uint32_t totalSteps, int8_t battery);

        bool sendPackets(const uint8_t* dailyLog, int length);

        int getOffset();

        bool isSubscribed();

        bool bluetoothOn();

        void bluetoothOff();
        
    private:
        BLEService _stepService; 
        BLECharacteristic _logCharacteristic;
        int _offset = 0;
        bool _isSetup = false;
};

#endif