#include "RotaryEncoder.hpp"

RotaryEncoder::RotaryEncoder(uint8_t clk_pin, uint8_t dta_pin)
    : clk_pin_(clk_pin)
    , dta_pin_(dta_pin)
    , clk_last_(0)
    , dta_last_(0)
    , state_(0)
    , moved_(0)
{
    pinMode(clk_pin_, INPUT);
    pinMode(dta_pin_, INPUT);
}

void RotaryEncoder::on_clk_change()
{
    uint8_t clk = digitalRead(clk_pin_);
    uint8_t dta = digitalRead(dta_pin_);
    if (clk != clk_last_)
    {
        clk_last_ = clk;
        if (dta != dta_last_)
        {
            dta_last_ =  dta;
            transition(clk == dta ? 2 : 1);
        }
    }
}

void RotaryEncoder::transition(uint8_t direction)
{
    static const int8_t movements[16] = {
        0, -1, 1, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0
    };

    state_  = ((state_ << 2) | direction) & 0x0f;

    moved_ += movements[state_];
    if (moved_ == 0) {
        state_ = 0;
    }
}

int RotaryEncoder::moved()
{
    return (moved_ ? reset() : 0);
}

int RotaryEncoder::reset()
{
    int wrk = moved_;
    moved_ = 0;
    return wrk;
}
