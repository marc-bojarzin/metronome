#ifndef TAP_TEMPO_HPP
#define TAP_TEMPO_HPP

#include <Arduino.h>

class TapTempo
{
    enum class state : uint8_t {
        released = 0
      , triggered
      , high
      , wait_low
    };

    static constexpr size_t NUM_TAPS = 4;

    static constexpr uint32_t debounce_time     =    5;  //   5  ms
    static constexpr uint32_t tap_lower_bound   =  250;  //  250 ms -> 240 BPM
    static constexpr uint32_t tap_upper_bound   = 1500;  // 1500 ms ->  40 BPM

    // Pin number of push button (with pull down resistor)
    uint8_t pin_;

    // Internal state.
    volatile state state_;

    // Timestamps of the last 2 taps detected.
    volatile uint32_t taps_[2];

    // Timestamp of last change on pin.
    uint32_t tap_debounce_;

    // Last valid BPM.
    uint32_t bpm_;

public:
    TapTempo(uint8_t pin);
    void reset();
    void trigger();
    void update();
    uint32_t bpm();

private:
    void __on_tap();
};

#endif // TAP_TEMPO_HPP
