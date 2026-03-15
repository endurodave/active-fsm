#ifndef _PLAYER_H
#define _PLAYER_H

#include "state-machine/StateMachine.h"

class Player : public StateMachine
{
public:
    Player();

    void OpenClose();
    void Play();
    void Stop();
    void Pause();
    void EndPause();

private:
    enum States
    {
        ST_EMPTY,
        ST_OPEN,
        ST_STOPPED,
        ST_PAUSED,
        ST_PLAYING,
        ST_MAX_STATES
    };

    // States
    STATE_DECLARE(Player, Empty, NoEventData)
    STATE_DECLARE(Player, Open, NoEventData)
    STATE_DECLARE(Player, Stopped, NoEventData)
    STATE_DECLARE(Player, Paused, NoEventData)
    STATE_DECLARE(Player, Playing, NoEventData)

    // State map
    BEGIN_STATE_MAP_EX
        STATE_MAP_ENTRY_EX(&Empty)
        STATE_MAP_ENTRY_EX(&Open)
        STATE_MAP_ENTRY_EX(&Stopped)
        STATE_MAP_ENTRY_EX(&Paused)
        STATE_MAP_ENTRY_EX(&Playing)
    END_STATE_MAP_EX
};

#endif
