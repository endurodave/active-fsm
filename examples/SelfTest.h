#ifndef _SELF_TEST_H
#define _SELF_TEST_H

#include "state-machine/StateMachine.h"

/// @brief SelfTest is a subclass state machine for other self tests to
/// inherit from. The class has common states for all derived classes to share.
class SelfTest : public StateMachine
{
public:
    SelfTest(uint8_t maxStates);

    virtual void Start() = 0;
    void Cancel();

protected:
    enum States
    {
        ST_IDLE,
        ST_COMPLETED,
        ST_FAILED,
        ST_MAX_STATES
    };

    // States
    STATE_DECLARE(SelfTest, Idle, NoEventData)
    STATE_DECLARE(SelfTest, Completed, NoEventData)
    STATE_DECLARE(SelfTest, Failed, NoEventData)

    // Entry
    ENTRY_DECLARE(SelfTest, EntryIdle, NoEventData)
};

#endif
