#include "TapTempo.hpp"

TapTempo::TapTempo(uint8_t pin)
  : pin_(pin)
  , last_tap_(0)
  , taps_()
  , index_(0)
{
    pinMode(pin_, INPUT);
}

void TapTempo::trigger()
{
}

void TapTempo::update()
{
    uint32_t now = millis();

    auto duration = (now - last_tap_);
    if (duration < debounce_time)
    {
        return;
    }

    if (duration > tap_upper_bound)  // timeout ( < 40 BPM)
    {
        // reset taps recorded
        taps_[0] = 0;
        taps_[1] = 0;
        taps_[2] = 0;
        taps_[3] = 0;
        index_ = 0;

        return;
    }

    duration = max(tap_lower_bound, duration);
}

void TapTempo::reset_taps()
{
    for (auto i = 0; i < NUM_TAPS; ++i) {
        taps_[i] = 0;
    }
}
