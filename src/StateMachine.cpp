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
        
    }
}

void StateMachine::sendingSteps(){

}

void StateMachine::waitAck(){

}

void StateMachine::transitionTo(void(StateMachine::*nextTask)()){
    _currentTask = nextTask;
}