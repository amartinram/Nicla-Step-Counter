#ifndef NICLA_COUNTER_H
#define NICLA_COUNTER_H

#include <Arduino.h>
#include "Arduino_BHY2.h"
#include "Nicla_System.h" 
#include <mbed.h>

template <size_t DUMP_DAY, size_t MAX_DAYS>
class NiclaCounter{
    public:
        NiclaCounter(unsigned long minInterval);

        void beginSensor();
        void update();
        void cleanBuffer();

        bool hasPendingData() const;
        const uint8_t* getBuffer() const;
        int getDumpDay() const;
        uint32_t getTotalSteps() const;
        int getMinute() const;

    private:
        uint8_t _activeLog[DUMP_DAY];
        uint8_t _sendLog[DUMP_DAY * MAX_DAYS];

        int _currentMinuteIndex = 0;
        int _sendHead = 0;    
        int _sendTail = 0;  
        int _daysStored = 0; 

        unsigned long _minInterval;
        uint32_t _lastTotalSteps = 0;

        Sensor _stepCounter{SENSOR_ID_STC};
        mbed::Ticker _ticker;
        volatile bool _tickerOk = false;

        void recordSteps();
        void irqHandler();
        void pushDay(); 
};

template <size_t DUMP_DAY, size_t MAX_DAYS>
NiclaCounter<DUMP_DAY, MAX_DAYS>::NiclaCounter(unsigned long minInterval):
    _minInterval(minInterval) {
    memset(_activeLog, 0, DUMP_DAY);
    memset(_sendLog, 0, DUMP_DAY * MAX_DAYS);
}

template <size_t DUMP_DAY, size_t MAX_DAYS>
void NiclaCounter<DUMP_DAY, MAX_DAYS>::beginSensor(){
    nicla::begin(); 
    nicla::leds.setColor(off);
    BHY2.begin(); 
    _stepCounter.begin(0.0f, _minInterval);
    _ticker.attach(mbed::callback(this,&NiclaCounter::irqHandler),(float)_minInterval/1000);
}

template <size_t DUMP_DAY, size_t MAX_DAYS>
void NiclaCounter<DUMP_DAY, MAX_DAYS>::pushDay(){
    memcpy(&_sendLog[_sendTail * DUMP_DAY], _activeLog, DUMP_DAY);

    if (_daysStored == MAX_DAYS) {
        _sendHead = (_sendHead + 1) % MAX_DAYS;
    } else {
        _daysStored++;
    }
    
    _sendTail = (_sendTail + 1) % MAX_DAYS;

    _currentMinuteIndex = 0;
    memset(_activeLog, 0, DUMP_DAY); 
}

template <size_t DUMP_DAY, size_t MAX_DAYS>
void NiclaCounter<DUMP_DAY, MAX_DAYS>::recordSteps(){
    uint32_t currentTotalSteps = _stepCounter.value();
    uint32_t stepsDiff;

    if (currentTotalSteps >= _lastTotalSteps) {
      stepsDiff = currentTotalSteps - _lastTotalSteps;
    } else {
      stepsDiff = currentTotalSteps; 
    }
    
    uint8_t stepsThisMinute = (stepsDiff > 255) ? 255 : (uint8_t)stepsDiff; 
    _lastTotalSteps = currentTotalSteps;
    
    if (_currentMinuteIndex < DUMP_DAY) {
      _activeLog[_currentMinuteIndex++] = stepsThisMinute;
    }
    
    if (_currentMinuteIndex >= DUMP_DAY) {
        pushDay();
    }
}

template <size_t DUMP_DAY, size_t MAX_DAYS>
void NiclaCounter<DUMP_DAY, MAX_DAYS>::irqHandler(){
    _tickerOk = true;
}

template <size_t DUMP_DAY, size_t MAX_DAYS>
void NiclaCounter<DUMP_DAY, MAX_DAYS>::update(){
    BHY2.update(); 
    if(_tickerOk){
        _tickerOk = false;
        recordSteps();
    }
}

template <size_t DUMP_DAY, size_t MAX_DAYS>
void NiclaCounter<DUMP_DAY, MAX_DAYS>::cleanBuffer(){
    if (_daysStored > 0) {
        memset(&_sendLog[_sendHead * DUMP_DAY], 0, DUMP_DAY); 
        _sendHead = (_sendHead + 1) % MAX_DAYS;
        _daysStored--;
    }
}

template <size_t DUMP_DAY, size_t MAX_DAYS>
bool NiclaCounter<DUMP_DAY, MAX_DAYS>::hasPendingData() const{
    return _daysStored > 0;
}

template <size_t DUMP_DAY, size_t MAX_DAYS>
const uint8_t* NiclaCounter<DUMP_DAY, MAX_DAYS>::getBuffer() const{
    return &_sendLog[_sendHead * DUMP_DAY];
}

template <size_t DUMP_DAY, size_t MAX_DAYS>
int NiclaCounter<DUMP_DAY, MAX_DAYS>::getDumpDay() const{
    return DUMP_DAY;
}

template <size_t DUMP_DAY, size_t MAX_DAYS>
uint32_t NiclaCounter<DUMP_DAY, MAX_DAYS>::getTotalSteps() const {
    uint32_t total = 0; 
    int startIdx = _sendHead * DUMP_DAY;

    for (int i = 0; i < DUMP_DAY; i++) { 
        total += _sendLog[startIdx + i]; 
    }
    return total;
}

template <size_t DUMP_DAY, size_t MAX_DAYS>
int NiclaCounter<DUMP_DAY, MAX_DAYS>::getMinute() const{
    return _currentMinuteIndex;
}

#endif