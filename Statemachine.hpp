#ifndef STATEMACHINE_HPP
#define STATEMACHINE_HPP

#include <Arduino.h>

static const int EV_NOEVENT            =  0;
static const int EV_INACTIVE           =  1;  // no action for 15 seconds
static const int EV_BTN_PRESSED        =  2;  // button pressed and released
static const int EV_BTN_PRESSED_ALT    =  3;  // button pressed and released after 3 seconds
static const int EV_ENCODER_MOVED      =  4;  // rotary encoder moved
static const int EV_ENCODER_MOVED_ALT  =  5;  // rotary encoder moved while pressed

class Statemachine
{
    // Standby threshold
    static const uint32_t STANDBY_THD   = (15ul * 1000ul * 1000ul);  // 15 seconds
    static const uint32_t LONGPRESS_THD = ( 3ul * 1000ul * 1000ul);  //  3 seconds

    enum class state {
        btn_released = 0
      , btn_pressed
      , btn_hold
    } state_;

    uint32_t inactive_since_;
    uint32_t holding_since_;
    int event_;

public:
    /// Constructor.
    Statemachine();

    /// Updates statemachine with external triggers.
    void update(int, int, uint32_t);

    /// Returns the last event.
    int event();
};

#endif // STATEMACHINE_HPP
