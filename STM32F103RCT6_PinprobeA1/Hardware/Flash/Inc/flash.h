/*
 * flash.h
 *
 *  Created on: 2026-04-28
 *      Author: STM32 Project
 *  Description: Flash storage module for device configuration
 *               Stores SCPI identification, baudrate, and switch info
 */

#ifndef APP_FLASH_INC_FLASH_H_
#define APP_FLASH_INC_FLASH_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>
#include <stddef.h>

/* Exported constants --------------------------------------------------------*/

/**
 * @brief 配置存储的Flash页号
 * @note STM32F103RCT6 256KB Flash, 2KB/页, 共128页
 *       使用最后一页 (page 127, 地址 0x0803F800) 存储配置
 *       链接脚本已将FLASH减少2KB (254K) 防止代码覆盖
 */
#define FLASH_CONFIG_PAGE          127U

/**
 * @brief 配置存储起始地址
 * @note FLASH_BASE = 0x08000000
 *       FLASH_CONFIG_ADDR = FLASH_BASE + FLASH_CONFIG_PAGE * FLASH_PAGE_SIZE
 *                          = 0x08000000 + 127 * 0x800
 *                          = 0x0803F800
 */
#define FLASH_CONFIG_ADDR          (0x08000000UL + (FLASH_CONFIG_PAGE * FLASH_PAGE_SIZE))

/**
 * @brief Flash页大小 (STM32F103 high-density: 2KB)
 */
#define FLASH_CONFIG_PAGE_SIZE     0x800U

/**
 * @brief 配置魔数 - 用于判断Flash中是否存储了有效配置
 */
#define FLASH_CONFIG_MAGIC         0xA5A5A5A5UL

/**
 * @brief 配置结构版本号
 */
#define FLASH_CONFIG_VERSION       0x00020000UL  /* v2.0.0 - added SCPI IDN fields */

/**
 * @brief 设备名称最大长度
 */
#define FLASH_DEVICE_NAME_LEN      24U

/**
 * @brief 设备序列号字符串长度
 */
#define FLASH_SERIAL_LEN           16U

/**
 * @brief SCPI *IDN? 各字段最大长度 (含终止符)
 */
#define FLASH_SCPI_IDN1_LEN        12U   /**< 厂商名       */
#define FLASH_SCPI_IDN2_LEN        20U   /**< 产品型号     */
#define FLASH_SCPI_IDN3_LEN        12U   /**< 序列号/日期  */
#define FLASH_SCPI_IDN4_LEN        12U   /**< 固件版本     */

/* Exported types ------------------------------------------------------------*/

/**
 * @brief 开关设备配置结构体
 * @note 存储在Flash最后2KB页中，总计约124字节，有充足余量
 *       所有字段对齐到4字节边界
 */
typedef struct {
    /* ===== 元数据 ===== */
    uint32_t magic;             /**< 魔数 (FLASH_CONFIG_MAGIC)，判断配置是否有效        */
    uint32_t version;           /**< 结构体版本号 (FLASH_CONFIG_VERSION)                */

    /* ===== 设备标识 ===== */
    uint8_t  device_id;         /**< RS485设备地址 (1-247, 0为未设置)                    */
    uint8_t  hw_version;        /**< 硬件版本号 (BCD格式: 0x10 = v1.0)                  */
    uint8_t  fw_version_major;  /**< 固件主版本号                                       */
    uint8_t  fw_version_minor;  /**< 固件次版本号                                       */
    uint32_t serial_number;     /**< 序列号                                             */
    char     device_name[FLASH_DEVICE_NAME_LEN]; /**< 设备名称 (字符串, 含终止符)        */

    /* ===== 运行配置 ===== */
    uint8_t  default_channel;   /**< 上电默认开关通道 (0-based, 0~9)                     */
    uint8_t  default_mode;      /**< 上电默认IO模式 (0=DIRECT, 1=COMMAND)               */
    uint32_t rs485_baudrate;    /**< RS485波特率 (如 115200)                            */

    /* ===== SCPI *IDN? 标识字符串 ===== */
    char     scpi_idn1[FLASH_SCPI_IDN1_LEN]; /**< SCPI厂商名 (如 "GTS")                  */
    char     scpi_idn2[FLASH_SCPI_IDN2_LEN]; /**< SCPI产品型号 (如 "PINPROBEA1")         */
    char     scpi_idn3[FLASH_SCPI_IDN3_LEN]; /**< SCPI序列号/日期 (如 "20250626")        */
    char     scpi_idn4[FLASH_SCPI_IDN4_LEN]; /**< SCPI固件版本 (如 "V0.0.1")             */

    /* ===== 外设配置 ===== */
    uint8_t  estop_type;        /**< 急停输入类型: 0=常闭(NC), 1=常开(NO)                 */
    uint8_t  reserved[15];      /**< 保留字节，确保未来扩展性，初始化为0                 */

    /* ===== CRC校验 (必须在结构体末尾) ===== */
    uint32_t crc;               /**< CRC32校验 (从 magic 到 reserved 末尾)              */

} Flash_Config_t;

/**
 * @brief Flash操作结果枚举
 */
typedef enum {
    FLASH_OK        = 0x00,    /**< 操作成功                                           */
    FLASH_ERR_MAGIC = 0x01,    /**< 魔数不匹配 (配置未初始化或被破坏)                     */
    FLASH_ERR_CRC   = 0x02,    /**< CRC校验失败                                         */
    FLASH_ERR_HAL   = 0x03,    /**< HAL硬件操作失败                                     */
    FLASH_ERR_PARAM = 0x04,    /**< 参数错误                                             */
    FLASH_ERR_BUSY  = 0x05,    /**< Flash正忙                                            */
} Flash_Status_t;

/* Exported macro ------------------------------------------------------------*/

/**
 * @brief 获取配置结构体大小 (4字节对齐)
 */
#define FLASH_CONFIG_SIZE        ((sizeof(Flash_Config_t) + 3) & ~3U)

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief 初始化Flash配置模块
 * @note 从Flash加载配置并进行验证。若无效则加载默认配置。
 *       必须在系统启动早期调用，确保其他模块能读取设备信息。
 * @retval Flash_Status_t 操作状态
 */
Flash_Status_t Flash_Init(void);

/**
 * @brief 将当前配置保存到Flash
 * @note 执行页擦除+写入操作，会短暂阻塞。
 *       调用前需确保Flash解锁。
 * @retval Flash_Status_t 操作状态
 */
Flash_Status_t Flash_Save(void);

/**
 * @brief 将配置恢复为默认值 (不写入Flash)
 * @note 仅修改内存中的配置缓存，需要使用 Flash_Save() 持久化
 */
void Flash_LoadDefaults(void);

/**
 * @brief 获取配置缓存指针 (只读)
 * @retval const Flash_Config_t* 配置指针，不会为NULL
 */
const Flash_Config_t* Flash_GetConfig(void);

/**
 * @brief 设置RS485设备地址
 * @param id 设备地址 (1-247, 0=未设置)
 * @retval Flash_Status_t
 */
Flash_Status_t Flash_SetDeviceID(uint8_t id);

/**
 * @brief 获取RS485设备地址
 * @retval uint8_t 设备地址
 */
uint8_t Flash_GetDeviceID(void);

/**
 * @brief 设置设备名称
 * @param name 设备名称字符串 (最大 FLASH_DEVICE_NAME_LEN-1 字符)
 * @retval Flash_Status_t
 */
Flash_Status_t Flash_SetDeviceName(const char *name);

/**
 * @brief 获取设备名称
 * @retval const char* 设备名称指针
 */
const char* Flash_GetDeviceName(void);

/**
 * @brief 设置序列号
 * @param sn 序列号
 * @retval Flash_Status_t
 */
Flash_Status_t Flash_SetSerialNumber(uint32_t sn);

/**
 * @brief 获取序列号
 * @retval uint32_t 序列号
 */
uint32_t Flash_GetSerialNumber(void);

/**
 * @brief 设置上电默认通道
 * @param channel 通道号 (0-based, 0~9)
 * @retval Flash_Status_t
 */
Flash_Status_t Flash_SetDefaultChannel(uint8_t channel);

/**
 * @brief 获取上电默认通道
 * @retval uint8_t 通道号
 */
uint8_t Flash_GetDefaultChannel(void);

/**
 * @brief 检查配置是否已初始化 (魔数有效+CRC校验通过)
 * @retval 0 无效  1 有效
 */
uint8_t Flash_IsValid(void);

/* ===== SCPI *IDN? 字段访问函数 ===== */

/**
 * @brief 设置SCPI厂商名 (*IDN? 字段1)
 * @param str 字符串 (最大 FLASH_SCPI_IDN1_LEN-1 字符)
 * @retval Flash_Status_t
 */
Flash_Status_t Flash_SetScpiIdn1(const char *str);

/**
 * @brief 获取SCPI厂商名
 * @retval const char*
 */
const char* Flash_GetScpiIdn1(void);

/**
 * @brief 设置SCPI产品型号 (*IDN? 字段2)
 * @param str 字符串 (最大 FLASH_SCPI_IDN2_LEN-1 字符)
 * @retval Flash_Status_t
 */
Flash_Status_t Flash_SetScpiIdn2(const char *str);

/**
 * @brief 获取SCPI产品型号
 * @retval const char*
 */
const char* Flash_GetScpiIdn2(void);

/**
 * @brief 设置SCPI序列号/日期 (*IDN? 字段3)
 * @param str 字符串 (最大 FLASH_SCPI_IDN3_LEN-1 字符)
 * @retval Flash_Status_t
 */
Flash_Status_t Flash_SetScpiIdn3(const char *str);

/**
 * @brief 获取SCPI序列号/日期
 * @retval const char*
 */
const char* Flash_GetScpiIdn3(void);

/**
 * @brief 设置SCPI固件版本 (*IDN? 字段4)
 * @param str 字符串 (最大 FLASH_SCPI_IDN4_LEN-1 字符)
 * @retval Flash_Status_t
 */
Flash_Status_t Flash_SetScpiIdn4(const char *str);

/**
 * @brief 获取SCPI固件版本
 * @retval const char*
 */
const char* Flash_GetScpiIdn4(void);

/* ===== 波特率访问函数 ===== */

/**
 * @brief 设置RS485波特率
 * @param baud 波特率值
 * @retval Flash_Status_t
 */
Flash_Status_t Flash_SetBaudrate(uint32_t baud);

/**
 * @brief 获取RS485波特率
 * @retval uint32_t 波特率值
 */
uint32_t Flash_GetBaudrate(void);

/* ===== 急停类型访问函数 ===== */

Flash_Status_t Flash_SetEstopType(uint8_t type);
uint8_t Flash_GetEstopType(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_FLASH_INC_FLASH_H_ */
