#ifndef __RS485_H
#define __RS485_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "usart.h"
#include "stm32f1xx_hal.h"

uint8_t WriteIO(uint8_t index, uint8_t status);
uint8_t* ReadIO(uint8_t index, uint8_t* status);
uint8_t* IOWriteOrder(uint8_t index, uint8_t num);
uint8_t* IOReadOrder(uint8_t index, uint16_t num);
uint16_t modbus_crc16(uint16_t length, uint8_t *data);
bool modbus_crc_compare(uint16_t length, uint8_t *data, uint8_t *compareData);
#endif // !__RS485_H