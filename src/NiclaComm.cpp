#include "NiclaComm.h"

NiclaComm::NiclaComm():
    _stepService("00001814-0000-1000-8000-00805f9b34fb"), 
    _logCharacteristic("00002a37-0000-1000-8000-00805f9b34fb", BLERead | BLENotify | BLEWrite, 512)
    {}

void NiclaComm::begin(){
  if (!BLE.begin()) {
    while (1);
  }
  
  BLE.setLocalName("Nicla_Steps"); 
  BLE.setAdvertisedService(_stepService);
  _stepService.addCharacteristic(_logCharacteristic);
  BLE.addService(_stepService);
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
    //First two bytes preamble to mark the start of a transaction
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

void NiclaComm::advertise(){
  BLE.advertise(); 
}

void NiclaComm::stopAdvertise(){
    BLE.stopAdvertise();
}