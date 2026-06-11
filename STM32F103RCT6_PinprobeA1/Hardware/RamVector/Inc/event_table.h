/*
 * event_table.h — 事件→命令 向量映射表
 *
 *  将所有输入源 (按钮/传感器/SCPI) 映射为统一的 Vector_Cmd_t
 *  状态机只需: 收集事件 → 查表 → RamVector_PostCmd(cmd)
 */

#ifndef APP_RAMVECTOR_INC_EVENT_TABLE_H_
#define APP_RAMVECTOR_INC_EVENT_TABLE_H_

#include "ram_vector.h"
#include <stdint.h>

/* 条件函数签名: 返回 true 允许触发 */
typedef bool (*EventGuard_t)(const Vector_IOState_t *io);

/* 事件向量条目 */
typedef struct {
    Vector_SysState_t from_state;   /* 允许的状态 (V_STATE_ANY=0xFF 表示任意) */
    EventID_t         event;        /* 事件 ID */
    EventGuard_t      guard;        /* 附加条件 (NULL=无条件) */
    Vector_Cmd_t      cmd;          /* 输出的命令 */
} EventVectorEntry_t;

#define V_STATE_ANY  0xFF           /* 通配: 任意状态 */

/* 事件表 — 在 event_table.c 中定义 */
extern const EventVectorEntry_t event_table[];
extern const uint8_t event_table_size;

/* 查表: 返回匹配的命令, 无匹配返回 VCMD_NONE */
Vector_Cmd_t EventTable_Lookup(Vector_SysState_t state, EventID_t event,
                               const Vector_IOState_t *io);

#endif /* APP_RAMVECTOR_INC_EVENT_TABLE_H_ */
