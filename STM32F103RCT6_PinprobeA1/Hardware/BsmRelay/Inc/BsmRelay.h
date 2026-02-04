#ifndef __BSMRELAY_H
#define __BSMRELAY_H

#include "stm32f1xx_hal.h"
#include "scpi-def.h"

#define CHECK_NUM 5

// 输入IO位定义
typedef enum {
    DOOR_SENSOR_UP      = 0x0001,
    DOOR_SENSOR_DOWN    = 0x0002,
    DOOR_SENSOR_MID     = 0x0004,
    USB_SENSOR_UP       = 0x0008,
    USB_SENSOR_DOWN     = 0x0010,
    LASER_SENSOR1       = 0x0020,
    LASER_SENSOR2       = 0x0040,
    LASER_SENSOR3       = 0x0080,
    LASER_SENSOR4       = 0x0100,
    DOOR_BUTTON1        = 0x0200,
    DOOR_BUTTON2        = 0x0400,
    STOP_BUTTON         = 0x0800,
    POWER_BUTTON        = 0x1000
} InputBits;

// 输出IO位定义
typedef enum {
    DOOR_OPEN       = 0x0001,
    DOOR_CLOSE      = 0x0002,
    USB_PNE_IN      = 0x0004,
    USB_PNE_OUT     = 0x0008,
    LED_GREEN       = 0x0010,
    LED_RED         = 0x0020,
    LED_YELLOW      = 0x0040,
    POWER_OUT       = 0x0080,
    POWER_LED       = 0x0100
} OutputBits;

// 气缸控制接口
uint8_t Cylinder_Write(uint32_t cylinder_id, scpi_choice_def_t cylinder_value);
scpi_choice_def_t Cylinder_Status(uint32_t cylinder_id);

// 锁定控制接口
uint8_t Lock_Write(scpi_choice_def_t lock_value);
scpi_choice_def_t Lock_Status(void);

// LED控制接口
uint8_t LED_Write(scpi_choice_def_t led_value);
scpi_choice_def_t LED_Status(void);

// 系统状态接口
scpi_choice_def_t SYS_Status(void);

// IO读取接口
uint8_t IO_Read(uint8_t checkNum, uint8_t direction, uint8_t* trueData);
uint8_t* InputIO_Read(uint8_t checkNum);
uint8_t* OutputIO_Read(uint8_t checkNum);

#endif // __BSMRELAY_H
