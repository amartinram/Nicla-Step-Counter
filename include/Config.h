#ifndef CONFIG_H
#define CONFIG_H

namespace Config {
    constexpr int DUMP_DAY = 30; 
    constexpr int MAX_DAYS_TO_STORE = 2; 
    constexpr int MAX_BUFFER = DUMP_DAY * MAX_DAYS_TO_STORE; 
    constexpr long MINUTE_INTERVAL =1000;
}

#endif