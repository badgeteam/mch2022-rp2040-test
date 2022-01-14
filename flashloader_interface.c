/**
 * Copyright (c) 2022 Nicolai Electronics
 * Copyright (c) 2021 Richard Hulme
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "hardware/sync.h"
#include "hardware/flash.h"
#include "hardware/watchdog.h"
#include "hardware/structs/watchdog.h"
#include "flashloader.h"
#include "flashloader_interface.h"

static const uint32_t FLASH_IMAGE_OFFSET = 128 * 1024;

// Simple CRC32 (no reflection, no final XOR) implementation.
// This can be done with a lookup table or using the DMA sniffer too.
uint32_t crc32(const uint8_t *data, uint32_t len, uint32_t crc)
{
    while(len--)
    {
        crc ^= (*data++ << 24);

        for(int bit = 0; bit < 8; bit++)
        {
            if(crc & (1L << 31))
                crc = (crc << 1) ^ 0x04C11DB7;
            else
                crc = (crc << 1);
        }
    }
    return crc;
}

void flash_image(tFlashHeader* header, uint32_t length)
{
    // Calculate length of header plus length of data
    uint32_t totalLength = sizeof(tFlashHeader) + length;

    // Round erase length up to next 4096 byte boundary
    uint32_t eraseLength = (totalLength + 4095) & 0xfffff000;
    uint32_t status;

    header->magic1 = FLASH_MAGIC1;
    header->magic2 = FLASH_MAGIC2;
    header->length = length;
    header->crc32  = crc32(header->data, length, 0xffffffff);

    printf("Storing new image in flash and then rebooting\r\n");

    status = save_and_disable_interrupts();

    flash_range_erase(FLASH_IMAGE_OFFSET, eraseLength);
    flash_range_program(FLASH_IMAGE_OFFSET, (uint8_t*)header, totalLength);

    restore_interrupts(status);
    printf("Rebooting into flashloader in 1 second\r\n");

    // Set up watchdog scratch registers so that the flashloader knows
    // what to do after the reset
    watchdog_hw->scratch[0] = FLASH_MAGIC1;
    watchdog_hw->scratch[1] = XIP_BASE + FLASH_IMAGE_OFFSET;
    watchdog_reboot(0x00000000, 0x00000000, 1000);

    // Wait for the reset
    while(true) {
        tight_loop_contents();
    }
}
