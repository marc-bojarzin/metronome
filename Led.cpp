#include "Led.hpp"

Led::Led(byte pin)
    : pin_(pin)
    , tp0_(millis())
    , status_(led_off)
{
    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, LOW);
}

byte Led::pin() const 
{
    return pin_;
}

void Led::off()
{
    status_ = led_off;
    digitalWrite(pin_, LOW);
}

void Led::on()
{
    status_ = led_on;
    digitalWrite(pin_, HIGH);
}

void Led::blink_slow()
{
    status_ = led_blink_slow;
}

void Led::blink_fast()
{
    status_ = led_blink_fast;
}

void Led::flash(unsigned duration)
{
    status_ = led_flash_once;
    digitalWrite(pin_, HIGH);
    tp0_ = millis() + duration;
}

void Led::update()
{
    switch(status_) 
    {
    case led_blink_slow: 
        __blink(LED_SLOW_FREQ);
        break;

    case led_blink_fast: 
        __blink( LED_FAST_FREQ);
        break;
    
    case led_flash_once:
        if (millis() > tp0_) 
        {
            digitalWrite(pin_, LOW);
            status_ = led_off;
        }

    default: return;
    }

}

void Led::__blink(unsigned frequency)
{
    if (frequency && (millis() - tp0_ > frequency))
    {
        tp0_ = millis();
        digitalWrite(pin_, !digitalRead(pin_));
    }
}
