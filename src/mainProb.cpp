#include "StateMachine.h"

StateMachine worker;

void setup() {
    worker.initialize();
}

void loop() {
    worker.run();
    rtos::ThisThread::sleep_for(std::chrono::milliseconds(worker.getSleepTime()));
}