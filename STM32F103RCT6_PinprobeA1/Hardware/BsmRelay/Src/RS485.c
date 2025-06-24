#include "RS485.h"

/// @brief Write BSM IO status
/// @param index IO index
/// @param status IO status
/// @return IO status
uint8_t WriteIO(uint8_t index, uint8_t status){
    HAL_Delay(10);
    uint8_t *data = IOWriteOrder(index-1,status);
    HAL_UART_Transmit(&huart3,data,8,0XFFFF);
    HAL_Delay(10);
    return status;
}

/// @brief Read BSM IO status
/// @param index IO index
/// @param status IO status
/// @return IO status
uint8_t* ReadIO(uint8_t index, uint8_t*status){
    HAL_Delay(10);
    uint8_t *data = IOReadOrder(index,16);
    HAL_UART_Transmit(&huart3,data,8,0XFFFF);
    HAL_Delay(10);
    return status;
}


/// @brief product Order 
/// @param index IO index
/// @param status IO status
/// @return Order
uint8_t* IOWriteOrder(uint8_t index, uint8_t status){
    static uint8_t stress_send[9];
    uint8_t buffer[6];
    uint16_t crc;
    uint8_t crc_data[2];
    buffer[0] = 0x01;
    buffer[1] = 0x05;
    buffer[2] = index>>8;
    buffer[3] = index&0xff;
    if(status == 0){
        buffer[4] = 0x00;
        buffer[5] = 0x00;
    }else if(status == 1){
        buffer[4] = 0xff;
        buffer[5] = 0x00;
    }
    crc = modbus_crc16(6, buffer);
    
    crc_data[1] = crc >>8;
    crc_data[0] = (crc & 0x00FF);

    stress_send[0] = buffer[0];
    stress_send[1] = buffer[1];
    stress_send[2] = buffer[2];
    stress_send[3] = buffer[3];
    stress_send[4] = buffer[4];
    stress_send[5] = buffer[5];
    stress_send[6] = crc_data[0];
    stress_send[7] = crc_data[1];
    stress_send[8] = 0;

    return stress_send;
    
}

/// @brief product Order 
/// @param index IO Index
/// @param num IO num
/// @return 
uint8_t* IOReadOrder(uint8_t index, uint16_t num){
    static uint8_t stress_send[9];
    uint8_t buffer[6];
    uint16_t crc;
    uint8_t crc_data[2];
    buffer[0] = 0x01;
    buffer[1] = index;
    buffer[2] = 0x00;
    buffer[3] = 0x00;
    buffer[4] = (num>>8);
    buffer[5] = (num&0xff);

    crc = modbus_crc16(6, buffer);
    
    crc_data[1] = crc >>8;
    crc_data[0] = (crc & 0x00FF);

    stress_send[0] = buffer[0];
    stress_send[1] = buffer[1];
    stress_send[2] = buffer[2];
    stress_send[3] = buffer[3];
    stress_send[4] = buffer[4];
    stress_send[5] = buffer[5];
    stress_send[6] = crc_data[0];
    stress_send[7] = crc_data[1];
    stress_send[8] = 0;
    return stress_send;
}


uint16_t modbus_crc16(uint16_t length, uint8_t *data)
{
    uint16_t crc = 0xffff;
    bool t = 0;
    uint16_t R = 0xa001;
    while(length){
        crc = crc ^ *data++;
        for(int a=0; a<8; ){
            t = crc % 2;
            crc = crc / 2;
            a++;
            if(t == 1){
                crc = crc ^ R;
            } 
        }
        length--;
    }
    return crc;
}

bool modbus_crc_compare(uint16_t length, uint8_t *data, uint8_t *compareData)
{
    uint16_t crc;
    uint8_t crc_data[2];

    crc = modbus_crc16(length, data);
    crc_data[1] = crc >>8;
    crc_data[0] = (crc & 0x00FF);

    if(crc_data[0]== compareData[0] && crc_data[1] == compareData[1])
    {
        return true;
    }
    return false;
}