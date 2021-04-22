#ifndef LED_HPP
#define LED_HPP

#include <Arduino.h>

#define LED_SLOW_FREQ 500
#define LED_FAST_FREQ 125

enum LedStatus
{
    led_off         = 0
  , led_on          = 1
  , led_blink_slow  = 2
  , led_blink_fast  = 3
  , led_flash_once  = 4
};

class Led
{
    // The output pin this LED is connected to. 
    byte pin_;
   
    // Timepoint of last state transition.
    unsigned long tp0_;

    // Status of the LED.
    LedStatus status_;

public:
    // Not default constructible.
    Led() = delete;

    // Constructor.
    Led(byte pin);

    // Returns the pin number.
    byte pin() const;

    // Sets the Led to OFF
    void off();

    // Sets the Led to ON
    void on();

    // Sets the Led to SLOW blinking.
    void blink_slow();

    // Sets the Led to FAST blinking.
    void blink_fast();

    void flash(unsigned duration);

    // Update
    void update();

private:
    void __blink(unsigned frequency);

};

#endif // LED_HPP
