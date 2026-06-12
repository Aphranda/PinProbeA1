#ifndef __RS485_H
#define __RS485_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "usart.h"
#include "stm32f1xx_hal.h"

// RS485 发送超时(ms)，避免总线故障时死锁
#define RS485_TX_TIMEOUT_MS  100

// USART3 接收帧长度（在 stm32f1xx_it.c 的 IDLE 中断中更新）
extern uint32_t Uart3_RxLength;

bool WriteIO(uint8_t index, uint8_t status);
// 需要在 stm32f1xx_it.c 中定义
extern uint8_t Usart3_RX_BUF1_IsReady;

void ReadIO(uint8_t func);
bool RS485_WaitRx(uint32_t timeout_ms);
uint8_t* IOWriteOrder(uint8_t index, uint8_t num);
uint8_t* IOReadOrder(uint8_t index, uint16_t num);
uint16_t modbus_crc16(uint16_t data_len, uint8_t *data);
bool modbus_crc_compare(uint16_t data_len, uint8_t *data, uint8_t *compareData);

/// @brief 读 ModBus 保持寄存器 (功能码 03)
/// @param reg_addr 寄存器地址 (0-based)
/// @param out_hi   输出: 寄存器高字节
/// @param out_lo   输出: 寄存器低字节
/// @return true=成功, false=超时或CRC错误
bool ReadHoldingRegister(uint16_t reg_addr, uint8_t *out_hi, uint8_t *out_lo);
#endif // !__RS485_H
