/*
 * ram_vector.c — RAM 向量表实现 (1024B @ 0x2000A000, 对齐主线)
 *
 *  单机: 操作 cmd + io_state[0] + node_status[0]
 *  多机: 启用 version/CRC/CAN 同步 + node_status[1..7]
 */

#include "ram_vector.h"
#include <string.h>

/* ===== 固定地址向量表 ===== */
static RAM_Vector_t ram_vector __attribute__((section("RamVector"), aligned(4)));

/* 本节点 ID: 0=单机出厂模式, 1~8=多机模式 */
static uint8_t local_node = 0;

/* 内部: node → 数组索引 (0→0, 1→0, 2→1, ..., 8→7) */
static inline uint8_t node_idx(void) {
    return (local_node == 0) ? 0 : (local_node - 1);
}

/* ===== 初始化 ===== */
void RamVector_Init(uint8_t node_id)
{
    memset(&ram_vector, 0, sizeof(ram_vector));
    ram_vector.magic     = 0x50504156;   /* "PPAV" */
    ram_vector.end_magic = 0x454E4400;   /* "END\0" */
    local_node = (node_id <= RAM_VECTOR_MAX_NODES) ? node_id : 0;
    if (local_node > 0)
        ram_vector.node_status[node_idx()].online = 1;
}

RAM_Vector_t* RamVector_Get(void)
{
    return &ram_vector;
}

/* ===== 命令 ===== */
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
    memcpy(&ram_vector.io_state[node_idx()], io, sizeof(Vector_IOState_t));
}

const Vector_IOState_t* RamVector_GetLocalIO(void)
{
    return &ram_vector.io_state[node_idx()];
}

/* ===== 状态 ===== */
void RamVector_SetState(Vector_SysState_t s)
{
    ram_vector.global_state = (uint8_t)s;
    ram_vector.node_status[node_idx()].state = (uint8_t)s;
}

Vector_SysState_t RamVector_GetState(void)
{
    return (Vector_SysState_t)ram_vector.global_state;
}

/* ===== 心跳 (仅多机模式) ===== */
void RamVector_Heartbeat(void)
{
    if (local_node > 0)
        ram_vector.node_status[node_idx()].heartbeat++;
}

uint16_t RamVector_GetHeartbeat(void)
{
    return ram_vector.node_status[node_idx()].heartbeat;
}
