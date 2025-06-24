#include "scpi_switch.h"


uint8_t Link_Write(uint16_t link_id, uint32_t link_value){
    switch (link_value)
    {
    case 0:
        Switch_Write(0,0);
        break;
    case 1:
        Switch_Write(2,0);
        break;
    case 2:
        Switch_Write(2,1);
        break;
    case 3:
        Switch_Write(2,2);
        break;
    case 4:
        Switch_Write(2,3);
        break;
    case 5:
        Switch_Write(2,4);
        break;
    case 6:
        Switch_Write(2,5);
        break;
    case 7:
        Switch_Write(2,6);
        break;
    case 8:
        Switch_Write(2,7);
        break;
    case 9:
        Switch_Write(3,0);
        break;
    case 10:
        Switch_Write(3,1);
        break;
    case 11:
        Switch_Write(3,2);
        break;
    case 12:
        Switch_Write(3,3);
        break;
    case 13:
        Switch_Write(3,4);
        break;
    case 14:
        Switch_Write(3,5);
        break;
    case 15:
        Switch_Write(3,6);
        break;
    case 16:
        Switch_Write(3,7);
        break;
    default:
        break;
    }
    return 0;
}

uint8_t Link_Read(uint16_t link_id, uint32_t* link_value){
    GPIO_PinState C1 =  HAL_GPIO_ReadPin(RF_C1_GPIO_Port, RF_C1_Pin);
    GPIO_PinState C2 =  HAL_GPIO_ReadPin(RF_C2_GPIO_Port, RF_C2_Pin);

    GPIO_PinState C3 =  HAL_GPIO_ReadPin(RF_C3_GPIO_Port, RF_C3_Pin);
    GPIO_PinState C4 =  HAL_GPIO_ReadPin(RF_C4_GPIO_Port, RF_C4_Pin);
    GPIO_PinState C5 =  HAL_GPIO_ReadPin(RF_C5_GPIO_Port, RF_C5_Pin);

    GPIO_PinState C6 =  HAL_GPIO_ReadPin(RF_C6_GPIO_Port, RF_C6_Pin);
    GPIO_PinState C7 =  HAL_GPIO_ReadPin(RF_C7_GPIO_Port, RF_C7_Pin);
    GPIO_PinState C8 =  HAL_GPIO_ReadPin(RF_C8_GPIO_Port, RF_C8_Pin);
    if(C1 ==(GPIO_PinState)1)
    {
        *link_value = 0;
    }
    else
    {
        if(C2 == (GPIO_PinState)1)
        {
            *link_value = ((C3)|(C4<<1)|(C5<<2))+1;
        }
        else
        {
            *link_value = ((C6)|(C7<<1)|(C8<<2)) + 9;
        }
    }
    return 0;
}

uint8_t Switch_Write(uint8_t switch_id, uint32_t switch_value)
{
    switch (switch_id)
    {
    case 0:
        HAL_GPIO_WritePin(RF_C1_GPIO_Port, RF_C1_Pin, (GPIO_PinState)1);
        break;
    case 1:
        HAL_GPIO_WritePin(RF_C1_GPIO_Port, RF_C1_Pin, (GPIO_PinState)0);
        HAL_GPIO_WritePin(RF_C2_GPIO_Port, RF_C2_Pin, (GPIO_PinState)switch_value);
        break;
    case 2:
        HAL_GPIO_WritePin(RF_C1_GPIO_Port, RF_C1_Pin, (GPIO_PinState)0);
        HAL_GPIO_WritePin(RF_C2_GPIO_Port, RF_C2_Pin, (GPIO_PinState)1);
        HAL_GPIO_WritePin(RF_C3_GPIO_Port, RF_C3_Pin, (GPIO_PinState)(switch_value & 0x01));
        HAL_GPIO_WritePin(RF_C4_GPIO_Port, RF_C4_Pin, (GPIO_PinState)((switch_value & 0x02)>>1));
        HAL_GPIO_WritePin(RF_C5_GPIO_Port, RF_C5_Pin, (GPIO_PinState)((switch_value & 0x04)>>2));
        break;
    case 3:
        HAL_GPIO_WritePin(RF_C1_GPIO_Port, RF_C1_Pin, (GPIO_PinState)0);
        HAL_GPIO_WritePin(RF_C2_GPIO_Port, RF_C2_Pin, (GPIO_PinState)0);
        HAL_GPIO_WritePin(RF_C6_GPIO_Port, RF_C6_Pin, (GPIO_PinState)(switch_value & 0x01));
        HAL_GPIO_WritePin(RF_C7_GPIO_Port, RF_C7_Pin, (GPIO_PinState)((switch_value & 0x02)>>1));
        HAL_GPIO_WritePin(RF_C8_GPIO_Port, RF_C8_Pin, (GPIO_PinState)((switch_value & 0x04)>>2));
        break;
    default:
        break;
    }
    return 0;
}

uint8_t Switch_Read(uint16_t switch_id, uint32_t* switch_value){
    GPIO_PinState C1 =  (GPIO_PinState)0;
    GPIO_PinState C2 =  (GPIO_PinState)0;

    GPIO_PinState C3 =  (GPIO_PinState)0;
    GPIO_PinState C4 =  (GPIO_PinState)0;
    GPIO_PinState C5 =  (GPIO_PinState)0;

    GPIO_PinState C6 =  (GPIO_PinState)0;
    GPIO_PinState C7 =  (GPIO_PinState)0;
    GPIO_PinState C8 =  (GPIO_PinState)0;

    switch (switch_id)
    {
        case 1:
            C1 =  HAL_GPIO_ReadPin(RF_C1_GPIO_Port, RF_C1_Pin);
            C2 =  HAL_GPIO_ReadPin(RF_C2_GPIO_Port, RF_C2_Pin);
            if(C1==(GPIO_PinState)1)
            {
                *switch_value = 0;
            }
            else if (C2 == (GPIO_PinState)1)
            {
                *switch_value = 1;
            }
            else if (C2 == (GPIO_PinState)0)
            {
                *switch_value = 0;
            }
            break;
        case 2:
            C3 =  HAL_GPIO_ReadPin(RF_C3_GPIO_Port, RF_C3_Pin);
            C4 =  HAL_GPIO_ReadPin(RF_C4_GPIO_Port, RF_C4_Pin);
            C5 =  HAL_GPIO_ReadPin(RF_C5_GPIO_Port, RF_C5_Pin);
            *switch_value = ((C3)|(C4<<1)|(C5<<2));
            break;
        case 3:
            C6 =  HAL_GPIO_ReadPin(RF_C6_GPIO_Port, RF_C6_Pin);
            C7 =  HAL_GPIO_ReadPin(RF_C7_GPIO_Port, RF_C7_Pin);
            C8 =  HAL_GPIO_ReadPin(RF_C8_GPIO_Port, RF_C8_Pin);
            *switch_value = ((C6)|(C7<<1)|(C8<<2));
            break;
        default:
            break;
    }
    return 0;
}