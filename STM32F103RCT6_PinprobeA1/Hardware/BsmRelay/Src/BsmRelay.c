#include "BsmRelay.h"
#include "ram_vector.h"
#include "flash.h"
#include <math.h>


extern scpi_choice_def_t cylinder_source[];
extern scpi_choice_def_t lock_source[];
extern scpi_choice_def_t led_source[];
extern scpi_choice_def_t sys_source[];
// extern uint8_t system_status;  /* [DEPRECATED] 旧 StateMachine.c 全局变量, 已改用 RamVector_GetState() */

// RS485通信状态标志：读/写操作失败时置false，禁止状态机使用不可靠数据
static bool rs485_io_ok = true;

static uint16_t LED_IoToMask(uint8_t io)
{
    if (io == 0U || io > 16U) {
        return 0U;
    }
    return (uint16_t)(1U << (io - 1U));
}

static bool LED_GetIoMap(uint8_t *green_io, uint8_t *red_io, uint8_t *yellow_io)
{
    Flash_GetLedMap(green_io, red_io, yellow_io);

    if (green_io == NULL || red_io == NULL || yellow_io == NULL) {
        return false;
    }

    if (*green_io < 5U || *green_io > 7U ||
        *red_io < 5U || *red_io > 7U ||
        *yellow_io < 5U || *yellow_io > 7U) {
        *green_io = 5U;
        *red_io = 6U;
        *yellow_io = 7U;
        return false;
    }

    if (*green_io == *red_io || *green_io == *yellow_io || *red_io == *yellow_io) {
        *green_io = 5U;
        *red_io = 6U;
        *yellow_io = 7U;
        return false;
    }

    return true;
}

uint16_t LED_GetMask(uint8_t led_tag)
{
    uint8_t green_io, red_io, yellow_io;
    (void)LED_GetIoMap(&green_io, &red_io, &yellow_io);

    switch (led_tag) {
    case 1: return LED_IoToMask(green_io);
    case 2: return LED_IoToMask(red_io);
    case 3: return LED_IoToMask(yellow_io);
    default: return 0U;
    }
}

uint16_t LED_GetAllMask(void)
{
    return (uint16_t)(LED_GetMask(1U) | LED_GetMask(2U) | LED_GetMask(3U));
}

uint8_t LED_DecodeState(uint16_t out_bits)
{
    if ((out_bits & LED_GetMask(1U)) != 0U) return 1U;
    if ((out_bits & LED_GetMask(2U)) != 0U) return 2U;
    if ((out_bits & LED_GetMask(3U)) != 0U) return 4U;
    return 0U;
}

uint8_t Cylinder_Write(uint32_t cylinder_id, scpi_choice_def_t cylinder_value)
{
    uint8_t ok = 1U;

    switch (cylinder_id)
    {
    case 1:
        if (cylinder_value.tag != 0) {
            ok &= WriteIO(2, 0) ? 1U : 0U;
            ok &= WriteIO(1, 1) ? 1U : 0U;
        } else {
            ok &= WriteIO(1, 0) ? 1U : 0U;
            ok &= WriteIO(2, 1) ? 1U : 0U;
        }
        break;
    case 2:
        if (cylinder_value.tag != 0) {
            ok &= WriteIO(4, 0) ? 1U : 0U;
            ok &= WriteIO(3, 1) ? 1U : 0U;
        } else {
            ok &= WriteIO(3, 0) ? 1U : 0U;
            ok &= WriteIO(4, 1) ? 1U : 0U;
        }
        break;
    default:
        ok = 0U;
        break;
    }
    return ok;
}

/* ── [DEPRECATED] 以下函数仅被旧 StateMachine.c 调用, 已随旧状态机废止 ── */

scpi_choice_def_t Cylinder_Status(uint32_t cylinder_id)
{
    uint8_t* O_status = OutputIO_Read(CHECK_NUM);
    uint8_t out_01_08 = O_status[0];
    uint8_t* I_status = InputIO_Read(CHECK_NUM);
    uint8_t in_01_08 = I_status[0];
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
    uint8_t ok = 1U;

    switch (lock_value.tag)
    {
    case 0:
        ok &= WriteIO(8, 1) ? 1U : 0U;
        ok &= WriteIO(9, 1) ? 1U : 0U;
        break;
    case 1:
        ok &= WriteIO(8, 0) ? 1U : 0U;
        ok &= WriteIO(9, 0) ? 1U : 0U;
        break;
    default:
        ok = 0U;
        break;
    }
    return ok;
}

scpi_choice_def_t Lock_Status(){
    uint8_t* O_status = OutputIO_Read(CHECK_NUM);
    uint8_t out_01_08 = O_status[0];

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
    uint8_t ok = 1U;
    uint8_t green_io, red_io, yellow_io;
    (void)LED_GetIoMap(&green_io, &red_io, &yellow_io);

    switch (led_value.tag)
    {
    case 0: // led OFF
        ok &= WriteIO(green_io, 0) ? 1U : 0U;
        ok &= WriteIO(red_io, 0) ? 1U : 0U;
        ok &= WriteIO(yellow_io, 0) ? 1U : 0U;
        break;
    case 1: // led G
        ok &= WriteIO(green_io, 1) ? 1U : 0U;
        ok &= WriteIO(red_io, 0) ? 1U : 0U;
        ok &= WriteIO(yellow_io, 0) ? 1U : 0U;
        break;
    case 2: // led R
        ok &= WriteIO(green_io, 0) ? 1U : 0U;
        ok &= WriteIO(red_io, 1) ? 1U : 0U;
        ok &= WriteIO(yellow_io, 0) ? 1U : 0U;
        break;
    case 3: // led Y
        ok &= WriteIO(green_io, 0) ? 1U : 0U;
        ok &= WriteIO(red_io, 0) ? 1U : 0U;
        ok &= WriteIO(yellow_io, 1) ? 1U : 0U;
        break;
    default:
        ok = 0U;
        break;
    }
    return ok;
}

scpi_choice_def_t LED_Status(){
    uint8_t* O_status = OutputIO_Read(CHECK_NUM);
    uint16_t out_bits = (uint16_t)O_status[0] | ((uint16_t)O_status[1] << 8);
    uint8_t state = LED_DecodeState(out_bits);

    if(state == 1U){
        return led_source[1];
    }
    else if(state == 2U){
        return led_source[2];
    }
    else if(state == 4U){
        return led_source[3];
    }
    else if((out_bits & LED_GetAllMask()) == 0U)
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

    if (trueData == NULL || (direction != 1 && direction != 2))
        return false;

    while (State_count < checkNum)
    {
        // Read bsm input IO status
        if (!ReadIO(direction))
        {
            State_count++;
            continue;
        }

        // 等待从机响应（50ms超时，替代原来的盲等HAL_Delay）
        if (!RS485_WaitRx(50))
        {
            State_count++;
            continue;
        }

        // 校验接收帧长度：地址+功能码+字节数+2数据+2CRC = 7字节
        if (Uart3_RxLength < 7)
        {
            State_count++;
            continue;
        }

        if (Uart3_BuffIsReady[0] != 0x01 ||
            Uart3_BuffIsReady[1] != direction ||
            Uart3_BuffIsReady[2] != 0x02)
        {
            State_count++;
            continue;
        }

        uint8_t crcData[2];
        uint8_t IOData[5];

        memcpy(IOData, Uart3_BuffIsReady, 5);
        crcData[0] = Uart3_BuffIsReady[5];
        crcData[1] = Uart3_BuffIsReady[6];

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
/* ── [DEPRECATED END] ── */
