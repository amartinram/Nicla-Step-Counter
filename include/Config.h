#ifndef CONFIG_H
#define CONFIG_H

namespace Config {
    constexpr int DUMP_DAY = 5; 
    constexpr int MAX_DAYS_TO_STORE = 2; 
    constexpr int MAX_BUFFER = DUMP_DAY * MAX_DAYS_TO_STORE; 
    constexpr long MINUTE_INTERVAL =1000;

    constexpr unsigned long BLE_RETRY_INTERVAL = 10000; 
    constexpr unsigned long BLE_ADVERTISE_TIMEOUT = 120000; 
    constexpr unsigned long BLE_ACK_TIMEOUT = 10000;
}

#endif