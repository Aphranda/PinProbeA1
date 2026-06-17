#include "RS485.h"
#include "tim.h"

/* 前向声明 */
static void IOWriteOrder(uint8_t index, uint8_t status, uint8_t *out_buf);
static void IOReadOrder(uint8_t index, uint16_t num, uint8_t *out_buf);

/// @brief 写 BSM IO 状态（Modbus 05 功能码）
/// @param index  IO 索引 (1-based)
/// @param status 0=OFF, 1=ON
/// @return true=发送成功
bool WriteIO(uint8_t index, uint8_t status){
    HAL_Delay(2);
    uint8_t frame[8];
    IOWriteOrder(index-1, status, frame);
    HAL_StatusTypeDef ret = HAL_UART_Transmit(&huart3, frame, 8, RS485_TX_TIMEOUT_MS);
    HAL_Delay(2);
    return (ret == HAL_OK);
}

/// @brief Read BSM IO status
/// @param func 功能码 (1=读输出, 2=读输入)
bool ReadIO(uint8_t func){
    HAL_Delay(2);  // 总线保护间隔
    uint8_t frame[8];
    IOReadOrder(func, 16, frame);
    HAL_StatusTypeDef ret = HAL_UART_Transmit(&huart3, frame, 8, RS485_TX_TIMEOUT_MS);
    HAL_Delay(2);  // 总线释放
    return (ret == HAL_OK);
}

/// @brief 等待RS485从机响应（轮询双缓冲就绪标志）
/// @param timeout_ms 超时(ms)
/// @return true=收到响应, false=超时
bool RS485_WaitRx(uint32_t timeout_ms)
{
    uint32_t start = GetTim1Ms();
    uint8_t prev_ready = Usart3_RX_BUF1_IsReady;

    while ((GetTim1Ms() - start) < timeout_ms)
    {
        if (Usart3_RX_BUF1_IsReady != prev_ready)
        {
            return true; // 缓冲区切换，新数据到达
        }
        HAL_Delay(1); // 1ms 忙等，不切换RTOS任务
    }
    return false;
}


/// @brief 构造 Modbus 05 (写单线圈) 帧
/// @param index   IO 索引 (0-based)
/// @param status  0=OFF, 1=ON
/// @param out_buf  输出缓冲区 (8 字节, 调用方提供)
static void IOWriteOrder(uint8_t index, uint8_t status, uint8_t *out_buf){
    uint8_t buffer[6];
    uint16_t crc;

    buffer[0] = 0x01;
    buffer[1] = 0x05;
    buffer[2] = index >> 8;
    buffer[3] = index & 0xFF;
    buffer[4] = (status == 1) ? 0xFF : 0x00;
    buffer[5] = 0x00;

    crc = modbus_crc16(6, buffer);

    out_buf[0] = buffer[0];
    out_buf[1] = buffer[1];
    out_buf[2] = buffer[2];
    out_buf[3] = buffer[3];
    out_buf[4] = buffer[4];
    out_buf[5] = buffer[5];
    out_buf[6] = crc & 0xFF;
    out_buf[7] = crc >> 8;
}

/// @brief 构造 Modbus 读 IO 帧 (功能码 01 或 02)
/// @param index   功能码 (1=读线圈, 2=读离散输入)
/// @param num     读取数量
/// @param out_buf 输出缓冲区 (8 字节, 调用方提供)
static void IOReadOrder(uint8_t index, uint16_t num, uint8_t *out_buf){
    uint8_t buffer[6];
    uint16_t crc;

    buffer[0] = 0x01;
    buffer[1] = index;
    buffer[2] = 0x00;
    buffer[3] = 0x00;
    buffer[4] = (num >> 8) & 0xFF;
    buffer[5] = num & 0xFF;

    crc = modbus_crc16(6, buffer);

    out_buf[0] = buffer[0];
    out_buf[1] = buffer[1];
    out_buf[2] = buffer[2];
    out_buf[3] = buffer[3];
    out_buf[4] = buffer[4];
    out_buf[5] = buffer[5];
    out_buf[6] = crc & 0xFF;
    out_buf[7] = crc >> 8;
}


// Modbus CRC16 查表计算（结果等同于逐位算法，手动验证通过）
static const uint16_t crc16_table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

/// @brief Modbus CRC16 查表计算（data_len 为数据字节数，不含 CRC）
uint16_t modbus_crc16(uint16_t data_len, uint8_t *data)
{
    uint16_t crc = 0xFFFF;
    while (data_len--)
    {
        crc = (crc >> 8) ^ crc16_table[(crc ^ *data++) & 0xFF];
    }
    return crc;
}

/// @brief 校验接收帧 CRC（compareData 为帧中 CRC 字段，低字节在前）
bool modbus_crc_compare(uint16_t data_len, uint8_t *data, uint8_t *compareData)
{
    uint16_t crc;
    uint8_t crc_data[2];

    crc = modbus_crc16(data_len, data);
    crc_data[1] = crc >>8;           // CRC 高字节
    crc_data[0] = (crc & 0x00FF);   // CRC 低字节

    if(crc_data[0]== compareData[0] && crc_data[1] == compareData[1])
    {
        return true;
    }
    return false;
}

/// @brief 读 ModBus 保持寄存器 (功能码 03)
bool ReadHoldingRegister(uint16_t reg_addr, uint8_t *out_hi, uint8_t *out_lo)
{
    uint8_t cmd[8];
    uint16_t crc;

    cmd[0] = 0x01;                       /* 从机地址 */
    cmd[1] = 0x03;                       /* 功能码: 读保持寄存器 */
    cmd[2] = (reg_addr >> 8) & 0xFF;     /* 寄存器地址高字节 */
    cmd[3] = reg_addr & 0xFF;            /* 寄存器地址低字节 */
    cmd[4] = 0x00;                       /* 寄存器数量高字节 */
    cmd[5] = 0x01;                       /* 寄存器数量低字节 (读 1 个) */
    crc = modbus_crc16(6, cmd);
    cmd[6] = crc & 0xFF;                 /* CRC 低字节 */
    cmd[7] = (crc >> 8) & 0xFF;          /* CRC 高字节 */

    HAL_Delay(2);
    HAL_UART_Transmit(&huart3, cmd, 8, RS485_TX_TIMEOUT_MS);
    HAL_Delay(2);

    if (!RS485_WaitRx(60))
        return false;

    /* 校验响应: 地址+功能码+字节数+2数据+2CRC = 7 字节 */
    if (Uart3_RxLength < 7)
        return false;

    uint8_t *resp = Uart3_BuffIsReady;
    if (resp[0] != 0x01 || resp[1] != 0x03 || resp[2] != 0x02)
        return false;

    uint8_t crc_buf[2];
    crc_buf[0] = resp[5];
    crc_buf[1] = resp[6];
    if (!modbus_crc_compare(5, resp, crc_buf))
        return false;

    *out_hi = resp[3];
    *out_lo = resp[4];
    return true;
}
