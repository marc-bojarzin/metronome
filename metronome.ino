
#include <Arduino.h>
#include <limits.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "Led.hpp"
#include "Button.hpp"
#include "Statemachine.hpp"
#include "CustomChars.hpp"
#include "DeJitter.hpp"

//-------------------------------------------------------------------------------------------------
// Global constants


// Rotary encoder pins.
static const uint8_t ENCODER_CLK_PIN =  2;  // INT0
static const uint8_t ENCODER_DT_PIN  =  4;
static const uint8_t ENCODER_BTN_PIN =  5;

// Select button.
static const uint8_t SELECT_BTN_PIN  =  6;

// LEDs
static const uint8_t RED_LED_PIN     =  7;
static const uint8_t GREEN_LED_PIN   =  8;

// Speaker
static const uint8_t SPEAKER_PIN     =  9;

// Standby
static const uint32_t STANDBY = (15ul * 1000ul * 1000ul);  // 15 seconds

// Clear display line
static const char* BLANK_LINE = "                ";

//-------------------------------------------------------------------------------------------------
// Global objects

uint32_t inactive_since__ = micros();

// Display:
// Connect to LCD via I2C, default address 0x27 (A0-A2 not jumpered).
LiquidCrystal_I2C lcd (0x27, 16, 2);
DeJitter dsply (&lcd);

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

// Select button.
Button select_btn (SELECT_BTN_PIN);
Statemachine fsm = Statemachine();

enum {
    mode_standby = 0
  , mode_tempo
  , mode_metre
  , mode_volume
}   edit_mode__;


// The last time measured
unsigned long tick__ = 0;

// Beats per minute.
unsigned bpm__ = 104;

// Metre n/4, where n can be of [1,2,3,4,5,6,7]
int metre__ = 1;
int beat__  = 1;

// Duration of one beat in us.
uint32_t cycle__ = ((60 * 1000000) / bpm__);

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

// Volume.
int volume__;

// Flicker free display line buffers.
static char line_buf__[16];

//-------------------------------------------------------------------------------------------------
// Global functions


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
            bpm__ = max( 40, bpm__);
            bpm__ = min(240, bpm__);
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
void tock()
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

void render_volume_old()
{
    // Use the 13 rightmost characters to render the horizontal volume bar.
    // With every character having 5 colums, we have 65 pixels for the bar.
    static const double bars_per_unit = (65.0 / 100.0);

    // Use custom characters stored in the LCD character map at this offset.
    static const uint8_t offset = 0;

    lcd.setCursor(0, 1);
    lcd.print(BLANK_LINE);

    lcd.setCursor(0, 1);
    if (volume__ < 10) {
        lcd.print(' ');
    }
    lcd.print(volume__);

    lcd.setCursor(3, 1);
    double volume = volume__;
    int bars = floor(volume * bars_per_unit);
    bars += 1;

    // Render 'full' bars
    int quot = bars / 5;
    for (int i = 0; i < quot; ++i) {
        lcd.write(4);
    }

    // Render 'partial' bars
    int mod = bars % 5;
    if (mod) {
        uint8_t index = offset + (mod - 1);
        lcd.write(index);
    }
}

void render_volume()
{
    // Use the 13 rightmost characters to render the horizontal volume bar.
    // With every character having 5 colums, we have 65 pixels for the bar.
    static const double bars_per_unit = (65.0 / 100.0);

    // Use custom characters stored in the LCD character map at this offset.
    static const uint8_t offset = 0;

    clear_line_buffer();
    sprintf(line_buf__, "%2d ", volume__);
    int col = 3;

    double volume = volume__;
    int bars = floor(volume * bars_per_unit);
    bars += 1;

    // Render 'full' bars
    int quot = bars / 5;
    for (int i = 0; i < quot; ++i) {
        line_buf__[col++] = 4;
    }

    // Render 'partial' bars
    int modulus = bars % 5;
    if (modulus) {
        line_buf__[col++] = offset + (modulus - 1);
    }

    //Serial.println(line_buf__);

    dsply.update(1, line_buf__);
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
        noInterrupts();
        value += encoder_moved__;
        encoder_moved__ &= 0x0;
        interrupts();
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
    lcd.setCursor(0, 0);
    lcd.print(BLANK_LINE);
    lcd.setCursor(0, 1);
    lcd.print(BLANK_LINE);
}

void enter_standby_mode()
{
    lcd.noBacklight();
    lcd.noDisplay();
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
}

void enter_metre_edit_mode()
{
    lcd_clear();
    lcd.setCursor(0, 0);
    lcd.print("Metrum w\x06hlen");
    render_metre();
}

void enter_volume_edit_mode()
{
    lcd_clear();
    lcd.setCursor(0, 0);
    lcd.print("Lautst\x06rke");
    dsply.clear();
    render_volume();
}

// Cycles through edit modes.
void cycle_edit_mode()
{
    switch (edit_mode__) 
    {
    case mode_standby:
        return wakeup();

    case mode_tempo:
        edit_mode__ = mode_metre;
        return enter_metre_edit_mode();

    case mode_metre:
        edit_mode__ = mode_volume;
        return enter_volume_edit_mode();
    
    case mode_volume:
        edit_mode__ = mode_tempo;
        return enter_tempo_edit_mode();
    };
}

void save_preset()
{
    // TODO: implement
}

void wakeup()
{
    edit_mode__ = mode_tempo;
    lcd.display();
    lcd.backlight();
    enter_tempo_edit_mode();
}

//-------------------------------------------------------------------------------------------------
// Standard functions

void setup() 
{
    // Pull audio pin to ground.
    pinMode(SPEAKER_PIN, OUTPUT);
    digitalWrite(SPEAKER_PIN, LOW);

    edit_mode__ = mode_tempo;
    tick__ = micros();
    elapsed__ = 0;

    // TODO: read last bpm choosen from EEPROM.
    // TODO: read last metre from EEPROM.
    // TODO: read last volume setting from EEPROM.

    bpm__ = 104;
    tempo_name__ = nullptr;
    volume__ = 50;

    // Initialize display.
    lcd.init();
    lcd.createChar(0, bar_1);
    lcd.createChar(1, bar_2);
    lcd.createChar(2, bar_3);
    lcd.createChar(3, bar_4);
    lcd.createChar(4, bar_5);
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

    volume__ = 50;

    memset(line_buf__, ' ', 16);

    Serial.begin(9600);
    Serial.println("Running ...");
}

void loop() 
{
    // Measure time elapsed since last tick
    tock();

    // Update LEDs.
    led_red.update();
    led_green.update();

    // Read rorary encoder.
    encoder_btn.update();
    int encoder_moved = read_encoder();

    // Detect beat
    if (elapsed__ >= cycle__)
    {
        uint32_t offset = (elapsed__ - cycle__);
        cycle__ = ((60 * 1000000) / bpm__);
        cycle__ -= offset;
        elapsed__ = 0;

        if (beat__ == 1)
        {
            tone(SPEAKER_PIN, 1000, 20);
            led_red.flash(50);
        }
        else 
        {
            tone(SPEAKER_PIN, 500, 20);
            led_green.flash(50);
        }

        if (edit_mode__ == mode_tempo) {
            render_beat();
        }

        if (++beat__ > metre__) {
            beat__ = 1;
        }
    }

    // Evaluate SELECT button
    select_btn.update();
    fsm.update(select_btn.status());
    switch (fsm.poll()) 
    {
    case EV_BTN_PRESSED_SHORT:
        cycle_edit_mode();
        inactive_since__ = tick__;
        break;

    case EV_BTN_PRESSED_LONG:
        save_preset();
        inactive_since__ = tick__;
        break;

    case EV_NOEVENT:
    default:
        break;
    }

    // Wakeup from standby?
    if (edit_mode__ == mode_standby && encoder_moved)
    {
        wakeup();
    }

    // Take actions according to edit mode choosen
    switch (edit_mode__)
    {
    case mode_standby:
        break;

    case mode_tempo:
        if (encoder_moved)
        {
            if (encoder_btn.pressed())
            {
                bpm__ += encoder_moved;
                bpm__ = max( 40, bpm__);
                bpm__ = min(240, bpm__);
            }
            else
            {
                advance_tempo(encoder_moved);
            }
            cycle__ = ((60 * 1000000) / bpm__);
            render_bpm();
            inactive_since__ = tick__;
        }
        break;
    
    case mode_metre:
        if (encoder_moved)
        {
            metre__ += encoder_moved;
            metre__ = max(1, metre__);
            metre__ = min(7, metre__);
            render_metre();
            inactive_since__ = tick__;
        }
        break;

    case mode_volume:
        if (encoder_moved)
        {
            if (encoder_btn.pressed()) {
                volume__ += encoder_moved;
            }
            else {
                volume__ = (volume__ / 5) * 5;
                volume__ += (encoder_moved * 5);
            }
            volume__ = max(0, volume__);
            volume__ = min(99, volume__);
            render_volume();
            inactive_since__ = tick__;
        }
        break;
    }

    // Enter standby mode?
    if (edit_mode__ != mode_standby && (tick__ - inactive_since__ > STANDBY))
    {
        edit_mode__ = mode_standby;
        enter_standby_mode();
    }
}
