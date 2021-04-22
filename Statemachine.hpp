#ifndef STATEMACHINE_HPP
#define STATEMACHINE_HPP

#include <Arduino.h>

#define EV_NOEVENT 0
#define EV_BTN_PRESSED_SHORT 1
#define EV_BTN_PRESSED_LONG 2

class Statemachine
{    
    enum {
        s_released = 0
      , s_pressed_down
      , s_pressed_hold
    } state_;

    unsigned long holding_since_;
    int last_event_;

public:
    Statemachine();
    void update(int);
    int poll();
};



#endif
