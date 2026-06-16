/*
 * cmd_exec.c — VCMD → BsmRelay IO 写操作 (三通道)
 */

#include "cmd_exec.h"
#include "BsmRelay.h"
#include "app_log.h"

/* 外部 SCPI choice 表 */
extern scpi_choice_def_t cylinder_source[];
extern scpi_choice_def_t lock_source[];
extern scpi_choice_def_t led_source[];

void CmdExec_Init(void)
{
    /* nothing yet */
}

static void CmdExec_Lock(Vector_Cmd_t cmd)
{
    switch (cmd) {
    case VCMD_LOCK:
        Lock_Write(lock_source[1]);            /* LOCKED */
        break;
    case VCMD_UNLOCK:
        Lock_Write(lock_source[0]);            /* UNLOCK */
        break;
    default:
        break;
    }
}

static void CmdExec_Cylinder(Vector_Cmd_t cmd)
{
    const Vector_IOState_t *io = RamVector_GetLocalIO();
    uint8_t out_lo = io->raw_out_lo;

    switch (cmd) {
    case VCMD_CYLINDER_OPEN:
        if ((out_lo & 0x01U) == 0U)
            AppLog_Action(APPLOG_ACT_CYLINDER_OPEN, 0, 1);
        Cylinder_Write(1, cylinder_source[1]); /* OPEN */
        break;
    case VCMD_CYLINDER_CLOSE:
        if ((out_lo & 0x02U) == 0U)
            AppLog_Action(APPLOG_ACT_CYLINDER_CLOSE, 0, 1);
        Cylinder_Write(1, cylinder_source[0]); /* CLOSE */
        break;
    case VCMD_CYLINDER2_OPEN:
        if ((out_lo & 0x04U) == 0U)
            AppLog_Action(APPLOG_ACT_CYLINDER_OPEN, 0, 2);
        Cylinder_Write(2, cylinder_source[1]); /* USB 插入 */
        break;
    case VCMD_CYLINDER2_CLOSE:
        if ((out_lo & 0x08U) == 0U)
            AppLog_Action(APPLOG_ACT_CYLINDER_CLOSE, 0, 2);
        Cylinder_Write(2, cylinder_source[0]); /* USB 拔出 */
        break;
    default:
        break;
    }
}

static void CmdExec_LED(Vector_Cmd_t cmd)
{
    const Vector_IOState_t *io = RamVector_GetLocalIO();
    uint8_t out_lo = io->raw_out_lo;

    switch (cmd) {
    case VCMD_LED_OFF:
        if ((out_lo & 0x70U) != 0U)
            AppLog_Action(APPLOG_ACT_LED_OFF, 0, 0);
        LED_Write(led_source[0]);
        break;
    case VCMD_LED_GREEN:
        if ((out_lo & 0x10U) == 0U)
            AppLog_Action(APPLOG_ACT_LED_GREEN, 0, 0);
        LED_Write(led_source[1]);
        break;
    case VCMD_LED_RED:
        if ((out_lo & 0x20U) == 0U)
            AppLog_Action(APPLOG_ACT_LED_RED, 0, 0);
        LED_Write(led_source[2]);
        break;
    case VCMD_LED_YELLOW:
        if ((out_lo & 0x40U) == 0U)
            AppLog_Action(APPLOG_ACT_LED_YELLOW, 0, 0);
        LED_Write(led_source[3]);
        break;
    default: break;
    }
}

/* 三通道依次执行, 执行后清零对应槽位 */
void CmdExec_ExecuteAll(void)
{
    Vector_Cmd_t cmd;

    cmd = RamVector_GetLockCmd();
    if (cmd != VCMD_NONE) CmdExec_Lock(cmd);

    cmd = RamVector_GetCylinderCmd();
    if (cmd != VCMD_NONE) CmdExec_Cylinder(cmd);

    cmd = RamVector_GetLEDCmd();
    if (cmd != VCMD_NONE) CmdExec_LED(cmd);

    RamVector_ClearCmd();
}
