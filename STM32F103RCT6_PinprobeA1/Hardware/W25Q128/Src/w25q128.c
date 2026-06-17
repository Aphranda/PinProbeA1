/*
 * w25q128.c
 *
 * Polling SPI driver for W25Q128-compatible external NOR flash.
 */

#include "w25q128.h"

#include "main.h"
#include "spi.h"

#include <string.h>

#define W25Q128_CMD_WRITE_ENABLE       0x06U
#define W25Q128_CMD_WRITE_DISABLE      0x04U
#define W25Q128_CMD_READ_STATUS1       0x05U
#define W25Q128_CMD_READ_STATUS2       0x35U
#define W25Q128_CMD_PAGE_PROGRAM       0x02U
#define W25Q128_CMD_READ_DATA          0x03U
#define W25Q128_CMD_FAST_READ          0x0BU
#define W25Q128_CMD_SECTOR_ERASE       0x20U
#define W25Q128_CMD_BLOCK_ERASE_32K    0x52U
#define W25Q128_CMD_BLOCK_ERASE_64K    0xD8U
#define W25Q128_CMD_CHIP_ERASE         0xC7U
#define W25Q128_CMD_JEDEC_ID           0x9FU
#define W25Q128_CMD_UNIQUE_ID          0x4BU
#define W25Q128_CMD_POWER_DOWN         0xB9U
#define W25Q128_CMD_RELEASE_POWER_DOWN 0xABU

#define W25Q128_STATUS_BUSY            0x01U
#define W25Q128_STATUS_WEL             0x02U
#define W25Q128_SPI_TIMEOUT_MS         10U
#define W25Q128_WRITE_TIMEOUT_MS       1000U
#define W25Q128_ERASE_TIMEOUT_MS       5000U
#define W25Q128_BLOCK_ERASE_TIMEOUT_MS 20000U
#define W25Q128_CHIP_ERASE_TIMEOUT_MS  200000U

extern SPI_HandleTypeDef hspi3;

static W25Q128_YieldCallback_t yield_callback;

static void yield_if_needed(void)
{
    if (yield_callback != NULL) {
        yield_callback();
    }
}

void W25Q128_SetYieldCallback(W25Q128_YieldCallback_t callback)
{
    yield_callback = callback;
}

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

static W25Q128_Status_t transmit_command(uint8_t command)
{
    cs_low();
    HAL_StatusTypeDef hal = HAL_SPI_Transmit(&hspi3, &command, 1U, W25Q128_SPI_TIMEOUT_MS);
    cs_high();
    return hal_to_status(hal);
}

static W25Q128_Status_t read_status(uint8_t command, uint8_t *status)
{
    if (status == NULL) {
        return W25Q128_ERR_PARAM;
    }

    cs_low();
    HAL_StatusTypeDef hal = HAL_SPI_Transmit(&hspi3, &command, 1U, W25Q128_SPI_TIMEOUT_MS);
    if (hal == HAL_OK) {
        hal = HAL_SPI_Receive(&hspi3, status, 1U, W25Q128_SPI_TIMEOUT_MS);
    }
    cs_high();
    return hal_to_status(hal);
}

static W25Q128_Status_t erase_command(uint8_t command, uint32_t address, uint32_t alignment, uint32_t timeout_ms)
{
    if ((address % alignment) != 0UL) {
        return W25Q128_ERR_PARAM;
    }
    if (!W25Q128_IsAddressRangeValid(address, alignment)) {
        return W25Q128_ERR_PARAM;
    }

    W25Q128_Status_t status = W25Q128_WaitReady(timeout_ms);
    if (status != W25Q128_OK) {
        return status;
    }
    status = W25Q128_WriteEnable();
    if (status != W25Q128_OK) {
        return status;
    }

    uint8_t cmd[4];
    cmd[0] = command;
    cmd[1] = (uint8_t)(address >> 16);
    cmd[2] = (uint8_t)(address >> 8);
    cmd[3] = (uint8_t)address;

    cs_low();
    HAL_StatusTypeDef hal = HAL_SPI_Transmit(&hspi3, cmd, sizeof(cmd), W25Q128_SPI_TIMEOUT_MS);
    cs_high();
    if (hal != HAL_OK) {
        return W25Q128_ERR_HAL;
    }
    return W25Q128_WaitReady(timeout_ms);
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

W25Q128_Status_t W25Q128_ReadStatus1(uint8_t *status)
{
    return read_status(W25Q128_CMD_READ_STATUS1, status);
}

W25Q128_Status_t W25Q128_ReadStatus2(uint8_t *status)
{
    return read_status(W25Q128_CMD_READ_STATUS2, status);
}

W25Q128_Status_t W25Q128_WaitReady(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    uint8_t status = 0U;

    do {
        yield_if_needed();
        W25Q128_Status_t result = W25Q128_ReadStatus1(&status);
        if (result != W25Q128_OK) {
            return result;
        }
        if ((status & W25Q128_STATUS_BUSY) == 0U) {
            return W25Q128_OK;
        }
    } while ((HAL_GetTick() - start) < timeout_ms);

    return W25Q128_ERR_TIMEOUT;
}

W25Q128_Status_t W25Q128_WriteEnable(void)
{
    W25Q128_Status_t status = transmit_command(W25Q128_CMD_WRITE_ENABLE);
    if (status != W25Q128_OK) {
        return status;
    }

    uint8_t sr1 = 0U;
    status = W25Q128_ReadStatus1(&sr1);
    if (status != W25Q128_OK) {
        return status;
    }
    return ((sr1 & W25Q128_STATUS_WEL) != 0U) ? W25Q128_OK : W25Q128_ERR_HAL;
}

W25Q128_Status_t W25Q128_WriteDisable(void)
{
    return transmit_command(W25Q128_CMD_WRITE_DISABLE);
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

W25Q128_Status_t W25Q128_ReadJedecId(W25Q128_JedecId_t *id)
{
    if (id == NULL) {
        return W25Q128_ERR_PARAM;
    }

    uint8_t raw[3] = {0};
    W25Q128_Status_t status = W25Q128_ReadId(raw);
    if (status != W25Q128_OK) {
        return status;
    }

    id->manufacturer_id = raw[0];
    id->memory_type = raw[1];
    id->capacity = raw[2];
    return W25Q128_OK;
}

W25Q128_Status_t W25Q128_ReadUniqueId(uint8_t id[W25Q128_UNIQUE_ID_SIZE])
{
    if (id == NULL) {
        return W25Q128_ERR_PARAM;
    }

    uint8_t cmd[5] = {W25Q128_CMD_UNIQUE_ID, 0U, 0U, 0U, 0U};
    cs_low();
    HAL_StatusTypeDef hal = HAL_SPI_Transmit(&hspi3, cmd, sizeof(cmd), W25Q128_SPI_TIMEOUT_MS);
    if (hal == HAL_OK) {
        hal = HAL_SPI_Receive(&hspi3, id, W25Q128_UNIQUE_ID_SIZE, W25Q128_SPI_TIMEOUT_MS);
    }
    cs_high();
    return hal_to_status(hal);
}

static W25Q128_Status_t read_impl(uint8_t command, uint32_t address, void *data, size_t length, bool dummy_byte)
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

    uint8_t cmd[5];
    cmd[0] = command;
    cmd[1] = (uint8_t)(address >> 16);
    cmd[2] = (uint8_t)(address >> 8);
    cmd[3] = (uint8_t)address;
    cmd[4] = 0U;
    uint16_t cmd_len = dummy_byte ? 5U : 4U;

    cs_low();
    HAL_StatusTypeDef hal = HAL_SPI_Transmit(&hspi3, cmd, cmd_len, W25Q128_SPI_TIMEOUT_MS);
    uint8_t *dst = (uint8_t *)data;
    while (hal == HAL_OK && length > 0U) {
        yield_if_needed();
        uint16_t chunk = (length > 0xFFFFU) ? 0xFFFFU : (uint16_t)length;
        hal = HAL_SPI_Receive(&hspi3, dst, chunk, W25Q128_SPI_TIMEOUT_MS);
        dst += chunk;
        length -= chunk;
    }
    cs_high();
    return hal_to_status(hal);
}

W25Q128_Status_t W25Q128_Read(uint32_t address, void *data, size_t length)
{
    return read_impl(W25Q128_CMD_READ_DATA, address, data, length, false);
}

W25Q128_Status_t W25Q128_FastRead(uint32_t address, void *data, size_t length)
{
    return read_impl(W25Q128_CMD_FAST_READ, address, data, length, true);
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

    W25Q128_Status_t status = W25Q128_WaitReady(W25Q128_WRITE_TIMEOUT_MS);
    if (status != W25Q128_OK) {
        return status;
    }
    status = W25Q128_WriteEnable();
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
    return W25Q128_WaitReady(W25Q128_WRITE_TIMEOUT_MS);
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
        yield_if_needed();
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

W25Q128_Status_t W25Q128_Verify(uint32_t address, const void *data, size_t length)
{
    if (data == NULL && length > 0U) {
        return W25Q128_ERR_PARAM;
    }
    if (!W25Q128_IsAddressRangeValid(address, length)) {
        return W25Q128_ERR_PARAM;
    }

    const uint8_t *src = (const uint8_t *)data;
    uint8_t buffer[64];
    while (length > 0U) {
        yield_if_needed();
        size_t chunk = (length > sizeof(buffer)) ? sizeof(buffer) : length;
        W25Q128_Status_t status = W25Q128_Read(address, buffer, chunk);
        if (status != W25Q128_OK) {
            return status;
        }
        if (memcmp(buffer, src, chunk) != 0) {
            return W25Q128_ERR_VERIFY;
        }
        address += (uint32_t)chunk;
        src += chunk;
        length -= chunk;
    }
    return W25Q128_OK;
}

W25Q128_Status_t W25Q128_EraseSector(uint32_t address)
{
    return erase_command(W25Q128_CMD_SECTOR_ERASE, address, W25Q128_SECTOR_SIZE, W25Q128_ERASE_TIMEOUT_MS);
}

W25Q128_Status_t W25Q128_EraseBlock32K(uint32_t address)
{
    return erase_command(W25Q128_CMD_BLOCK_ERASE_32K, address, W25Q128_BLOCK32_SIZE, W25Q128_BLOCK_ERASE_TIMEOUT_MS);
}

W25Q128_Status_t W25Q128_EraseBlock64K(uint32_t address)
{
    return erase_command(W25Q128_CMD_BLOCK_ERASE_64K, address, W25Q128_BLOCK_SIZE, W25Q128_BLOCK_ERASE_TIMEOUT_MS);
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
        yield_if_needed();
        W25Q128_Status_t status = W25Q128_EraseSector(address);
        if (status != W25Q128_OK) {
            return status;
        }
        address += W25Q128_SECTOR_SIZE;
        length -= W25Q128_SECTOR_SIZE;
    }
    return W25Q128_OK;
}

W25Q128_Status_t W25Q128_EraseChip(void)
{
    W25Q128_Status_t status = W25Q128_WaitReady(W25Q128_ERASE_TIMEOUT_MS);
    if (status != W25Q128_OK) {
        return status;
    }
    status = W25Q128_WriteEnable();
    if (status != W25Q128_OK) {
        return status;
    }
    status = transmit_command(W25Q128_CMD_CHIP_ERASE);
    if (status != W25Q128_OK) {
        return status;
    }
    return W25Q128_WaitReady(W25Q128_CHIP_ERASE_TIMEOUT_MS);
}

W25Q128_Status_t W25Q128_PowerDown(void)
{
    return transmit_command(W25Q128_CMD_POWER_DOWN);
}

W25Q128_Status_t W25Q128_ReleasePowerDown(void)
{
    return transmit_command(W25Q128_CMD_RELEASE_POWER_DOWN);
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
