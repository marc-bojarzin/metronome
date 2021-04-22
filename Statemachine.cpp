#include "Statemachine.hpp"

Statemachine::Statemachine()
  : state_(s_released)
  , holding_since_(0)
  , last_event_(EV_NOEVENT)
{
}

void Statemachine::update(int probe)
{
    switch (state_)
    {
    case s_released:
        if (probe == HIGH)
        {
            state_ = s_pressed_down;
            holding_since_ = millis();
        }
        break;

    case s_pressed_down:
        if (probe == LOW)
        {
            state_ = s_released;
            last_event_ = EV_BTN_PRESSED_SHORT;
        }
        else if ((millis() - holding_since_) > 3000)
        {
            state_ = s_pressed_hold;
            last_event_ = EV_BTN_PRESSED_LONG;
        }
        break;
    case s_pressed_hold:
        if (probe == LOW)
        {
            state_ = s_released;
        }
        break;
    }
}

int Statemachine::poll()
{
    auto wrk = last_event_;
    last_event_ = EV_NOEVENT;
    return wrk;
}
