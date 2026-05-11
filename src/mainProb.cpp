#include "StateMachine.h"

StateMachine worker;

void setup() {
    worker.initialize();
}

void loop() {
    worker.run();
    worker.sleep(worker.getSleepTime());
}