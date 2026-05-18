#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "NiclaCounter.h"
#include "NiclaComm.h"
#include "Config.h"

class StateMachine{
    public:
        StateMachine();

        void initialize();

        void run();

        uint32_t getSleepTime();

        void sleep(uint32_t ms);

    private:
        typedef void (StateMachine::*StatePtr)();
        StatePtr _currentTask;

        void idle();

        void advertising();

        void sendingSteps();

        void waitAck();
        
        void transitionTo(StatePtr nextTask);

        NiclaCounter _counter;
        NiclaComm _comm;
        
        uint32_t _lastPacketTime;
        uint32_t _lastAttempt;
        uint32_t _stateStartTime;
        uint32_t _delayStart;

        bool _headerSent;
        bool _waitingBetweenDays;
};

#endif