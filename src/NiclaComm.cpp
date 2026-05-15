#include "NiclaComm.h"

NiclaComm::NiclaComm():
    _stepService("00001814-0000-1000-8000-00805f9b34fb"), 
    _logCharacteristic("00002a37-0000-1000-8000-00805f9b34fb", BLERead | BLENotify | BLEWrite, 512)
{}

void NiclaComm::begin(){
    while (!BLE.begin()) {
        rtos::ThisThread::sleep_for(std::chrono::milliseconds(100));
    }
    
    BLE.setLocalName("Nicla_Steps"); 
    BLE.setAdvertisedService(_stepService);
    _stepService.addCharacteristic(_logCharacteristic);
    BLE.addService(_stepService);
    
    _logCharacteristic.writeValue((uint8_t)0);
}

bool NiclaComm::ackReceived(){
    bool ok = false;
    if(_logCharacteristic.written() && _logCharacteristic.valueLength() > 0 &&
        _logCharacteristic.value()[0] == 0xCC){
        ok = true;
    }
    return ok;
}

bool NiclaComm::centralConnected(){
    return BLE.connected();
}

void NiclaComm::sendHeader(uint16_t length, uint32_t totalSteps, int8_t battery){
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

bool NiclaComm::sendPackets(const uint8_t* dailyLog, uint16_t length){
    uint16_t bytesRemaining = length - _offset;
    uint16_t chunkSize = (bytesRemaining > 244) ? 244 : bytesRemaining;
            
    _logCharacteristic.writeValue(dailyLog + _offset, chunkSize);
    _offset += chunkSize;
    
    return _offset == length;
}

bool NiclaComm::isSubscribed(){
    return _logCharacteristic.subscribed();
}

void NiclaComm::bluetoothOn(){
    BLE.advertise();
}

void NiclaComm::bluetoothOff(){
    BLE.disconnect();
    rtos::ThisThread::sleep_for(std::chrono::milliseconds(50));
    BLE.stopAdvertise();
}