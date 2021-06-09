#ifndef ROTARY_ENCODER_HPP
#define ROTARY_ENCODER_HPP

#include <Arduino.h>

class RotaryEncoder
{
    // CLK (clock) pin A.
    uint8_t clk_pin_;

    // DTA (data) pin B.
    uint8_t dta_pin_;

    // Last state on CLK pin.
    volatile uint8_t clk_last_;

    // Last state on DTA pin.
    volatile uint8_t dta_last_;

    // Statemeachine.
    volatile uint8_t state_;

    // Moved stept since last call to moved()
    volatile int moved_;

public:
    // Constructor.
    RotaryEncoder(uint8_t clk_pin, uint8_t dta_pin);

    // Update internal state.
    void transition(uint8_t direction);

    // Triggered by ISR in case of any edge detected on CLK.
    void on_clk_change();

    // Returns steps recorded since last iteration.
    int moved();

    // Resets state
    int reset();

private:

    // 
    void detect_zero();
};

#endif // ROTARY_ENCODER_HPP
