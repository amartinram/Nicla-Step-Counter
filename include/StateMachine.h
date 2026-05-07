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

        int getSleepTime();


    private:

        typedef void (StateMachine::*StatePtr)();
        StatePtr _currentTask;

        void idle();

        void advertising();

        void sendingSteps();

        void waitAck();

        void transitionTo(StatePtr nextTask);

        NiclaCounter<Config::MAX_BUFFER> _counter;
        NiclaComm _comm;
        
        bool _headerSent;
        unsigned long _lastPacketTime;
        unsigned long _lastAttempt;
        unsigned long _stateStartTime;
};

#endif