/**
 * ESP-32 IDF library for control TM1637 LED 7-Segment display
 *
 * Author: Petro <petro@petro.ws>
 *
 * Project homepage: https://github.com/petrows/esp-32-tm1637
 * Example: https://github.com/petrows/esp-32-tm1637-example
 *
 */

#include "tm1637.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#if CONFIG_IDF_TARGET_ESP32
#include <esp32/rom/ets_sys.h>
#elif CONFIG_IDF_TARGET_ESP32S2
#include <esp32s2/rom/ets_sys.h>
#elif CONFIG_IDF_TARGET_ESP32S3
#include <esp32s3/rom/ets_sys.h>
#elif CONFIG_IDF_TARGET_ESP32C3
#include <esp32c3/rom/ets_sys.h>
#else
#error "Unsupported ESP chip"
#endif


#define TM1637_ADDR_AUTO  0x40
#define TM1637_ADDR_FIXED 0x44

#define MINUS_SIGN_IDX  16

static const int8_t tm1637_symbols[] = {
                // XGFEDCBA
        0x3f, // 0b00111111,    // 0
        0x06, // 0b00000110,    // 1
        0x5b, // 0b01011011,    // 2
        0x4f, // 0b01001111,    // 3
        0x66, // 0b01100110,    // 4
        0x6d, // 0b01101101,    // 5
        0x7d, // 0b01111101,    // 6
        0x07, // 0b00000111,    // 7
        0x7f, // 0b01111111,    // 8
        0x6f, // 0b01101111,    // 9
        0x77, // 0b01110111,    // A
        0x7c, // 0b01111100,    // b
        0x39, // 0b00111001,    // C
        0x5e, // 0b01011110,    // d
        0x79, // 0b01111001,    // E
        0x71, // 0b01110001     // F
        0x40, // 0b01000000     // minus sign
};

static void tm1637_start(tm1637_led_t * led);
static void tm1637_stop(tm1637_led_t * led);
static void tm1637_send_byte(tm1637_led_t * led, uint8_t byte);
static void tm1637_delay();

static inline float nearestf(float val,int precision) {
    int scale = pow(10,precision);
    return roundf(val * scale) / scale;
}

void tm1637_start(tm1637_led_t * led)
{
    // Send start signal
    // Both outputs are expected to be HIGH beforehand
    gpio_set_level(led->m_pin_dta, 0);
    tm1637_delay();
}

void tm1637_stop(tm1637_led_t * led)
{
    // Send stop signal
    // CLK is expected to be LOW beforehand
    gpio_set_level(led->m_pin_dta, 0);
    tm1637_delay();
    gpio_set_level(led->m_pin_clk, 1);
    tm1637_delay();
    gpio_set_level(led->m_pin_dta, 1);
    tm1637_delay();
}

void tm1637_send_byte(tm1637_led_t * led, uint8_t byte)
{
    for (uint8_t i=0; i<8; ++i)
    {
        gpio_set_level(led->m_pin_clk, 0);
        tm1637_delay();
        gpio_set_level(led->m_pin_dta, byte & 0x01); // Send current bit
        byte >>= 1;
        tm1637_delay();
        gpio_set_level(led->m_pin_clk, 1);
        tm1637_delay();
    }

    // The TM1637 signals an ACK by pulling DIO low from the falling edge of
    // CLK after sending the 8th bit, to the next falling edge of CLK.
    // DIO needs to be set as input during this time to avoid having both
    // chips trying to drive DIO at the same time.
    gpio_set_direction(led->m_pin_dta, GPIO_MODE_INPUT);
    gpio_set_level(led->m_pin_clk, 0); // TM1637 starts ACK (pulls DIO low)
    tm1637_delay();
    gpio_set_level(led->m_pin_clk, 1);
    tm1637_delay();
    gpio_set_level(led->m_pin_clk, 0); // TM1637 ends ACK (releasing DIO)
    tm1637_delay();
    gpio_set_direction(led->m_pin_dta, GPIO_MODE_OUTPUT);
}

void tm1637_delay()
{
    ets_delay_us(CONFIG_DELAY_BLOCKING_TIME);
}

// PUBLIC PART:

tm1637_led_t * tm1637_init(gpio_num_t pin_clk, gpio_num_t pin_data) {
    tm1637_led_t * led = (tm1637_led_t *) malloc(sizeof(tm1637_led_t));
    led->m_pin_clk = pin_clk;
    led->m_pin_dta = pin_data;
    led->m_brightness = CONFIG_TM1637_BRIGHTNESS;
    // Set CLK to low during DIO initialization to avoid sending a start signal by mistake
    gpio_set_direction(pin_clk, GPIO_MODE_OUTPUT);
    gpio_set_level(pin_clk, 0);
    tm1637_delay();
    gpio_set_direction(pin_data, GPIO_MODE_OUTPUT);
    gpio_set_level(pin_data, 1);
    tm1637_delay();
    gpio_set_level(pin_clk, 1);
    tm1637_delay();
    return led;
}

void tm1637_set_brightness(tm1637_led_t * led, uint8_t level)
{
    if (level > 0x07) { level = 0x07; } // Check max level
    led->m_brightness = level;
}

void tm1637_set_segment_number(tm1637_led_t * led, const uint8_t segment_idx, const uint8_t num, const bool dot)
{
    uint8_t seg_data = 0x00;

    if (num < (sizeof(tm1637_symbols)/sizeof(tm1637_symbols[0]))) {
        seg_data = tm1637_symbols[num]; // Select proper segment image
    }

    if (dot) {
        seg_data |= 0x80; // Set DOT segment flag
    }

    tm1637_set_segment_raw(led, segment_idx, seg_data);
}

void tm1637_set_segment_raw(tm1637_led_t * led, const uint8_t segment_idx, const uint8_t data)
{
    tm1637_start(led);
    tm1637_send_byte(led, TM1637_ADDR_FIXED);
    tm1637_stop(led);
    tm1637_start(led);
    tm1637_send_byte(led, segment_idx | 0xc0);
    tm1637_send_byte(led, data);
    tm1637_stop(led);
    tm1637_start(led);
    tm1637_send_byte(led, led->m_brightness | 0x88);
    tm1637_stop(led);
}

void tm1637_set_number(tm1637_led_t * led, uint16_t number)
{
    tm1637_set_number_lead_dot(led, number, false, 0x00);
}

void tm1637_set_number_lead(tm1637_led_t * led, uint16_t number, const bool lead_zero)
{
    tm1637_set_number_lead_dot(led, number, lead_zero, 0x00);
}

void tm1637_set_number_lead_dot(tm1637_led_t * led, uint16_t number, bool lead_zero, const uint8_t dot_mask)
{
    uint8_t lead_number = lead_zero ? 0xFF : tm1637_symbols[0];

    if (number < 10) {
        tm1637_set_segment_number(led, 3, number, dot_mask & 0x01);
        tm1637_set_segment_number(led, 2, lead_number, dot_mask & 0x02);
        tm1637_set_segment_number(led, 1, lead_number, dot_mask & 0x04);
        tm1637_set_segment_number(led, 0, lead_number, dot_mask & 0x08);
    } else if (number < 100) {
        tm1637_set_segment_number(led, 3, number % 10, dot_mask & 0x01);
        tm1637_set_segment_number(led, 2, (number / 10) % 10, dot_mask & 0x02);
        tm1637_set_segment_number(led, 1, lead_number, dot_mask & 0x04);
        tm1637_set_segment_number(led, 0, lead_number, dot_mask & 0x08);
    } else if (number < 1000) {
        tm1637_set_segment_number(led, 3, number % 10, dot_mask & 0x01);
        tm1637_set_segment_number(led, 2, (number / 10) % 10, dot_mask & 0x02);
        tm1637_set_segment_number(led, 1, (number / 100) % 10, dot_mask & 0x04);
        tm1637_set_segment_number(led, 0, lead_number, dot_mask & 0x08);
    } else {
        tm1637_set_segment_number(led, 3, number % 10, dot_mask & 0x01);
        tm1637_set_segment_number(led, 2, (number / 10) % 10, dot_mask & 0x02);
        tm1637_set_segment_number(led, 1, (number / 100) % 10, dot_mask & 0x04);
        tm1637_set_segment_number(led, 0, (number / 1000) % 10, dot_mask & 0x08);
    }
}

void tm1637_set_float(tm1637_led_t * led, float n) {
    if( n < 0 ) {
        tm1637_set_segment_number(led, 0, MINUS_SIGN_IDX, 0);
        float absn = nearestf(fabs(n),1);
        int int_part = (int)absn;
        float fx_part = absn - int_part;
        if( absn < 10 ) {
            fx_part *= 100;
            tm1637_set_segment_number(led, 1, (int)(absn + 0.5), 1 );
            tm1637_set_segment_number(led, 2, ((int)fx_part/10) % 10, 0 );
            tm1637_set_segment_number(led, 3, ((int)fx_part) % 10, 0 );
        }
        else if( n < 100 ) {
            fx_part *= 100;
            uint8_t f = ((int)fx_part % 10);
            
            tm1637_set_segment_number(led, 1, (int_part/10) % 10, 0 );
            tm1637_set_segment_number(led, 2, int_part % 10, 1 );
            tm1637_set_segment_number(led, 3, ((int)fx_part/10) % 10 + ((f > 4)?1:0), 0 );
        }
        else if( n < 1000 ) {
            tm1637_set_segment_number(led, 1, (int_part/100) % 10, 0 );
            tm1637_set_segment_number(led, 2, (int_part/10) % 10, 0 );
            tm1637_set_segment_number(led, 3, (int_part % 10) + ((fx_part >= 0.5 )?1:0), 0 );
        }
    }
    else {
        //  positive number
        int int_part = (int)n;
        float fx_part = n - int_part;
        if( n < 10 ) {
            n = nearestf(n,1);
            int_part = (int)n;
            fx_part = 10000 * (n - int_part);
            
            tm1637_set_segment_number(led, 0, int_part, 1);
            tm1637_set_segment_number(led, 1, ((int)fx_part/1000) % 10, 0 );
            tm1637_set_segment_number(led, 2, ((int)fx_part/100) % 10, 0 );
            tm1637_set_segment_number(led, 3, ((int)fx_part/10) % 10, 0 );
        }
        else if( n < 100 ) {
            n = nearestf(n,2);
            int_part = (int)n;
            fx_part = 1000 * (n - int_part);
            
            tm1637_set_segment_number(led, 0, (int_part/10) % 10, 0);
            tm1637_set_segment_number(led, 1, int_part % 10, 1 );
            tm1637_set_segment_number(led, 2, ((int)fx_part/100) % 10, 0 );
            tm1637_set_segment_number(led, 3, ((int)fx_part/10) % 10,0);
        }
        else if( n < 1000 ) {
            n = nearestf(n,2);
            int_part = (int)n;
            fx_part = 100 * (n - int_part);
            
            tm1637_set_segment_number(led, 0, (int_part/100) % 10, 0);
            tm1637_set_segment_number(led, 1, (int_part/10) % 10, 0 );
            tm1637_set_segment_number(led, 2, int_part % 10, 1 );
            tm1637_set_segment_number(led, 3, ((int)fx_part/10) % 10, 0 );
        }
    }
}
