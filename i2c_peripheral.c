/*
 * Copyright (c) 2021 Valentin Milea <valentin.milea@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <i2c_fifo.h>
#include <i2c_slave.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>

#include "i2c_peripheral.h"

void setup_i2c_peripheral(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t address, uint32_t baudrate, i2c_slave_handler_t handler) {
    gpio_init(sda_pin);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);

    gpio_init(scl_pin);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(scl_pin);

    i2c_init(i2c, baudrate);
    i2c_slave_init(i2c, address, handler);
}


/* MCH2022 I2C peripheral */

#include "hardware.h"

struct {
    uint8_t registers[256];
    bool modified[256];
    bool read_only[256];
    uint8_t address;
    bool transfer_in_progress;
} i2c_registers;

enum {
    I2C_REGISTER_FW_VER,
    I2C_REGISTER_GPIO_DIR,
    I2C_REGISTER_GPIO_IN,
    I2C_REGISTER_GPIO_OUT,
    I2C_REGISTER_LCD_MODE,
    I2C_REGISTER_LCD_BACKLIGHT,
};

uint8_t i2c_controlled_gpios[] = {SAO_IO0_PIN, SAO_IO1_PIN, PROTO_0_PIN, PROTO_1_PIN, PROTO_2_PIN};

void setup_i2c_registers() {
    for (uint16_t reg = 0; reg < 256; reg++) {
        i2c_registers.registers[reg] = 0;
        i2c_registers.modified[reg] = false;
        i2c_registers.read_only[reg] = false;
    }

    i2c_registers.registers[I2C_REGISTER_FW_VER] = 0x01;
    i2c_registers.read_only[I2C_REGISTER_FW_VER] = true;
    i2c_registers.read_only[I2C_REGISTER_GPIO_IN] = true;
    
    for (uint8_t pin = 0; pin < sizeof(i2c_controlled_gpios); pin++) {
        gpio_init(i2c_controlled_gpios[pin]);
    }
}

void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {
    // In ISR context, don't block and quickly complete!
    switch (event) {
        case I2C_SLAVE_RECEIVE:
            if (!i2c_registers.transfer_in_progress) {
                i2c_registers.address = i2c_read_byte(i2c);
                i2c_registers.transfer_in_progress = true;
            } else {
                if (!i2c_registers.read_only[i2c_registers.address]) {
                    i2c_registers.registers[i2c_registers.address] = i2c_read_byte(i2c);
                    i2c_registers.modified[i2c_registers.address] = true;
                }
                i2c_registers.address++;
            }
            break;
        case I2C_SLAVE_REQUEST:
            i2c_write_byte(i2c, i2c_registers.registers[i2c_registers.address]);
            i2c_registers.address++;
            break;
        case I2C_SLAVE_FINISH:
            i2c_registers.transfer_in_progress = false;
            break;
        default:
            break;
    }
}


void i2c_handle_register_write(uint8_t reg, uint8_t value) {
    char text_buffer[64];
    switch (reg) {
        case I2C_REGISTER_GPIO_DIR:
            for (uint8_t pin = 0; pin < sizeof(i2c_controlled_gpios); pin++) {
                gpio_set_dir(i2c_controlled_gpios[pin], (value & (1 << pin)) >> pin);
            }
            break;
        case I2C_REGISTER_GPIO_OUT:
            for (uint8_t pin = 0; pin < sizeof(i2c_controlled_gpios); pin++) {
                gpio_put(i2c_controlled_gpios[pin], (value & (1 << pin)) >> pin);
            }
            break;
        case I2C_REGISTER_LCD_MODE:
            // To be done
            break;
        case I2C_REGISTER_LCD_BACKLIGHT:
            // To be done
            break;
        default:
            break;
    };
}

void i2c_task() {
    for (uint16_t reg = 0; reg < 256; reg++) {
        if (i2c_registers.modified[reg]) {
            i2c_handle_register_write(reg, i2c_registers.registers[reg]);
            i2c_registers.modified[reg] = false;
        }       
    }
    
    // Read GPIO pins
    uint8_t gpio_in_value = 0;
    for (uint8_t pin = 0; pin < sizeof(i2c_controlled_gpios); pin++) {
        gpio_in_value |= gpio_get(i2c_controlled_gpios[pin]) << pin;
    }
    i2c_registers.registers[I2C_REGISTER_GPIO_IN] = gpio_in_value;
}
