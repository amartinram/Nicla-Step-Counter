#include "StateMachine.h"


StateMachine::StateMachine():
    _counter(Config::DUMP_DAY,Config::MINUTE_INTERVAL), 
    _currentTask(&StateMachine::idle),
    _headerSent(false),
    _lastPacketTime(0)
{}
void StateMachine::initialize(){
    _counter.beginSensor();
    _comm.begin();
}

void StateMachine::run(){
    (this->*_currentTask)();
}

void StateMachine::idle(){
    _counter.update();
    if(_counter.getMinute() >= Config::DUMP_DAY){
        _comm.advertise();
        transitionTo(&StateMachine::advertising);
    }
}

void StateMachine::advertising(){
    if(_comm.centralConnected()){
        _headerSent = false;
        transitionTo(&StateMachine::sendingSteps);
    }
}

void StateMachine::sendingSteps(){
    if(!_headerSent){
        _comm.sendHeader(Config::DUMP_DAY,
            _counter.getTotalSteps(),
            nicla::getBatteryVoltagePercentage());
        _headerSent = true;
        _lastPacketTime = millis();
    }else if(millis() - _lastPacketTime >= 40){
        bool finished = _comm.sendPackets(_counter.getBuffer(),Config::DUMP_DAY);

        if(finished){
            transitionTo(&StateMachine::waitAck);
        }
        _lastPacketTime = millis();
    }
}

void StateMachine::waitAck(){
    if(_comm.ackReceived()){
        _counter.cleanBuffer();
        _comm.stopAdvertise();
        transitionTo(&StateMachine::idle);
    }
}

void StateMachine::transitionTo(void(StateMachine::*nextTask)()){
    _currentTask = nextTask;
}