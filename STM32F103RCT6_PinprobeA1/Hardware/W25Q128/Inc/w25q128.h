/*
 * w25q128.h
 *
 * Polling SPI driver for W25Q128-compatible external NOR flash.
 */

#ifndef HARDWARE_W25Q128_INC_W25Q128_H_
#define HARDWARE_W25Q128_INC_W25Q128_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define W25Q128_TOTAL_SIZE      0x1000000UL
#define W25Q128_SECTOR_SIZE     0x1000UL
#define W25Q128_BLOCK32_SIZE    0x8000UL
#define W25Q128_BLOCK_SIZE      0x10000UL
#define W25Q128_PAGE_SIZE       256U
#define W25Q128_UNIQUE_ID_SIZE  8U

typedef enum {
    W25Q128_OK = 0,
    W25Q128_ERR_PARAM,
    W25Q128_ERR_TIMEOUT,
    W25Q128_ERR_HAL,
    W25Q128_ERR_ID,
    W25Q128_ERR_VERIFY,
} W25Q128_Status_t;

typedef struct {
    uint8_t manufacturer_id;
    uint8_t memory_type;
    uint8_t capacity;
} W25Q128_JedecId_t;

typedef void (*W25Q128_YieldCallback_t)(void);

void W25Q128_SetYieldCallback(W25Q128_YieldCallback_t callback);
W25Q128_Status_t W25Q128_Init(void);
W25Q128_Status_t W25Q128_ReadId(uint8_t id[3]);
W25Q128_Status_t W25Q128_ReadJedecId(W25Q128_JedecId_t *id);
W25Q128_Status_t W25Q128_ReadUniqueId(uint8_t id[W25Q128_UNIQUE_ID_SIZE]);
W25Q128_Status_t W25Q128_ReadStatus1(uint8_t *status);
W25Q128_Status_t W25Q128_ReadStatus2(uint8_t *status);
W25Q128_Status_t W25Q128_WriteEnable(void);
W25Q128_Status_t W25Q128_WriteDisable(void);
W25Q128_Status_t W25Q128_WaitReady(uint32_t timeout_ms);
W25Q128_Status_t W25Q128_Read(uint32_t address, void *data, size_t length);
W25Q128_Status_t W25Q128_FastRead(uint32_t address, void *data, size_t length);
W25Q128_Status_t W25Q128_PageProgram(uint32_t address, const void *data, size_t length);
W25Q128_Status_t W25Q128_Write(uint32_t address, const void *data, size_t length);
W25Q128_Status_t W25Q128_Verify(uint32_t address, const void *data, size_t length);
W25Q128_Status_t W25Q128_EraseSector(uint32_t address);
W25Q128_Status_t W25Q128_EraseBlock32K(uint32_t address);
W25Q128_Status_t W25Q128_EraseBlock64K(uint32_t address);
W25Q128_Status_t W25Q128_EraseRange(uint32_t address, size_t length);
W25Q128_Status_t W25Q128_EraseChip(void);
W25Q128_Status_t W25Q128_PowerDown(void);
W25Q128_Status_t W25Q128_ReleasePowerDown(void);
bool W25Q128_IsAddressRangeValid(uint32_t address, size_t length);
const char *W25Q128_StatusName(W25Q128_Status_t status);

#ifdef __cplusplus
}
#endif

#endif /* HARDWARE_W25Q128_INC_W25Q128_H_ */
