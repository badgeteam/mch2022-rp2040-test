#pragma once

void setup_leds();
void enable_leds();
void disable_leds();
void put_pixel(uint32_t pixel_grb);
uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b);
