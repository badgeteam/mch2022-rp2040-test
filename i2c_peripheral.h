#pragma once

#include <stdint.h>
#include <i2c_fifo.h>
#include <i2c_slave.h>
#include <pico/stdlib.h>

void setup_i2c_peripheral(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t address, uint32_t baudrate, i2c_slave_handler_t handler);

void setup_i2c_registers();

void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event);
void i2c_task();

void i2c_register_write(uint8_t reg, uint8_t value);

enum {
    I2C_REGISTER_FW_VER,
    I2C_REGISTER_GPIO_DIR,
    I2C_REGISTER_GPIO_IN,
    I2C_REGISTER_GPIO_OUT,
    I2C_REGISTER_LCD_MODE,
    I2C_REGISTER_LCD_BACKLIGHT,
    I2C_REGISTER_LED_MODE,
    I2C_REGISTER_LED_VALUE0,
    I2C_REGISTER_LED_VALUE1,
    I2C_REGISTER_LED_VALUE2,
    I2C_REGISTER_LED_VALUE3,
    I2C_REGISTER_LED_VALUE4,
    I2C_REGISTER_LED_VALUE5,
    I2C_REGISTER_LED_VALUE6,
    I2C_REGISTER_LED_VALUE7,
    I2C_REGISTER_LED_VALUE8,
    I2C_REGISTER_LED_VALUE9,
    I2C_REGISTER_LED_VALUE10,
    I2C_REGISTER_LED_VALUE11,
    I2C_REGISTER_LED_VALUE12,
    I2C_REGISTER_LED_VALUE13,
    I2C_REGISTER_LED_VALUE14
};
