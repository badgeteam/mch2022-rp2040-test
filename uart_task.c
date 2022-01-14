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
#include "uart_task.h"
#include "hardware.h"



void setup_uart() {
    uart_init(UART_ESP32, 115200);
    gpio_set_function(UART_ESP32_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_ESP32_RX_PIN, GPIO_FUNC_UART);
    irq_set_exclusive_handler(UART0_IRQ, on_esp32_uart_rx);
    irq_set_enabled(UART0_IRQ, true);
    uart_set_irq_enables(UART_ESP32, true, false);

    uart_init(UART_FPGA, 115200);
    gpio_set_function(UART_FPGA_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_FPGA_RX_PIN, GPIO_FUNC_UART);
    irq_set_exclusive_handler(UART1_IRQ, on_fpga_uart_rx);
    irq_set_enabled(UART1_IRQ, true);
    uart_set_irq_enables(UART_FPGA, true, false);
}

void cdc_send(uint8_t itf, uint8_t* buf, uint32_t count) {
  for(uint32_t i=0; i<count; i++) {
    tud_cdc_n_write_char(itf, buf[i]);
  }
  tud_cdc_n_write_flush(itf);
}

void cdc_task(void) {
    // For handling CDC 0 and CDC 1
    for (uint8_t interface = 0; interface < 2; interface++) {
        if ( tud_cdc_n_available(interface) ) {
            uint8_t buffer[64];
            uint32_t length = tud_cdc_n_read(interface, buffer, sizeof(buffer));
            for (uint32_t position = 0; position < length; position++) {
                uart_tx_wait_blocking(interface ? UART_FPGA : UART_ESP32);
                uart_putc_raw(interface ? UART_FPGA : UART_ESP32, buffer[position]);
            }
        }
    }
}

// Serial port interrupt handlers

void on_esp32_uart_rx() {
    uint8_t buffer[64];
    uint32_t length = 0;
    while (uart_is_readable(UART_ESP32)) {
        buffer[length] = uart_getc(UART_ESP32);
        length++;
        if (length >= sizeof(buffer)) {
            cdc_send(0, buffer, length);
            length = 0;
        }
    }
    if (length > 0) {
        cdc_send(0, buffer, length);
        length = 0;
    }
}

void on_fpga_uart_rx() {
    uint8_t buffer[64];
    uint32_t length = 0;
    while (uart_is_readable(UART_FPGA)) {
        buffer[length] = uart_getc(UART_FPGA);
        length++;
        if (length >= sizeof(buffer)) {
            cdc_send(1, buffer, length);
            length = 0;
        }
    }
    if (length > 0) {
        cdc_send(1, buffer, length);
        length = 0;
    }
}
