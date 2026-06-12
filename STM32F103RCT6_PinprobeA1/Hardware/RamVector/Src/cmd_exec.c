/*
 * cmd_exec.c — VCMD → BsmRelay IO 写操作
 */

#include "cmd_exec.h"
#include "BsmRelay.h"

/* 外部 SCPI choice 表 — 从 scpi-def.c 引用 */
extern scpi_choice_def_t cylinder_source[];
extern scpi_choice_def_t lock_source[];
extern scpi_choice_def_t led_source[];

void CmdExec_Init(void)
{
    /* 引用 event_table_size 消除未使用警告 (预留给事件驱动架构) */
    extern const uint8_t event_table_size;
    (void)event_table_size;
}

void CmdExec_Execute(Vector_Cmd_t cmd)
{
    switch (cmd) {

    case VCMD_CYLINDER_OPEN:
        Cylinder_Write(1, cylinder_source[1]); /* OPEN */
        break;

    case VCMD_CYLINDER_CLOSE:
        Cylinder_Write(1, cylinder_source[0]); /* CLOSE */
        break;

    case VCMD_LOCK:
        Lock_Write(lock_source[1]);            /* LOCKED */
        LED_Write(led_source[0]);              /* LED OFF */
        break;

    case VCMD_UNLOCK:
        Lock_Write(lock_source[0]);            /* UNLOCK */
        LED_Write(led_source[0]);              /* LED OFF */
        break;

    case VCMD_LED_OFF:    LED_Write(led_source[0]); break;
    case VCMD_LED_GREEN:  LED_Write(led_source[1]); break;
    case VCMD_LED_RED:    LED_Write(led_source[2]); break;
    case VCMD_LED_YELLOW: LED_Write(led_source[3]); break;

    case VCMD_ESTOP:
        /* E-STOP 在 Emerge_Action 中已单独处理, 此处为占位 */
        break;

    case VCMD_DOOR_READY:
        LED_Write(led_source[3]);              /* 黄灯亮 */
        break;

    default:
        break;
    }
}
