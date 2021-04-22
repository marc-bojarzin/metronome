#ifndef POTENTIOMETER_HPP
#define POTENTIOMETER_HPP

#include <Arduino.h>

/**
 * Potentiometer class.
 */
class Potentiometer
{
public:
    /// Not default constructible.
    Potentiometer() = delete;

    /// Constructor.
    Potentiometer(byte pin);

    /// Returns the analog input pinnumber.
    byte pin() const;

    /// Returns the last value sampled.
    uint16_t value() const;

private:
    // The analog input pin the potentiometer is attached to.
    const byte pin_;
};

#endif // POTENTIOMETER_HPP
