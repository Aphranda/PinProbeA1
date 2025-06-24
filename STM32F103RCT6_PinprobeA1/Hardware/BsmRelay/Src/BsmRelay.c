#include "BsmRelay.h"
#include <math.h>


extern scpi_choice_def_t cylinder_source[];
extern scpi_choice_def_t lock_source[];
extern scpi_choice_def_t led_source[];
extern scpi_choice_def_t sys_source[];
extern uint8_t system_status;


uint8_t Cylinder_Write(uint32_t cylinder_id, scpi_choice_def_t cylinder_value)
{
    switch (cylinder_id)
    {
    case 1:
        WriteIO(1,cylinder_value.tag);
        WriteIO(2,!cylinder_value.tag);
        break;
    case 2:
        WriteIO(3,cylinder_value.tag);
        WriteIO(4,!cylinder_value.tag);
        break;
    default:
        break;
    }
    return 0;
}

scpi_choice_def_t Cylinder_Status(uint32_t cylinder_id)
{
    uint8_t* O_status = OutputIO_Read(CHECK_NUM);
    uint8_t out_01_08 = O_status[0];
    uint8_t out_09_16 = O_status[1];
    uint8_t* I_status = InputIO_Read(CHECK_NUM);
    uint8_t in_01_08 = I_status[0];
    uint8_t in_09_16 = I_status[1];
    if(out_01_08&door_open) // 气缸伸出
    {
        if(in_01_08&door_sensor_up) // 触发前限位传感
        {
            return cylinder_source[5]; // 气缸到达前限位
        }
        return cylinder_source[3]; // 气缸回缩中
    }
    if(out_01_08&door_close) //气缸回缩
    {
        if(in_01_08&door_sensor_down) // 触发后限位传感
        {
            return cylinder_source[4]; // 气缸到达后限位
        }
        return cylinder_source[2]; // 气缸回缩中
    }
    return cylinder_source[6];
}

uint8_t Lock_Write(scpi_choice_def_t lock_value)
{
    switch (lock_value.tag)
    {
    case 0:
        WriteIO(8,1);
        WriteIO(9,1);
        break;
    case 1:
        WriteIO(8,0);
        WriteIO(9,0);
        break;
    default:
        break;
    }
    return 0;
}

scpi_choice_def_t Lock_Status(){
    uint8_t* O_status = OutputIO_Read(CHECK_NUM);
    uint8_t out_01_08 = O_status[0];
    uint8_t out_09_16 = O_status[1];
    uint8_t* I_status = InputIO_Read(CHECK_NUM);
    uint8_t in_01_08 = I_status[0];
    uint8_t in_09_16 = I_status[1];

    if(out_01_08&power_out)
    {
        return lock_source[0];
    }
    else if(!out_01_08&power_out)
    {
        return lock_source[1];
    }
    return lock_source[2];
}

uint8_t LED_Write(scpi_choice_def_t led_value)
{
    switch (led_value.tag)
    {
    case 0: // led OFF
        WriteIO(5,0);
        WriteIO(6,0);
        WriteIO(7,0);
        break;
    case 1: // led G
        WriteIO(5,1);
        WriteIO(6,0);
        WriteIO(7,0);
        break;
    case 2: // led R
        WriteIO(5, 0);
        WriteIO(6, 1);
        WriteIO(7, 0);
        break;
    case 3: // led Y
        WriteIO(5, 0);
        WriteIO(6, 0);
        WriteIO(7, 1);
        break;
    default:
        break;
    }
    return 0;
}

scpi_choice_def_t LED_Status(){
    uint8_t* O_status = OutputIO_Read(CHECK_NUM);
    uint8_t out_01_08 = O_status[0];
    uint8_t out_09_16 = O_status[1];
    uint8_t* I_status = InputIO_Read(CHECK_NUM);
    uint8_t in_01_08 = I_status[0];
    uint8_t in_09_16 = I_status[1];

    if(out_01_08&led_green){
        return led_source[1];
    }
    else if(out_01_08&led_red){
        return led_source[2];
    }
    else if(out_01_08&led_yellow){
        return led_source[3];
    }
    else if((!out_01_08&led_green)&&(!out_01_08&led_red)&&(!out_01_08&led_yellow))
    {
        return led_source[0];
    }
    return led_source[4];
}

scpi_choice_def_t SYS_Status(){
    return sys_source[system_status];
}

uint8_t IO_Read(uint8_t checkNum, uint8_t direction, uint8_t* trueData){
    uint8_t State_count = 0;
    while (State_count < checkNum)
    {   
        // Read bsm input IO status
        uint8_t* data;
        ReadIO(direction, data);

        uint8_t crcData[2];
        uint8_t IOData[5];

        memcpy(IOData, usart3_buff_IsReady, 5);
        crcData[0] = usart3_buff_IsReady[5];
        crcData[1] = usart3_buff_IsReady[6];

        if(modbus_crc_compare(5, IOData, crcData))
        {
            trueData[0] = IOData[3];
            trueData[1] = IOData[4];
            return 0;
        }
        State_count++;
    }
    return 0; 
}

uint8_t* InputIO_Read(uint8_t checkNum){
    uint8_t data[2];
    IO_Read(checkNum, 2, data);
    return data;
}

uint8_t* OutputIO_Read(uint8_t checkNum){
    uint8_t data[2];
    IO_Read(checkNum, 1, data);
    return data;
}