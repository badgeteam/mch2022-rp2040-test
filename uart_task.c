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

static bool esp32_reset_active = false;
static uint8_t esp32_reset_state = 0;
static uint8_t esp32_reset_app_state = 0;
static absolute_time_t esp32_reset_timeout = 0;

static bool esp32_wakeup_active = false;
static absolute_time_t esp32_wakeup_timeout = 0;

static bool cdc_enabled = false;
void cdc_control(bool state) {
    cdc_enabled = state;
}

void setup_uart() {
    uart_init(UART_ESP32, 115200);
    gpio_set_function(UART_ESP32_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_ESP32_RX_PIN, GPIO_FUNC_UART);
    irq_set_exclusive_handler(UART0_IRQ, on_esp32_uart_rx);
    irq_set_enabled(UART0_IRQ, true);
    uart_set_irq_enables(UART_ESP32, true, false);

    gpio_init(ESP32_BL_PIN);
    gpio_set_dir(ESP32_BL_PIN, true);
    gpio_put(ESP32_BL_PIN, true);

    gpio_init(ESP32_EN_PIN);
    gpio_set_dir(ESP32_EN_PIN, true);
    gpio_put(ESP32_EN_PIN, false);

    gpio_init(ESP32_WK_PIN); // This is the PCA9555 interrupt line, do not set to output high!
    gpio_set_dir(ESP32_WK_PIN, false);

    uart_init(UART_FPGA, 115200);
    gpio_set_function(UART_FPGA_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_FPGA_RX_PIN, GPIO_FUNC_UART);
    irq_set_exclusive_handler(UART1_IRQ, on_fpga_uart_rx);
    irq_set_enabled(UART1_IRQ, true);
    uart_set_irq_enables(UART_FPGA, true, false);
}

void wake_up_esp32() {
    gpio_set_dir(ESP32_WK_PIN, true);
    gpio_put(ESP32_WK_PIN, false);
    esp32_wakeup_active = true;
    esp32_wakeup_timeout = delayed_by_ms(get_absolute_time(), 1);
}

/*void send_interrupt_to_esp32() {
    gpio_put(ESP32_BL_PIN, false);
    esp32_reset_active = true;
    esp32_reset_timeout = delayed_by_ms(get_absolute_time(), 5);
}*/

void on_esp32_uart_rx() {
    uint8_t buffer[64];
    uint32_t length = 0;
    while (uart_is_readable(UART_ESP32)) {
        buffer[length] = uart_getc(UART_ESP32);
        length++;
        if (length >= sizeof(buffer)) {
            if (cdc_enabled && tud_ready()) {
                cdc_send(0, buffer, length);
            }
            length = 0;
        }
    }
    if (length > 0) {
        if (cdc_enabled && tud_ready()) {
            cdc_send(0, buffer, length);
        }
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
            if (cdc_enabled && tud_ready()) {
                cdc_send(1, buffer, length);
            }
            length = 0;
        }
    }
    if (length > 0) {
        if (cdc_enabled && tud_ready()) {
            cdc_send(1, buffer, length);
        }
        length = 0;
    }
}

void cdc_send(uint8_t itf, uint8_t* buf, uint32_t count) {
    if ((!cdc_enabled) || (!tud_ready())) {
        return;
    }
    tud_cdc_n_write(itf, buf, count);
    tud_cdc_n_write_flush(itf);
}

void cdc_task(void) {
    uint8_t buffer[64];
    
    if ((!cdc_enabled) || (!tud_ready())) {
        return;
    }
    
    if (tud_cdc_n_available(USB_CDC_ESP32)) {
        uint32_t length = tud_cdc_n_read(USB_CDC_ESP32, buffer, sizeof(buffer));
        for (uint32_t position = 0; position < length; position++) {
            uart_tx_wait_blocking(UART_ESP32);
            uart_putc_raw(UART_ESP32, buffer[position]);
        }
    }

    if (tud_cdc_n_available(USB_CDC_FPGA)) {
        uint32_t length = tud_cdc_n_read(USB_CDC_FPGA, buffer, sizeof(buffer));
        for (uint32_t position = 0; position < length; position++) {
            uart_tx_wait_blocking(UART_FPGA);
            uart_putc_raw(UART_FPGA, buffer[position]);
        }
    }

    absolute_time_t now = get_absolute_time();
    if((esp32_reset_state || esp32_reset_active) && now > esp32_reset_timeout) {
        if (esp32_reset_active) {
            gpio_put(ESP32_EN_PIN, true);
            esp32_reset_active = false;
            esp32_reset_state = 0xFF;
            esp32_reset_app_state = 0xFF;
            esp32_reset_timeout = delayed_by_ms(get_absolute_time(), 50);
        } else {
            esp32_reset_state = 0x00;
            esp32_reset_app_state = 0x00;
            gpio_put(ESP32_BL_PIN, true);
            gpio_put(ESP32_EN_PIN, true);
        }
    }
    
    if (esp32_wakeup_active && now > esp32_wakeup_timeout) {
        esp32_wakeup_active = false;
        gpio_set_dir(ESP32_WK_PIN, false);
    }
}

void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const* p_line_coding) {
    if ((!cdc_enabled) || (!tud_ready())) {
        return;
    }

    uart_inst_t* uart;
    switch (itf) {
        case USB_CDC_ESP32:
            uart = UART_ESP32;
            break;
        case USB_CDC_FPGA:
            uart = UART_FPGA;
            break;
        default:
            return; // Other CDC interfaces are ignored
    }

    uint data_bits;
    switch (p_line_coding->data_bits) {
        case 5: data_bits = 5; break;
        case 6: data_bits = 6; break;
        case 7: data_bits = 7; break;
        case 8: data_bits = 8; break;
        case 16: default: data_bits = 8; break; // Not supported
    }
    
    uint stop_bits;
    switch (p_line_coding->stop_bits) {
        case 0: stop_bits = 1; break; // 1 stop bit
        case 2: stop_bits = 2; break; // 2 stop bits
        case 1: default: stop_bits = 1; break; // 1.5 stop bits (not supported)
    }
    
    uart_parity_t parity;
    switch (p_line_coding->parity) {
        case 0: parity = UART_PARITY_NONE; break;
        case 1: parity = UART_PARITY_ODD; break;
        case 2: parity = UART_PARITY_EVEN; break;
        case 3: case 4: default: parity = UART_PARITY_NONE; break; //3: mark, 4: space (not supported)
    }
    
    uart_set_baudrate(uart, p_line_coding->bit_rate);
    uart_set_format(uart, data_bits, stop_bits, parity);
    
    printf("ESP32 UART settings changed to %u baud, %s parity, %d stop bits, %d data bits\r\n", p_line_coding->bit_rate, (p_line_coding->parity == 2) ? "even" : (p_line_coding->parity == 1) ? "odd" : "no", p_line_coding->stop_bits + 1, p_line_coding->data_bits);
}

void esp32_reset(bool download_mode) {
    gpio_put(ESP32_BL_PIN, !download_mode);
    gpio_put(ESP32_EN_PIN, false);
    esp32_reset_active = true;
    esp32_reset_timeout = delayed_by_ms(get_absolute_time(), 25);
    printf("ESP32 reset to %s\r\n", download_mode ? "DL" : "APP");
}

bool prev_dtr = false;
bool prev_rts = false;
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
    if ((!cdc_enabled) || (!tud_ready())) {
        return;
    }
    if(itf == USB_CDC_ESP32) {
        bool dtr2 = dtr || prev_dtr;
        bool rts2 = rts || prev_rts;
        
        if ((esp32_reset_state == 0) && ( dtr2) && ( rts2)) esp32_reset_state = 1;
        if ((esp32_reset_state == 1) && ( dtr2) && (!rts2)) esp32_reset_state = 2;
        if ((esp32_reset_state == 2) && (!dtr2) && ( rts2)) esp32_reset_state = 3;
        if ((esp32_reset_state == 3) && ( dtr2) && ( rts2)) {
            esp32_reset_state = 4;
            esp32_reset(true);
        }
        
        if ((esp32_reset_app_state == 0) && ((!dtr2) && rts2)) esp32_reset_app_state = 1;
        if ((esp32_reset_app_state == 1) && ((!dtr2) && (!rts2))) {
            esp32_reset_app_state = 2;
            esp32_reset(false);
        }
        
        prev_dtr = dtr;
        prev_rts = rts;
        
        if (!esp32_reset_active) {
            esp32_reset_timeout = delayed_by_ms(get_absolute_time(), 1000);
        }
    }
}
