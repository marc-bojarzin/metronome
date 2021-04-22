#ifndef BUTTON_HPP
#define BUTTON_HPP

#include <Arduino.h>

/**
 * Simple pushbutton class with software debouncing.
 */
class Button 
{
    static const unsigned long debounce_duration = 5;

public:
    // Not default constructible.
    Button() = delete;

    // Constructor.
    Button(byte pin);

    // Returns the pin-number the pushbutton is attached to.
    byte pin() const;

    void update();

    // Returns the last stable button status.
    // 0: button released
    // 1: button pressed
    byte status() const;

    // Returns 'TRUE' if the stable button state is pressed.
    bool pressed() const;

    // Returns 'TRUE' if the stable button state is released.
    bool released() const;

private:
    // The input pin this Button is attached to.
    const byte pin_;

    // The current stable reading from the input pin.
    byte stable_state_;

    // The previous reading from the input pin.
    byte last_state_;

    // The last time the state was changed, due to noise or pressing.
    unsigned long last_debounce_time_;
};

#endif // BUTTON_HPP
