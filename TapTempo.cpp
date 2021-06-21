#include "TapTempo.hpp"

TapTempo::TapTempo(uint8_t pin)
  : pin_(pin)
  , state_(state::released)
  , taps_()
  , tap_debounce_(0)
  , bpm_(0)
{
    pinMode(pin_, INPUT);
    taps_[0] = 0;
    taps_[1] = 0;
}

void TapTempo::trigger()
{
    // Rising edge detected on input pin, i.e. the button was pressed
    if (state_ == state::released)
    {
        state_ = state::triggered;
        taps_[0] = millis();
    }
}

void TapTempo::reset()
{
    bpm_ = 0;
    taps_[0] = 0;
    taps_[1] = 0;
    state_ = state::released;
}

void TapTempo::update()
{
    switch(state_)
    {
    case state::released:      // waiting for next rising edge on pin
        break;

    case state::triggered:     // rising edged detected
        state_ = state::high;
        break;

    case state::high:
        if (digitalRead(pin_) == LOW)
        {
            tap_debounce_ = millis();
            state_ = state::wait_low;
        }
        break;
    
    case state::wait_low:
        if (digitalRead(pin_) == HIGH)
        {
            state_ = state::high;
        }
        else if (millis() - tap_debounce_ > debounce_time)
        {
            __on_tap();
            state_ = state::released;
        }
    }
}

void TapTempo::__on_tap()
{
    if (taps_[0] > 0 && taps_[1] > 0)    // got 2 valid taps ?
    {
        auto duration = (taps_[0] - taps_[1]);

        // Serial.print("__on_tap:");
        // Serial.print(taps_[0]);
        // Serial.print(" ");
        // Serial.print(taps_[1]);
        // Serial.print(" ");
        // Serial.print(duration);

        if (duration > tap_upper_bound)
        {
            // (t0-t1) > 1500ms (i.e. bpm < 40) => TIMED OUT
            taps_[0] = 0;
            bpm_ = 0;
        }
        else
        {
            // (t0-t1) capped 250ms (i.e. bpm = 240)
            duration = max(tap_lower_bound, duration);
            bpm_ = (60000 / duration);
            // Serial.print(" ");
            // Serial.print(bpm_);
        }
    }
    // Serial.println();
    taps_[1] = taps_[0];
}

uint32_t TapTempo::bpm()
{
    uint32_t wrk = 0;
    if (bpm_ > 0)
    {
        auto wrk = bpm_;
        bpm_ = 0;
        return wrk;
    }
    return 0;
}
