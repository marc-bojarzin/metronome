#ifndef DE_JITTER_HPP
#define DE_JITTER_HPP

#include <LiquidCrystal_I2C.h>

class DeJitter
{
    LiquidCrystal_I2C* lcd_;
    char line_[16];

public:
    DeJitter() = delete;
    DeJitter(LiquidCrystal_I2C* lcd);
    void clear();
    void update(uint8_t row, char* content);
};

#endif
