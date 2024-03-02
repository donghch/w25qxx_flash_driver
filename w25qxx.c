/*
MIT License

Copyright (c) 2024 Houchuan Dong

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "w25qxx.h"
#include "string.h"
#include <Arduino.h>

#ifndef NULL
#define NULL 0
#endif

#ifdef W25Q_MEMORY_MANAGEMENT
unsigned char w25q_mem_mark_sector(struct w25q_flash *flash, unsigned short sector, unsigned char used) {

    struct w25q_memory_map *map = flash->mem_map;
    unsigned short byte_num = sector / 8;
    unsigned char bit_num = sector % 8;

    if (sector >= map->size) {
        return 0;
    } else {
        if (used) {
            map->mapping[byte_num] |= (1 << bit_num);
        } else {
            map->mapping[byte_num] &= ~(1 << bit_num);
        }
    }
    return 1;
    
}
#endif

/* Helper fucntions */

static void set_dummy_bytes(void *p, unsigned size) {
    unsigned char *dummy_buf = (unsigned char *)p;
    for (unsigned i = 0; i < size; i++, dummy_buf++) {
        *dummy_buf = 0xff;
    }
}

/**
 * @brief Check parameters passed from the user
*/
#ifndef TEST
static 
#endif
unsigned char w25q_check_param(struct w25q_flash *flash, unsigned address, void *buffer, unsigned buffer_size) {

    unsigned int page_num;

    // Do instance & address & buffer checking
    if (flash == NULL) {
        return 0;
    }
    page_num = (address & 0xffffff00) >> 8;
    // Check start address
    if (page_num >= flash->size) {
        return 0;
    }
    // Check end address
    page_num = ((address + buffer_size) & 0xffffff00) >> 8;
    if (page_num > flash->size) {
        return 0;
    }
    if (buffer == NULL) {
        return 0;
    }

    return 1;

}

/**
 * @brief Read SPI flash's status registers
 * @param[in] flash SPI flash instance
 * @param[out] buffer Buffer to store status data. Must be at least 3 bytes long.
 *                    Returned data begins at buffer[0] with length = 2.
 * 
*/
#ifndef TEST
static 
#endif
void w25q_read_status_regs(struct w25q_flash *flash, void *buffer) {

    unsigned char *result_buffer = (unsigned char *)buffer;
    set_dummy_bytes(result_buffer, 3);

    // Get the first status reg data
    result_buffer[0] = W25Q_READ_STATUS_REG_1;
    flash->spi_delay_func(W25Q_DELAY_TIME);
    flash->spi_send(result_buffer, result_buffer, 2);
    flash->spi_delay_func(W25Q_DELAY_TIME);
    result_buffer[0] = result_buffer[1];

    // Get the second status reg datav
    result_buffer[1] = W25Q_READ_STATUS_REG_2;
    flash->spi_delay_func(W25Q_DELAY_TIME);
    flash->spi_send(&result_buffer[1], &result_buffer[1], 2);
    flash->spi_delay_func(W25Q_DELAY_TIME);
    result_buffer[1] = result_buffer[2];

}

/**
 * @brief Enable data writes
 * 
 * @param[in] flash SPI flash instance
*/
#ifndef TEST
static 
#endif
void w25q_write_enable(struct w25q_flash *flash) {

    flash->spi_delay_func(W25Q_DELAY_TIME);
    unsigned char cmd = W25Q_WRITE_ENABLE;
    flash->spi_send(&cmd, &cmd, 1);
    flash->spi_delay_func(W25Q_DELAY_TIME);

}

/**
 * @brief Disable data writes
 * 
 * @param[in] flash SPI flash instance
*/
#ifndef TEST
static 
#endif
void w25q_write_disable(struct w25q_flash *flash) {

    unsigned char cmd = W25Q_WRITE_DISABLE;

    flash->spi_delay_func(W25Q_DELAY_TIME);
    flash->spi_send(&cmd, &cmd, 1);
    flash->spi_delay_func(W25Q_DELAY_TIME);

}

/**
 * @brief Enable volatile SR write
 * 
 * @param[in] flash SPI flash instance
*/
#ifndef TEST
static 
#endif
void w25q_sr_write_enable(struct w25q_flash *flash) {

    unsigned char cmd  = W25Q_VOLATILE_SR_WRITE_ENABLE;

    flash->spi_delay_func(W25Q_DELAY_TIME);
    flash->spi_send(&cmd, &cmd, 1);
    flash->spi_delay_func(W25Q_DELAY_TIME);

}

/**
 * @brief Read SPI flash JEDEC ID
 * 
 * @param[in] flash SPI flash instance
 * @param[out] buffer JEDEC ID buffer. Its size must be at least 4 bytes.
 *                  Returned value begins at data[1];
*/
#ifndef TEST
static 
#endif
void w25q_read_jedec(struct w25q_flash *flash, void *buffer) {

    unsigned char *return_data = (unsigned char *)buffer;
    set_dummy_bytes(return_data, 4);

    return_data[0] = W25Q_READ_JEDEC_ID;
    flash->spi_delay_func(W25Q_DELAY_TIME);
    flash->spi_send(return_data, return_data, 4);
    flash->spi_delay_func(W25Q_DELAY_TIME);

}

/**
 * Wait until the flash finish the last operation
 * @param[in] flash SPI Flash instance
*/
#ifndef TEST
static 
#endif
void w25q_wait_until_available(struct w25q_flash *flash) {

    unsigned char status_data[3];

    while (1) {
        w25q_read_status_regs(flash, (void *)status_data);
        if ((status_data[0] & 0x1) == 0) {
            break;
        }
        flash->spi_delay_func(W25Q_DELAY_TIME);
    }
}

/**
 * @brief Program data to a page
 * @param[in] flash SPI flash instance
 * @param[in] buffer Data buffer, first 4 bytes must be empty for hardware instruction
 * @param[in] buffer_size Buffer size
 * 
 * @return Number of bytes get written in the operation
*/
#ifndef TEST
static 
#endif
unsigned short w25q_page_program(struct w25q_flash *flash, unsigned address, void *buffer, unsigned buffer_size) {

    unsigned char *buf = (unsigned char *)buffer;

    /* Should we wait here for the chip to complete previous instructions? */
    w25q_wait_until_available(flash);

    w25q_write_enable(flash);

    /* Take min between buffer size and un-programmed bytes in the page */
    unsigned limit;
    if (buffer_size - 4 <= 256 - (address & 0xff)) {
        limit = buffer_size;
    } else {
        limit = 256 - (address & 0xff) + 4;
    }

    buf[0] = W25Q_PAGE_PROGRAM;
    /* Still, just support 3-byte addressing */
    buf[1] = (address >> 16) & 0xff;
    buf[2] = (address >> 8) & 0xff;
    buf[3] = address & 0xff;
    flash->spi_delay_func(W25Q_DELAY_TIME);
    flash->spi_send(buf, buf, limit);
    flash->spi_delay_func(W25Q_DELAY_TIME);

    w25q_write_disable(flash);

    return (limit - 4) & 0x1ff;
}

/**
 * @brief Erase a flash sector
 * 
 * @param[in] flash SPI flash instance
 * @param[in] address sector address.
*/
#ifndef TEST
static 
#endif
unsigned char w25q_sector_erase(struct w25q_flash *flash, unsigned address) {

    unsigned sector_num = (address >> 12) & 0xfffff;
    unsigned char cmd[4];

    if (sector_num >= flash->size) {
        return 0;
    }

    cmd[0] = W25Q_SECTOR_ERASE;
    cmd[1] = (address >> 16) & 0xff;
    cmd[2] = (address >> 8) & 0xf0;
    cmd[3] = 0;

    w25q_wait_until_available(flash);
    w25q_write_enable(flash);
    flash->spi_delay_func(W25Q_DELAY_TIME);
    flash->spi_send(&cmd, &cmd, 4);
    flash->spi_delay_func(W25Q_DELAY_TIME);
    w25q_wait_until_available(flash);

    return 1;
}

#ifndef TEST
static 
#endif
unsigned char w25q_32k_blk_erase(struct w25q_flash *flash, unsigned address) {

    unsigned sector_num = (address >> 12) & 0xfffff;
    unsigned char cmd[4];

    if (sector_num >= flash->size) {
        return 0;
    }

    cmd[0] = W25Q_32K_BLK_ERASE;
    cmd[1] = (address >> 16) & 0xff;
    cmd[2] = (address >> 8) & 0;
    cmd[3] = 0;

    return 1;
}

/**
 * @brief Erase a 64KB data block
 * 
 * @param[in] flash SPI flash instance
 * @param[in] address Data block address
*/
#ifndef TEST
static 
#endif
unsigned char w25q_64k_blk_erase(struct w25q_flash *flash, unsigned address) {

    unsigned sector_num = (address >> 12) & 0xfffff;
    unsigned char cmd[4];

    if (sector_num >= flash->size) {
        return 0;
    }

    cmd[0] = W25Q_64K_BLK_ERASE;
    cmd[1] = (address >> 16) & 0xff;
    cmd[2] = 0;
    cmd[3] = 0;

    w25q_wait_until_available(flash);
    w25q_write_enable(flash);
    flash->spi_delay_func(W25Q_DELAY_TIME);
    flash->spi_send(&cmd, &cmd, 4);
    flash->spi_delay_func(W25Q_DELAY_TIME);
    w25q_wait_until_available(flash);

    return 1;

}

/* Standard Functions */

struct w25q_flash * w25q_mount(struct w25q_flash *flash, w25q_spi_transfer_fn spi_data_func, w25q_delay_fn delay_fn) {
    
    struct w25q_flash *f_instance = flash;
    unsigned char part_data[4];

    f_instance->spi_delay_func = delay_fn;
    f_instance->spi_send = spi_data_func;

    w25q_read_jedec(f_instance, (void *)part_data);
    // Check Manufacturer
    if (part_data[1] != W25Q_PRODUCER_ID) {     // Wrong part or spi error
        return NULL;
    }

    // Check model
    for (int id = W25Q10_ID, size = W25Q10_SIZE; id <= W25Q512_ID; id++, size *= 2) {
        if (part_data[2] == ((id >> 8) & 0xff) && part_data[3] == (id & 0xff)) {
            f_instance->model = (enum w25q_id_t)id;
            f_instance->size = (enum w25q_size_t)size;
            return f_instance;
        }
    }
    
    // Oops, can't identify the model!
    return NULL;

}

unsigned char w25q_read(struct w25q_flash *flash, unsigned address, void *buffer, unsigned buffer_size) {

    unsigned char *buf = (unsigned char *)buffer;

    // Check parameters
    if (w25q_check_param(flash, address, buffer, buffer_size) == 0)
        return 0;

    buf[0] = W25Q_READ_DATA;
    buf[1] = (address >> 16) & 0xff;
    buf[2] = (address >> 8) & 0xff;
    buf[3] = address & 0xff;
    /* Still, 3-byte addressing... */
    flash->spi_send(buf, buf, buffer_size);
    printf("%s\n",buf);
    flash->spi_delay_func(W25Q_DELAY_TIME);
    return 1;

}

unsigned char w25q_write(struct w25q_flash *flash, unsigned address, void *buffer, unsigned buffer_size) {

    unsigned char *buf = (unsigned char *)buffer;
    unsigned char temp_buf[265];
    unsigned limit;

    // Check parameters
    if (w25q_check_param(flash, address, buffer, buffer_size) == 0)
        return 0;

    for (unsigned programmed_bytes = 0; programmed_bytes < buffer_size; ) {
        if (buffer_size - programmed_bytes <= 256) {
            limit = buffer_size - programmed_bytes;
        } else {
            limit = 256;
        }
        memcpy(&temp_buf[4], &buf[programmed_bytes], limit);
        /* Here we should have a function to check potential timeouts */
        programmed_bytes += w25q_page_program(flash, address + programmed_bytes, temp_buf, limit + 4);
    }

    return 1;
    
}

unsigned char w25q_erase(struct w25q_flash *flash, unsigned start_address, unsigned end_address) {
    
    if (end_address <= start_address)
        return 0;

    if ((end_address >> 8) > flash->size)
        return 0;

    while (start_address < end_address) {
        w25q_sector_erase(flash, start_address);
        start_address += 4096;
    }

    return 1;
}

unsigned char w25q_erase_all(struct w25q_flash *flash) {

    unsigned char cmd = W25Q_CHIP_ERASE;

    if (flash == NULL) {
        return 0;
    }
    w25q_write_enable(flash);
    w25q_wait_until_available(flash);

    flash->spi_delay_func(W25Q_DELAY_TIME);
    flash->spi_send(&cmd, &cmd, 1);
    flash->spi_delay_func(W25Q_DELAY_TIME);

    w25q_wait_until_available(flash);
    w25q_write_disable(flash);

    return 1;

}
