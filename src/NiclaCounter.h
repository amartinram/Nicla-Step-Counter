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

        enum OpMode{
            STEPCOUNTING,
            STEPSENDING
        };
    
        OpMode getMode();

        const uint8_t* getBuffer();
        const int getDumpDay();
        uint32_t getTotalSteps();
        int getMinute();

    private:
        uint8_t _dailyLog[MAX_BUFFER];
        int _currentMinuteIndex = 0;
        int _dumpDay;
        unsigned long _minInterval;
        uint32_t _totalSteps = 0;
        OpMode _currentMode = STEPCOUNTING;
        bool _hasPendingData = false;

        Sensor _stepCounter{SENSOR_ID_STC};
        mbed::Ticker _ticker;
        volatile bool _tickerOk = false;

        void recordSteps();
        void irqHandler();
        
};

template <size_t MAX_BUFFER>
NiclaCounter<MAX_BUFFER>::NiclaCounter(int dumpDay, unsigned long minInterval):
     _dumpDay(dumpDay), _minInterval(minInterval) 
{}


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
    BHY2.update(); 
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
      _dailyLog[_currentMinuteIndex++] = stepsThisMinute;
    }
    if (_currentMinuteIndex >= _dumpDay) {
      _hasPendingData = true;
      _currentMode = STEPSENDING;
    }
}

template <size_t MAX_BUFFER>
void NiclaCounter<MAX_BUFFER>::irqHandler(){
    _tickerOk = true;
}

template <size_t MAX_BUFFER>
void NiclaCounter<MAX_BUFFER>::update(){
    if(_tickerOk){
        _tickerOk = false;
        recordSteps();
    }
}

template <size_t MAX_BUFFER>
void NiclaCounter<MAX_BUFFER>::cleanBuffer(){
    int remaining = _currentMinuteIndex - _dumpDay;

    if(remaining > 0) {
        memmove(_dailyLog, _dailyLog + _dumpDay, remaining);
    }
    _currentMinuteIndex = remaining;
    
    if(_currentMinuteIndex < _dumpDay) {
        _hasPendingData = false;
        _currentMode = STEPCOUNTING; 
    }
}

template <size_t MAX_BUFFER>
typename NiclaCounter<MAX_BUFFER>::OpMode NiclaCounter<MAX_BUFFER>::getMode(){
    return _currentMode;
}

template <size_t MAX_BUFFER>
const uint8_t* NiclaCounter<MAX_BUFFER>::getBuffer(){
    return _dailyLog;
}

template <size_t MAX_BUFFER>
const int NiclaCounter<MAX_BUFFER>::getDumpDay(){
    return _dumpDay;
}

template <size_t MAX_BUFFER>
uint32_t NiclaCounter<MAX_BUFFER>::getTotalSteps(){
    _totalSteps = 0;
    for (int i = 0; i < _dumpDay; i++) { 
        _totalSteps += _dailyLog[i]; 
    }
    return _totalSteps;
}

template <size_t MAX_BUFFER>
int NiclaCounter<MAX_BUFFER>::getMinute(){
    return _currentMinuteIndex;
}

#endif