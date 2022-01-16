#pragma once

#include <stdint.h>
#include <pico/stdlib.h>

#define SPI_MAX_TRANSFER_LENGTH 4094 // Same as ESP32

void setup_spi_slave(spi_inst_t *spi, uint32_t baudrate, uint8_t rx_pin, uint8_t tx_pin, uint8_t sck_pin, uint32_t cs_pin);

void spi_slave_task();
