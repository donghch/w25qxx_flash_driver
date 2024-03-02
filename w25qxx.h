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

#ifndef _W25QXX_H_
#define _W25QXX_H_

/* SPI delay time in ms*/
#define W25Q_DELAY_TIME 1

/* Flash Producer ID */
#define W25Q_PRODUCER_ID 0xef

#ifdef __cplusplus
extern "C" {
#endif

/* Device Definitions(ID + Size) */

enum w25q_id_t {
    W25Q10_ID = 0x4011, 
    W25Q20_ID, 
    W25Q40_ID, 
    W25Q80_ID, 
    W25Q16_ID, 
    W25Q32_ID, 
    W25Q64_ID, 
    W25Q128_ID, 
    W25Q256_ID, 
    W25Q512_ID
};

/* Opcode Definitions */
enum w25q_opcode_t {
    W25Q_WRITE_ENABLE = 0x6, 
    W25Q_VOLATILE_SR_WRITE_ENABLE = 0x50, 
    W25Q_WRITE_DISABLE = 0x4, 
    W25Q_READ_STATUS_REG_1 = 0x5, 
    W25Q_READ_STATUS_REG_2 = 0x35, 
    W25Q_READ_JEDEC_ID = 0x9f, 
    W25Q_READ_DATA = 0x3, 
    W25Q_PAGE_PROGRAM = 0x2, 
    W25Q_ENABLE_RESET = 0x66, 
    W25Q_RESET = 0x99, 
    W25Q_POWER_DOWN = 0xb9, 
    W25Q_CHIP_ERASE = 0xc7, 
    W25Q_SECTOR_ERASE = 0x20, 
    W25Q_32K_BLK_ERASE = 0x52, 
    W25Q_64K_BLK_ERASE = 0xd8
    //More need to be added...
};

/* Size map for different size models (Unit: 256b pages)*/
enum w25q_size_t {
    W25Q10_SIZE = 512,
    W25Q20_SIZE = 1024, 
    W25Q40_SIZE = 2048, 
    W25Q80_SIZE = 4096, 
    W25Q16_SIZE = 8192, 
    W25Q32_SIZE = 16384, 
    W25Q64_SIZE = 32768, 
    W25Q128_SIZE = 65536
};

#ifdef W25Q_MEMORY_MANAGEMENT

/* Extended functionality on flash memory usage management */

/**
 * @brief Memeory mapping representation
*/
struct w25q_memory_map {
    unsigned char *mapping;
    unsigned short size;
};

#endif 


/* User-defined functions */
typedef void (*w25q_spi_transfer_fn)(void *data_in, void *data_out, unsigned size);      // SPI data transmission function
typedef void * (*w25q_memory_allocator)(unsigned size);       // Memory allocation function
typedef void (*w25q_memory_free_fn)(void *p);                        // Memory free function
typedef void (*w25q_debug_printer)(char *data);                     // Debug print function
typedef void (*w25q_delay_fn)(unsigned t);                          // Time delay function

struct w25q_flash {
    enum w25q_id_t model;
    enum w25q_size_t size;
    w25q_spi_transfer_fn spi_send;
    w25q_delay_fn spi_delay_func;
    #ifdef W25Q_MEMORY_MANAGEMENT
    struct w25q_memory_map *mem_map;
    #endif
};

/* Function definitions */

#ifdef W25Q_MEMORY_MANAGEMENT

/* Memory Management Functions */
/**
 * @brief Change a sector's status in spi memory mapping
 * @param[in] flash SPI flash instance
 * @param[in] sector Sector number
 * @param[in] used Flag to mark whether it is being used
 * 
 * @return 0 When sector is out of range, otherwise 1
*/
unsigned char w25q_mem_mark_sector(struct w25q_flash *flash, unsigned short sector, unsigned char used);

/**
 * @brief Check if a sector is being used
*/
unsigned char w25q_mem_check_sector(struct w25q_flash *flash, unsigned short sector);

/**
 * @brief Print SPI flash info and usage (Debugging purpose)
 * 
 * @param[in] flash Flash instance
 * @param[in] printer Debug message printer function
*/
void w25q_debug_mem_summary(struct w25q_flash *flash, w25q_debug_printer printer);

#endif

/* Flash Functions */

/**
 * @brief Mount SPI Flash
 * 
 * @param[in] flash SPI flash instance
 * @param[in] spi_data_func SPI data transfer function
 * @param[in] delay_fn Delay function
 * 
 * @return A struct representing a w25qxx flash or NULL
 * 
 * @note When calling the function, you can either pass a existing flash instance or a memory allocator. When an 
 * existing instance is given, values will be assigned to that instance and return NULL;
 * The function prefers the existing instance when both arguments are available.
*/
struct w25q_flash * w25q_mount(struct w25q_flash *flash, w25q_spi_transfer_fn spi_data_func, w25q_delay_fn delay_fn);
/**
 * @brief Read bytes starting from a specific address
 * 
 * @param[in] flash SPI flash instance
 * @param[in] address SPI flash address
 * @param[out] buffer Target buffer, must be valid pointer.Its size must be "data size" + 4 for instruction purpose.
 *                    Returned data will be available at buffer[4]
 * @param[in] buffer_size Target buffer size
 * 
 * @return 1 on success, 0 on failure
*/
unsigned char w25q_read(struct w25q_flash *flash, unsigned address, void *buffer, unsigned buffer_size);

/**
 * @brief Write bytes to SPI flash staring from a specific address
 * 
 * @param[in] flash SPI flash instance
 * @param[in] address SPI flash address
 * @param[in] buffer Source buffer, must be valid pointer
 * @param[in] buffer_size Source buffer size
 * 
 * @return 1 on success, 0 on failure
*/
unsigned char w25q_write(struct w25q_flash *flash, unsigned address, void *buffer, unsigned buffer_size);

/**
 * @brief Erase data on SPI based on given address range
 * 
 * @param[in] flash SPI flash instance
 * @param[in] start_address Start address, must be 4k-aligned
 * @param[in] end_address End address, must be 4k-aligned
 * 
 * @return 1 on success, 0 on failure
*/
unsigned char w25q_erase(struct w25q_flash *flash, unsigned start_address, unsigned end_address);

/**
 * @brief Erase the entire SPI flash chip
 * 
 * @param[in] flash SPI flash instance
 * 
 * @return 1 on success, 0 on failure
*/
unsigned char w25q_erase_all(struct w25q_flash *flash);

/* Some temp helper functions */

enum w25q_id_t w25q_check_model(w25q_spi_transfer_fn handler);

/* Expose helper functions for testing purpose */
#ifdef TEST

unsigned char w25q_check_param(struct w25q_flash *flash, unsigned address, void *buffer, unsigned buffer_size);

void w25q_read_status_regs(struct w25q_flash *flash, void *buffer);

void w25q_write_enable(struct w25q_flash *flash);

void w25q_write_disable(struct w25q_flash *flash);

void w25q_sr_write_enable(struct w25q_flash *flash);

void w25q_read_jedec(struct w25q_flash *flash, void *buffer);

void w25q_wait_until_available(struct w25q_flash *flash);

unsigned short w25q_page_program(struct w25q_flash *flash, unsigned address, void *buffer, unsigned char buffer_size);

unsigned char w25q_sector_erase(struct w25q_flash *flash, unsigned address);

unsigned char w25q_32k_blk_erase(struct w25q_flash *flash, unsigned address);

unsigned char w25q_64k_blk_erase(struct w25q_flash *flash, unsigned address);

#endif



#ifdef __cplusplus
}
#endif


#endif

