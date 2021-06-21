
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
#include "RotaryEncoder.hpp"
#include "TapTempo.hpp"
#include "samples.hpp"

#undef DEBUG_SERIAL

//-------------------------------------------------------------------------------------------------
// Global constants

// Tap tempo button
constexpr uint8_t TAPTEMPO_BTN_PIN  =   2;     // INT1

// Rotary encoder pins.
constexpr uint8_t ENCODER_CLK_PIN   =   3;     // INT2
constexpr uint8_t ENCODER_DTA_PIN   =   4;
constexpr uint8_t ENCODER_BTN_PIN   =   5;

// LEDs
constexpr uint8_t RED_LED_PIN       =  A2;
constexpr uint8_t GREEN_LED_PIN     =  A3;

// PWM Audio output
constexpr uint8_t PWM_AUDIO_PIN     =  11;

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
RotaryEncoder encoder(ENCODER_CLK_PIN, ENCODER_DTA_PIN);
Button encoder_btn (ENCODER_BTN_PIN);

// Tap tempo button
TapTempo taptempo (TAPTEMPO_BTN_PIN);

// Statemachine to react on user input.
Statemachine fsm = Statemachine();

enum class edit_mode {
    low_power = 0
  , tempo
  , metre
}   edit_mode__;


// The last time measured
uint32_t tick__ = 0;

// Beats per minute.
uint32_t bpm__ = 104;

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
    TIMSK1 &= (~(1 << OCIE1A));
}

// Resume audio interrupt
void resume_audio()
{
    TIMSK1 |= (1 << OCIE1A);
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
    if (edit_mode__ == edit_mode::tempo) 
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
    if (edit_mode__ == edit_mode::tempo)
    {
        lcd.setCursor(6, 0);
        lcd.print(beat__);
    }
}

// Used for debugging only
void render_offset(uint32_t offset)
{
    lcd.setCursor(10, 0);
    lcd.print("        ");
    lcd.setCursor(10, 0);
    lcd.print(offset);
}

// Interrupt service routine for the rotary encoder CLK signal.
void isr_encoder()
{
    encoder.on_clk_change();
}

// Interrupt service routine for the rotary encoder CLK signal.
void isr_taptempo()
{
    taptempo.trigger();
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

void enter_low_power_mode()
{
    edit_mode__ = edit_mode::low_power;

    lcd.noBacklight();
    lcd.noDisplay();
}

void enter_tempo_edit_mode()
{
    edit_mode__ = edit_mode::tempo;

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
}

void enter_metre_edit_mode()
{
    edit_mode__ = edit_mode::metre;

    lcd_clear();
    lcd.setCursor(0, 0);
    lcd.print("Metrum w\x06hlen");
    render_metre();
}

// Cycles through edit modes.
void cycle_edit_mode()
{
    switch (edit_mode__) 
    {
    case edit_mode::low_power:
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
    TCCR1A  =  0x0;                              // clear TCCR1A register
    TCCR1B  =  0x0;                              // clear TCCR1B register
    TCNT1   =  0x0;                              // initialize counter value
    OCR1A   = 1999;                              // 16000000 / (1 * 8000) - 1
    TCCR1B |= (1 << WGM12);                      // turn on CTC mode
    TCCR1B |= (1 << CS10);                       // no prescaling
    TIMSK1 &= (~(1 << TIMSK1));                  // disable audio interrupt
    sei();                                       // allow interrupts
}

/**
 * @brief Enable Fast PWM on TIMER2
 * Sets 8-bit fast PWM on TIMER2 at 16000000/256 = 62.5 kHz
 */
void setup_pwm()
{
    TCCR2A  = 0x0;                               // clear TCCR2A register
    TCCR2B  = 0x0;                               // clear TCCR2B register
    TCCR2A |= (1 << WGM21);                      // Fast PWM non-inverting mode
    TCCR2A |= (1 << WGM20);                      // |
    TCCR2A |= (1 << COM2A1);                     //-+
    TCCR2B |= (1 << CS20);                       // no prescaling
    OCR2A   = 128;                               // initial DC level ~2.5v
    OCR2B   = 128;                               //-+
    DDRB    = 0x0;                               // clear DDRB register
    DDRB   |= (1 << 3);                          // PWM pin (11)
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

    // Rotary encoder CLK interrupt
    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK_PIN), isr_encoder, CHANGE);

    // Tap-tempo button interrupt
    attachInterrupt(digitalPinToInterrupt(TAPTEMPO_BTN_PIN), isr_taptempo, RISING);

    memset(line_buf__, ' ', 16);

#ifdef DEBUG_SERIAL
    Serial.begin(115200);
    Serial.println("Running ...");
#endif

    tick__ = micros();
    cycle__ = 0;
    elapsed__ = 0;

    resume_audio();
    encoder.reset();
    taptempo.reset();
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
        // Serial.print(beat__);
        // Serial.print("\t");
        // Serial.print(elapsed);
        // Serial.print("\t");
        // Serial.print(cycle__);
        // Serial.print("\t");
        // Serial.println(offset);
#endif

        if (metre__ > 1 && beat__ == 1)
        {
            play_pulse_2000();
            led_green.flash(50);
            led_red.flash(50);
        }
        else 
        {
            play_pulse_1000();
            led_red.flash(50);
        }

        render_beat();

        if (++beat__ > metre__) {
            beat__ = 1;
        }
    }

    // Read tap tempo
    taptempo.update();
    auto tapped_bpm = taptempo.bpm();

    if (tapped_bpm > 0 && tapped_bpm != bpm__)
    {
#ifdef DEBUG_SERIAL
        Serial.print("got tapped bpm:");
        Serial.println(tapped_bpm);
#endif
        bpm__ = tapped_bpm;
        if (edit_mode__ == edit_mode::low_power) {
            wakeup();
        }
        else if (edit_mode__ == edit_mode::metre) {
            enter_tempo_edit_mode();
        }
        update_bpm();
    }

    // Read rotrary encoder.
    encoder_btn.update();
    int encoder_moved = encoder.moved();

    // Update the state machine with button and encoder changes
    fsm.update(encoder_btn.status(), encoder_moved, tapped_bpm, tick__);
    auto event = fsm.event();

#ifdef DEBUG_SERIAL
    if (event > EV_INACTIVE) {
        static char buffer[128];
        sprintf(buffer, "got event #%d %d\n", event, encoder_moved);
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

    case EV_INACTIVE: if (edit_mode__ != edit_mode::low_power)
        return enter_low_power_mode();
    }

    // Apply encoder movement.
    switch (edit_mode__)
    {
    case edit_mode::low_power: 
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
