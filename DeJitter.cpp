#include "DeJitter.hpp"

DeJitter::DeJitter(LiquidCrystal_I2C* lcd)
  : lcd_(lcd)
  , line_()
{
    clear();
}

void DeJitter::clear()
{
    memset(line_, ' ', 16);
}

void DeJitter::update(uint8_t row, char* content)
{
    //Serial.println(line_buf__);
    for (int i = 0; i < 16; ++i)
    {
        if (line_[i] != content[i])
        {
            line_[i] = content[i];
            lcd_->setCursor(i, row);
            lcd_->write(line_[i]);
        }
    }
}
