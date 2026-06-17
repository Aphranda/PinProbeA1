/*
 * w25q128.c
 *
 * Polling SPI driver for W25Q128 external flash.
 */

#include "w25q128.h"

#include "main.h"
#include "spi.h"

#include <string.h>

#define W25Q128_CMD_WRITE_ENABLE       0x06U
#define W25Q128_CMD_READ_STATUS1       0x05U
#define W25Q128_CMD_PAGE_PROGRAM       0x02U
#define W25Q128_CMD_READ_DATA          0x03U
#define W25Q128_CMD_SECTOR_ERASE       0x20U
#define W25Q128_CMD_JEDEC_ID           0x9FU

#define W25Q128_STATUS_BUSY            0x01U
#define W25Q128_SPI_TIMEOUT_MS         100U
#define W25Q128_WRITE_TIMEOUT_MS       1000U
#define W25Q128_ERASE_TIMEOUT_MS       5000U

extern SPI_HandleTypeDef hspi3;

static void cs_low(void)
{
    HAL_GPIO_WritePin(SPI3_CS_GPIO_Port, SPI3_CS_Pin, GPIO_PIN_RESET);
}

static void cs_high(void)
{
    HAL_GPIO_WritePin(SPI3_CS_GPIO_Port, SPI3_CS_Pin, GPIO_PIN_SET);
}

static W25Q128_Status_t hal_to_status(HAL_StatusTypeDef status)
{
    return (status == HAL_OK) ? W25Q128_OK : W25Q128_ERR_HAL;
}

bool W25Q128_IsAddressRangeValid(uint32_t address, size_t length)
{
    if (length == 0U) {
        return true;
    }
    if (address >= W25Q128_TOTAL_SIZE) {
        return false;
    }
    return length <= (size_t)(W25Q128_TOTAL_SIZE - address);
}

static W25Q128_Status_t read_status1(uint8_t *status)
{
    if (status == NULL) {
        return W25Q128_ERR_PARAM;
    }

    uint8_t cmd = W25Q128_CMD_READ_STATUS1;
    cs_low();
    HAL_StatusTypeDef hal = HAL_SPI_Transmit(&hspi3, &cmd, 1U, W25Q128_SPI_TIMEOUT_MS);
    if (hal == HAL_OK) {
        hal = HAL_SPI_Receive(&hspi3, status, 1U, W25Q128_SPI_TIMEOUT_MS);
    }
    cs_high();
    return hal_to_status(hal);
}

static W25Q128_Status_t wait_ready(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    uint8_t status = 0U;

    do {
        W25Q128_Status_t result = read_status1(&status);
        if (result != W25Q128_OK) {
            return result;
        }
        if ((status & W25Q128_STATUS_BUSY) == 0U) {
            return W25Q128_OK;
        }
    } while ((HAL_GetTick() - start) < timeout_ms);

    return W25Q128_ERR_TIMEOUT;
}

static W25Q128_Status_t write_enable(void)
{
    uint8_t cmd = W25Q128_CMD_WRITE_ENABLE;
    cs_low();
    HAL_StatusTypeDef hal = HAL_SPI_Transmit(&hspi3, &cmd, 1U, W25Q128_SPI_TIMEOUT_MS);
    cs_high();
    return hal_to_status(hal);
}

W25Q128_Status_t W25Q128_Init(void)
{
    uint8_t id[3] = {0};
    W25Q128_Status_t status = W25Q128_ReadId(id);
    if (status != W25Q128_OK) {
        return status;
    }

    if (id[0] == 0x00U || id[0] == 0xFFU) {
        return W25Q128_ERR_ID;
    }
    return W25Q128_OK;
}

W25Q128_Status_t W25Q128_ReadId(uint8_t id[3])
{
    if (id == NULL) {
        return W25Q128_ERR_PARAM;
    }

    uint8_t cmd = W25Q128_CMD_JEDEC_ID;
    cs_low();
    HAL_StatusTypeDef hal = HAL_SPI_Transmit(&hspi3, &cmd, 1U, W25Q128_SPI_TIMEOUT_MS);
    if (hal == HAL_OK) {
        hal = HAL_SPI_Receive(&hspi3, id, 3U, W25Q128_SPI_TIMEOUT_MS);
    }
    cs_high();
    return hal_to_status(hal);
}

W25Q128_Status_t W25Q128_Read(uint32_t address, void *data, size_t length)
{
    if (data == NULL && length > 0U) {
        return W25Q128_ERR_PARAM;
    }
    if (!W25Q128_IsAddressRangeValid(address, length)) {
        return W25Q128_ERR_PARAM;
    }
    if (length == 0U) {
        return W25Q128_OK;
    }

    uint8_t cmd[4];
    cmd[0] = W25Q128_CMD_READ_DATA;
    cmd[1] = (uint8_t)(address >> 16);
    cmd[2] = (uint8_t)(address >> 8);
    cmd[3] = (uint8_t)address;

    cs_low();
    HAL_StatusTypeDef hal = HAL_SPI_Transmit(&hspi3, cmd, sizeof(cmd), W25Q128_SPI_TIMEOUT_MS);
    uint8_t *dst = (uint8_t *)data;
    while (hal == HAL_OK && length > 0U) {
        uint16_t chunk = (length > 0xFFFFU) ? 0xFFFFU : (uint16_t)length;
        hal = HAL_SPI_Receive(&hspi3, dst, chunk, W25Q128_SPI_TIMEOUT_MS);
        dst += chunk;
        length -= chunk;
    }
    cs_high();
    return hal_to_status(hal);
}

W25Q128_Status_t W25Q128_PageProgram(uint32_t address, const void *data, size_t length)
{
    if (data == NULL && length > 0U) {
        return W25Q128_ERR_PARAM;
    }
    if (length == 0U) {
        return W25Q128_OK;
    }
    if (length > W25Q128_PAGE_SIZE) {
        return W25Q128_ERR_PARAM;
    }
    if ((address / W25Q128_PAGE_SIZE) != ((address + length - 1U) / W25Q128_PAGE_SIZE)) {
        return W25Q128_ERR_PARAM;
    }
    if (!W25Q128_IsAddressRangeValid(address, length)) {
        return W25Q128_ERR_PARAM;
    }

    W25Q128_Status_t status = wait_ready(W25Q128_WRITE_TIMEOUT_MS);
    if (status != W25Q128_OK) {
        return status;
    }
    status = write_enable();
    if (status != W25Q128_OK) {
        return status;
    }

    uint8_t cmd[4];
    cmd[0] = W25Q128_CMD_PAGE_PROGRAM;
    cmd[1] = (uint8_t)(address >> 16);
    cmd[2] = (uint8_t)(address >> 8);
    cmd[3] = (uint8_t)address;

    cs_low();
    HAL_StatusTypeDef hal = HAL_SPI_Transmit(&hspi3, cmd, sizeof(cmd), W25Q128_SPI_TIMEOUT_MS);
    if (hal == HAL_OK) {
        hal = HAL_SPI_Transmit(&hspi3, (uint8_t *)data, (uint16_t)length, W25Q128_SPI_TIMEOUT_MS);
    }
    cs_high();

    if (hal != HAL_OK) {
        return W25Q128_ERR_HAL;
    }
    return wait_ready(W25Q128_WRITE_TIMEOUT_MS);
}

W25Q128_Status_t W25Q128_Write(uint32_t address, const void *data, size_t length)
{
    if (data == NULL && length > 0U) {
        return W25Q128_ERR_PARAM;
    }
    if (!W25Q128_IsAddressRangeValid(address, length)) {
        return W25Q128_ERR_PARAM;
    }

    const uint8_t *src = (const uint8_t *)data;
    while (length > 0U) {
        size_t page_remaining = W25Q128_PAGE_SIZE - (address % W25Q128_PAGE_SIZE);
        size_t chunk = (length < page_remaining) ? length : page_remaining;

        W25Q128_Status_t status = W25Q128_PageProgram(address, src, chunk);
        if (status != W25Q128_OK) {
            return status;
        }

        address += (uint32_t)chunk;
        src += chunk;
        length -= chunk;
    }

    return W25Q128_OK;
}

W25Q128_Status_t W25Q128_EraseSector(uint32_t address)
{
    if ((address % W25Q128_SECTOR_SIZE) != 0UL) {
        return W25Q128_ERR_PARAM;
    }
    if (!W25Q128_IsAddressRangeValid(address, W25Q128_SECTOR_SIZE)) {
        return W25Q128_ERR_PARAM;
    }

    W25Q128_Status_t status = wait_ready(W25Q128_ERASE_TIMEOUT_MS);
    if (status != W25Q128_OK) {
        return status;
    }
    status = write_enable();
    if (status != W25Q128_OK) {
        return status;
    }

    uint8_t cmd[4];
    cmd[0] = W25Q128_CMD_SECTOR_ERASE;
    cmd[1] = (uint8_t)(address >> 16);
    cmd[2] = (uint8_t)(address >> 8);
    cmd[3] = (uint8_t)address;

    cs_low();
    HAL_StatusTypeDef hal = HAL_SPI_Transmit(&hspi3, cmd, sizeof(cmd), W25Q128_SPI_TIMEOUT_MS);
    cs_high();
    if (hal != HAL_OK) {
        return W25Q128_ERR_HAL;
    }
    return wait_ready(W25Q128_ERASE_TIMEOUT_MS);
}

W25Q128_Status_t W25Q128_EraseRange(uint32_t address, size_t length)
{
    if (length == 0U) {
        return W25Q128_OK;
    }
    if ((address % W25Q128_SECTOR_SIZE) != 0UL ||
        (length % W25Q128_SECTOR_SIZE) != 0U) {
        return W25Q128_ERR_PARAM;
    }
    if (!W25Q128_IsAddressRangeValid(address, length)) {
        return W25Q128_ERR_PARAM;
    }

    while (length > 0U) {
        W25Q128_Status_t status = W25Q128_EraseSector(address);
        if (status != W25Q128_OK) {
            return status;
        }
        address += W25Q128_SECTOR_SIZE;
        length -= W25Q128_SECTOR_SIZE;
    }
    return W25Q128_OK;
}

const char *W25Q128_StatusName(W25Q128_Status_t status)
{
    switch (status) {
    case W25Q128_OK: return "OK";
    case W25Q128_ERR_PARAM: return "PARAM";
    case W25Q128_ERR_TIMEOUT: return "TIMEOUT";
    case W25Q128_ERR_HAL: return "HAL";
    case W25Q128_ERR_ID: return "ID";
    case W25Q128_ERR_VERIFY: return "VERIFY";
    default: return "UNKNOWN";
    }
}
