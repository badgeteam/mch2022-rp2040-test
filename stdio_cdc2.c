/*
 * Copyright (c) 2022 Nicolai Electronics
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "bsp/board.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/time.h"
#include "pico/stdio/driver.h"
#include "pico/binary_info.h"
#include "pico/mutex.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "stdio_cdc2.h"

static mutex_t stdio_usb_mutex;

static void low_priority_worker_irq(void) {
    // if the mutex is already owned, then we are in user code
    // in this file which will do a tud_task itself, so we'll just do nothing
    // until the next tick; we won't starve
    if (mutex_try_enter(&stdio_usb_mutex, NULL)) {
        tud_task();
        mutex_exit(&stdio_usb_mutex);
    }
}

static int64_t timer_task(__unused alarm_id_t id, __unused void *user_data) {
    irq_set_pending(PICO_STDIO_USB_LOW_PRIORITY_IRQ);
    return PICO_STDIO_USB_TASK_INTERVAL_US;
}

static void stdio_usb_out_chars_cdc2(const char *buf, int length) {
    static uint64_t last_avail_time;
    uint32_t owner;
    if (!mutex_try_enter(&stdio_usb_mutex, &owner)) {
        if (owner == get_core_num()) return; // would deadlock otherwise
        mutex_enter_blocking(&stdio_usb_mutex);
    }
    if (tud_cdc_n_connected(CDC_CONSOLE)) {
        for (int i = 0; i < length;) {
            int n = length - i;
            int avail = (int) tud_cdc_n_write_available(CDC_CONSOLE);
            if (n > avail) n = avail;
            if (n) {
                int n2 = (int) tud_cdc_n_write(CDC_CONSOLE, buf + i, (uint32_t)n);
                tud_task();
                tud_cdc_n_write_flush(CDC_CONSOLE);
                i += n2;
                last_avail_time = time_us_64();
            } else {
                tud_task();
                tud_cdc_n_write_flush(CDC_CONSOLE);
                if (!tud_cdc_n_connected(CDC_CONSOLE) ||
                    (!tud_cdc_n_write_available(CDC_CONSOLE) && time_us_64() > last_avail_time + PICO_STDIO_USB_STDOUT_TIMEOUT_US)) {
                    break;
                }
            }
        }
    } else {
        // reset our timeout
        last_avail_time = 0;
    }
    mutex_exit(&stdio_usb_mutex);
}

bool stdio_usb_connected_cdc2(void) {
    return tud_cdc_n_connected(CDC_CONSOLE);
}

int stdio_usb_in_chars_cdc2(char *buf, int length) {
    uint32_t owner;
    if (!mutex_try_enter(&stdio_usb_mutex, &owner)) {
        if (owner == get_core_num()) return PICO_ERROR_NO_DATA; // would deadlock otherwise
        mutex_enter_blocking(&stdio_usb_mutex);
    }
    int rc = PICO_ERROR_NO_DATA;
    if (tud_cdc_n_connected(CDC_CONSOLE) && tud_cdc_available()) {
        int count = (int) tud_cdc_n_read(CDC_CONSOLE, buf, (uint32_t) length);
        rc =  count ? count : PICO_ERROR_NO_DATA;
    }
    mutex_exit(&stdio_usb_mutex);
    return rc;
}

stdio_driver_t stdio_usb_cdc2 = {
    .out_chars = stdio_usb_out_chars_cdc2,
    .in_chars = stdio_usb_in_chars_cdc2
};

bool stdio_usb_init_cdc2(void) {
    irq_set_exclusive_handler(PICO_STDIO_USB_LOW_PRIORITY_IRQ, low_priority_worker_irq);
    irq_set_enabled(PICO_STDIO_USB_LOW_PRIORITY_IRQ, true);

    mutex_init(&stdio_usb_mutex);
    bool rc = add_alarm_in_us(PICO_STDIO_USB_TASK_INTERVAL_US, timer_task, NULL, true);
    if (rc) {
        stdio_set_driver_enabled(&stdio_usb_cdc2, true);
    }
    return rc;
}
