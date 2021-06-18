#ifndef TAP_TEMPO_HPP
#define TAP_TEMPO_HPP

#include <Arduino.h>

class TapTempo
{
    static constexpr size_t NUM_TAPS = 4;

    static constexpr uint32_t debounce_time     =    5;  //   5  ms
    static constexpr uint32_t tap_lower_bound   =  250;  //  250 ms -> 240 BPM
    static constexpr uint32_t tap_upper_bound   = 1500;  // 1500 ms ->  40 BPM

    // Pin number of push button (with pull down resistor)
    uint8_t pin_;

    // System time of last regular tap recorded.
    uint32_t last_tap_;

    // Duration between the last 4 taps.
    uint32_t taps_[NUM_TAPS];

    // Index into the taps_ array.
    size_t   index_;

public:
    TapTempo(uint8_t pin);
    void trigger();
    void update();

private:
    void reset_taps();
};

#endif // TAP_TEMPO_HPP
