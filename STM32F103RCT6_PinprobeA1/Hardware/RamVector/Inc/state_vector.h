/*
 * state_vector.h — 向量表状态机
 */

#ifndef APP_RAMVECTOR_INC_STATE_VECTOR_H_
#define APP_RAMVECTOR_INC_STATE_VECTOR_H_

#include <stdbool.h>

/* ── 运行时调试开关 (由 SCPI CONFigure:DEBUg:xxx 控制) ── */
typedef struct {
    bool state;   /* 状态切换 + 耗时 */
    bool action;  /* 动作: CLOSE_START/DONE, OPEN_START/DONE, LOCK/UNLOCK */
    bool event;   /* 事件: ESTOP, LASER */
    bool io;      /* IO 原始值 (刷屏, 按需开) */
} VectorDebugFlags_t;

extern VectorDebugFlags_t vector_debug_flags;

void StateVector_Input(void);

#endif
