/*
 * ram_vector.c — RAM 向量表单机实现
 *
 *  向量表位于 0x2000A000, 使用 __attribute__((section)) 固定地址
 *  单机: 只操作 cmd + io_state[0], 其他字段为 0
 *  升级到多机: 启用 version/CRC/CAN 同步, 无需改布局
 */

#include "ram_vector.h"
#include <string.h>

/* ===== 固定地址向量表 ===== */
static RAM_Vector_t ram_vector __attribute__((section(".RamVector"), aligned(4)));

/* ===== 本节点 ID ===== */
static uint8_t local_node = 0;

/* ===== 初始化 ===== */
void RamVector_Init(uint8_t node_id)
{
    memset(&ram_vector, 0, sizeof(ram_vector));
    ram_vector.magic     = 0x50504156;   /* "PPAV" */
    ram_vector.end_magic = 0x454E4400;   /* "END\0" */
    ram_vector.initialized = 1;
    local_node = node_id;
    if (local_node == 0) local_node = 1;
}

RAM_Vector_t* RamVector_Get(void)
{
    return &ram_vector;
}

/* ===== 命令注入 ===== */
void RamVector_PostCmd(Vector_Cmd_t cmd)
{
    ram_vector.cmd = (uint16_t)cmd;
}

Vector_Cmd_t RamVector_GetCmd(void)
{
    return (Vector_Cmd_t)ram_vector.cmd;
}

void RamVector_ClearCmd(void)
{
    ram_vector.cmd = VCMD_NONE;
}

/* ===== IO 状态 ===== */
void RamVector_UpdateLocalIO(const Vector_IOState_t *io)
{
    if (local_node > 0 && local_node <= RAM_VECTOR_MAX_NODES) {
        memcpy(&ram_vector.io_state[local_node - 1], io, sizeof(Vector_IOState_t));
    }
}

const Vector_IOState_t* RamVector_GetLocalIO(void)
{
    if (local_node > 0 && local_node <= RAM_VECTOR_MAX_NODES) {
        return &ram_vector.io_state[local_node - 1];
    }
    return &ram_vector.io_state[0];
}

/* ===== 状态 ===== */
void RamVector_SetState(Vector_SysState_t s)
{
    ram_vector.global_state = (uint8_t)s;
}

Vector_SysState_t RamVector_GetState(void)
{
    return (Vector_SysState_t)ram_vector.global_state;
}

/* ===== 心跳 ===== */
void RamVector_Heartbeat(void)
{
    ram_vector.local_heartbeat++;
}

uint16_t RamVector_GetHeartbeat(void)
{
    return ram_vector.local_heartbeat;
}
