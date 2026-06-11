/*
 * state_vector.c — 纯逻辑状态机 (不直接读写 RS485)
 *
 *  IO 数据来自 RamVector (ModBusTask 异步更新)
 *  事件 → PostCmd → ModBusTask 执行
 */

#include "state_vector.h"
#include "ram_vector.h"
#include "tim.h"
#include "flash.h"
#include <string.h>

/* ── 调试 ── */
#define VECTOR_DEBUG
#ifdef VECTOR_DEBUG
#define VEC_ACTION(n,e)  U1_Printf("[%s] %u ms\r\n", n, (unsigned int)(e))
#define VEC_STATE(s)     U1_Printf("%s\r\n", (s))
#define VEC_EVENT(e)     U1_Printf("[EVENT] %s\r\n", (e))
#else
#define VEC_ACTION(n,e)  do{}while(0)
#define VEC_STATE(s)     do{}while(0)
#define VEC_EVENT(e)     do{}while(0)
#endif

static const char* state_name(uint8_t s) {
    switch(s){case 0:return"LOCK";case 1:return"IDLE";case 2:return"READY";
    case 3:return"RUNNING";case 4:return"EMERGENCY";case 5:return"COMPLETE";default:return"INIT";}
}

#define LOCK_PRESS_MS        300
#define LOCK_IDLE_MS         1000
#define DOOR_READY_MS        200
#define DOOR_CLOSE_CONFIRM_MS 500
#define DOOR_OPEN_CONFIRM_MS 200
#define RELEASE_DELAY_MS     200
#define DOOR_DEBOUNCE_CNT    3

void StateVector_Input(void)
{
    static uint32_t lock_press_tick, lock_release_tick;
    static uint8_t  lock_released = 1;
    static uint32_t door_ready_tick, door_open_confirm_tick, door_close_confirm_tick;
    static uint32_t release_start_tick;
    static uint32_t door_close_start_tick, door_close_done_tick, door_open_start_tick;
    static uint32_t door_close_default_ms = 2500, air_last_check_tick;
    static uint8_t  door_close_timing, door_close_from_full, door_close_time_learned;
    static uint8_t  poweron_position_ok;
    static uint8_t  door_up_cnt, door_down_cnt, door_up_db, door_down_db;
    static uint8_t  system_status = V_STATE_INIT;
    static uint8_t  rs485_err_cnt;

    /* ── 1. 从向量表读 IO (ModBusTask 异步更新) ── */
    const Vector_IOState_t* vio = RamVector_GetLocalIO();
    uint8_t in_01_08  = vio->raw_in_lo;
    uint8_t in_09_16  = vio->raw_in_hi;
    uint8_t out_01_08 = vio->raw_out_lo;
    uint8_t out_09_16 = vio->raw_out_hi;
    bool io_ok = (vio->rs485_ok != 0);

    if (!io_ok) {
        if (++rs485_err_cnt >= 40) { U1_Printf("[RS485] COMM ERROR\r\n"); rs485_err_cnt = 0; }
        return;
    }
    rs485_err_cnt = 0;

    /* IO 变化时输出 */
    { static uint8_t li0=0xFF,li1=0xFF,lo0=0xFF,lo1=0xFF;
      if(in_01_08!=li0||in_09_16!=li1){U1_Printf("[IO] IN  0x%02X,0x%02X\r\n",in_01_08,in_09_16);li0=in_01_08;li1=in_09_16;}
      if(out_01_08!=lo0||out_09_16!=lo1){U1_Printf("[IO] OUT 0x%02X,0x%02X\r\n",out_01_08,out_09_16);lo0=out_01_08;lo1=out_09_16;}
    }

    /* ── 2. 消抖 ── */
    if (in_01_08 & 0x01) { if (++door_up_cnt >= DOOR_DEBOUNCE_CNT) door_up_db = 1; door_down_cnt = 0; }
    else { door_up_cnt = 0; door_up_db = 0; }
    if (in_01_08 & 0x02) { if (++door_down_cnt >= DOOR_DEBOUNCE_CNT) door_down_db = 1; door_up_cnt = 0; }
    else { door_down_cnt = 0; door_down_db = 0; }
    if (door_up_db)   in_01_08 |= 0x01; else in_01_08 &= ~0x01;
    if (door_down_db) in_01_08 |= 0x02; else in_01_08 &= ~0x02;

    uint32_t now = GetTim1Ms();

    /* ══════════════════════════════════════════
     *  IO 观测 → 状态 (先读世界, 再纠偏)
     * ══════════════════════════════════════════ */

    if (system_status == V_STATE_INIT) system_status = V_STATE_LOCK;

    /* power_out=0 → 全局回 Lock */
    if (!(out_01_08 & 0x80) && system_status != V_STATE_EMERGENCY && system_status != V_STATE_INIT)
        system_status = V_STATE_LOCK;

    /* Lock → Idle */
    if (system_status == V_STATE_LOCK && (out_01_08 & 0x80))
        system_status = V_STATE_IDLE;

    /* Idle 观测 */
    if (system_status == V_STATE_IDLE) {
        if (in_01_08 & 0x01)
            system_status = (out_01_08 & 0x40) ? V_STATE_READY : V_STATE_IDLE;
        else if (in_01_08 & 0x02)
            system_status = V_STATE_COMPLETE;
    }

    /* Ready → Idle / Running */
    if (system_status == V_STATE_READY) {
        if (!(out_01_08 & 0x40)) system_status = V_STATE_IDLE;
        if ((out_01_08 & 0x02) && !(in_01_08 & 0x02)) system_status = V_STATE_RUNNING;
    }

    /* Running → Complete */
    if (system_status == V_STATE_RUNNING) {
        if ((out_01_08 & 0x02) && (in_01_08 & 0x02)) {
            RamVector_PostCmd(VCMD_LED_GREEN);
            door_close_timing = 0; door_open_confirm_tick = 0;
            if (door_close_from_full) {
                door_close_default_ms = now - door_close_start_tick;
                door_close_time_learned = 1;
            }
            door_close_done_tick = now; door_close_from_full = 0;
            VEC_ACTION("CLOSE_DONE", door_close_default_ms);
            system_status = V_STATE_COMPLETE;
        }
    }

    /* Complete → Idle */
    if (system_status == V_STATE_COMPLETE) {
        if ((out_01_08 & 0x01) && (in_01_08 & 0x01)) {
            RamVector_PostCmd(VCMD_LED_OFF);
            VEC_ACTION("OPEN_DONE", now - door_open_start_tick);
            door_open_start_tick = 0;
            system_status = V_STATE_IDLE;
        }
    }

    /* Emergency → Lock + LED */
    if (system_status == V_STATE_EMERGENCY) {
        RamVector_PostCmd(VCMD_LED_RED);
        if (((in_01_08 & 0xE0) != 0xE0) && ((in_09_16 & 0x01) != 0x01)) {
            system_status = V_STATE_LOCK;
            door_close_done_tick = 0;
        }
    }

    /* ══════════════════════════════════════════
     *  急停 / 激光 (安全优先, 直接 PostCmd)
     * ══════════════════════════════════════════ */

    bool estop = (Flash_GetEstopType() == 1)
        ? ((in_09_16 & 0x08) != 0) : ((in_09_16 & 0x08) == 0);
    if (estop) {
        VEC_EVENT("ESTOP");
        system_status = V_STATE_EMERGENCY;
        if (!(in_01_08 & 0x01)) RamVector_PostCmd(VCMD_CYLINDER_OPEN);
        RamVector_PostCmd(VCMD_LOCK);
        door_close_done_tick = 0;
    }
    else if ((in_01_08 & 0xE0) || (in_09_16 & 0x01)) {
        uint32_t de = door_close_start_tick ? (now - door_close_start_tick) : 0;
        if (door_close_time_learned && door_close_timing && door_close_start_tick &&
            !(in_01_08 & 0x02) && (de > door_close_default_ms * 2 / 3)) {
            VEC_EVENT("LASER");
            system_status = V_STATE_EMERGENCY;
            RamVector_PostCmd(VCMD_CYLINDER_OPEN);
            RamVector_PostCmd(VCMD_LOCK);
            door_close_start_tick = 0; door_close_timing = 0; door_close_done_tick = 0;
        }
    }

    /* ══════════════════════════════════════════
     *  事件 → PostCmd (状态由 IO 观测驱动)
     * ══════════════════════════════════════════ */
    if (!estop && !((in_01_08 & 0xE0) || (in_09_16 & 0x01))) {

        /* power_button → LOCK / UNLOCK */
        if (in_09_16 & 0x10) {
            if (!lock_press_tick) lock_press_tick = now;
        } else { lock_press_tick = 0; lock_released = 1; }
        if ((in_09_16 & 0x10) && ((now - lock_press_tick) >= LOCK_PRESS_MS) && lock_released &&
            (!lock_release_tick || (now - lock_release_tick) >= LOCK_IDLE_MS)) {
            if (out_01_08 & 0x80) {
                RamVector_PostCmd(VCMD_LOCK); VEC_ACTION("LOCK", now - lock_press_tick);
            } else {
                RamVector_PostCmd(VCMD_UNLOCK); VEC_ACTION("UNLOCK", now - lock_press_tick);
            }
            lock_press_tick = 0; lock_released = 0; lock_release_tick = now;
        }

        /* 上电位置确认 */
        if (system_status == V_STATE_IDLE && !poweron_position_ok) {
            if (in_01_08 & 0x01) { RamVector_PostCmd(VCMD_CYLINDER_OPEN); poweron_position_ok = 1; }
            else if (in_01_08 & 0x02) { RamVector_PostCmd(VCMD_CYLINDER_CLOSE); poweron_position_ok = 1; }
        }

        /* 关门按钮 → DOOR_READY (Idle) */
        if (system_status == V_STATE_IDLE) {
            if ((in_09_16 & 0x06) && !(out_01_08 & 0x02)) {
                if (!door_ready_tick) door_ready_tick = now;
            } else { door_ready_tick = 0; }
            if (door_ready_tick && ((now - door_ready_tick) >= DOOR_READY_MS) && !release_start_tick) {
                RamVector_PostCmd(VCMD_DOOR_READY);
                door_ready_tick = 0; poweron_position_ok = 1;
            }
        }

        /* 双按钮 → CYLINDER_CLOSE (Ready) */
        if (system_status == V_STATE_READY) {
            uint8_t drs = 0;
            if ((in_09_16 & 0x06) && !(out_01_08 & 0x02) && !release_start_tick) {
                if (!door_close_confirm_tick) door_close_confirm_tick = now;
            } else { door_close_confirm_tick = 0; }
            if (door_close_confirm_tick && ((now - door_close_confirm_tick) >= DOOR_CLOSE_CONFIRM_MS)) drs = 1;
            if ((in_09_16 & 0x06) == 0x06 && drs && !release_start_tick) {
                if (!(out_01_08 & 0x02)) {
                    RamVector_PostCmd(VCMD_CYLINDER_CLOSE);
                    door_close_confirm_tick = 0; release_start_tick = now;
                    door_close_start_tick = now; door_close_timing = 1;
                    door_close_from_full = (in_01_08 & 0x01) ? 1 : 0;
                    air_last_check_tick = 0;
                    VEC_ACTION("CLOSE_START", 0);
                }
            }
        }

        /* 按钮 → CYLINDER_OPEN (Complete) */
        if (system_status == V_STATE_COMPLETE) {
            uint8_t dcs = 0;
            if ((in_09_16 & 0x06) && !(out_01_08 & 0x01) && (in_01_08 & 0x02)) {
                if (!door_open_confirm_tick) door_open_confirm_tick = now;
            } else { door_open_confirm_tick = 0; }
            if (door_open_confirm_tick && ((now - door_open_confirm_tick) >= DOOR_OPEN_CONFIRM_MS)) dcs = 1;
            if ((in_09_16 & 0x06) && dcs && !release_start_tick) {
                if (!(out_01_08 & 0x01)) {
                    RamVector_PostCmd(VCMD_CYLINDER_OPEN);
                    door_open_start_tick = now; door_open_confirm_tick = 0; release_start_tick = now;
                    VEC_ACTION("OPEN_START", 0);
                }
            }
        }

        /* 气压检测 */
        if ((in_01_08 & 0x02) && door_close_done_tick) {
            uint32_t el = now - door_close_done_tick;
            if (el >= 3000 && (!air_last_check_tick || ((now - air_last_check_tick) >= 1500)))
                air_last_check_tick = now;
        }
    }

    /* ── 释放检测 ── */
    if (!(in_09_16 & 0x06)) {
        if (release_start_tick && ((now - release_start_tick) >= RELEASE_DELAY_MS))
            release_start_tick = 0;
    } else { if (release_start_tick) release_start_tick = now; }

    /* 状态变化输出 */
    { static uint8_t last = 0xFF;
      if (system_status != last) { VEC_STATE(state_name(system_status)); last = system_status; } }

    RamVector_SetState((Vector_SysState_t)system_status);
    RamVector_Heartbeat();
}
