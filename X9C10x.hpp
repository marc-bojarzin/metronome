#ifndef X9C10X_HPP
#define X9C10X_HPP

#include <Arduino.h>
#include <CircularBuffer.h>

class X9C10x
{
    // Operating characteristics
    static const unsigned long T_IL  =     1;    // INC LOW Period (1 us)
    static const unsigned long T_IH  =     1;    // INC HIGH Period (1 us)
    static const unsigned long T_CPH = 20000;    // CS Deselect Time (20 ms)

private:
    // UP/DOWN control pin.
    byte ud_pin_;

    // INCREMENT control pin.
    byte inc_pin_;

    // CHIP SELECT pin.
    byte cs_pin_;

    // Current wiper position [0..99]
    byte pos_;

    // Operations/cycles left.
    int delta_;

    // FIFO buffer 
    CircularBuffer<int, 5> fifo_;

    // Used for timing.
    unsigned long time_;

    // FSM.
    enum {
        s_start = 0
      , s_select
      , s_begin_cycle
      , s_inc_low_period
      , s_inc_high_period
      , s_deselect
    } state_;

public:
    X9C10x(byte ud_pin, byte inc_pin, byte cs_pin);
    void update();

public:
    byte pos() const;

    bool stable() const;

    void move(int steps);

    void moveto(byte pos);

    void low()
        { move(-100); }

    void high()
        { move(100); }

    void up()
        { move(1); }

    void down()
        { move(-1); }

private:
    void __deselect();
};

#endif // X9C10X_HPP
