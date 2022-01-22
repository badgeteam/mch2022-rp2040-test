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
    lcd_init();
    setup_leds();
    tusb_init();
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
        hid_task();
        cdc_task();
        webusb_task();
        i2c_task();

        char buffer[16];
        int nread = stdio_usb_in_chars_cdc2(buffer, 16);
        
        if ((nread != PICO_ERROR_NO_DATA) && (nread > 0)) {
            for (int i = 0; i < nread; i++) {
                switch (buffer[i]) {                    
                    case 'B':
                        printf("Reset to USB bootloader...\r\n");
                        reset_usb_boot(0, 0);
                        break;
                    case 'L':
                        lcd_backlight(255);
                        printf("Backlight on\r\n");
                        break;
                    case 'l':
                        lcd_backlight(0);
                        printf("Backlight off\r\n");
                        break;
                    case 'e':
                        esp32_reset(false);
                        break;
                    case 'E':
                        esp32_reset(true);
                        break;
                    case 'p':
                        lcd_mode(true);
                        printf("LCD: parallel mode\r\n");
                        break;
                    case 's':
                        lcd_mode(false);
                        printf("LCD: SPI mode\r\n");
                        break;
                    case 'q': {
                        bool usb = gpio_get(USB_DET_PIN);
                        bool charging = gpio_get(BATT_CHRG_PIN);
                        printf("USB: %d, CHRG: %d\r\n", usb, charging);
                        break;
                    }
                    case 'z': {
                        adc_select_input(ANALOG_TEMP_ADC);
                        sleep_ms(2);
                        uint16_t result = adc_read();
                        float voltage = (result * 3.3) / 4096.0;
                        printf("Battery temperature: %f volt\n", voltage); // to-do: convert to temperature
                        break;
                    }
                    case 'x': {
                        adc_select_input(ANALOG_VBAT_ADC);
                        sleep_ms(2);
                        uint16_t result = adc_read();
                        float voltage = (result * 3.3 * 2) / 4096;
                        printf("Battery voltage: %f volt\n", voltage);
                        break;
                    }
                    case 'c': {
                        adc_select_input(ANALOG_VUSB_ADC);
                        sleep_ms(2);
                        uint16_t result = adc_read();
                        float voltage = (result * 3.3 * 2 * 0.925) / 4096;
                        printf("USB voltage: %f volt\n", voltage);
                        break;
                    }
                    
                    case 'M': {
                        i2c_register_write(I2C_REGISTER_LED_MODE, 0x03); 
                        break;
                    }
                    
                    case 'm': {
                        i2c_register_write(I2C_REGISTER_LED_MODE, 0x00); 
                        break;
                    }
                    
                    case 'N': {
                        i2c_register_write(I2C_REGISTER_LED_VALUE0, 0xFF);
                        i2c_register_write(I2C_REGISTER_LED_VALUE1, 0x00);
                        i2c_register_write(I2C_REGISTER_LED_VALUE2, 0x00);
                        
                        i2c_register_write(I2C_REGISTER_LED_VALUE3, 0x00);
                        i2c_register_write(I2C_REGISTER_LED_VALUE4, 0xFF);
                        i2c_register_write(I2C_REGISTER_LED_VALUE5, 0x00);
                        
                        i2c_register_write(I2C_REGISTER_LED_VALUE6, 0x00);
                        i2c_register_write(I2C_REGISTER_LED_VALUE7, 0x00);
                        i2c_register_write(I2C_REGISTER_LED_VALUE8, 0xFF);
                        
                        i2c_register_write(I2C_REGISTER_LED_VALUE9, 0xFF);
                        i2c_register_write(I2C_REGISTER_LED_VALUE10, 0xFF);
                        i2c_register_write(I2C_REGISTER_LED_VALUE11, 0x00);
                        
                        i2c_register_write(I2C_REGISTER_LED_VALUE12, 0x00);
                        i2c_register_write(I2C_REGISTER_LED_VALUE13, 0xFF);
                        i2c_register_write(I2C_REGISTER_LED_VALUE14, 0xFF);
                        break;
                    }

                    case 'G': {
                        for (uint8_t offset = 0; offset < 15; offset++) {
                            i2c_register_write(I2C_REGISTER_LED_VALUE0 + offset, 0xFF);
                        }
                        break;
                    }
                    
                    case 'g': {
                        for (uint8_t offset = 0; offset < 7; offset++) {
                            i2c_register_write(I2C_REGISTER_LED_VALUE0 + (offset*2), 0xFF);
                        }
                        break;
                    }

                    case 'n': {
                        for (uint8_t offset = 0; offset < 15; offset++) {
                            i2c_register_write(I2C_REGISTER_LED_VALUE0 + offset, 0x00);
                        }
                        break;
                    }
                    
                    default:
                        printf("Unknown command %c\r\n", buffer[i]);
                }
            }
        }
        
        if (board_button_read()) {
            printf("Reset to USB bootloader...\r\n");
            reset_usb_boot(0, 0);
        }
    }

    return 0;
}

// Invoked when device is mounted
void tud_mount_cb(void) {
    cdc_control(true);
    cdc2_control(true);
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
    cdc_control(false);
    cdc2_control(false);
}

// Invoked when usb bus is suspended
void tud_suspend_cb(bool remote_wakeup_en) {
  (void) remote_wakeup_en;
  cdc_control(false);
  cdc2_control(false);
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
  cdc_control(true);
  cdc2_control(true);
}
