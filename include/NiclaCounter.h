#ifndef NICLA_COUNTER_H
#define NICLA_COUNTER_H

#include <Arduino.h>
#include "Arduino_BHY2.h"
#include "Nicla_System.h" 
#include <mbed.h>
#include <rtos.h>
#include "Config.h"

class NiclaCounter{
    public:
        NiclaCounter(uint32_t minInterval);

        void beginSensor();

        void update();

        void cleanBuffer();

        bool hasPendingData() const;

        const uint8_t* getBuffer() const;

        uint32_t getTotalSteps() const;

        void waitForInterrupt(uint32_t ms);

    private:
        uint8_t _activeLog[Config::DUMP_DAY];
        uint8_t _sendLog[Config::DUMP_DAY * Config::MAX_DAYS];
        uint8_t _sendHead = 0;    
        uint8_t _sendTail = 0;  
        uint8_t _daysStored = 0; 

        uint16_t _currentMinuteIndex = 0;

        uint32_t _minInterval;
        uint32_t _lastTotalSteps = 0;

        Sensor _stepCounter{SENSOR_ID_STC};
        mbed::Ticker _ticker;
        volatile bool _tickerOk = false;

        rtos::Semaphore _wakeSignal{0, 1};

        void recordSteps();

        void irqHandler();
        
        void pushDay(); 
};

#endif