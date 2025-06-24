#ifndef __SCPI_SWITCH_H
#define __SCPI_SWITCH_H


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include<string.h>
#include "stm32f1xx_hal.h"
#include "gpio.h"

uint8_t Link_Write(uint16_t link_id, uint32_t link_value);
uint8_t Link_Read(uint16_t link_id, uint32_t* link_value);
uint8_t Switch_Write(uint8_t switch_id, uint32_t switch_value);
uint8_t Switch_Read(uint16_t switch_id, uint32_t* switch_value);
#endif // !__SCPI_SWITCH_H