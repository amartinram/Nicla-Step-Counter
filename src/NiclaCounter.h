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

        void timerInterrupt();

        void update();

        void cleanBuffer(int minuteSend);

        enum OpMode{
            STEPCOUNTING,
            STEPSENDING
        };
    
        OpMode getMode();

        uint8_t* getBuffer();

    private:
        uint8_t _dailyLog[MAX_BUFFER];
        int _currentMinuteIndex;
        int _dumpDay;
        unsigned long _minInterval;
        uint32_t _lastTotalSteps;
        OpMode _currentMode;
        boolean _hasPendingData;

        Sensor _stepCounter{SENSOR_ID_STC};
        mbed::Ticker _ticker;
        volatile bool _tickerOk = false;

        void recordSteps();
        void irqHandler();
};

template <size_t MAX_BUFFER>
NiclaCounter<MAX_BUFFER>::NiclaCounter(int dumpDay, unsigned long minInterval){
    _dumpDay = dumpDay;
    _minInterval = minInterval;
    _currentMinuteIndex = 0;
    _lastTotalSteps = 0;
    _currentMode = STEPCOUNTING;
    _hasPendingData = false;
}

template <size_t MAX_BUFFER>
void NiclaCounter<MAX_BUFFER>::beginSensor(){
    nicla::begin(); 
    nicla::leds.setColor(off);
    BHY2.begin(); 
    _stepCounter.begin();
    _ticker.attach(mbed::callback(this,NiclaCounter::irqHandler,60.0));
}



template <size_t MAX_BUFFER>
void NiclaCounter<MAX_BUFFER>::recordSteps(){
    BHY2.update(); 
    uint32_t currentTotalSteps = _stepCounter.value();
    uint32_t stepsDiff;

    if (currentTotalSteps >= lastTotalSteps) {
      stepsDiff = currentTotalSteps - lastTotalSteps;
    } else {
      stepsDiff = currentTotalSteps; 
    }
    
    uint8_t stepsThisMinute = (stepsDiff > 255) ? 255 : (uint8_t)stepsDiff; 
    lastTotalSteps = currentTotalSteps;
    
    if (_currentMinuteIndex < MAX_BUFFER) {
      _dailyLog[_currentMinuteIndex++] = stepsThisMinute;
    }
    if (_currentMinuteIndex >= DUMP_DAY) {
      _hasPendingData = true;
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
void NiclaCounter<MAX_BUFFER>::timerInterrupt(){
     
}

template <size_t MAX_BUFFER>
void NiclaCounter<MAX_BUFFER>::cleanBuffer(int minuteSend){
    memmove(_dailyLog, _dailyLog + _dumpDay, _currentMinuteIndex - _dumpDay);
    _currentMinuteIndex -= _dumpDay;
      
    if (_currentMinuteIndex < DUMP_DAY) {
       _hasPendingData = false;
    }
}

template <size_t MAX_BUFFER>
typename NiclaCounter<MAX_BUFFER>::OpMode NiclaCounter<MAX_BUFFER>::getMode(){
    return _currentMode;
}

template <size_t MAX_BUFFER>
uint8_t* NiclaCounter<MAX_BUFFER>::getBuffer(){
    return _dailyLog;
}

#endif