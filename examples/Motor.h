#ifndef _MOTOR_H
#define _MOTOR_H

#include "state-machine/StateMachine.h"

class MotorData : public EventData
{
public:
    int speed;
};

class Motor : public StateMachine
{
public:
    Motor();

    // External events
    void SetSpeed(const MotorData* data);
    void Halt();

private:
    int m_currentSpeed;

    enum States
    {
        ST_IDLE,
        ST_STOP,
        ST_START,
        ST_CHANGE_SPEED,
        ST_MAX_STATES
    };

    // State functions
    STATE_DECLARE(Motor, Idle, NoEventData)
    STATE_DECLARE(Motor, Stop, NoEventData)
    STATE_DECLARE(Motor, Start, MotorData)
    STATE_DECLARE(Motor, ChangeSpeed, MotorData)

    // State map
    BEGIN_STATE_MAP_EX
        STATE_MAP_ENTRY_EX(&Idle)
        STATE_MAP_ENTRY_EX(&Stop)
        STATE_MAP_ENTRY_EX(&Start)
        STATE_MAP_ENTRY_EX(&ChangeSpeed)
    END_STATE_MAP_EX
};

#endif
