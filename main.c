/**
 * Copyright (c) 2022 Nicolai Electronics
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bsp/board.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "usb_hid.h"
#include "blinking_led.h"

void cdc_task(void);
void webusb_task(void);

int main(void) {
  board_init();
  tusb_init();

  while (1) {
    tud_task(); // tinyusb device task
    led_blinking_task();
    hid_task();
    cdc_task();
    webusb_task();
  }

  return 0;
}

// USB

// Invoked when device is mounted
void tud_mount_cb(void) {
  //led_set_interval(BLINK_MOUNTED);
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
  //led_set_interval(BLINK_NOT_MOUNTED);
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
  (void) remote_wakeup_en;
  //led_set_interval(BLINK_SUSPENDED);
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
  //led_set_interval(BLINK_MOUNTED);
}

// USB CDC

void cdc_send(uint8_t itf, uint8_t buf[], uint32_t count) {
  for(uint32_t i=0; i<count; i++) {
    tud_cdc_n_write_char(itf, buf[i]);
  }
  tud_cdc_n_write_flush(itf);
}

bool webusb_connected = false;

void cdc_task(void) {
  for (uint8_t itf = 0; itf < CFG_TUD_CDC; itf++) {
    // connected() check for DTR bit
    // Most but not all terminal client set this when making connection
    //if ( tud_cdc_n_connected(itf) ) {
      if ( tud_cdc_n_available(itf) ) {
        uint8_t buf[64];
        uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));
        cdc_send(itf, buf, count);
        if (webusb_connected) {
            tud_vendor_n_write(itf, buf, count);
        }
      }
    //}
  }
}

// USB WebUSB

void webusb_task() {
    uint8_t buffer[64];
    uint8_t text_buffer[64];
    for (uint8_t idx = 0; idx < CFG_TUD_VENDOR; idx++) {
        int available = tud_vendor_n_available(idx);
        if (available > 0) {
            uint32_t read = tud_vendor_n_read(idx, buffer, sizeof(buffer));
            cdc_send(idx, buffer, read);
        }
    }
}

#define WEBUSB_LANDING_PAGE_URL "webusb.bodge.team"

tusb_desc_webusb_url_t desc_url = {
  .bLength         = 3 + sizeof(WEBUSB_LANDING_PAGE_URL) - 1,
  .bDescriptorType = 3, // WEBUSB URL type
  .bScheme         = 1, // 0: http, 1: https
  .url             = WEBUSB_LANDING_PAGE_URL
};

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request) {
    // nothing to with DATA & ACK stage
    if (stage != CONTROL_STAGE_SETUP) return true;

    switch (request->bmRequestType_bit.type) {
        case TUSB_REQ_TYPE_VENDOR:
            switch (request->bRequest) {
            case VENDOR_REQUEST_WEBUSB:
                // match vendor request in BOS descriptor
                // Get landing page url
                return tud_control_xfer(rhport, request, (void*)(uintptr_t) &desc_url, desc_url.bLength);

            case VENDOR_REQUEST_MICROSOFT:
                if ( request->wIndex == 7 ) {
                // Get Microsoft OS 2.0 compatible descriptor
                uint16_t total_len;
                memcpy(&total_len, desc_ms_os_20+8, 2);

                return tud_control_xfer(rhport, request, (void*)(uintptr_t) desc_ms_os_20, total_len);
                } else {
                return false;
                }
            default: break;
            }
        break;

        case TUSB_REQ_TYPE_CLASS:
            if (request->bRequest == 0x22) {
                webusb_connected = (request->wValue != 0);
                if (webusb_connected) {
                    led_set_interval(BLINK_NOT_MOUNTED);
                } else {
                    led_set_interval(BLINK_SUSPENDED);
                }
                // response with status OK
                return tud_control_status(rhport, request);
            }
            if (request->bRequest == 0x23) {
                //request->wValue;
                // response with status OK
                return tud_control_status(rhport, request);
            }
        break;

        default: break;
    }

    // stall unknown request
    return false;
}

bool tud_vendor_control_complete_cb(uint8_t rhport, tusb_control_request_t const * request) {
  (void) rhport;
  (void) request;
  return true;
}
