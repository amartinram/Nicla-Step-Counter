#ifndef NICLA_COUNTER_H
#define NICLA_COUNTER_H

#include <Arduino.h>
#include "Arduino_BHY2.h"
#include "Nicla_System.h" 
#include <mbed.h>

template <size_t MAX_BUFFER>
class NiclaCounter{
    public:
        NiclaCounter(int dumpDay, unsigned long minInterval);

        void beginSensor();

        void update();

        void cleanBuffer();

        bool hasPendingData() const;

        const uint8_t* getBuffer() const;

        int getDumpDay() const;

        uint32_t getTotalSteps();

        int getMinute();

    private:
        uint8_t _dailyLog[2][MAX_BUFFER];
        uint8_t _activeBank = 0;
        uint8_t _sendBank = 1;
        int _currentMinuteIndex = 0;
        int _dumpDay;
        unsigned long _minInterval;
        uint32_t _lastTotalSteps = 0;

        bool _hasPendingData = false;

        Sensor _stepCounter{SENSOR_ID_STC};
        mbed::Ticker _ticker;
        volatile bool _tickerOk = false;

        void recordSteps();
        void irqHandler();
};

template <size_t MAX_BUFFER>
NiclaCounter<MAX_BUFFER>::NiclaCounter(int dumpDay, unsigned long minInterval):
     _dumpDay(dumpDay), _minInterval(minInterval) {}

template <size_t MAX_BUFFER>
void NiclaCounter<MAX_BUFFER>::beginSensor(){
    nicla::begin(); 
    nicla::leds.setColor(off);
    BHY2.begin(); 
    _stepCounter.begin();
    _ticker.attach(mbed::callback(this,&NiclaCounter::irqHandler),(float)_minInterval/1000);
}

template <size_t MAX_BUFFER>
void NiclaCounter<MAX_BUFFER>::recordSteps(){
    uint32_t currentTotalSteps = _stepCounter.value();
    uint32_t stepsDiff;

    if (currentTotalSteps >= _lastTotalSteps) {
      stepsDiff = currentTotalSteps - _lastTotalSteps;
    } else {
      stepsDiff = currentTotalSteps; 
    }
    
    uint8_t stepsThisMinute = (stepsDiff > 255) ? 255 : (uint8_t)stepsDiff; 
    _lastTotalSteps = currentTotalSteps;
    
    if (_currentMinuteIndex < MAX_BUFFER) {
      _dailyLog[_activeBank][_currentMinuteIndex++] = stepsThisMinute;
    }
    
    if (_currentMinuteIndex >= _dumpDay) {
        _sendBank = _activeBank;
        _activeBank = 1 - _activeBank;
        _currentMinuteIndex = 0;
        _hasPendingData = true;
    }
}

template <size_t MAX_BUFFER>
void NiclaCounter<MAX_BUFFER>::irqHandler(){
    _tickerOk = true;
}

template <size_t MAX_BUFFER>
void NiclaCounter<MAX_BUFFER>::update(){
    BHY2.update(); 
    
    if(_tickerOk){
        _tickerOk = false;
        recordSteps();
    }
}

template <size_t MAX_BUFFER>
void NiclaCounter<MAX_BUFFER>::cleanBuffer(){
    memset(_dailyLog[_sendBank], 0, MAX_BUFFER); 
    _hasPendingData = false;
}

template <size_t MAX_BUFFER>
bool NiclaCounter<MAX_BUFFER>::hasPendingData() const{
    return _hasPendingData;
}

template <size_t MAX_BUFFER>
const uint8_t* NiclaCounter<MAX_BUFFER>::getBuffer() const{
    return _dailyLog[_sendBank];
}

template <size_t MAX_BUFFER>
int NiclaCounter<MAX_BUFFER>::getDumpDay() const{
    return _dumpDay;
}

template <size_t MAX_BUFFER>
uint32_t NiclaCounter<MAX_BUFFER>::getTotalSteps() {
    uint32_t total = 0; 
    for (int i = 0; i < _dumpDay; i++) { 
        total += _dailyLog[_sendBank][i]; 
    }
    return total;
}

template <size_t MAX_BUFFER>
int NiclaCounter<MAX_BUFFER>::getMinute(){
    return _currentMinuteIndex;
}

#endif