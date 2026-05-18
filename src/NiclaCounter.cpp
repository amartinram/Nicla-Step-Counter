#include "NiclaCounter.h"


NiclaCounter::NiclaCounter(uint32_t minInterval):
    _minInterval(minInterval) {
    memset(_activeLog, 0, Config::DUMP_DAY);
    memset(_sendLog, 0, Config::DUMP_DAY * Config::MAX_DAYS);
}


void NiclaCounter::beginSensor(){
    nicla::begin(); 
    nicla::leds.setColor(off);
    BHY2.begin(NICLA_STANDALONE); 
    _stepCounter.begin();
    _ticker.attach(mbed::callback(this,&NiclaCounter::irqHandler),std::chrono::milliseconds(_minInterval));
}

void NiclaCounter::pushDay(){
    memcpy(&_sendLog[_sendTail * Config::DUMP_DAY], _activeLog, Config::DUMP_DAY);

    if (_daysStored == Config::MAX_DAYS) {
        _sendHead = (_sendHead + 1) % Config::MAX_DAYS;
    } else {
        _daysStored++;
    }
    
    _sendTail = (_sendTail + 1) % Config::MAX_DAYS;

    _currentMinuteIndex = 0;
    memset(_activeLog, 0, Config::DUMP_DAY); 
}

void NiclaCounter::recordSteps(){
    uint32_t currentTotalSteps = _stepCounter.value();
    uint32_t stepsDiff;

    if (currentTotalSteps >= _lastTotalSteps) {
      stepsDiff = currentTotalSteps - _lastTotalSteps;
    } else {
      stepsDiff = currentTotalSteps; 
    }
    
    uint8_t stepsThisMinute = (stepsDiff > 255) ? 255 : (uint8_t)stepsDiff; 
    _lastTotalSteps = currentTotalSteps;
    
    if (_currentMinuteIndex < Config::DUMP_DAY) {
      _activeLog[_currentMinuteIndex++] = stepsThisMinute;
    }
    
    if (_currentMinuteIndex >= Config::DUMP_DAY) {
        pushDay();
    }
}

void NiclaCounter::irqHandler(){
    _tickerOk = true;
    _wakeSignal.release();
}

void NiclaCounter::update(){ 
    if(_tickerOk){
        BHY2.update();
        _tickerOk = false;
        recordSteps();
    }
}

void NiclaCounter::cleanBuffer(){
    if (_daysStored > 0) {
        memset(&_sendLog[_sendHead * Config::DUMP_DAY], 0, Config::DUMP_DAY); 
        _sendHead = (_sendHead + 1) % Config::MAX_DAYS;
        _daysStored--;
    }
}

bool NiclaCounter::hasPendingData() const{
    return _daysStored > 0;
}

const uint8_t* NiclaCounter::getBuffer() const{
    return &_sendLog[_sendHead * Config::DUMP_DAY];
}

uint32_t NiclaCounter::getTotalSteps() const {
    uint32_t total = 0; 
    uint16_t startIdx = _sendHead * Config::DUMP_DAY;

    for (uint16_t i = 0; i < Config::DUMP_DAY; i++) { 
        total += _sendLog[startIdx + i]; 
    }
    return total;
}

void NiclaCounter::waitForInterrupt(uint32_t ms){
    _wakeSignal.try_acquire_for(std::chrono::milliseconds(ms));
}
