#ifndef __BSMRELAY_H
#define __BSMRELAY_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "usart.h"
#include "stm32f1xx_hal.h"
#include "scpi-def.h"
#include "RS485.h"

#define CHECK_NUM 5

extern uint8_t* usart1_buff_IsReady;
extern uint8_t* usart1_buff_Occupied;
extern uint8_t* usart3_buff_IsReady;
extern uint8_t* usart3_buff_Occupied;

typedef enum {
    door_sensor_up      = 0x0001,
    door_sensor_down    = 0x0002,
    door_sensor_mid     = 0x0004,
    usb_sensor_up       = 0x0008,
    usb_sensor_down     = 0x0010,
    laser_sensor1       = 0x0020,
    laser_sensor2       = 0x0040,
    laser_sensor3       = 0x0080,
    laser_sensor4       = 0x0100,
    door_button1        = 0x0200,
    door_button2        = 0x0400,
    stop_button         = 0x0800,
    power_button        = 0x1000,
}inputIO;

typedef enum{
    door_open       = 0x0001,
    door_close      = 0x0002,
    usb_pne_in      = 0x0004,
    usb_pne_out     = 0x0008,
    led_green       = 0x0010,
    led_red         = 0x0020,
    led_yellow      = 0x0040,
    power_out       = 0x0080,
    power_led       = 0x0100,
}outputIO;

uint8_t Cylinder_Write(uint32_t cylinder_id, scpi_choice_def_t cylinder_value);

scpi_choice_def_t Cylinder_Status(uint32_t cylinder_id);

uint8_t Lock_Write(scpi_choice_def_t lock_value);

scpi_choice_def_t Lock_Status();

uint8_t LED_Write(scpi_choice_def_t led_value);

scpi_choice_def_t LED_Status();

scpi_choice_def_t SYS_Status();

uint8_t IO_Read(uint8_t checkNum, uint8_t direction, uint8_t* trueData);

uint8_t* InputIO_Read(uint8_t checkNum);

uint8_t* OutputIO_Read(uint8_t checkNum);

#endif // !__BSMRELAY_H
