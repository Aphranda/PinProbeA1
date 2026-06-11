/*
 * flash.c
 *
 *  Created on: 2026-04-28
 *      Author: STM32 Project
 *  Description: Flash storage module implementation for device configuration
 *
 *  功能说明：
 *  本模块使用STM32F103RCT6的最后2KB Flash页存储设备配置信息。
 *  适配STM32F1xx HAL擦除API (PageAddress而非Page索引)。
 *  存储内容含SCPI *IDN? 标识字符串、RS485波特率、设备识别信息等。
 *  通过魔数(MAGIC)和CRC32校验判断配置是否有效。
 */

/* Includes ------------------------------------------------------------------*/
#include "flash.h"
#include "stm32f1xx_hal.h"
#include <string.h>

/* Private define ------------------------------------------------------------*/

/**
 * @brief 默认RS485波特率
 */
#define FLASH_DEFAULT_BAUDRATE      115200U

/**
 * @brief CRC32多项式 (标准IEEE 802.3)
 */
#define CRC32_POLYNOMIAL            0xEDB88320UL

/* Private variables ---------------------------------------------------------*/

/**
 * @brief 配置缓存 (RAM副本)
 * @note 所有读写操作均针对此缓存，Save()时一次性写入Flash
 */
static Flash_Config_t config_cache;

/**
 * @brief 配置有效标志
 * @note 0=无效 (未初始化或CRC错误)  1=有效
 */
static uint8_t config_valid = 0;

/* Private function prototypes -----------------------------------------------*/

static uint32_t Flash_CalculateCRC(const uint32_t *data, size_t word_count);
static Flash_Status_t Flash_VerifyConfig(const Flash_Config_t *cfg);
static Flash_Status_t Flash_ErasePage(void);
static Flash_Status_t Flash_WriteData(uint32_t address, const uint32_t *data, size_t word_count);

/* ========================================================================== */
/*              CRC32 计算 (软件实现)                                          */
/* ========================================================================== */

/**
 * @brief 计算CRC32校验值
 * @param data      数据指针 (按32位字对齐)
 * @param word_count 32位字数
 * @retval CRC32值
 * @note 使用标准CRC32 (IEEE 802.3) 多项式 0xEDB88320
 */
static uint32_t Flash_CalculateCRC(const uint32_t *data, size_t word_count)
{
    uint32_t crc = 0xFFFFFFFFUL;

    for (size_t i = 0; i < word_count; i++)
    {
        crc ^= data[i];
        for (int j = 0; j < 32; j++)
        {
            if (crc & 0x80000000UL)
                crc = (crc << 1) ^ CRC32_POLYNOMIAL;
            else
                crc = (crc << 1);
        }
    }

    return crc ^ 0xFFFFFFFFUL;
}

/* ========================================================================== */
/*              配置验证                                                       */
/* ========================================================================== */

/**
 * @brief 验证配置数据的完整性
 * @param cfg 配置指针
 * @retval FLASH_OK       验证通过
 * @retval FLASH_ERR_MAGIC 魔数不匹配
 * @retval FLASH_ERR_CRC   CRC校验失败
 */
static Flash_Status_t Flash_VerifyConfig(const Flash_Config_t *cfg)
{
    if (cfg == NULL)
        return FLASH_ERR_PARAM;

    /* 检查魔数 */
    if (cfg->magic != FLASH_CONFIG_MAGIC)
        return FLASH_ERR_MAGIC;

    /* 检查版本兼容性 (主版本号必须匹配) */
    uint32_t stored_major = (cfg->version >> 16) & 0xFFFF;
    uint32_t current_major = (FLASH_CONFIG_VERSION >> 16) & 0xFFFF;
    if (stored_major != current_major)
        return FLASH_ERR_MAGIC;

    /* CRC校验: 对整个结构体除crc字段外的数据进行校验 */
    /* CRC范围: 从 magic 到 reserved 末尾 (跳过 crc 字段) */
    size_t crc_cover_size = offsetof(Flash_Config_t, crc);
    size_t crc_cover_words = crc_cover_size / sizeof(uint32_t);

    uint32_t calculated_crc = Flash_CalculateCRC(
        (const uint32_t *)cfg, crc_cover_words);

    if (calculated_crc != cfg->crc)
        return FLASH_ERR_CRC;

    return FLASH_OK;
}

/* ========================================================================== */
/*              Flash 硬件操作                                                 */
/* ========================================================================== */

/**
 * @brief 擦除配置页
 * @note STM32F1xx HAL擦除结构使用 PageAddress (地址) 而非 Page (页号)
 * @retval FLASH_OK     成功
 * @retval FLASH_ERR_HAL HAL操作失败
 */
static Flash_Status_t Flash_ErasePage(void)
{
    HAL_StatusTypeDef hal_status;
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error = 0;

    /* 解锁Flash */
    HAL_FLASH_Unlock();

    /* 配置页擦除参数 (F1使用PageAddress地址) */
    erase_init.TypeErase   = FLASH_TYPEERASE_PAGES;
    erase_init.Banks       = FLASH_BANK_1;
    erase_init.PageAddress = FLASH_CONFIG_ADDR;
    erase_init.NbPages     = 1;

    /* 执行擦除 */
    hal_status = HAL_FLASHEx_Erase(&erase_init, &page_error);

    /* 锁定Flash */
    HAL_FLASH_Lock();

    if (hal_status != HAL_OK || page_error != 0xFFFFFFFFU)
        return FLASH_ERR_HAL;

    return FLASH_OK;
}

/**
 * @brief 向Flash写入数据 (32位字)
 * @param address    目标地址 (必须按双字对齐)
 * @param data       数据指针
 * @param word_count 32位字数
 * @retval FLASH_OK     成功
 * @retval FLASH_ERR_HAL HAL操作失败
 */
static Flash_Status_t Flash_WriteData(uint32_t address, const uint32_t *data, size_t word_count)
{
    HAL_StatusTypeDef hal_status;

    /* 解锁Flash */
    HAL_FLASH_Unlock();

    /* 逐双字(64位)编程 */
    for (size_t i = 0; i < word_count; i += 2)
    {
        uint64_t double_word;

        /* 组装64位数据 */
        double_word = data[i];
        if ((i + 1) < word_count)
            double_word |= ((uint64_t)data[i + 1] << 32);
        else
            double_word |= 0x00000000ULL;  /* 最后一个字不足时补0 */

        /* 编程双字 */
        hal_status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                                       address + (i * sizeof(uint32_t)),
                                       double_word);
        if (hal_status != HAL_OK)
        {
            HAL_FLASH_Lock();
            return FLASH_ERR_HAL;
        }
    }

    /* 锁定Flash */
    HAL_FLASH_Lock();

    return FLASH_OK;
}

/* ========================================================================== */
/*              默认配置                                                       */
/* ========================================================================== */

/**
 * @brief 加载默认配置到缓存
 * @note 不写入Flash，仅修改内存缓存
 */
void Flash_LoadDefaults(void)
{
    /* 清零整个结构体 */
    memset(&config_cache, 0, sizeof(config_cache));

    /* 设置元数据 */
    config_cache.magic   = FLASH_CONFIG_MAGIC;
    config_cache.version = FLASH_CONFIG_VERSION;

    /* 设备标识 - 默认值 */
    config_cache.device_id        = 0;        /* 0 = 未设置, 等待用户配置 */
    config_cache.hw_version       = 0x10;     /* v1.0 BCD */
    config_cache.fw_version_major = 1;
    config_cache.fw_version_minor = 0;
    config_cache.serial_number    = 0;        /* 0 = 未设置 */

    /* 设备名称 - 默认 */
    strncpy(config_cache.device_name, "PPA-SP10T", FLASH_DEVICE_NAME_LEN - 1);
    config_cache.device_name[FLASH_DEVICE_NAME_LEN - 1] = '\0';

    /* 运行配置 - 默认值 */
    config_cache.default_channel  = 0;        /* 通道1 (0-based) */
    config_cache.default_mode     = 0;        /* DIRECT模式 */
    config_cache.rs485_baudrate   = FLASH_DEFAULT_BAUDRATE;

    /* SCPI *IDN? 标识字符串 - 默认值 */
    strncpy(config_cache.scpi_idn1, "GTS",         FLASH_SCPI_IDN1_LEN - 1);
    config_cache.scpi_idn1[FLASH_SCPI_IDN1_LEN - 1] = '\0';

    strncpy(config_cache.scpi_idn2, "PINPROBEA1",  FLASH_SCPI_IDN2_LEN - 1);
    config_cache.scpi_idn2[FLASH_SCPI_IDN2_LEN - 1] = '\0';

    strncpy(config_cache.scpi_idn3, "20250626",    FLASH_SCPI_IDN3_LEN - 1);
    config_cache.scpi_idn3[FLASH_SCPI_IDN3_LEN - 1] = '\0';

    strncpy(config_cache.scpi_idn4, "V0.0.1",      FLASH_SCPI_IDN4_LEN - 1);
    config_cache.scpi_idn4[FLASH_SCPI_IDN4_LEN - 1] = '\0';

    /* 保留字段已初始化为0 (memset) */

    /* 计算CRC */
    size_t crc_cover_size = offsetof(Flash_Config_t, crc);
    size_t crc_cover_words = crc_cover_size / sizeof(uint32_t);

    config_cache.crc = Flash_CalculateCRC(
        (const uint32_t *)&config_cache, crc_cover_words);

    config_valid = 1;
}

/* ========================================================================== */
/*              公开接口实现                                                    */
/* ========================================================================== */

/**
 * @brief 初始化Flash配置模块
 *
 * 从Flash加载配置并验证完整性。
 * 若验证失败，则加载默认配置到内存缓存。
 * 应在系统启动早期调用，使其他模块能读取设备信息。
 */
Flash_Status_t Flash_Init(void)
{
    Flash_Config_t *flash_cfg = (Flash_Config_t *)FLASH_CONFIG_ADDR;

    /* 从Flash读取配置 (直接指针访问Flash映射地址) */
    memcpy(&config_cache, flash_cfg, sizeof(Flash_Config_t));

    /* 验证配置完整性 */
    Flash_Status_t status = Flash_VerifyConfig(&config_cache);

    if (status == FLASH_OK)
    {
        config_valid = 1;
    }
    else
    {
        /* 配置无效，加载默认值 */
        config_valid = 0;
        Flash_LoadDefaults();
    }

    return status;
}

/**
 * @brief 将当前配置保存到Flash
 *
 * 执行步骤:
 * 1. 验证当前缓存数据
 * 2. 重新计算CRC
 * 3. 擦除Flash页
 * 4. 写入新配置
 * 5. 回读验证
 */
Flash_Status_t Flash_Save(void)
{
    Flash_Status_t status;

    /* 更新CRC */
    size_t crc_cover_size = offsetof(Flash_Config_t, crc);
    size_t crc_cover_words = crc_cover_size / sizeof(uint32_t);

    config_cache.crc = Flash_CalculateCRC(
        (const uint32_t *)&config_cache, crc_cover_words);

    /* 自验证 */
    status = Flash_VerifyConfig(&config_cache);
    if (status != FLASH_OK)
        return status;

    /* 确保当前配置大小不超过页大小 */
    uint32_t write_size = FLASH_CONFIG_SIZE;
    if (write_size > FLASH_CONFIG_PAGE_SIZE)
        return FLASH_ERR_PARAM;

    /* 准备写入缓冲 (补齐到双字对齐) */
    uint32_t write_buffer[FLASH_CONFIG_PAGE_SIZE / sizeof(uint32_t)];
    memset(write_buffer, 0xFF, sizeof(write_buffer));
    memcpy(write_buffer, &config_cache, sizeof(Flash_Config_t));

    /* 擦除页 */
    status = Flash_ErasePage();
    if (status != FLASH_OK)
        return status;

    /* 写入配置 */
    uint32_t word_count = write_size / sizeof(uint32_t);
    status = Flash_WriteData(FLASH_CONFIG_ADDR, write_buffer, word_count);
    if (status != FLASH_OK)
        return status;

    /* 回读验证 */
    Flash_Config_t readback;
    memcpy(&readback, (const void *)FLASH_CONFIG_ADDR, sizeof(Flash_Config_t));
    status = Flash_VerifyConfig(&readback);
    if (status == FLASH_OK)
        config_valid = 1;

    return status;
}

/**
 * @brief 获取配置缓存指针 (只读)
 * @retval const Flash_Config_t* 配置指针
 */
const Flash_Config_t* Flash_GetConfig(void)
{
    return &config_cache;
}

/**
 * @brief 设置RS485设备地址
 * @param id 设备地址 (1-247, 0=未设置)
 */
Flash_Status_t Flash_SetDeviceID(uint8_t id)
{
    if (id > 247)
        return FLASH_ERR_PARAM;

    config_cache.device_id = id;
    return FLASH_OK;
}

/**
 * @brief 获取RS485设备地址
 */
uint8_t Flash_GetDeviceID(void)
{
    return config_cache.device_id;
}

/**
 * @brief 设置设备名称
 * @param name 设备名称字符串
 */
Flash_Status_t Flash_SetDeviceName(const char *name)
{
    if (name == NULL)
        return FLASH_ERR_PARAM;

    strncpy(config_cache.device_name, name, FLASH_DEVICE_NAME_LEN - 1);
    config_cache.device_name[FLASH_DEVICE_NAME_LEN - 1] = '\0';
    return FLASH_OK;
}

/**
 * @brief 获取设备名称
 */
const char* Flash_GetDeviceName(void)
{
    return config_cache.device_name;
}

/**
 * @brief 设置序列号
 * @param sn 序列号
 */
Flash_Status_t Flash_SetSerialNumber(uint32_t sn)
{
    config_cache.serial_number = sn;
    return FLASH_OK;
}

/**
 * @brief 获取序列号
 */
uint32_t Flash_GetSerialNumber(void)
{
    return config_cache.serial_number;
}

/**
 * @brief 设置上电默认通道
 * @param channel 通道号 (0-based, 0~9)
 */
Flash_Status_t Flash_SetDefaultChannel(uint8_t channel)
{
    if (channel > 9)
        return FLASH_ERR_PARAM;

    config_cache.default_channel = channel;
    return FLASH_OK;
}

/**
 * @brief 获取上电默认通道
 */
uint8_t Flash_GetDefaultChannel(void)
{
    return config_cache.default_channel;
}

/**
 * @brief 检查配置是否已初始化
 * @retval 0 无效  1 有效
 */
uint8_t Flash_IsValid(void)
{
    return config_valid;
}

/* ========================================================================== */
/*              SCPI *IDN? 字段访问函数                                       */
/* ========================================================================== */

Flash_Status_t Flash_SetScpiIdn1(const char *str)
{
    if (str == NULL)
        return FLASH_ERR_PARAM;

    strncpy(config_cache.scpi_idn1, str, FLASH_SCPI_IDN1_LEN - 1);
    config_cache.scpi_idn1[FLASH_SCPI_IDN1_LEN - 1] = '\0';
    return FLASH_OK;
}

const char* Flash_GetScpiIdn1(void)
{
    return config_cache.scpi_idn1;
}

Flash_Status_t Flash_SetScpiIdn2(const char *str)
{
    if (str == NULL)
        return FLASH_ERR_PARAM;

    strncpy(config_cache.scpi_idn2, str, FLASH_SCPI_IDN2_LEN - 1);
    config_cache.scpi_idn2[FLASH_SCPI_IDN2_LEN - 1] = '\0';
    return FLASH_OK;
}

const char* Flash_GetScpiIdn2(void)
{
    return config_cache.scpi_idn2;
}

Flash_Status_t Flash_SetScpiIdn3(const char *str)
{
    if (str == NULL)
        return FLASH_ERR_PARAM;

    strncpy(config_cache.scpi_idn3, str, FLASH_SCPI_IDN3_LEN - 1);
    config_cache.scpi_idn3[FLASH_SCPI_IDN3_LEN - 1] = '\0';
    return FLASH_OK;
}

const char* Flash_GetScpiIdn3(void)
{
    return config_cache.scpi_idn3;
}

Flash_Status_t Flash_SetScpiIdn4(const char *str)
{
    if (str == NULL)
        return FLASH_ERR_PARAM;

    strncpy(config_cache.scpi_idn4, str, FLASH_SCPI_IDN4_LEN - 1);
    config_cache.scpi_idn4[FLASH_SCPI_IDN4_LEN - 1] = '\0';
    return FLASH_OK;
}

const char* Flash_GetScpiIdn4(void)
{
    return config_cache.scpi_idn4;
}

/* ========================================================================== */
/*              波特率访问函数                                                 */
/* ========================================================================== */

Flash_Status_t Flash_SetBaudrate(uint32_t baud)
{
    config_cache.rs485_baudrate = baud;
    return FLASH_OK;
}

uint32_t Flash_GetBaudrate(void)
{
    return config_cache.rs485_baudrate;
}

/* ===== 急停类型访问函数 ===== */

Flash_Status_t Flash_SetEstopType(uint8_t type)
{
    if (type > 1) return FLASH_ERR_PARAM;
    config_cache.estop_type = type;
    return FLASH_OK;
}

uint8_t Flash_GetEstopType(void)
{
    return config_cache.estop_type;
}
