/**
 * Copyright (c) 2022 Nicolai Electronics
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "bsp/board.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "usb_hid.h"
#include "i2c_peripheral.h"
#include "uart_task.h"
#include "webusb_task.h"
#include "stdio_cdc2.h"
//#include "spi_slave.h"
#include "ws2812.h"
#include "hardware.h"
#include "lcd.h"
#include "hardware/adc.h"

int map(int s, int a1, int a2, int b1, int b2) {
    return b1 + (s - a1) * (b2 - b1) / (a2 - a1);
}

int main(void) {
    board_init();
    tusb_init();
    lcd_init();
    setup_leds();
    stdio_usb_init_cdc2();
    setup_uart();
    setup_i2c_registers();
    setup_i2c_peripheral(I2C_SYSTEM, I2C_SYSTEM_SDA_PIN, I2C_SYSTEM_SCL_PIN, 0x17, 100000, i2c_slave_handler);
    //setup_i2c_peripheral(I2C_EXT, I2C_EXT_SDA_PIN, I2C_EXT_SCL_PIN, 0x17, 100000, i2c_slave_handler);
    
    //setup_spi_slave(spi0, 1000000, SPI_RX, SPI_TX, SPI_SCK, SPI_CS);
    
    gpio_init(USB_DET_PIN);
    gpio_init(BATT_CHRG_PIN);
    gpio_set_dir(USB_DET_PIN, false);
    gpio_set_dir(BATT_CHRG_PIN, false);
    
    adc_init();
    adc_gpio_init(ANALOG_TEMP_PIN);
    adc_gpio_init(ANALOG_VBAT_PIN);
    adc_gpio_init(ANALOG_VUSB_PIN);
    
    esp32_reset(false); // Start ESP32

    while (1) {
        tud_task(); // tinyusb device task
        cdc_task();
        webusb_task();
        hid_task();
        i2c_task();

        if (board_button_read()) {
            printf("Reset to USB bootloader...\r\n");
            reset_usb_boot(0, 0);
        }
    }

    return 0;
}

// Invoked when device is mounted
void tud_mount_cb(void) {
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
}

// Invoked when usb bus is suspended
void tud_suspend_cb(bool remote_wakeup_en) {
  (void) remote_wakeup_en;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
}
