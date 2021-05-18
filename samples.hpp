#ifndef SAMPLES_HPP
#define SAMPLES_HPP

// Sample rate: 8000 samples per second
// Sample format: 8 bit unsigned

// All waves have same length of 400 sample (50ms)
static const size_t SAMPLE_LENGTH = 400;

// Strong pulse at 1kHz ("tock")
static PROGMEM const int8_t PULSE_1000 [ SAMPLE_LENGTH ] = 
{
    -128, -127, -126, -125,  123,  122,  121,  120, -120, -119, -118, -117,  116,  115,  114,  113,
    -113, -112, -111, -110,  109,  108,  107,  106, -106, -105, -105, -104,  102,  101,  101,  100,
    -100,  -99,  -98,  -98,   96,   95,   95,   94,  -94,  -93,  -92,  -92,   90,   90,   89,   88,
     -88,  -87,  -87,  -86,   85,   84,   83,   83,  -83,  -82,  -81,  -81,   79,   79,   78,   78,
     -78,  -77,  -76,  -76,   75,   74,   73,   73,  -73,  -72,  -72,  -71,   70,   69,   69,   68,
     -68,  -68,  -67,  -66,   65,   65,   64,   64,  -64,  -63,  -63,  -62,   61,   61,   60,   60,
     -60,  -59,  -59,  -58,   57,   57,   56,   56,  -56,  -55,  -55,  -54,   54,   53,   53,   52,
     -52,  -52,  -51,  -51,   50,   50,   49,   49,  -49,  -48,  -48,  -47,   47,   46,   46,   45,
     -45,  -45,  -45,  -44,   43,   43,   43,   42,  -42,  -42,  -42,  -41,   40,   40,   40,   39,
     -39,  -39,  -39,  -38,   38,   37,   37,   37,  -37,  -36,  -36,  -36,   35,   35,   34,   34,
     -34,  -34,  -33,  -33,   33,   32,   32,   32,  -32,  -31,  -31,  -31,   30,   30,   30,   29,
     -29,  -29,  -29,  -28,   28,   28,   27,   27,  -27,  -27,  -27,  -26,   26,   26,   25,   25,
     -25,  -25,  -24,  -24,   24,   24,   23,   23,  -23,  -23,  -23,  -22,   22,   22,   21,   21,
     -21,  -21,  -21,  -21,   20,   20,   20,   20,  -19,  -19,  -19,  -19,   19,   18,   18,   18,
     -18,  -18,  -17,  -17,   17,   17,   17,   16,  -16,  -16,  -16,  -16,   16,   15,   15,   15,
     -15,  -15,  -15,  -14,   14,   14,   14,   14,  -14,  -13,  -13,  -13,   13,   13,   13,   12,
     -12,  -12,  -12,  -12,   12,   11,   11,   11,  -11,  -11,  -11,  -11,   10,   10,   10,   10,
     -10,  -10,  -10,  -10,    9,    9,    9,    9,   -9,   -9,   -9,   -9,    8,    8,    8,    8,
      -8,   -8,   -8,   -8,    7,    7,    7,    7,   -7,   -7,   -7,   -7,    7,    6,    6,    6,
      -6,   -6,   -6,   -6,    6,    6,    6,    5,   -5,   -5,   -5,   -5,    5,    5,    5,    5,
      -5,   -5,   -4,   -4,    4,    4,    4,    4,   -4,   -4,   -4,   -4,    4,    4,    3,    3,
      -3,   -3,   -3,   -3,    3,    3,    3,    3,   -3,   -3,   -3,   -2,    2,    2,    2,    2,
      -2,   -2,   -2,   -2,    2,    2,    2,    2,   -2,   -2,   -1,   -1,    1,    1,    1,    1,
      -1,   -1,   -1,   -1,    1,    1,    1,    1,   -1,   -1,   -1,   -1,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0
};

// Strong pulse at 2kHz ("tick")
static PROGMEM const int8_t PULSE_2000 [ SAMPLE_LENGTH ] = 
{
    -128, -127,  125,  124, -124, -123,  121,  120, -120, -119,  117,  116, -116, -116,  114,  113,
    -113, -112,  110,  110, -110, -109,  107,  106, -106, -105,  104,  103, -103, -102,  101,  100,
    -100,  -99,   98,   97,  -97,  -96,   95,   94,  -94,  -93,   92,   91,  -91,  -90,   89,   88,
     -88,  -87,   86,   85,  -85,  -85,   83,   83,  -83,  -82,   81,   80,  -80,  -79,   78,   78,
     -78,  -77,   76,   75,  -75,  -75,   73,   73,  -73,  -72,   71,   70,  -70,  -70,   69,   68,
     -68,  -68,   67,   66,  -66,  -65,   64,   64,  -64,  -63,   62,   62,  -62,  -61,   60,   60,
     -60,  -59,   58,   58,  -58,  -57,   56,   56,  -56,  -55,   54,   54,  -54,  -53,   53,   52,
     -52,  -52,   51,   50,  -50,  -50,   49,   49,  -49,  -48,   47,   47,  -47,  -47,   46,   45,
     -45,  -45,   44,   44,  -44,  -43,   43,   42,  -42,  -42,   41,   41,  -41,  -40,   40,   39,
     -39,  -39,   38,   38,  -38,  -38,   37,   37,  -37,  -36,   36,   35,  -35,  -35,   34,   34,
     -34,  -34,   33,   33,  -33,  -32,   32,   32,  -32,  -31,   31,   30,  -30,  -30,   30,   29,
     -29,  -29,   28,   28,  -28,  -28,   27,   27,  -27,  -27,   26,   26,  -26,  -26,   25,   25,
     -25,  -25,   24,   24,  -24,  -24,   23,   23,  -23,  -23,   22,   22,  -22,  -22,   21,   21,
     -21,  -21,   21,   20,  -20,  -20,   20,   20,  -19,  -19,   19,   19,  -19,  -18,   18,   18,
     -18,  -18,   17,   17,  -17,  -17,   17,   16,  -16,  -16,   16,   16,  -16,  -15,   15,   15,
     -15,  -15,   14,   14,  -14,  -14,   14,   14,  -14,  -13,   13,   13,  -13,  -13,   13,   12,
     -12,  -12,   12,   12,  -12,  -12,   11,   11,  -11,  -11,   11,   11,  -11,  -10,   10,   10,
     -10,  -10,   10,   10,   -9,   -9,    9,    9,   -9,   -9,    9,    9,   -8,   -8,    8,    8,
      -8,   -8,    8,    8,   -8,   -7,    7,    7,   -7,   -7,    7,    7,   -7,   -7,    6,    6,
      -6,   -6,    6,    6,   -6,   -6,    6,    5,   -5,   -5,    5,    5,   -5,   -5,    5,    5,
      -5,   -5,    4,    4,   -4,   -4,    4,    4,   -4,   -4,    4,    4,   -4,   -4,    3,    3,
      -3,   -3,    3,    3,   -3,   -3,    3,    3,   -3,   -3,    3,    2,   -2,   -2,    2,    2,
      -2,   -2,    2,    2,   -2,   -2,    2,    2,   -2,   -2,    1,    1,   -1,   -1,    1,    1,
      -1,   -1,    1,    1,   -1,   -1,    1,    1,   -1,   -1,    1,    1,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0
};

#endif
