#include "StateMachine.h"


StateMachine::StateMachine():
    _counter(Config::DUMP_DAY,Config::MINUTE_INTERVAL), 
    _currentTask(&StateMachine::idle),
    _headerSent(false),
    _lastPacketTime(0)
{}
void StateMachine::initialize(){
    /*Serial.begin(115200);
    _counter.beginSensor();
    _comm.begin();
    Serial.println("System Initialized. State: IDLE");*/
    Serial.begin(115200); 
    
    Serial.begin(115200); 
    
    // REMOVED: while(!Serial);
    // ADDED: A 5-second hard delay. 
    delay(5000); 

    // Added \n\n to create space in the monitor so it's easy to see
    Serial.println("\n\n--- NICLA BOOTING ---");
    
    Serial.println("1. Starting Nicla System...");
    nicla::begin(); 
    nicla::leds.setColor(off);

    Serial.println("2. Starting BHY2 Sensors...");
    BHY2.begin(); 
    
    Serial.println("3. Starting Step Counter...");
    _counter.beginSensor(); 
    
    Serial.println("4. Starting BLE Radio...");
    _comm.begin();

    Serial.println("--- SYSTEM INITIALIZED. STATE: IDLE ---");
}


void StateMachine::run(){
    BLE.poll();
    _counter.update();
    (this->*_currentTask)();
}

void StateMachine::idle(){
    if(_counter.getMode() == NiclaCounter<Config::MAX_BUFFER>::STEPSENDING){
        Serial.println("Buffer Full! Transitioning to ADVERTISING...");
        _comm.advertise();
        transitionTo(&StateMachine::advertising);
    }
}

void StateMachine::advertising(){
    if(_comm.centralConnected() && _comm.isSuscribed()){
        Serial.println("Phone Connected & Subscribed! Transitioning to SENDING...");
        _headerSent = false;
        transitionTo(&StateMachine::sendingSteps);
    }
}

void StateMachine::sendingSteps(){
    if(_comm.centralConnected()){
        if(!_headerSent){
            Serial.println("Sending Header...");
            _comm.sendHeader(Config::DUMP_DAY,
            _counter.getTotalSteps(),
            nicla::getBatteryVoltagePercentage());
            _headerSent = true;
            Serial.println("Header Sent!");
            _lastPacketTime = millis();
        }else if(millis() - _lastPacketTime >= 100){
            Serial.println("Sending Data Packets...");
            bool finished = _comm.sendPackets(_counter.getBuffer(),Config::DUMP_DAY);

            if(finished){
                Serial.println("Data Sent! Transitioning to WAIT ACK...");
                transitionTo(&StateMachine::waitAck);
            }
            _lastPacketTime = millis();
        }
    }else{
        Serial.println("Connection lost during send. Back to IDLE.");
        _comm.stopAdvertise();
        transitionTo(&StateMachine::idle);
    }
    
}

void StateMachine::waitAck(){
    if(_comm.centralConnected()){
        if(_comm.ackReceived()){
            Serial.println("ACK 0xCC Received! Cleaning buffer...");
            _counter.cleanBuffer();
            _comm.stopAdvertise();
            transitionTo(&StateMachine::idle);
        }
    }else{
        Serial.println("Connection lost waiting for ACK. Back to IDLE.");
        _comm.stopAdvertise();
        transitionTo(&StateMachine::idle);
    }
    
}

void StateMachine::transitionTo(void(StateMachine::*nextTask)()){
    _currentTask = nextTask;
}