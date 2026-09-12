/*
 * state_vector.h — 向量表状态机
 */

#ifndef APP_RAMVECTOR_INC_STATE_VECTOR_H_
#define APP_RAMVECTOR_INC_STATE_VECTOR_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    CONTROL_MODE_MIXED  = 0,
    CONTROL_MODE_LOCAL  = 1,
    CONTROL_MODE_REMOTE = 2,
} ControlMode_t;

/* ── 运行时调试开关 (由 SCPI CONFigure:DEBUg:xxx 控制) ── */
typedef struct {
    bool state;   /* 状态切换 + 耗时 */
    bool action;  /* 动作: CLOSE_START/DONE, OPEN_START/DONE, LOCK/UNLOCK */
    bool event;   /* 事件: ESTOP, LASER */
    bool io;      /* IO 原始值 (刷屏, 按需开) */
} VectorDebugFlags_t;

extern VectorDebugFlags_t vector_debug_flags;

ControlMode_t ControlMode_Get(void);
bool ControlMode_Set(ControlMode_t mode);
bool ControlMode_AllowsScpiAction(void);
bool ControlMode_AllowsPhysicalAction(void);

void StateVector_RequestDoorClose(void);
void StateVector_Input(void);

#endif
