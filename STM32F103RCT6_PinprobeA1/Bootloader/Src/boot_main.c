/*
 * PinProbe A1 minimal OTA bootloader.
 */

#include "main.h"

#include "flash.h"
#include "ota_manifest.h"
#include "ota_boot_request.h"
#include "w25q128.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define BOOT_SRAM_BASE          0x20000000UL
#define BOOT_SRAM_SIZE          0x0000C000UL
#define BOOT_APP_FLASH_END      (OTA_APP_BASE_ADDR + OTA_APP_MAX_SIZE)
#define BOOT_COPY_CHUNK_SIZE    256U
#define BOOT_CRC32_POLYNOMIAL   0xEDB88320UL

SPI_HandleTypeDef hspi3;

static bool boot_diag_uart_enabled;

static void SystemClock_Config(void);
static void Boot_GPIO_Init(void);
static void Boot_USART1_Init(void);
static void Boot_Diag(const char *text);
static void Boot_Puts(const char *text);
static bool Boot_SPI3_Init(void);
static void Boot_ExtendWatchdog(void);
static void Boot_KickWatchdog(void);
static bool Boot_ReadDiagConfig(void);
static uint32_t Boot_ConfigCrc32(const uint32_t *data, size_t word_count);
static uint32_t Boot_GetOtaRequest(void);
static bool Boot_IsValidVector(uint32_t stack, uint32_t reset);
static bool Boot_IsValidApp(uint32_t app_base);
static bool Boot_IsValidSlotImage(const OTA_Manifest_t *manifest);
static void Boot_JumpToApp(void);
static bool Boot_LoadPending(OTA_BootFlags_t *flags, OTA_Manifest_t *manifest);
static bool Boot_VerifySlotCrc(const OTA_Manifest_t *manifest);
static bool Boot_CopySlotToApp(const OTA_Manifest_t *manifest);
static bool Boot_UpdateFlags(OTA_BootFlags_t *flags, uint8_t state, uint32_t last_error);

int main(void)
{
    HAL_Init();
    Boot_ExtendWatchdog();
    SystemClock_Config();
    Boot_ExtendWatchdog();
    Boot_KickWatchdog();

    OTA_BootFlags_t flags;
    OTA_Manifest_t manifest;
    uint32_t requested_sequence = Boot_GetOtaRequest();
    if (requested_sequence != 0UL) {
        boot_diag_uart_enabled = Boot_ReadDiagConfig();
        Boot_GPIO_Init();
        if (boot_diag_uart_enabled) {
            Boot_USART1_Init();
        }
        Boot_Diag("\r\nR\r\n");
        if (Boot_SPI3_Init()) {
            Boot_Diag("S\r\n");
            W25Q128_SetYieldCallback(Boot_KickWatchdog);
            W25Q128_Status_t flash_status = W25Q128_Init();
            Boot_Diag((flash_status == W25Q128_OK) ? "I\r\n" : "i\r\n");
            if (flash_status == W25Q128_OK && Boot_LoadPending(&flags, &manifest)) {
                Boot_Diag("F\r\n");
                if (manifest.sequence == requested_sequence) {
                    bool copy_started = false;
                    bool copy_ok = false;
                    flags.attempt_count++;

                    bool writing_flags_ok = Boot_UpdateFlags(&flags, OTA_BOOT_STATE_WRITING, OTA_OK);
                    bool crc_ok = false;
                    bool slot_ok = false;
                    if (!writing_flags_ok) {
                        Boot_Diag("w\r\n");
                    } else {
                        Boot_Diag("C\r\n");
                        crc_ok = Boot_VerifySlotCrc(&manifest);
                        if (crc_ok) {
                            slot_ok = Boot_IsValidSlotImage(&manifest);
                            if (slot_ok) {
                                Boot_Diag("P\r\n");
                                copy_started = true;
                                copy_ok = Boot_CopySlotToApp(&manifest);
                            } else {
                                Boot_Diag("V\r\n");
                            }
                        }
                    }
                    if (writing_flags_ok && crc_ok && slot_ok && copy_ok) {
                        if (Boot_UpdateFlags(&flags, OTA_BOOT_STATE_WRITTEN, OTA_OK)) {
                            Boot_Diag("D\r\n");
                            OTA_BootRequestClear();
                        } else {
                            Boot_Diag("W\r\n");
                        }
                    } else {
                        uint32_t error = OTA_ERR_BOOT_FAIL;
                        if (!writing_flags_ok) {
                            error = OTA_ERR_FLASH_FAIL;
                        } else if (!crc_ok) {
                            error = OTA_ERR_CRC_FAIL;
                        } else if (!slot_ok) {
                            error = OTA_ERR_BAD_MANIFEST;
                        }
                        Boot_Diag("X\r\n");
                        if (copy_started) {
                            (void)error;
                        } else {
                            (void)Boot_UpdateFlags(&flags, OTA_BOOT_STATE_FAILED, error);
                            OTA_BootRequestClear();
                        }
                    }
                } else {
                    Boot_Diag("Q\r\n");
                    OTA_BootRequestClear();
                }
            } else {
                Boot_Diag("N\r\n");
                OTA_BootRequestClear();
            }
        } else {
            Boot_Diag("s\r\n");
            OTA_BootRequestClear();
        }
    }

    if (requested_sequence != 0UL) {
        Boot_Diag("J\r\n");
    }
    Boot_JumpToApp();

    while (1) {
    }
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        Error_Handler();
    }

    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0) != HAL_OK) {
        Error_Handler();
    }
}

static void Boot_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_AFIO_REMAP_SWJ_NOJTAG();

    HAL_GPIO_WritePin(SPI3_CS_GPIO_Port, SPI3_CS_Pin, GPIO_PIN_SET);
    gpio.Pin = SPI3_CS_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI3_CS_GPIO_Port, &gpio);
}

static bool Boot_SPI3_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_SPI3_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_3 | GPIO_PIN_5;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);

    gpio.Pin = GPIO_PIN_4;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &gpio);

    hspi3.Instance = SPI3;
    hspi3.Init.Mode = SPI_MODE_MASTER;
    hspi3.Init.Direction = SPI_DIRECTION_2LINES;
    hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi3.Init.NSS = SPI_NSS_SOFT;
    hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi3.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi3) != HAL_OK) {
        return false;
    }
    return true;
}

static void Boot_USART1_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    USART1->CR1 = 0U;
    USART1->BRR = (uint16_t)(8000000UL / 115200UL);
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

static void Boot_Diag(const char *text)
{
    if (boot_diag_uart_enabled) {
        Boot_Puts(text);
    }
}

static void Boot_Puts(const char *text)
{
    if (text == NULL) {
        return;
    }
    while (*text != '\0') {
        while ((USART1->SR & USART_SR_TXE) == 0U) {
            Boot_KickWatchdog();
        }
        USART1->DR = (uint16_t)(uint8_t)*text;
        Boot_KickWatchdog();
        ++text;
    }
    while ((USART1->SR & USART_SR_TC) == 0U) {
        Boot_KickWatchdog();
    }
}

static void Boot_ExtendWatchdog(void)
{
    uint32_t timeout;

    IWDG->KR = 0x5555U;
    timeout = 100000UL;
    while (((IWDG->SR & (IWDG_SR_PVU | IWDG_SR_RVU)) != 0U) && (timeout > 0UL)) {
        IWDG->KR = 0xAAAAU;
        --timeout;
    }

    IWDG->PR = IWDG_PRESCALER_256;
    IWDG->RLR = 0x0FFFU;

    timeout = 100000UL;
    while (((IWDG->SR & (IWDG_SR_PVU | IWDG_SR_RVU)) != 0U) && (timeout > 0UL)) {
        IWDG->KR = 0xAAAAU;
        --timeout;
    }
    IWDG->KR = 0xAAAAU;
}

static void Boot_KickWatchdog(void)
{
    IWDG->KR = 0xAAAAU;
}

static bool Boot_ReadDiagConfig(void)
{
    const Flash_Config_t *cfg = (const Flash_Config_t *)FLASH_CONFIG_ADDR;
    uint32_t stored_major = (cfg->version >> 16) & 0xFFFFUL;
    uint32_t current_major = (FLASH_CONFIG_VERSION >> 16) & 0xFFFFUL;
    size_t crc_cover_size = offsetof(Flash_Config_t, crc);
    size_t crc_cover_words = crc_cover_size / sizeof(uint32_t);

    if (cfg->magic != FLASH_CONFIG_MAGIC || stored_major != current_major) {
        return false;
    }
    if (Boot_ConfigCrc32((const uint32_t *)cfg, crc_cover_words) != cfg->crc) {
        return false;
    }
    return cfg->boot_diag_uart != 0U;
}

static uint32_t Boot_ConfigCrc32(const uint32_t *data, size_t word_count)
{
    uint32_t crc = 0xFFFFFFFFUL;

    for (size_t i = 0U; i < word_count; ++i) {
        crc ^= data[i];
        for (uint32_t bit = 0U; bit < 32U; ++bit) {
            if ((crc & 0x80000000UL) != 0UL) {
                crc = (crc << 1) ^ BOOT_CRC32_POLYNOMIAL;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc ^ 0xFFFFFFFFUL;
}

static uint32_t Boot_GetOtaRequest(void)
{
    return OTA_BootRequestGet();
}

static bool Boot_IsValidApp(uint32_t app_base)
{
    uint32_t stack = *(const uint32_t *)app_base;
    uint32_t reset = *(const uint32_t *)(app_base + 4UL);

    return Boot_IsValidVector(stack, reset);
}

static bool Boot_IsValidVector(uint32_t stack, uint32_t reset)
{
    if (stack < BOOT_SRAM_BASE || stack > (BOOT_SRAM_BASE + BOOT_SRAM_SIZE)) {
        return false;
    }
    if (reset < OTA_APP_BASE_ADDR || reset >= BOOT_APP_FLASH_END || ((reset & 1UL) == 0UL)) {
        return false;
    }
    return true;
}

static bool Boot_IsValidSlotImage(const OTA_Manifest_t *manifest)
{
    uint32_t vector[2];

    if (manifest == NULL ||
        manifest->image_size < sizeof(vector) ||
        manifest->slot_addr != OTA_SLOT_A_ADDR) {
        return false;
    }
    if (W25Q128_Read(manifest->slot_addr, vector, sizeof(vector)) != W25Q128_OK) {
        return false;
    }
    return Boot_IsValidVector(vector[0], vector[1]);
}

static void Boot_JumpToApp(void)
{
    if (!Boot_IsValidApp(OTA_APP_BASE_ADDR)) {
        Error_Handler();
    }

    uint32_t app_stack = *(const uint32_t *)OTA_APP_BASE_ADDR;
    uint32_t app_reset = *(const uint32_t *)(OTA_APP_BASE_ADDR + 4UL);
    void (*app_entry)(void) = (void (*)(void))app_reset;

    HAL_RCC_DeInit();
    HAL_DeInit();
    __disable_irq();
    for (uint32_t i = 0U; i < 8U; ++i) {
        NVIC->ICER[i] = 0xFFFFFFFFUL;
        NVIC->ICPR[i] = 0xFFFFFFFFUL;
    }
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;
    SCB->VTOR = OTA_APP_BASE_ADDR;
    __set_MSP(app_stack);
    __enable_irq();
    app_entry();
}

static bool Boot_LoadPending(OTA_BootFlags_t *flags, OTA_Manifest_t *manifest)
{
    if (flags == NULL || manifest == NULL) {
        return false;
    }
    if (W25Q128_Read(OTA_BOOT_FLAGS_ADDR, flags, sizeof(*flags)) != W25Q128_OK ||
        W25Q128_Read(OTA_METADATA_A_ADDR, manifest, sizeof(*manifest)) != W25Q128_OK) {
        return false;
    }
    if (!OTA_BootFlagsValidate(flags) || !OTA_ManifestValidate(manifest)) {
        return false;
    }
    if (flags->pending_slot != (uint8_t)OTA_SLOT_A ||
        flags->active_manifest_sequence != manifest->sequence) {
        return false;
    }
    if (flags->update_state != (uint8_t)OTA_BOOT_STATE_PENDING &&
        flags->update_state != (uint8_t)OTA_BOOT_STATE_WRITING &&
        flags->update_state != (uint8_t)OTA_BOOT_STATE_WRITTEN) {
        return false;
    }
    if (flags->update_state != (uint8_t)OTA_BOOT_STATE_WRITING &&
        flags->attempt_count >= flags->max_attempts) {
        return false;
    }
    return true;
}

static bool Boot_VerifySlotCrc(const OTA_Manifest_t *manifest)
{
    uint8_t buffer[BOOT_COPY_CHUNK_SIZE];
    uint32_t remaining;
    uint32_t offset = 0UL;
    uint32_t crc = 0xFFFFFFFFUL;

    if (manifest == NULL ||
        manifest->image_size == 0UL ||
        manifest->image_size > OTA_APP_MAX_SIZE ||
        manifest->slot_addr != OTA_SLOT_A_ADDR) {
        return false;
    }

    remaining = manifest->image_size;
    while (remaining > 0UL) {
        uint32_t chunk = (remaining > sizeof(buffer)) ? sizeof(buffer) : remaining;
        if (W25Q128_Read(manifest->slot_addr + offset, buffer, chunk) != W25Q128_OK) {
            return false;
        }

        for (uint32_t i = 0UL; i < chunk; ++i) {
            if ((i & 0x3FU) == 0UL) {
                Boot_KickWatchdog();
            }
            crc ^= buffer[i];
            for (uint8_t bit = 0U; bit < 8U; ++bit) {
                if ((crc & 1UL) != 0UL) {
                    crc = (crc >> 1) ^ 0xEDB88320UL;
                } else {
                    crc >>= 1;
                }
            }
        }

        offset += chunk;
        remaining -= chunk;
    }

    crc ^= 0xFFFFFFFFUL;
    return crc == manifest->image_crc32;
}

static bool Boot_EraseApp(uint32_t image_size)
{
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0UL;
    uint32_t erase_size = (image_size + FLASH_PAGE_SIZE - 1UL) & ~(FLASH_PAGE_SIZE - 1UL);
    uint32_t page_count = erase_size / FLASH_PAGE_SIZE;

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.Banks = FLASH_BANK_1;
    erase.NbPages = 1UL;

    HAL_FLASH_Unlock();
    for (uint32_t page = 0UL; page < page_count; ++page) {
        Boot_KickWatchdog();
        erase.PageAddress = OTA_APP_BASE_ADDR + (page * FLASH_PAGE_SIZE);
        if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK) {
            HAL_FLASH_Lock();
            return false;
        }
    }
    HAL_FLASH_Lock();
    Boot_KickWatchdog();
    return true;
}

static bool Boot_ProgramHalfWords(uint32_t address, const uint8_t *data, uint32_t length)
{
    HAL_FLASH_Unlock();
    for (uint32_t offset = 0UL; offset < length; offset += 2UL) {
        if ((offset & 0x3FUL) == 0UL) {
            Boot_KickWatchdog();
        }
        uint16_t halfword = data[offset];
        if ((offset + 1UL) < length) {
            halfword |= (uint16_t)data[offset + 1UL] << 8;
        } else {
            halfword |= 0xFF00U;
        }

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                              address + offset,
                              halfword) != HAL_OK) {
            HAL_FLASH_Lock();
            return false;
        }
    }
    HAL_FLASH_Lock();
    Boot_KickWatchdog();
    return true;
}

static bool Boot_CopySlotToApp(const OTA_Manifest_t *manifest)
{
    uint8_t buffer[BOOT_COPY_CHUNK_SIZE];
    uint32_t remaining;
    uint32_t offset = 0UL;

    if (manifest == NULL ||
        manifest->image_size == 0UL ||
        manifest->image_size > OTA_APP_MAX_SIZE ||
        manifest->slot_addr != OTA_SLOT_A_ADDR) {
        return false;
    }
    if (!Boot_EraseApp(manifest->image_size)) {
        return false;
    }

    remaining = manifest->image_size;
    while (remaining > 0UL) {
        uint32_t chunk = (remaining > sizeof(buffer)) ? sizeof(buffer) : remaining;
        if (W25Q128_Read(manifest->slot_addr + offset, buffer, chunk) != W25Q128_OK) {
            return false;
        }
        Boot_KickWatchdog();
        if (!Boot_ProgramHalfWords(OTA_APP_BASE_ADDR + offset, buffer, chunk)) {
            return false;
        }
        if (memcmp((const void *)(OTA_APP_BASE_ADDR + offset), buffer, chunk) != 0) {
            return false;
        }
        Boot_KickWatchdog();
        offset += chunk;
        remaining -= chunk;
    }

    return Boot_IsValidApp(OTA_APP_BASE_ADDR);
}

static bool Boot_UpdateFlags(OTA_BootFlags_t *flags, uint8_t state, uint32_t last_error)
{
    if (flags == NULL) {
        return false;
    }

    flags->update_state = state;
    flags->last_error = last_error;
    OTA_BootFlagsFinalize(flags);

    if (W25Q128_EraseSector(OTA_BOOT_FLAGS_ADDR) != W25Q128_OK) {
        return false;
    }
    if (W25Q128_Write(OTA_BOOT_FLAGS_ADDR, flags, sizeof(*flags)) != W25Q128_OK) {
        return false;
    }
    return true;
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {
        Boot_KickWatchdog();
    }
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}
