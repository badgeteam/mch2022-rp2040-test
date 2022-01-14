#pragma once

#include "hardware.h"

void setup_uart();

// USB CDC serial port
void cdc_task(void);
void cdc_send(uint8_t itf, uint8_t* buf, uint32_t count);

// Hardware serial port
void on_esp32_uart_rx();
void on_fpga_uart_rx();

// Stdio via CDC 2
bool stdio_usb_init_cdc2();
