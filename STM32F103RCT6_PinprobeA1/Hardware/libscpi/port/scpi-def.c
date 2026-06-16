/*-
 * BSD 2-Clause License
 *
 * Copyright (c) 2012-2018, Jan Breuer
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file   scpi-def.c
 * @date   Thu Nov 15 10:58:45 UTC 2012
 *
 * @brief  SCPI parser test
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scpi/scpi.h"
#include "scpi-def.h"
#include "BsmRelay.h"
#include "flash.h"
#include "ram_vector.h"
#include "state_vector.h"
#include "tim.h"

/* 辅助: 推送 SCPI 错误并返回 ERR (附带描述信息) */
#define PUSH_ERR(ctx, code, info) do { \
    SCPI_ErrorPushEx((ctx), (code), (char*)(info), sizeof(info)-1); \
    return SCPI_RES_ERR; \
} while(0)

/* ========================================================================== */
/*            SCPI *IDN? 运行时缓冲区                                         */
/* ========================================================================== */

/**
 * @brief SCPI *IDN? 运行时缓冲 - 可在运行时通过 SCPI 命令修改
 * @note 这些缓冲区的指针被传递给 SCPI_Init() 并存储在 scpi_context.idn[] 中。
 *       通过 SCPI_SyncIdnFromFlash() 可从 Flash 配置同步数据到此缓冲区。
 */
char scpi_idn_buf1[SCPI_IDN_BUF1_SIZE] = SCPI_IDN1;
char scpi_idn_buf2[SCPI_IDN_BUF2_SIZE] = SCPI_IDN2;
char scpi_idn_buf3[SCPI_IDN_BUF3_SIZE] = SCPI_IDN3;
char scpi_idn_buf4[SCPI_IDN_BUF4_SIZE] = SCPI_IDN4;

/**
 * @brief 从 Flash 配置同步 SCPI IDN 字符串到运行时缓冲区
 * @note 调用此函数后，*IDN? 将返回 Flash 中存储的值。
 *       通常在 Flash_Init() 之后调用一次，也可在修改 Flash 配置后调用。
 */
void SCPI_SyncIdnFromFlash(void)
{
    const Flash_Config_t *cfg = Flash_GetConfig();

    /* 从 Flash 配置拷贝到运行时缓冲区 */
    strncpy(scpi_idn_buf1, cfg->scpi_idn1, SCPI_IDN_BUF1_SIZE - 1);
    scpi_idn_buf1[SCPI_IDN_BUF1_SIZE - 1] = '\0';

    strncpy(scpi_idn_buf2, cfg->scpi_idn2, SCPI_IDN_BUF2_SIZE - 1);
    scpi_idn_buf2[SCPI_IDN_BUF2_SIZE - 1] = '\0';

    strncpy(scpi_idn_buf3, cfg->scpi_idn3, SCPI_IDN_BUF3_SIZE - 1);
    scpi_idn_buf3[SCPI_IDN_BUF3_SIZE - 1] = '\0';

    strncpy(scpi_idn_buf4, cfg->scpi_idn4, SCPI_IDN_BUF4_SIZE - 1);
    scpi_idn_buf4[SCPI_IDN_BUF4_SIZE - 1] = '\0';

    /* 更新 SCPI 上下文的 IDN 指针指向运行时缓冲区 */
    scpi_context.idn[0] = scpi_idn_buf1;
    scpi_context.idn[1] = scpi_idn_buf2;
    scpi_context.idn[2] = scpi_idn_buf3;
    scpi_context.idn[3] = scpi_idn_buf4;
}

/* ========================================================================== */
/*            SCPI IDN 设置/查询 命令处理                                     */
/* ========================================================================== */

/**
 * @brief 设置 SCPI *IDN? 字段1 (厂商名)
 * @note 命令格式: SYSTem:IDN1 <string>
 *        例如: SYSTem:IDN1 "MyCompany"
 */
static scpi_result_t SCPI_SetIdn1(scpi_t *context)
{
    char buffer[SCPI_IDN_BUF1_SIZE];
    size_t copy_len = 0;

    /* 获取参数中的字符串 */
    memset(buffer, 0, sizeof(buffer));
    if (!SCPI_ParamCopyText(context, buffer, sizeof(buffer), &copy_len, TRUE))
    {
        return SCPI_RES_ERR;
    }

    /* 更新 Flash 配置并保存 */
    if (Flash_SetScpiIdn1(buffer) != FLASH_OK)
        PUSH_ERR(context, -320 /*Storage fault*/, "Flash write");
    if (Flash_Save() != FLASH_OK)
        PUSH_ERR(context, -320 /*Storage fault*/, "Flash save");

    /* 同步到运行时缓冲区 */
    SCPI_SyncIdnFromFlash();

    return SCPI_RES_OK;
}

/**
 * @brief 查询 SCPI *IDN? 字段1 (厂商名)
 * @note 命令格式: SYSTem:IDN1?
 */
static scpi_result_t SCPI_ReadIdn1Q(scpi_t *context)
{
    SCPI_ResultCharacters(context, scpi_idn_buf1, strlen(scpi_idn_buf1));
    return SCPI_RES_OK;
}

/**
 * @brief 设置 SCPI *IDN? 字段2 (产品型号)
 * @note 命令格式: SYSTem:IDN2 <string>
 */
static scpi_result_t SCPI_SetIdn2(scpi_t *context)
{
    char buffer[SCPI_IDN_BUF2_SIZE];
    size_t copy_len = 0;

    memset(buffer, 0, sizeof(buffer));
    if (!SCPI_ParamCopyText(context, buffer, sizeof(buffer), &copy_len, TRUE))
    {
        return SCPI_RES_ERR;
    }

    if (Flash_SetScpiIdn2(buffer) != FLASH_OK)
        PUSH_ERR(context, -320 /*Storage fault*/, "Flash write");
    if (Flash_Save() != FLASH_OK)
        PUSH_ERR(context, -320 /*Storage fault*/, "Flash save");

    SCPI_SyncIdnFromFlash();

    return SCPI_RES_OK;
}

/**
 * @brief 查询 SCPI *IDN? 字段2 (产品型号)
 * @note 命令格式: SYSTem:IDN2?
 */
static scpi_result_t SCPI_ReadIdn2Q(scpi_t *context)
{
    SCPI_ResultCharacters(context, scpi_idn_buf2, strlen(scpi_idn_buf2));
    return SCPI_RES_OK;
}

/**
 * @brief 设置 SCPI *IDN? 字段3 (序列号/日期)
 * @note 命令格式: SYSTem:IDN3 <string>
 */
static scpi_result_t SCPI_SetIdn3(scpi_t *context)
{
    char buffer[SCPI_IDN_BUF3_SIZE];
    size_t copy_len = 0;

    memset(buffer, 0, sizeof(buffer));
    if (!SCPI_ParamCopyText(context, buffer, sizeof(buffer), &copy_len, TRUE))
    {
        return SCPI_RES_ERR;
    }

    if (Flash_SetScpiIdn3(buffer) != FLASH_OK)
        PUSH_ERR(context, -320 /*Storage fault*/, "Flash write");
    if (Flash_Save() != FLASH_OK)
        PUSH_ERR(context, -320 /*Storage fault*/, "Flash save");

    SCPI_SyncIdnFromFlash();

    return SCPI_RES_OK;
}

/**
 * @brief 查询 SCPI *IDN? 字段3 (序列号/日期)
 * @note 命令格式: SYSTem:IDN3?
 */
static scpi_result_t SCPI_ReadIdn3Q(scpi_t *context)
{
    SCPI_ResultCharacters(context, scpi_idn_buf3, strlen(scpi_idn_buf3));
    return SCPI_RES_OK;
}

/**
 * @brief 设置 SCPI *IDN? 字段4 (固件版本)
 * @note 命令格式: SYSTem:IDN4 <string>
 */
static scpi_result_t SCPI_SetIdn4(scpi_t *context)
{
    char buffer[SCPI_IDN_BUF4_SIZE];
    size_t copy_len = 0;

    memset(buffer, 0, sizeof(buffer));
    if (!SCPI_ParamCopyText(context, buffer, sizeof(buffer), &copy_len, TRUE))
    {
        return SCPI_RES_ERR;
    }

    if (Flash_SetScpiIdn4(buffer) != FLASH_OK)
        PUSH_ERR(context, -320 /*Storage fault*/, "Flash write");
    if (Flash_Save() != FLASH_OK)
        PUSH_ERR(context, -320 /*Storage fault*/, "Flash save");

    SCPI_SyncIdnFromFlash();

    return SCPI_RES_OK;
}

/**
 * @brief 查询 SCPI *IDN? 字段4 (固件版本)
 * @note 命令格式: SYSTem:IDN4?
 */
static scpi_result_t SCPI_ReadIdn4Q(scpi_t *context)
{
    SCPI_ResultCharacters(context, scpi_idn_buf4, strlen(scpi_idn_buf4));
    return SCPI_RES_OK;
}


static scpi_result_t SCPI_Configurebaudrate(scpi_t *context)
{
    uint32_t baudrate;
    if(!SCPI_ParamUInt32(context, &baudrate, true))
    {
        return SCPI_RES_ERR;
    }
        // Verify the input baudrate is 115200
    if (baudrate != 115200) {
        SCPI_ResultText(context, "ERROR: Only 115200 baudrate is supported");
        return SCPI_RES_ERR;
    }

    // First change UART3 baudrate to 9600 to match BSM default
    huart3.Init.BaudRate = 9600;
    if (HAL_UART_Init(&huart3) != HAL_OK)
        PUSH_ERR(context, -240 /*Hardware error*/, "UART init");

    // Send command to change BSM baudrate to 115200
    uint8_t set_baud_cmd[] = {0x01, 0x06, 0x00, 0x02, 0x04, 0x80, 0x2B, 0x6A};
    HAL_UART_Transmit(&huart3, set_baud_cmd, sizeof(set_baud_cmd), HAL_MAX_DELAY);

    // Small delay to ensure command is processed
    HAL_Delay(100);

    // Write settings to flash
    uint8_t write_flash_cmd[] = {0x01, 0x06, 0x00, 0x09, 0x00, 0x6F, 0x19, 0xE4};
    HAL_UART_Transmit(&huart3, write_flash_cmd, sizeof(write_flash_cmd), HAL_MAX_DELAY);
    HAL_Delay(100);
 
    // Perform warm restart
    uint8_t warm_restart_cmd[] = {0x01, 0x06, 0x00, 0x09, 0x00, 0x7B, 0x19, 0xEB};
    HAL_UART_Transmit(&huart3, warm_restart_cmd, sizeof(warm_restart_cmd), HAL_MAX_DELAY);
    HAL_Delay(1000); // Longer delay for restart to complete

    // Change UART3 baudrate to new value (115200)
    huart3.Init.BaudRate = 115200;
    if (HAL_UART_Init(&huart3) != HAL_OK)
        PUSH_ERR(context, -240 /*Hardware error*/, "UART init");

    // Verify the baudrate was changed by reading it back
    uint8_t read_baud_cmd[] = {0x01, 0x03, 0x00, 0x02, 0x00, 0x01, 0x25, 0xCA};
    uint8_t response[8];

    HAL_UART_Transmit(&huart3, read_baud_cmd, sizeof(read_baud_cmd), HAL_MAX_DELAY);

    // Wait for response (adjust timeout as needed)
    if (HAL_UART_Receive(&huart3, response, sizeof(response), 100) != HAL_OK)
        PUSH_ERR(context, -240 /*Hardware error*/, "UART rx");

    // Check if response indicates 115200 baud (0x04 0x80)
    if (response[3] != 0x04 || response[4] != 0x80)
        PUSH_ERR(context, -360 /*Communication error*/, "Baudrate verify");

    SCPI_ResultText(context, "115200 baudrate is enable");

    return SCPI_RES_OK;
}


scpi_choice_def_t cylinder_source[] = {
    {"CLOSE", 0},
    {"OPEN", 1},
    {"CLOSING",2},
    {"OPENING",3},
    {"CLOSED",4},
    {"OPENED",5},
    {"CYL ERR",6},
    SCPI_CHOICE_LIST_END /* termination of option list */
};

static scpi_result_t SCPI_ConfigureCylinder(scpi_t *context)
{
    int32_t number[2] = {0,0};
    uint16_t cylinder_id = 1;
    SCPI_CommandNumbers(context, number, 1, 1);
    if(number[0] != 1)
    {
        cylinder_id = number[0];
    }
    int32_t param;
    const char *name;
    if (!SCPI_ParamChoice(context, cylinder_source, &param, TRUE)) {
        return SCPI_RES_ERR;
    }
    SCPI_ChoiceToName(cylinder_source, param, &name);
    /* 多气缸: id=1 门, id=2 USB */
    if (cylinder_id == 2)
        RamVector_PostCylinder((param == 0) ? VCMD_CYLINDER2_CLOSE : VCMD_CYLINDER2_OPEN, CMD_PRIO_USER);
    else
        RamVector_PostCylinder((param == 0) ? VCMD_CYLINDER_CLOSE : VCMD_CYLINDER_OPEN, CMD_PRIO_USER);
    SCPI_ResultCharacters(context, name, strlen(name));
    return SCPI_RES_OK;
}

static scpi_result_t SCPI_ReadCylinderState(scpi_t *context)
{
    int32_t number[2] = {0,0};
    uint16_t cylinder_id = 1;
    SCPI_CommandNumbers(context, number, 0, 1);
    if(number[0] == 2) cylinder_id = 2;

    const Vector_IOState_t* io = RamVector_GetLocalIO();
    const char *name = "CYL ERR";

    if (cylinder_id == 1) {
        /* 气缸1: 基于 door_state / cylinder_cmd[0] */
        if (io->door_moving)          name = (io->cylinder_cmd[0] ? "OPENING" : "CLOSING");
        else if (io->door_state == 1) name = "OPENED";
        else if (io->door_state == 0) name = "CLOSED";
    } else {
        /* 气缸2 (USB): 基于 raw_out_lo bits 0x04/0x08 (usb_pne_in/usb_pne_out) */
        uint8_t out = io->raw_out_lo;
        if (out & 0x04)           name = "OPENING";
        else if (out & 0x08)      name = "CLOSING";
        else                      name = "CLOSED";
    }

    SCPI_ResultCharacters(context, name, strlen(name));
    return SCPI_RES_OK;
}

scpi_choice_def_t lock_source[] = {
    {"UNLOCK", 0},
    {"LOCKED", 1},
    {"LOCK ERR",2},
    SCPI_CHOICE_LIST_END /* termination of option list */
};

static scpi_result_t SCPI_ConfigureLOCK(scpi_t *context)
{
    int32_t param;
    const char *name;
    if (!SCPI_ParamChoice(context, lock_source, &param, TRUE)) {
        return SCPI_RES_ERR;
    }
    SCPI_ChoiceToName(lock_source, param, &name);
    RamVector_PostLock((param == 0) ? VCMD_UNLOCK : VCMD_LOCK, CMD_PRIO_USER);
    SCPI_ResultCharacters(context, name, strlen(name));
    return SCPI_RES_OK;
}

static scpi_result_t SCPI_ReadLOCKState(scpi_t *context)
{
    const Vector_IOState_t* io = RamVector_GetLocalIO();
    const char *name = io->lock_state ? "UNLOCK" : "LOCKED";
    SCPI_ResultCharacters(context, name, strlen(name));
    return SCPI_RES_OK;
}

scpi_choice_def_t led_source[] = {
    {"OFF", 0},
    {"GREEN", 1},
    {"RED", 2},
    {"YELLOW", 3},
    {"LED ERR", 4},
    SCPI_CHOICE_LIST_END /* termination of option list */
};

static scpi_result_t SCPI_ConfigureLED(scpi_t *context)
{
    int32_t param;
    const char *name;
    if (!SCPI_ParamChoice(context, led_source, &param, TRUE)) {
        return SCPI_RES_ERR;
    }
    SCPI_ChoiceToName(led_source, param, &name);
    RamVector_PostLED((Vector_Cmd_t)(VCMD_LED_OFF + param), CMD_PRIO_USER);
    SCPI_ResultCharacters(context, name, strlen(name));
    return SCPI_RES_OK;
}

static scpi_result_t SCPI_ReadLEDState(scpi_t *context)
{
    const Vector_IOState_t* io = RamVector_GetLocalIO();
    const char *name;
    switch (io->led_state) {
        case 1: name = "GREEN"; break;
        case 2: name = "RED"; break;
        case 4: name = "YELLOW"; break;
        default: name = "OFF"; break;
    }
    SCPI_ResultCharacters(context, name, strlen(name));
    return SCPI_RES_OK;
}

scpi_choice_def_t sys_source[] = {
    {"LOCK", 0},
    {"IDLE", 1},
    {"READY", 2},
    {"RUNNING", 3},
    {"EMERGENCY", 4},
    {"COMPLETE",5},
    {"SYS ERR",6},
    SCPI_CHOICE_LIST_END /* termination of option list */
};
static scpi_result_t SCPI_ReadSystemState(scpi_t *context)
{
    const char *name = "SYS ERR";
    scpi_choice_def_t status =SYS_Status();
    name = status.name;
    SCPI_ResultCharacters(context, name, strlen(name));
    return SCPI_RES_OK;
}

/* ===== 急停输入类型配置 ===== */

scpi_choice_def_t estop_type_source[] = {
    {"NC", 0},   // 常闭 (Normally Closed) — 默认
    {"NO", 1},   // 常开 (Normally Open)
    SCPI_CHOICE_LIST_END
};

static scpi_result_t SCPI_ConfigureEstopType(scpi_t *context)
{
    int32_t param;
    if (!SCPI_ParamChoice(context, estop_type_source, &param, TRUE))
        return SCPI_RES_ERR;
    if (Flash_SetEstopType((uint8_t)param) != FLASH_OK)
        PUSH_ERR(context, -320 /*Storage fault*/, "Flash write");
    if (Flash_Save() != FLASH_OK)
        PUSH_ERR(context, -320 /*Storage fault*/, "Flash save");
    const char *name;
    SCPI_ChoiceToName(estop_type_source, param, &name);
    SCPI_ResultCharacters(context, name, strlen(name));
    return SCPI_RES_OK;
}

static scpi_result_t SCPI_ReadEstopTypeQ(scpi_t *context)
{
    uint8_t type = Flash_GetEstopType();
    const char *name;
    SCPI_ChoiceToName(estop_type_source, type, &name);
    SCPI_ResultCharacters(context, name, strlen(name));
    return SCPI_RES_OK;
}

/* ===== 风险模式 (气压传感器替代限位) ===== */

scpi_choice_def_t risk_mode_source[] = {
    {"OFF", 0},
    {"ON",  1},
    SCPI_CHOICE_LIST_END
};

static scpi_result_t SCPI_ConfigureRiskMode(scpi_t *context)
{
    int32_t param;
    if (!SCPI_ParamChoice(context, risk_mode_source, &param, TRUE))
        return SCPI_RES_ERR;
    if (Flash_SetRiskMode((uint8_t)param) != FLASH_OK)
        PUSH_ERR(context, -320 /*Storage fault*/, "Flash write");
    if (Flash_Save() != FLASH_OK)
        PUSH_ERR(context, -320 /*Storage fault*/, "Flash save");
    const char *name;
    SCPI_ChoiceToName(risk_mode_source, param, &name);
    SCPI_ResultCharacters(context, name, strlen(name));
    return SCPI_RES_OK;
}

static scpi_result_t SCPI_ReadRiskModeQ(scpi_t *context)
{
    uint8_t mode = Flash_GetRiskMode();
    const char *name;
    SCPI_ChoiceToName(risk_mode_source, mode, &name);
    SCPI_ResultCharacters(context, name, strlen(name));
    return SCPI_RES_OK;
}

/// @brief 查询所有IO状态（输入+输出）
/// @note 命令: READ:IO:ALL?
///       返回: IN:0xHH,0xHH OUT:0xHH,0xHH（16进制原始值）
static scpi_result_t SCPI_ReadIOAll(scpi_t *context)
{
    /* 从向量表读缓存, 不走 RS485 (避免和 ModBusTask 冲突) */
    const Vector_IOState_t* io = RamVector_GetLocalIO();
    char buf[64];
    snprintf(buf, sizeof(buf), "IN:0x%02X,0x%02X OUT:0x%02X,0x%02X",
             io->raw_in_lo, io->raw_in_hi, io->raw_out_lo, io->raw_out_hi);
    SCPI_ResultCharacters(context, buf, strlen(buf));
    return SCPI_RES_OK;
}

/* ===== 固件版本查询 ===== */

static scpi_result_t SCPI_SystemVersionQ(scpi_t *context)
{
    const char *ver = FW_VERSION "+" FW_GIT_HASH;
    SCPI_ResultCharacters(context, ver, strlen(ver));
    return SCPI_RES_OK;
}

static scpi_result_t SCPI_SystemUpTimeQ(scpi_t *context)
{
    /* 返回秒, uint32_t 秒可跑 136 年不溢出 (毫秒仅 49.7 天) */
    uint32_t sec = GetTim1Ms() / 1000;
    SCPI_ResultUInt32(context, sec);
    return SCPI_RES_OK;
}

/* ===== 调试开关 (运行时控制, 替代编译期宏) ===== */

scpi_choice_def_t debug_on_off[] = {
    {"OFF", 0},
    {"ON",  1},
    SCPI_CHOICE_LIST_END
};

static scpi_result_t SCPI_ConfigureDebugState(scpi_t *context)
{
    int32_t param;
    if (!SCPI_ParamChoice(context, debug_on_off, &param, TRUE))
        return SCPI_RES_ERR;
    vector_debug_flags.state = (param != 0);
    const char *name;
    SCPI_ChoiceToName(debug_on_off, param, &name);
    SCPI_ResultCharacters(context, name, strlen(name));
    return SCPI_RES_OK;
}

static scpi_result_t SCPI_ReadDebugStateQ(scpi_t *context)
{
    const char *name;
    SCPI_ChoiceToName(debug_on_off, vector_debug_flags.state ? 1 : 0, &name);
    SCPI_ResultCharacters(context, name, strlen(name));
    return SCPI_RES_OK;
}

static scpi_result_t SCPI_ConfigureDebugAction(scpi_t *context)
{
    int32_t param;
    if (!SCPI_ParamChoice(context, debug_on_off, &param, TRUE))
        return SCPI_RES_ERR;
    vector_debug_flags.action = (param != 0);
    const char *name;
    SCPI_ChoiceToName(debug_on_off, param, &name);
    SCPI_ResultCharacters(context, name, strlen(name));
    return SCPI_RES_OK;
}

static scpi_result_t SCPI_ReadDebugActionQ(scpi_t *context)
{
    const char *name;
    SCPI_ChoiceToName(debug_on_off, vector_debug_flags.action ? 1 : 0, &name);
    SCPI_ResultCharacters(context, name, strlen(name));
    return SCPI_RES_OK;
}

static scpi_result_t SCPI_ConfigureDebugEvent(scpi_t *context)
{
    int32_t param;
    if (!SCPI_ParamChoice(context, debug_on_off, &param, TRUE))
        return SCPI_RES_ERR;
    vector_debug_flags.event = (param != 0);
    const char *name;
    SCPI_ChoiceToName(debug_on_off, param, &name);
    SCPI_ResultCharacters(context, name, strlen(name));
    return SCPI_RES_OK;
}

static scpi_result_t SCPI_ReadDebugEventQ(scpi_t *context)
{
    const char *name;
    SCPI_ChoiceToName(debug_on_off, vector_debug_flags.event ? 1 : 0, &name);
    SCPI_ResultCharacters(context, name, strlen(name));
    return SCPI_RES_OK;
}

static scpi_result_t SCPI_ConfigureDebugIO(scpi_t *context)
{
    int32_t param;
    if (!SCPI_ParamChoice(context, debug_on_off, &param, TRUE))
        return SCPI_RES_ERR;
    vector_debug_flags.io = (param != 0);
    const char *name;
    SCPI_ChoiceToName(debug_on_off, param, &name);
    SCPI_ResultCharacters(context, name, strlen(name));
    return SCPI_RES_OK;
}

static scpi_result_t SCPI_ReadDebugIOQ(scpi_t *context)
{
    const char *name;
    SCPI_ChoiceToName(debug_on_off, vector_debug_flags.io ? 1 : 0, &name);
    SCPI_ResultCharacters(context, name, strlen(name));
    return SCPI_RES_OK;
}

const scpi_command_t scpi_commands[] = {
    /* IEEE Mandated Commands (SCPI std V1999.0 4.1.1) */
    {
        .pattern = "*CLS",
        .callback = SCPI_CoreCls,
    },
    {
        .pattern = "*IDN?",
        .callback = SCPI_CoreIdnQ,
    },
    {
        .pattern = "*RST",
        .callback = SCPI_CoreRst,
    },
    {
        .pattern = "*STB?",
        .callback = SCPI_CoreStbQ,
    },
    {
        .pattern = "*WAI",
        .callback = SCPI_CoreWai,
    },
    {
        .pattern = "*OPC?",
        .callback = SCPI_CoreOpcQ,
    },
    /* Required SCPI commands (SCPI std V1999.0 4.2.1) */
    {
        .pattern = "SYSTem:ERRor[:NEXT]?",
        .callback = SCPI_SystemErrorNextQ,
    },
    {
        .pattern = "SYSTem:ERRor:COUNt?",
        .callback = SCPI_SystemErrorCountQ,
    },
    /* 固件版本查询 */
    {
        .pattern = "SYSTem:VERSion?",
        .callback = SCPI_SystemVersionQ,
    },
    /* 运行时间查询 */
    {
        .pattern = "SYSTem:UPTime?",
        .callback = SCPI_SystemUpTimeQ,
    },
    /* SCPI *IDN? 字段配置命令 - 可运行时修改并持久化到 Flash */
    {
        .pattern = "SYSTem:IDN1",
        .callback = SCPI_SetIdn1,
    },
    {
        .pattern = "SYSTem:IDN1?",
        .callback = SCPI_ReadIdn1Q,
    },
    {
        .pattern = "SYSTem:IDN2",
        .callback = SCPI_SetIdn2,
    },
    {
        .pattern = "SYSTem:IDN2?",
        .callback = SCPI_ReadIdn2Q,
    },
    {
        .pattern = "SYSTem:IDN3",
        .callback = SCPI_SetIdn3,
    },
    {
        .pattern = "SYSTem:IDN3?",
        .callback = SCPI_ReadIdn3Q,
    },
    {
        .pattern = "SYSTem:IDN4",
        .callback = SCPI_SetIdn4,
    },
    {
        .pattern = "SYSTem:IDN4?",
        .callback = SCPI_ReadIdn4Q,
    },
    {
        .pattern = "CONFigure:BAUDrate",
        .callback = SCPI_Configurebaudrate,
    },
    {
        .pattern = "CONFigure:CYLInder#",
        .callback = SCPI_ConfigureCylinder,
    },
    {
        .pattern = "READ:CYLInder#:STATe?",
        .callback = SCPI_ReadCylinderState,
    },
    {
        .pattern = "CONFigure:LOCK",
        .callback = SCPI_ConfigureLOCK,
    },
    {
        .pattern = "READ:LOCK:STATe?",
        .callback = SCPI_ReadLOCKState,
    },
    {
        .pattern = "CONFigure:LED",
        .callback = SCPI_ConfigureLED,
    },
    {
        .pattern = "READ:LED:STATe?",
        .callback = SCPI_ReadLEDState,
    },
    {
        .pattern = "READ:SYSTem:STATe?",
        .callback = SCPI_ReadSystemState,
    },
    {
        .pattern = "READ:IO:ALL?",
        .callback = SCPI_ReadIOAll,
    },
    {
        .pattern = "CONFigure:ESTOP:TYPE",
        .callback = SCPI_ConfigureEstopType,
    },
    {
        .pattern = "CONFigure:ESTOP:TYPE?",
        .callback = SCPI_ReadEstopTypeQ,
    },
    {
        .pattern = "CONFigure:RISK:MODE",
        .callback = SCPI_ConfigureRiskMode,
    },
    {
        .pattern = "CONFigure:RISK:MODE?",
        .callback = SCPI_ReadRiskModeQ,
    },
    /* 调试开关 (运行时控制, 默认 OFF) */
    {
        .pattern = "CONFigure:DEBUg:STATe",
        .callback = SCPI_ConfigureDebugState,
    },
    {
        .pattern = "READ:DEBUg:STATe?",
        .callback = SCPI_ReadDebugStateQ,
    },
    {
        .pattern = "CONFigure:DEBUg:ACTion",
        .callback = SCPI_ConfigureDebugAction,
    },
    {
        .pattern = "READ:DEBUg:ACTion?",
        .callback = SCPI_ReadDebugActionQ,
    },
    {
        .pattern = "CONFigure:DEBUg:EVENt",
        .callback = SCPI_ConfigureDebugEvent,
    },
    {
        .pattern = "READ:DEBUg:EVENt?",
        .callback = SCPI_ReadDebugEventQ,
    },
    {
        .pattern = "CONFigure:DEBUg:IO",
        .callback = SCPI_ConfigureDebugIO,
    },
    {
        .pattern = "READ:DEBUg:IO?",
        .callback = SCPI_ReadDebugIOQ,
    },
    SCPI_CMD_LIST_END};

scpi_interface_t scpi_interface = {
    .error = SCPI_Error,
    .write = SCPI_Write,
    .control = SCPI_Control,
    .flush = SCPI_Flush,
    .reset = SCPI_Reset,
};

char scpi_input_buffer[SCPI_INPUT_BUFFER_LENGTH];
scpi_error_t scpi_error_queue_data[SCPI_ERROR_QUEUE_SIZE];

scpi_t scpi_context;
