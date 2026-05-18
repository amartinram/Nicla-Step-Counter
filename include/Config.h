#ifndef CONFIG_H
#define CONFIG_H

namespace Config {
    constexpr uint16_t DUMP_DAY = 1440; 
    constexpr uint8_t MAX_DAYS = 2; 

    constexpr uint32_t MINUTE_INTERVAL = 60000;
    constexpr uint32_t BLE_RETRY_INTERVAL = 1800000; 
    constexpr uint32_t BLE_ADVERTISE_TIMEOUT = 120000; 
    constexpr uint32_t BLE_ACK_TIMEOUT = 10000;
}

#endif