#ifndef CONFIG_H
#define CONFIG_H

namespace Config {
    constexpr int DUMP_DAY = 3; 
    constexpr int MAX_DAYS = 3; 
    constexpr long MINUTE_INTERVAL = 60000;

    constexpr unsigned long BLE_RETRY_INTERVAL = 100000; 
    constexpr unsigned long BLE_ADVERTISE_TIMEOUT = 120000; 
    constexpr unsigned long BLE_ACK_TIMEOUT = 10000;
}

#endif