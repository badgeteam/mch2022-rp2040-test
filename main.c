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
#include "tusb.h"
#include "usb_descriptors.h"
#include "usb_hid.h"
#include "i2c_peripheral.h"
#include "uart_task.h"
#include "webusb_task.h"
#include "stdio_cdc2.h"
#include "hardware.h"

int main(void) {
    board_init();
    tusb_init();
    stdio_usb_init_cdc2();
    setup_uart();
    setup_i2c_registers();
    setup_i2c_peripheral(I2C_SYSTEM, I2C_SYSTEM_SDA_PIN, I2C_SYSTEM_SCL_PIN, 0x17, 100000, i2c_slave_handler);
    //setup_i2c_peripheral(I2C_EXT, I2C_EXT_SDA_PIN, I2C_EXT_SCL_PIN, 0x17, 100000, i2c_slave_handler);
    
    while (1) {
        tud_task(); // tinyusb device task
        hid_task();
        cdc_task();
        webusb_task();
        i2c_task();
        
        if (board_button_read()) {
            printf("Reset to USB bootloader...\r\n");
            reset_usb_boot(0, 0);
        }
    }

    return 0;
}

// USB

// Invoked when device is mounted
void tud_mount_cb(void) {
    //led_set_interval(BLINK_MOUNTED);
    printf("tud_mount_cb\r\n");
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
  //led_set_interval(BLINK_NOT_MOUNTED);
    printf("tud_umount_cb\r\n");
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
  (void) remote_wakeup_en;
  //led_set_interval(BLINK_SUSPENDED);
  printf("tud_suspend_cb\r\n");
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
  //led_set_interval(BLINK_MOUNTED);
    printf("tud_resume_cb\r\n");
}
