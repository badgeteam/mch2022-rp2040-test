#include "spi_slave.h"

static uint8_t in_buf[SPI_MAX_TRANSFER_LENGTH], out_buf[SPI_MAX_TRANSFER_LENGTH];

static spi_inst_t* spi_dev;

void setup_spi_slave(spi_inst_t* spi, uint32_t baudrate, uint8_t rx_pin, uint8_t tx_pin, uint8_t sck_pin, uint32_t cs_pin) {
    spi_init(spi, speed);
    spi_set_slave(spi, true);
    gpio_set_function(sck_pin, GPIO_FUNC_SPI);
    gpio_set_function(cs_pin, GPIO_FUNC_SPI);
    gpio_set_function(rx_pin, GPIO_FUNC_SPI);
    gpio_set_function(tx_pin, GPIO_FUNC_SPI);
    memset(out_buf, 0, SPI_MAX_TRANSFER_LENGTH);
    spi_dev = spi;
}

void spi_slave_task() {
    if (spi_dev == NULL) return;
    if (spi_is_readable(spi_dev)) {
        
    }
}
