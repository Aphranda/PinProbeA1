#include "BsmRelay.h"
#include "ram_vector.h"
#include <math.h>


extern scpi_choice_def_t cylinder_source[];
extern scpi_choice_def_t lock_source[];
extern scpi_choice_def_t led_source[];
extern scpi_choice_def_t sys_source[];
extern uint8_t system_status;

// RS485通信状态标志：读/写操作失败时置false，禁止状态机使用不可靠数据
static bool rs485_io_ok = true;

uint8_t Cylinder_Write(uint32_t cylinder_id, scpi_choice_def_t cylinder_value)
{
    switch (cylinder_id)
    {
    case 1:
        rs485_io_ok = WriteIO(1,cylinder_value.tag) && rs485_io_ok;
        rs485_io_ok = WriteIO(2,!cylinder_value.tag) && rs485_io_ok;
        break;
    case 2:
        rs485_io_ok = WriteIO(3,cylinder_value.tag) && rs485_io_ok;
        rs485_io_ok = WriteIO(4,!cylinder_value.tag) && rs485_io_ok;
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
        rs485_io_ok = WriteIO(8,1) && rs485_io_ok;
        rs485_io_ok = WriteIO(9,1) && rs485_io_ok;
        break;
    case 1:
        rs485_io_ok = WriteIO(8,0) && rs485_io_ok;
        rs485_io_ok = WriteIO(9,0) && rs485_io_ok;
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
    else if(!(out_01_08&power_out))
    {
        return lock_source[1];
    }
    return lock_source[2];
}

uint8_t LED_Write(scpi_choice_def_t led_value)
{
    bool ok = true;
    switch (led_value.tag)
    {
    case 0: // led OFF
        ok = WriteIO(5,0) && ok;
        ok = WriteIO(6,0) && ok;
        ok = WriteIO(7,0) && ok;
        break;
    case 1: // led G
        ok = WriteIO(5,1) && ok;
        ok = WriteIO(6,0) && ok;
        ok = WriteIO(7,0) && ok;
        break;
    case 2: // led R
        ok = WriteIO(5,0) && ok;
        ok = WriteIO(6,1) && ok;
        ok = WriteIO(7,0) && ok;
        break;
    case 3: // led Y
        ok = WriteIO(5,0) && ok;
        ok = WriteIO(6,0) && ok;
        ok = WriteIO(7,1) && ok;
        break;
    default:
        break;
    }
    rs485_io_ok = ok && rs485_io_ok;
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
    else if((!(out_01_08&led_green))&&(!(out_01_08&led_red))&&(!(out_01_08&led_yellow)))
    {
        return led_source[0];
    }
    return led_source[4];
}

scpi_choice_def_t SYS_Status(){
    return sys_source[RamVector_GetState()];
}

// IO状态缓存，避免返回局部变量指针
static uint8_t input_io_cache[2] = {0, 0};
static uint8_t output_io_cache[2] = {0, 0};

bool IO_Read(uint8_t checkNum, uint8_t direction, uint8_t* trueData){
    uint8_t State_count = 0;
    while (State_count < checkNum)
    {
        // Read bsm input IO status
        ReadIO(direction);

        // 等待从机响应（50ms超时，替代原来的盲等HAL_Delay）
        if (!RS485_WaitRx(50))
        {
            State_count++;
            continue;
        }

        // 校验接收帧长度（至少6字节：地址+功能码+字节数+1数据+2CRC）
        if (usart3_rx_length < 6)
        {
            State_count++;
            continue;
        }

        uint8_t crcData[2];
        uint8_t IOData[5];

        memcpy(IOData, usart3_buff_IsReady, 5);
        crcData[0] = usart3_buff_IsReady[5];
        crcData[1] = usart3_buff_IsReady[6];

        if(modbus_crc_compare(5, IOData, crcData))
        {
            trueData[0] = IOData[3];
            trueData[1] = IOData[4];
            return true;
        }
        State_count++;
    }
    return false;
}

bool IsRS485_Ok(void) { return rs485_io_ok; }
void SetRS485_Ok(bool ok) { rs485_io_ok = ok; }

uint8_t* InputIO_Read(uint8_t checkNum){
    IO_Read(checkNum, 2, input_io_cache);
    return input_io_cache;
}

uint8_t* OutputIO_Read(uint8_t checkNum){
    IO_Read(checkNum, 1, output_io_cache);
    return output_io_cache;
}