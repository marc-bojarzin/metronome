
#include <Arduino.h>
#include <limits.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <limits.h>

#include "Led.hpp"
#include "Button.hpp"
#include "Statemachine.hpp"
#include "CustomChars.hpp"
#include "DeJitter.hpp"
#include "samples.hpp"

#define DEBUG_SERIAL

#define _SET(x,y) (x |= (1 << y))                //- bit set/clear macros
#define _CLR(x,y) (x &= (~(1 << y)))             //  |
#define _CHK(x,y) (x & (1 << y))                 //  |
#define _TOG(x,y) (x ^= (1 << y))                //--+ 

//-------------------------------------------------------------------------------------------------
// Global constants

// Rotary encoder pins.
static const uint8_t ENCODER_CLK_PIN   =  2;     // INT0
static const uint8_t ENCODER_DT_PIN    =  4;
static const uint8_t ENCODER_BTN_PIN   =  5;

// LEDs
static const uint8_t RED_LED_PIN       =  7;
static const uint8_t GREEN_LED_PIN     =  8;

// Tap tempo button
static const uint8_t TAPTEMPO_BTN_PIN  =  9;

// PWM Audio output
static const uint8_t PWM_AUDIO_PIN     =  11;

//-------------------------------------------------------------------------------------------------
// Global objects

// Waveforms:
volatile unsigned int sample_offset__  = 0;
volatile unsigned int waveforms[2];
volatile unsigned int waveform_lengths[2];
volatile int          active_waveform;

// Display:
// Connect to LCD via I2C, default address 0x27 (A0-A2 not jumpered).
LiquidCrystal_I2C lcd (0x27, 16, 2);
DeJitter dejitter (&lcd);

// LED that flashes on every beat.
Led led_red(RED_LED_PIN);
Led led_green(GREEN_LED_PIN);

// Rotary encoder:
// Wiring: CLK pin connected to INT0 pin with falling edge detection.
// Hardware debouncing RC filter applied on CLK pin.
// Assuming that the data pin has enough time to settle, no debouncing is needed.
// The encoder button is software-debounced.
Button encoder_btn (ENCODER_BTN_PIN);
volatile int encoder_moved__ = 0;

// Tap tempo button
Button taptempo_btn (TAPTEMPO_BTN_PIN);
uint32_t taptempo_last__ = 0;

// Statemachine to react on user input.
Statemachine fsm = Statemachine();

enum class edit_mode {
    standby = 0
  , tempo
  , metre
}   edit_mode__;


// The last time measured
uint32_t tick__ = 0;

// Beats per minute.
unsigned bpm__ = 104;

// Metre n/4, where n can be of [1,2,3,4,5,6,7]
int metre__ = 1;
int beat__  = 1;

// Duration of one beat in us.
uint32_t cycle__ = 0;

// Elapsed time during cylce.
uint32_t elapsed__ = 0;

// Points to a term describing the tempo choosen for display.
const char* tempo_name__ = nullptr;

// Associates a tempo name with its bpm lower boundary
struct tempo
{
    const char* const name;
    int value;
};

// Array of 16 tempo names to display.
const tempo tempi__[17] = 
{
    { "Grave      ",  40 }
  , { "Largo      ",  44 }
  , { "Lento      ",  48 }
  , { "Adagio     ",  54 }
  , { "Larghetto  ",  58 }
  , { "Adagietto  ",  64 }
  , { "Andante    ",  70 }
  , { "Andantino  ",  76 }
  , { "Maestoso   ",  84 }
  , { "Moderato   ",  92 }
  , { "Alegretto  ", 104 }
  , { "Animato    ", 116 }
  , { "Allegro    ", 126 }
  , { "Assai      ", 138 }
  , { "Vivace     ", 152 }
  , { "Presto     ", 176 }
  , { "Prestissimo", 202 }
};

// Flicker free display line buffers.
static char line_buf__[16];

//-------------------------------------------------------------------------------------------------
// Global functions

// Suspend audio interrupt
void suspend_audio()
{
    _CLR(TIMSK1, OCIE1A);
}

// Resume audio interrupt
void resume_audio()
{
    _SET(TIMSK1, OCIE1A);
}

// Plays a strong pulse at 2kHz for 50ms  (the "TICK" sound)
void play_pulse_2000()
{
    sample_offset__ = 0;
    active_waveform = 0;
}

// Plays a strong pulse at 1kHz for 50ms  (the "TACK" sound)
void play_pulse_1000()
{
    sample_offset__ = 0;
    active_waveform = 1;
}

/**
 * @brief TIMER 1 match compare ISR
 * Called every 1/8000 s (0.125 ms)
 */
SIGNAL(TIMER1_COMPA_vect)
{
    register int16_t sample = 0;
    if (sample_offset__ < waveform_lengths[active_waveform])
    {
        sample = pgm_read_byte(waveforms[active_waveform] + sample_offset__);
        sample += 128;
        OCR2A = OCR2B = static_cast<uint8_t>(sample);
        if (++sample_offset__ >= waveform_lengths[active_waveform])
        {
            OCR2A = OCR2B = 128;
        }
    }
}

// Fill display line buffer with blanks.
void clear_line_buffer()
{
    memset(line_buf__, ' ', 16);
}

// Translates beats per minute to tempo name.
const char* const get_tempo_name(unsigned bpm)
{
    for (int i = 16; i >= 0; --i) 
    {
        if (bpm >= tempi__[i].value) {
            return tempi__[i].name;
        }
    }
    return "-          ";
}

int find_tempo(unsigned bpm)
{
    for (int i = 16; i >= 0; --i) {
        if (bpm >= tempi__[i].value) {
            return i;
        }
    }
    return 0;
}

void advance_tempo(int steps)
{
    auto index = find_tempo(bpm__);
    if (steps == -1 && tempi__[index].value < bpm__) 
    {
        bpm__ = tempi__[index].value;
    }
    else
    {
        if (index == 16 && steps == 1)
        {
            bpm__ = (bpm__ / 5) * 5;
            bpm__ += (steps * 5);
        }
        else 
        {
            index += steps;
            index = max(index, 0);
            index = min(index, 16);
            bpm__ = tempi__[index].value;
        }
    }
}

void render_bpm()
{
    lcd.setCursor(2, 0);
    lcd.print("   ");
    lcd.setCursor(2, 0);
    lcd.print(bpm__);

    lcd.setCursor(0, 1);
    auto wrk = get_tempo_name(bpm__);
    if (wrk != tempo_name__)
    {
        tempo_name__ = wrk;
        lcd.setCursor(0, 1);
        lcd.print(tempo_name__);
    }
}

// Measure elapsed time in microseconds since the last tick.
void tick()
{
    uint32_t now = micros();
    elapsed__ += (now - tick__);
    tick__ = now;
}

void render_metre()
{
    lcd.setCursor(0, 1);
    lcd.print("    ");
    lcd.setCursor(0, 1);
    lcd.print(metre__);
    lcd.print("/4");
}

void render_beat()
{
    lcd.setCursor(6, 0);
    lcd.print(beat__);
}



// Used for debugging only
void render_offset(uint32_t offset)
{
    lcd.setCursor(10, 0);
    lcd.print("        ");
    lcd.setCursor(10, 0);
    lcd.print(offset);
}

// Detect movement on the rotary encoder
int read_encoder()
{
    int value = 0;
    if (encoder_moved__)
    {
        value += encoder_moved__;
        encoder_moved__ &= 0x0;
    }
    return value;
}

// Interrupt service routine for the rotary encoder.
// The interrupt is triggered by the falling edge of the CLK signal.
void isr_encoder()
{
    encoder_moved__ += (digitalRead(ENCODER_DT_PIN) == HIGH ? 1 : -1);
}

// LiquidCrystal_I2C::clear() takes too long. This is should be faster.
void lcd_clear()
{
    static const char* const BLANK_LINE = "                ";
    lcd.setCursor(0, 0);
    lcd.print(BLANK_LINE);
    lcd.setCursor(0, 1);
    lcd.print(BLANK_LINE);
}

void enter_standby_mode()
{
    lcd.noBacklight();
    lcd.noDisplay();

    edit_mode__ = edit_mode::standby;
}

void enter_tempo_edit_mode()
{
    lcd_clear();
    lcd.setCursor(0, 0); lcd.print("\x05=");
    tempo_name__ = nullptr;
    render_bpm();
    render_beat();

    // Print metre
    lcd.setCursor(7, 0);
    lcd.write('/');
    lcd.setCursor(8, 0);
    lcd.print(metre__);

    edit_mode__ = edit_mode::tempo;
}

void enter_metre_edit_mode()
{
    lcd_clear();
    lcd.setCursor(0, 0);
    lcd.print("Metrum w\x06hlen");
    render_metre();

    edit_mode__ = edit_mode::metre;
}

// Cycles through edit modes.
void cycle_edit_mode()
{
    switch (edit_mode__) 
    {
    case edit_mode::standby:
        return wakeup();

    case edit_mode::tempo:
        return enter_metre_edit_mode();

    case edit_mode::metre:
        return enter_tempo_edit_mode();
    };
}

void save_preset()
{
    // TODO: implement
}

void wakeup()
{
    edit_mode__ = edit_mode::tempo;
    lcd.display();
    lcd.backlight();
    enter_tempo_edit_mode();
}

void update_bpm()
{
    bpm__ = max( 40, bpm__);
    bpm__ = min(240, bpm__);
    cycle__ = ((60 * 1000000) / bpm__);
    render_bpm();
}

//-------------------------------------------------------------------------------------------------
// Custom setup functions

/**
 * @brief Set TIMER1 interrupt to 8kHz.
 * ISR copies one sample every 1/8000 seconds to the PWM OCR.
 */
void setup_timer()
{
    cli();                                       // disable interrupts
    TCCR1A = 0x0;                                // clear TCCR1A register
    TCCR1B = 0x0;                                // clear TCCR1B register
    TCNT1  = 0x0;                                // initialize counter value
    OCR1A = 1999;                                // 16000000 / (1 * 8000) - 1
    _SET(TCCR1B, WGM12);                         // turn on CTC mode
    _SET(TCCR1B, CS10);                          // no prescaling
    _CLR(TIMSK1, OCIE1A);                        // disable audio interrupt
    sei();                                       // allow interrupts
}

/**
 * @brief Enable Fast PWM on TIMER2
 * Sets 8-bit fast PWM on TIMER2 at 16000000/256 = 62.5 kHz
 */
void setup_pwm()
{
    TCCR2A = 0x0;                                // clear TCCR2A register
    TCCR2B = 0x0;                                // clear TCCR2B register
    _SET(TCCR2A, WGM21);                         // Fast PWM non-inverting mode
    _SET(TCCR2A, WGM20);                         // |
    _SET(TCCR2A, COM2A1);                        //-+
    _SET(TCCR2B, CS20);                          // no prescaling
    OCR2A = OCR2B = 128;                         // initial DC level ~2.5v
    _SET(DDRB, 3);                               // PWM pin (11)
}

//-------------------------------------------------------------------------------------------------
// Standard functions

void setup() 
{
    // Waveforms
    waveforms[0] = reinterpret_cast<unsigned int>(PULSE_2000);
    waveforms[1] = reinterpret_cast<unsigned int>(PULSE_1000);
    waveform_lengths[0] = PULSE_2000_LENGTH;
    waveform_lengths[1] = PULSE_1000_LENGTH;
    active_waveform     = 0;
    sample_offset__     = UINT_MAX;

    // Audio output PWM and audio sample ISR.
    setup_pwm();
    setup_timer();
    suspend_audio();

    beat__ = 1;
    metre__ = 1;

    // TODO: read last bpm choosen from EEPROM.
    // TODO: read last metre from EEPROM.
    
    bpm__ = 104;
    tempo_name__ = nullptr;

    // Initialize display.
    lcd.init();
    lcd.createChar(5, note_character);
    lcd.createChar(6, a_uml);
    lcd.backlight();

    enter_tempo_edit_mode();

    // Initialize LED.
    led_red.off();
    led_green.off();

    // Setup rotary encoder.
    pinMode(ENCODER_CLK_PIN, INPUT_PULLUP);
    pinMode(ENCODER_DT_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK_PIN), isr_encoder, FALLING);

    memset(line_buf__, ' ', 16);

#ifdef DEBUG_SERIAL
    Serial.begin(9600);
    Serial.println("Running ...");
#endif

    tick__ = micros();
    cycle__ = 0;
    elapsed__ = 0;

    resume_audio();
}

void loop() 
{
    // Measure time elapsed since last tick
    tick();

    // Update LEDs.
    led_red.update();
    led_green.update();

    // Detect beat
    if (elapsed__ >= cycle__)
    {
        uint32_t elapsed = elapsed__;
        uint32_t offset = (elapsed__ - cycle__);
        cycle__ = ((60 * 1000000) / bpm__);
        cycle__ -= offset;
        elapsed__ = 0;

#ifdef DEBUG_SERIAL
        Serial.print(beat__);
        Serial.print("\t");
        Serial.print(elapsed);
        Serial.print("\t");
        Serial.print(cycle__);
        Serial.print("\t");
        Serial.println(offset);
#endif

        if (beat__ == 1)
        {
            play_pulse_2000();
            led_red.flash(50);
        }
        else 
        {
            play_pulse_1000();
            led_green.flash(50);
        }

        if (edit_mode__ == edit_mode::tempo) {
            render_beat();
        }

        if (++beat__ > metre__) {
            beat__ = 1;
        }
    }

    // Read rorary encoder.
    encoder_btn.update();
    int encoder_moved = read_encoder();

    // Update the state machine with button and encoder changes
    fsm.update(encoder_btn.status(), encoder_moved, tick__);
    auto event = fsm.poll();

#ifdef DEBUG_SERIAL
    if (event > EV_INACTIVE) {
        static char buffer[128];
        sprintf(buffer, "Got event #%d %d\n", event, encoder_moved);
        Serial.print(buffer);
    }
#endif

    // Evaluate event produced by state machine upon user interaction
    switch (event) 
    {
    case EV_BTN_PRESSED:
        return cycle_edit_mode();

    case EV_BTN_PRESSED_ALT:
        return save_preset();

    case EV_INACTIVE: if (edit_mode__ != edit_mode::standby) 
        return enter_standby_mode();
    }

    // Apply encoder movement.
    switch (edit_mode__)
    {
    case edit_mode::standby: 
        if (event == EV_ENCODER_MOVED)
        {
            return wakeup();
        }

    case edit_mode::tempo:
        if (event == EV_ENCODER_MOVED)
        {
            advance_tempo(encoder_moved);
            update_bpm();
        }
        else if (event == EV_ENCODER_MOVED_ALT)
        {
            bpm__ += encoder_moved;
            update_bpm();
        }
        break;
    
    case edit_mode::metre:
        if (event == EV_ENCODER_MOVED)
        {
            metre__ += encoder_moved;
            metre__ = max(1, metre__);
            metre__ = min(7, metre__);
            render_metre();
        }
        break;
    }
}
