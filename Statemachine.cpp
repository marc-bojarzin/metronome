#include "Statemachine.hpp"

Statemachine::Statemachine()
  : state_(state::btn_released)
  , holding_since_(0)
  , inactive_since_(0)
  , event_(EV_NOEVENT)
{
    inactive_since_ = micros();
}

void Statemachine::update(int encoder_btn, 
                          int encoder_moved, 
                          uint32_t tapped_bpm, 
                          uint32_t tick)
{
    if (encoder_moved) {
        inactive_since_ = tick;
    }

    if (tapped_bpm) {
        inactive_since_ = tick;
    }

    switch (state_)
    {
    case state::btn_released:
        if (encoder_btn == HIGH)
        {
            state_ = state::btn_pressed;
            holding_since_ = tick;
        }
        else if (encoder_moved)
        {
            event_ = EV_ENCODER_MOVED;
        }
        else if ((tick - inactive_since_) > STANDBY_THD)
        {
            event_ = EV_INACTIVE;
        }
        break;

    case state::btn_pressed:
        if (encoder_btn == LOW)
        {
            state_ = state::btn_released;
            event_ = EV_BTN_PRESSED;
            inactive_since_ = tick;
        }
        else if (encoder_moved)
        {
            event_ = EV_ENCODER_MOVED_ALT;
            state_ = state::btn_hold;
        }
        else if ((tick - holding_since_) > LONGPRESS_THD)
        {
            state_ = state::btn_hold;
            event_ = EV_BTN_PRESSED_ALT;
        }
        break;

    case state::btn_hold:
        if (encoder_btn == LOW)
        {
            state_ = state::btn_released;
            inactive_since_ = tick;
        }
        else if (encoder_moved) 
        {
            event_ = EV_ENCODER_MOVED_ALT;
        }
        break;
    }
}

int Statemachine::event()
{
    auto wrk = event_;
    event_ = EV_NOEVENT;
    return wrk;
}
