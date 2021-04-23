
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

// LEDs
static const uint8_t RED_LED_PIN     =  7;
static const uint8_t GREEN_LED_PIN   =  8;

// Speaker
static const uint8_t SPEAKER_PIN     =  9;

//-------------------------------------------------------------------------------------------------
// Global objects

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

// Statemachine to react on user input.
Statemachine fsm = Statemachine();

enum class edit_mode {
    standby = 0
  , tempo
  , metre
  , volume
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

    // Update LCD display
    dejitter.update(1, line_buf__);
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

void enter_volume_edit_mode()
{
    lcd_clear();
    lcd.setCursor(0, 0);
    lcd.print("Lautst\x06rke");
    dejitter.clear();
    render_volume();

    edit_mode__ = edit_mode::volume;
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
        return enter_volume_edit_mode();
    
    case edit_mode::volume:
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

void update_volume()
{
    volume__ = max(0, volume__);
    volume__ = min(99, volume__);
    render_volume();
}

//-------------------------------------------------------------------------------------------------
// Standard functions

void setup() 
{
    // Pull audio pin to ground.
    pinMode(SPEAKER_PIN, OUTPUT);
    digitalWrite(SPEAKER_PIN, LOW);

    beat__ = 1;
    metre__ = 1;

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

    memset(line_buf__, ' ', 16);

    Serial.begin(38400);
    Serial.println("Running ...");

    tick__ = micros();
    cycle__ = 0;
    elapsed__ = 0;
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

        Serial.print(beat__);
        Serial.print("\t");
        Serial.print(elapsed);
        Serial.print("\t");
        Serial.print(cycle__);
        Serial.print("\t");
        Serial.println(offset);

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

    if (event > EV_INACTIVE) {
        static char buffer[128];
        sprintf(buffer, "Got event #%d %d\n", event, encoder_moved);
        Serial.print(buffer);
    }

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
    case edit_mode::standby: if (event == EV_ENCODER_MOVED)
        return wakeup();

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

    case edit_mode::volume:
        if (event == EV_ENCODER_MOVED)
        {
            volume__ = (volume__ / 5) * 5;
            volume__ += (encoder_moved * 5);
            update_volume();
        }
        else if (event == EV_ENCODER_MOVED_ALT)
        {
            volume__ += encoder_moved;
            update_volume();
        }
        break;
    }
}
