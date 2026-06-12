/*
 * cmd_exec.c — VCMD → BsmRelay IO 写操作 (三通道)
 */

#include "cmd_exec.h"
#include "BsmRelay.h"

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
    switch (cmd) {
    case VCMD_CYLINDER_OPEN:
        Cylinder_Write(1, cylinder_source[1]); /* OPEN */
        break;
    case VCMD_CYLINDER_CLOSE:
        Cylinder_Write(1, cylinder_source[0]); /* CLOSE */
        break;
    default:
        break;
    }
}

static void CmdExec_LED(Vector_Cmd_t cmd)
{
    switch (cmd) {
    case VCMD_LED_OFF:    LED_Write(led_source[0]); break;
    case VCMD_LED_GREEN:  LED_Write(led_source[1]); break;
    case VCMD_LED_RED:    LED_Write(led_source[2]); break;
    case VCMD_LED_YELLOW: LED_Write(led_source[3]); break;
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
