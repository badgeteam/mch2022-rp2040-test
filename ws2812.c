#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

#include "hardware.h"

void setup_leds() {
    gpio_set_dir(LED_PWR_PIN, true);
    gpio_put(LED_PWR_PIN, false);
}

void enable_leds() {
    uint offset = pio_add_program(pio0, &ws2812_program);
    ws2812_program_init(pio0, 0, offset, LED_DATA_PIN, 800000, false);
    gpio_put(LED_PWR_PIN, true);
}

void disable_leds() {
    gpio_put(LED_PWR_PIN, false);
    pio_sm_set_enabled(pio0, 0, false);
}

void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return  ((uint32_t) (r) << 8) |
            ((uint32_t) (g) << 16) |
            (uint32_t) (b);
}
