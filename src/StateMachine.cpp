#include "StateMachine.h"

StateMachine::StateMachine():
    _counter(Config::MINUTE_INTERVAL), 
    _currentTask(&StateMachine::idle),
    _headerSent(false),
    _lastPacketTime(0),
    _lastAttempt(0),
    _stateStartTime(0)
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
        }else if(millis() - _lastPacketTime >= 100){
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
    static unsigned long delayStart = 0;
    static bool isDelaying = false;
    bool exit = false;

    if (isDelaying) {
        if (millis() - delayStart >= 200) {
            isDelaying = false;
            _headerSent = false;
            _stateStartTime = millis();
            transitionTo(&StateMachine::sendingSteps);
        }
    } 
    else {
        if (!_comm.centralConnected() || (millis() - _stateStartTime >= Config::BLE_ACK_TIMEOUT)) {
            _lastAttempt = millis(); 
            exit = true;
        } else if (_comm.ackReceived()) {
            _counter.cleanBuffer();
            if (_counter.hasPendingData()) {
                isDelaying = true;
                delayStart = millis();
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

int StateMachine::getSleepTime(){
    int sleep = Config::MINUTE_INTERVAL - 50;
    if(_currentTask != &StateMachine::idle){
        sleep = 10;
    }
    return sleep;
}

void StateMachine::sleep(int ms){
    _counter.waitForInterrupt(ms);
}