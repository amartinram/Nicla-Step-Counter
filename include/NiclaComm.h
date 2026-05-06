#ifndef NICLA_COMM_H
#define NICLA_COMM_H

#include <Arduino.h>
#include <ArduinoBLE.h>

class NiclaComm{
    public:
        NiclaComm();

        void begin();

        bool ackReceived();

        bool centralConnected();

        void sendHeader(int length, uint32_t totalSteps, int8_t battery);

        bool sendPackets(const uint8_t* dailyLog, int length);

        int getOffset();

        void advertise();

        void stopAdvertise();

        bool isSuscribed();
        
    private:
        BLEService _stepService; 
        BLECharacteristic _logCharacteristic;
        int _offset = 0;

};

#endif