/*
 * event_table.c — 事件向量表 + 条件函数
 *
 *  输入: 当前状态 + 事件ID + IO镜像 → 输出: Vector_Cmd_t
 */

#include "event_table.h"

/* ===== 条件函数 ===== */

/* 门按钮已释放 (release_start_tick已归零) */
/* ===== 事件向量表 ===== */

/* clang-format off */
const EventVectorEntry_t event_table[] = {

    /* ── 按钮: power_button → LOCK / UNLOCK (任意状态都有效) ── */
    { V_STATE_ANY,  EV_POWER_BTN,   NULL,               VCMD_UNLOCK },  /* Lock下=解锁 */
    { V_STATE_ANY,  EV_POWER_BTN,   NULL,               VCMD_LOCK   },  /* 非Lock下=锁定 */
    /* 实际逻辑: 由上层根据 output_io 判断当前是Lock还是Unlock后选方向 */

    /* ── 关门按钮: Idle → Ready ── */
    { V_STATE_IDLE, EV_DOOR1_BTN,   NULL,               VCMD_DOOR_READY },
    { V_STATE_IDLE, EV_DOOR2_BTN,   NULL,               VCMD_DOOR_READY },

    /* ── 关门按钮×2: Ready → 关门 ── */
    { V_STATE_READY, EV_DOOR1_BTN,  NULL,               VCMD_CYLINDER_CLOSE },
    { V_STATE_READY, EV_DOOR2_BTN,  NULL,               VCMD_CYLINDER_CLOSE },

    /* ── 关门按钮: Complete → 开门 ── */
    { V_STATE_COMPLETE, EV_DOOR1_BTN, NULL,              VCMD_CYLINDER_OPEN },
    { V_STATE_COMPLETE, EV_DOOR2_BTN, NULL,              VCMD_CYLINDER_OPEN },

    /* ── SCPI 命令 (任意状态) ── */
    { V_STATE_ANY,  EV_SCPI_CYLINDER, NULL,              VCMD_CYLINDER_OPEN },  /* param区分开/关 */
    { V_STATE_ANY,  EV_SCPI_LOCK,     NULL,              VCMD_LOCK },          /* param区分锁/解锁 */
    { V_STATE_ANY,  EV_SCPI_LED,      NULL,              VCMD_LED_OFF },       /* param区分颜色 */

    /* ── 传感器触发: 急停 (任意状态) ── */
    { V_STATE_ANY,  EV_ESTOP_BTN,    NULL,               VCMD_ESTOP },
    { V_STATE_ANY,  EV_LASER_ANY,    NULL,               VCMD_ESTOP },

    /* ── 终止 ── */
    { (Vector_SysState_t)0, (EventID_t)0, NULL, VCMD_NONE }
};
/* clang-format on */

const uint8_t event_table_size = sizeof(event_table) / sizeof(event_table[0]) - 1;

/* ===== 查表函数 ===== */

Vector_Cmd_t EventTable_Lookup(Vector_SysState_t state, EventID_t event,
                               const Vector_IOState_t *io)
{
    for (int i = 0; event_table[i].cmd != VCMD_NONE; i++) {
        const EventVectorEntry_t *e = &event_table[i];

        /* 状态匹配 */
        if (e->from_state != V_STATE_ANY && e->from_state != state)
            continue;

        /* 事件匹配 */
        if (e->event != event)
            continue;

        /* 条件检查 */
        if (e->guard && !e->guard(io))
            continue;

        return e->cmd;
    }
    return VCMD_NONE;
}
