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

#ifndef __SCPI_DEF_H_
#define __SCPI_DEF_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "scpi/scpi.h"
#include "flash.h"

#define SCPI_INPUT_BUFFER_LENGTH 256
#define SCPI_ERROR_QUEUE_SIZE 17
#define SCPI_IDN1 "GTS"
#define SCPI_IDN2 "PINPROBEA1"
#define SCPI_IDN3 "20250626"
#define SCPI_IDN4 "V0.0.1"

/* 固件版本 (编译时嵌入, 发布前更新 git hash) */
#define FW_VERSION  "v1.0.0"
#define FW_GIT_HASH "87ea8cd199da98496c1d17343541c56bcb587c54"

/* 运行时 SCPI IDN 缓冲区 - 可在运行时通过 SCPI 命令修改 */
#define SCPI_IDN_BUF1_SIZE FLASH_SCPI_IDN1_LEN
#define SCPI_IDN_BUF2_SIZE FLASH_SCPI_IDN2_LEN
#define SCPI_IDN_BUF3_SIZE FLASH_SCPI_IDN3_LEN
#define SCPI_IDN_BUF4_SIZE FLASH_SCPI_IDN4_LEN

extern char scpi_idn_buf1[SCPI_IDN_BUF1_SIZE];
extern char scpi_idn_buf2[SCPI_IDN_BUF2_SIZE];
extern char scpi_idn_buf3[SCPI_IDN_BUF3_SIZE];
extern char scpi_idn_buf4[SCPI_IDN_BUF4_SIZE];

extern const scpi_command_t scpi_commands[];
extern scpi_interface_t scpi_interface;
extern char scpi_input_buffer[];
extern scpi_error_t scpi_error_queue_data[];
extern scpi_t scpi_context;

size_t SCPI_Write(scpi_t * context, const char * data, size_t len);
int SCPI_Error(scpi_t * context, int_fast16_t err);
scpi_result_t SCPI_Control(scpi_t * context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val);
scpi_result_t SCPI_Reset(scpi_t * context);
scpi_result_t SCPI_Flush(scpi_t * context);

/**
 * @brief 从 Flash 配置同步 SCPI IDN 字符串到运行时缓冲区
 * @note 调用此函数后，*IDN? 将返回 Flash 中存储的值
 */
void SCPI_SyncIdnFromFlash(void);

#ifdef __cplusplus
}
#endif
#endif /* __SCPI_DEF_H_ */

