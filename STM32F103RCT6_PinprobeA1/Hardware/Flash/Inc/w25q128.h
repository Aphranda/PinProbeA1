/*
 * w25q128.h
 *
 * Minimal W25Q128 SPI flash driver interface for OTA storage.
 */

#ifndef APP_FLASH_INC_W25Q128_H_
#define APP_FLASH_INC_W25Q128_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define W25Q128_TOTAL_SIZE      0x1000000UL
#define W25Q128_SECTOR_SIZE     0x1000UL
#define W25Q128_BLOCK_SIZE      0x10000UL
#define W25Q128_PAGE_SIZE       256U

typedef enum {
    W25Q128_OK = 0,
    W25Q128_ERR_PARAM,
    W25Q128_ERR_TIMEOUT,
    W25Q128_ERR_HAL,
    W25Q128_ERR_ID,
    W25Q128_ERR_VERIFY,
} W25Q128_Status_t;

W25Q128_Status_t W25Q128_Init(void);
W25Q128_Status_t W25Q128_ReadId(uint8_t id[3]);
W25Q128_Status_t W25Q128_Read(uint32_t address, void *data, size_t length);
W25Q128_Status_t W25Q128_PageProgram(uint32_t address, const void *data, size_t length);
W25Q128_Status_t W25Q128_Write(uint32_t address, const void *data, size_t length);
W25Q128_Status_t W25Q128_EraseSector(uint32_t address);
W25Q128_Status_t W25Q128_EraseRange(uint32_t address, size_t length);
bool W25Q128_IsAddressRangeValid(uint32_t address, size_t length);
const char *W25Q128_StatusName(W25Q128_Status_t status);

#ifdef __cplusplus
}
#endif

#endif /* APP_FLASH_INC_W25Q128_H_ */
