#include "Potentiometer.hpp"

Potentiometer::Potentiometer(byte pin)
    : pin_(pin)
{
}

byte Potentiometer::pin() const
{ 
    return pin_;
}

uint16_t Potentiometer::value() const
{
    auto sample = analogRead(pin_);
    return map(sample, 0, 1023, 0, 86);
}
