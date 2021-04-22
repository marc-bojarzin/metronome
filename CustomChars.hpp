#ifndef CUSTOM_CHARS_HPP
#define CUSTOM_CHARS_HPP

// A custom quater-note character.
static byte note_character[] = 
{
  B00001,
  B00001,
  B00001,
  B00111,
  B01111,
  B01111,
  B00110,
  B00000
};

// A umlaut
static byte a_uml[] = {
  B01010,
  B00000,
  B01110,
  B00001,
  B01111,
  B10001,
  B01111,
  B00000
};

// Partial bar 1/5
static byte bar_1[] = {
  B00000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B00000
};

// Partial bar 2/5
static byte bar_2[] = {
  B00000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B00000
};

// Partial bar 3/5
static byte bar_3[] = {
  B00000,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B00000
};

// Partial bar 4/5
static byte bar_4[] = {
  B00000,
  B11110,
  B11110,
  B11110,
  B11110,
  B11110,
  B11110,
  B00000
};

// Partial bar 5/5 (full)
static byte bar_5[] = {
  B00000,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B00000
};

#endif // CUSTOM_CHARS_HPP
