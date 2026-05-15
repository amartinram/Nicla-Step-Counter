#include "StateMachine.h"

StateMachine::StateMachine():
    _counter(Config::MINUTE_INTERVAL), 
    _currentTask(&StateMachine::idle),
    _headerSent(false),
    _lastPacketTime(0),
    _lastAttempt(0),
    _stateStartTime(0),
    _delayStart(0),
    _waitingBetweenDays(false)
{}

void StateMachine::initialize(){
    _counter.beginSensor();
    _comm.begin();
}

void StateMachine::run(){
    if(_currentTask != &StateMachine::idle){
        BLE.poll(); 
    }
    _counter.update();
    (this->*_currentTask)();
}

void StateMachine::idle(){
    if(_counter.hasPendingData() && (_lastAttempt == 0 || millis() - _lastAttempt >= Config::BLE_RETRY_INTERVAL)){
        _comm.bluetoothOn();
        _stateStartTime = millis(); 
        transitionTo(&StateMachine::advertising);
        
    }
}

void StateMachine::advertising(){
    if(_comm.centralConnected() && _comm.isSubscribed()){
        _headerSent = false;
        _lastAttempt = 0;
        transitionTo(&StateMachine::sendingSteps);
    }else if(millis() - _stateStartTime >= Config::BLE_ADVERTISE_TIMEOUT){
        _lastAttempt = millis(); 
        _comm.bluetoothOff();
        transitionTo(&StateMachine::idle);
    }
}

void StateMachine::sendingSteps(){
    if(_comm.centralConnected()){
        if(!_headerSent){
            _comm.sendHeader(Config::DUMP_DAY,_counter.getTotalSteps(),nicla::getBatteryVoltagePercentage());
            _headerSent = true;
            _lastPacketTime = millis();
        }else if(millis() - _lastPacketTime >= 20){
            bool finished = _comm.sendPackets(_counter.getBuffer(),Config::DUMP_DAY);
            if(finished){
                _stateStartTime = millis();
                transitionTo(&StateMachine::waitAck);
            }
            _lastPacketTime = millis();
        }
    }else{
        _lastAttempt = millis();
        _comm.bluetoothOff();
        transitionTo(&StateMachine::idle);
    }
}

void StateMachine::waitAck(){
    bool exit = false;

    if (_waitingBetweenDays) {
        if (millis() - _delayStart >= 200) {
            _waitingBetweenDays = false;
            _headerSent = false;
            _stateStartTime = millis();
            transitionTo(&StateMachine::sendingSteps);
        }
    }else {
        if (!_comm.centralConnected() || (millis() - _stateStartTime >= Config::BLE_ACK_TIMEOUT)) {
            _lastAttempt = millis(); 
            exit = true;
            _waitingBetweenDays = false; 
        } else if (_comm.ackReceived()) {
            _counter.cleanBuffer();
            if (_counter.hasPendingData()) {
                _waitingBetweenDays = true;
                _delayStart = millis();
            } else {
                exit = true;
            }
        }
        if (exit) {
            _comm.bluetoothOff();
            transitionTo(&StateMachine::idle);
        }
    }
}

void StateMachine::transitionTo(void(StateMachine::*nextTask)()){
    _currentTask = nextTask;
}

uint32_t StateMachine::getSleepTime(){
    uint16_t sleep = Config::MINUTE_INTERVAL;
    if(_currentTask == &StateMachine::sendingSteps || _currentTask == &StateMachine::waitAck){
        sleep = 10;
    } 
    else if(_currentTask == &StateMachine::advertising){
        sleep = 30;
    }
    return sleep;
}

void StateMachine::sleep(uint32_t ms){
    _counter.waitForInterrupt(ms);
}