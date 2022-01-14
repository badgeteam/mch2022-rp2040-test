#pragma once

void webusb_task(void);
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request);
bool tud_vendor_control_complete_cb(uint8_t rhport, tusb_control_request_t const * request);
