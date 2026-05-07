#include "StateMachine.h"

StateMachine::StateMachine():
    _counter(Config::DUMP_DAY,Config::MINUTE_INTERVAL), 
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
    BLE.poll();
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
    bool exit = false;

    if (!_comm.centralConnected() || (millis() - _stateStartTime >= Config::BLE_ACK_TIMEOUT)) {
        _lastAttempt = millis(); 
        exit = true;
    }else if (_comm.ackReceived()) {
        _counter.cleanBuffer();
        exit = true;
    }

    if (exit) {
        _comm.bluetoothOff();
        transitionTo(&StateMachine::idle);
    }
}

void StateMachine::transitionTo(void(StateMachine::*nextTask)()){
    _currentTask = nextTask;
}

int StateMachine::getSleepTime(){
    int sleep = 200;
    if(_currentTask != &StateMachine::idle){
        sleep = 10;
    }
    return sleep;
}