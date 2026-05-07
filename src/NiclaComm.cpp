#include "NiclaComm.h"


NiclaComm::NiclaComm():
    _stepService("00001814-0000-1000-8000-00805f9b34fb"), 
    _logCharacteristic("00002a37-0000-1000-8000-00805f9b34fb", BLERead | BLENotify | BLEWrite, 512)
    {}

bool NiclaComm::begin(){
    bool ok = false;
    int attempt = 3;

    while (!BLE.begin() && attempt > 0) {
        rtos::ThisThread::sleep_for(std::chrono::milliseconds(100));
        attempt--;
    }
    
    if(attempt > 0){
        if (!_isSetup) {
            BLE.setLocalName("Nicla_Steps"); 
            BLE.setAdvertisedService(_stepService);
            _stepService.addCharacteristic(_logCharacteristic);
            BLE.addService(_stepService);
            _isSetup = true; 
        }
        
        _logCharacteristic.writeValue((uint8_t)0);
        ok = true; 
    }
    
    return ok;

}

bool NiclaComm::ackReceived(){
    bool ok = false;
    if(_logCharacteristic.written() && _logCharacteristic.valueLength()>0 &&
        _logCharacteristic.value()[0] == 0xCC){
            ok = true;
    }
    return ok;
}

bool NiclaComm::centralConnected(){
    bool ok = true;
    if (!BLE.connected()) {
      ok = false;
    }
    return ok;
}

void NiclaComm::sendHeader(int length, uint32_t totalSteps, int8_t battery){

    uint8_t header[9];
    header[0] = 0xAA; 
    header[1] = 0xBB; 
    header[2] = (length >> 8) & 0xFF;
    header[3] = length & 0xFF;
    header[4] = (totalSteps >> 24) & 0xFF;
    header[5] = (totalSteps >> 16) & 0xFF;
    header[6] = (totalSteps >> 8) & 0xFF;
    header[7] = totalSteps & 0xFF;
    header[8] = battery;

    _logCharacteristic.writeValue(header, 9);
    _offset = 0;
}

int NiclaComm::getOffset(){
    return _offset;
}

bool NiclaComm::sendPackets(const uint8_t* dailyLog, int length){
    int bytesRemaining = length - _offset;
    int chunkSize = (bytesRemaining > 244) ? 244 : bytesRemaining;
            
    _logCharacteristic.writeValue(dailyLog + _offset, chunkSize);
    _offset += chunkSize;
    
    return _offset == length;
}


bool NiclaComm::isSubscribed(){
    return _logCharacteristic.subscribed();
}

bool NiclaComm::bluetoothOn(){
    bool ok = false;
    if(begin()){
        BLE.advertise();
        ok = true;
    }
    return ok;
}

void NiclaComm::bluetoothOff(){
    BLE.disconnect();
    rtos::ThisThread::sleep_for(std::chrono::milliseconds(50));
    BLE.end();
    _isSetup = false;
}