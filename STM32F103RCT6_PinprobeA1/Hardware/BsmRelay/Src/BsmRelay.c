#include "BsmRelay.h"
#include "StateMachine.h"
#include "RS485.h"

// 外部变量声明
extern scpi_choice_def_t cylinder_source[];
extern scpi_choice_def_t lock_source[];
extern scpi_choice_def_t led_source[];
extern scpi_choice_def_t sys_source[];
extern uint8_t* usart3_buff_IsReady;

/*************************************************
 * 气缸控制函数
 *************************************************/

/**
 * @brief 控制气缸动作
 * @param cylinder_id 气缸ID (1:门气缸, 2:USB气缸)
 * @param cylinder_value 控制值
 * @return 0表示成功
 */
uint8_t Cylinder_Write(uint32_t cylinder_id, scpi_choice_def_t cylinder_value) {
    switch (cylinder_id) {
        case 1: // 门气缸
            WriteIO(1, cylinder_value.tag);  // 开门输出
            WriteIO(2, !cylinder_value.tag); // 关门输出
            break;
            
        case 2: // USB气缸
            WriteIO(3, cylinder_value.tag);  // 伸出输出
            WriteIO(4, !cylinder_value.tag); // 缩回输出
            break;
            
        default:
            break;
    }
    return 0;
}

/**
 * @brief 获取气缸状态
 * @param cylinder_id 气缸ID
 * @return 气缸状态枚举值
 */
scpi_choice_def_t Cylinder_Status(uint32_t cylinder_id) {
    uint8_t* O_status = OutputIO_Read(CHECK_NUM);
    uint8_t out_01_08 = O_status[0];
    uint8_t* I_status = InputIO_Read(CHECK_NUM);
    uint8_t in_01_08 = I_status[0];

    if (cylinder_id == 1) { // 门气缸状态
        if (out_01_08 & DOOR_OPEN) { // 气缸伸出
            if (in_01_08 & DOOR_SENSOR_UP) {
                return cylinder_source[5]; // 气缸到达前限位
            }
            return cylinder_source[3]; // 气缸伸出中
        }
        
        if (out_01_08 & DOOR_CLOSE) { // 气缸缩回
            if (in_01_08 & DOOR_SENSOR_DOWN) {
                return cylinder_source[4]; // 气缸到达后限位
            }
            return cylinder_source[2]; // 气缸缩回中
        }
    }
    
    return cylinder_source[6]; // 未知状态
}

/*************************************************
 * 锁定控制函数
 *************************************************/

/**
 * @brief 控制锁定机构
 * @param lock_value 锁定值
 * @return 0表示成功
 */
uint8_t Lock_Write(scpi_choice_def_t lock_value) {
    switch (lock_value.tag) {
        case 0: // 解锁
            WriteIO(8, 1);
            WriteIO(9, 1);
            break;
            
        case 1: // 锁定
            WriteIO(8, 0);
            WriteIO(9, 0);
            break;
            
        default:
            break;
    }
    return 0;
}

/**
 * @brief 获取锁定状态
 * @return 锁定状态枚举值
 */
scpi_choice_def_t Lock_Status(void) {
    uint8_t* O_status = OutputIO_Read(CHECK_NUM);
    uint8_t out_01_08 = O_status[0];

    if (out_01_08 & POWER_OUT) {
        return lock_source[0]; // 解锁状态
    } else {
        return lock_source[1]; // 锁定状态
    }
}

/*************************************************
 * LED控制函数
 *************************************************/

/**
 * @brief 控制LED显示
 * @param led_value LED值
 * @return 0表示成功
 */
uint8_t LED_Write(scpi_choice_def_t led_value) {
    switch (led_value.tag) {
        case 0: // LED关闭
            WriteIO(5, 0);
            WriteIO(6, 0);
            WriteIO(7, 0);
            break;
            
        case 1: // 绿灯
            WriteIO(5, 1);
            WriteIO(6, 0);
            WriteIO(7, 0);
            break;
            
        case 2: // 红灯
            WriteIO(5, 0);
            WriteIO(6, 1);
            WriteIO(7, 0);
            break;
            
        case 3: // 黄灯
            WriteIO(5, 0);
            WriteIO(6, 0);
            WriteIO(7, 1);
            break;
            
        default:
            break;
    }
    return 0;
}

/**
 * @brief 获取LED状态
 * @return LED状态枚举值
 */
scpi_choice_def_t LED_Status(void) {
    uint8_t* O_status = OutputIO_Read(CHECK_NUM);
    uint8_t out_01_08 = O_status[0];

    if (out_01_08 & LED_GREEN) {
        return led_source[1]; // 绿灯
    } else if (out_01_08 & LED_RED) {
        return led_source[2]; // 红灯
    } else if (out_01_08 & LED_YELLOW) {
        return led_source[3]; // 黄灯
    } else {
        return led_source[0]; // 关闭
    }
}

/*************************************************
 * 系统状态函数
 *************************************************/

/**
 * @brief 获取系统状态
 * @return 系统状态枚举值
 * @note 通过状态机模块的公共接口获取状态，避免直接访问全局变量
 */
scpi_choice_def_t SYS_Status(void) {
    SystemState currentState = StateMachine_GetCurrentState();
    return sys_source[currentState];
}

/*************************************************
 * IO读取函数
 *************************************************/

/**
 * @brief 读取IO状态
 * @param checkNum 检查次数
 * @param direction 方向 (1:输出, 2:输入)
 * @param trueData 返回的数据
 * @return 0表示成功
 */
uint8_t IO_Read(uint8_t checkNum, uint8_t direction, uint8_t* trueData) {
    uint8_t State_count = 0;
    
    while (State_count < checkNum) {   
        uint8_t* data;
        ReadIO(direction, data);

        uint8_t crcData[2];
        uint8_t IOData[5];

        memcpy(IOData, usart3_buff_IsReady, 5);
        crcData[0] = usart3_buff_IsReady[5];
        crcData[1] = usart3_buff_IsReady[6];

        if (modbus_crc_compare(5, IOData, crcData)) {
            trueData[0] = IOData[3];
            trueData[1] = IOData[4];
            return 0;
        }
        State_count++;
    }
    return 0; 
}

/**
 * @brief 读取输入IO状态
 * @param checkNum 检查次数
 * @return 输入状态数据
 */
uint8_t* InputIO_Read(uint8_t checkNum) {
    static uint8_t data[2];
    IO_Read(checkNum, 2, data);
    return data;
}

/**
 * @brief 读取输出IO状态
 * @param checkNum 检查次数
 * @return 输出状态数据
 */
uint8_t* OutputIO_Read(uint8_t checkNum) {
    static uint8_t data[2];
    IO_Read(checkNum, 1, data);
    return data;
}
