#include "X9C10x.hpp"

X9C10x::X9C10x(byte ud_pin, byte inc_pin, byte cs_pin)
  : ud_pin_(ud_pin)
  , inc_pin_(inc_pin)
  , cs_pin_(cs_pin_)
  , pos_(0)
  , time_(micros())
{
    pinMode(ud_pin_, OUTPUT);
    pinMode(inc_pin_, OUTPUT);
    pinMode(cs_pin_, OUTPUT);
    __deselect();
}

void X9C10x::update()
{
    switch (state_) 
    {
    case s_start: 
        if (fifo_.size()) 
        {
            delta_ = fifo_.shift();
            if (delta_ >= 1000) {
                delta_ = (delta_ - 1000 - pos_);
            }
            if (delta_ != 0) {
                state_ = s_select;
            }
        }
        break;

    case s_select:
        digitalWrite(ud_pin_, (delta_ > 0 ? HIGH : LOW));
        digitalWrite(cs_pin_, LOW);
        state_ = s_begin_cycle;
        break;

    case s_begin_cycle:
        digitalWrite(inc_pin_, LOW);
        state_ = s_inc_low_period;
        time_ = micros();
        break;

    case s_inc_low_period:
        if (micros() - time_ >= T_IL)
        {
            pos_ += (delta_ > 0 ? 1 : -1);
            pos_ = (pos_ == 255 ? 0 : min(pos_, 99));

            delta_ += (delta_ > 0 ? -1 : 1);
            if (delta_ == 0)
            {
                digitalWrite(cs_pin_, HIGH);  // no store, return to standby
                state_ = s_deselect;
            }
            else 
            {
                state_= s_inc_high_period;
            }
            digitalWrite(inc_pin_, HIGH);
            time_ = micros();
        }
        break;

    case s_inc_high_period:
        if (micros() - time_ >= T_IH)
        {
            digitalWrite(inc_pin_, LOW);
            state_ = s_inc_low_period;
            time_ = micros();
        }
        break;

    case s_deselect:
        if (micros() - time_ >= T_CPH)
        {
            state_ = s_start;
        }
        break;
    }
}

bool X9C10x::stable() const
{
    return fifo_.isEmpty();
}

byte X9C10x::pos() const
{
    return pos_;
}

void X9C10x::move(int steps)
{
    fifo_.push(steps);
}

void X9C10x::moveto(byte pos)
{
    pos = max(0, pos);
    pos = min(pos, 99);
    move(1000 + pos);
}

void X9C10x::__deselect()
{
    digitalWrite(inc_pin_, LOW);
    delayMicroseconds(1);
    digitalWrite(cs_pin_, HIGH);
    digitalWrite(inc_pin_, HIGH);
}
