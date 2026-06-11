/*
 * state_vector.c — 向量表状态机实现
 */

#include "state_vector.h"
#include "ram_vector.h"
#include "cmd_exec.h"
#include "BsmRelay.h"
#include "tim.h"
#include "flash.h"
#include <string.h>

/* ── 调试宏 (定义 VECTOR_DEBUG 开启) ── */
// #define VECTOR_DEBUG
#ifdef VECTOR_DEBUG
#define VEC_LOG_ACTION(name, elapsed) \
    U1_Printf("[%s] %u ms\r\n", name, (unsigned int)(elapsed))
#define VEC_LOG_STATE(s)              U1_Printf("%s\r\n", (s))
#define VEC_LOG_DOOR(s)               U1_Printf("%s\r\n", (s))
#define VEC_LOG_CMD(cmd)              U1_Printf("[VCMD] %s\r\n", cmd_name(cmd))
#define VEC_LOG_EVENT(evt)            U1_Printf("[EVENT] %s\r\n", event_name(evt))
#define VEC_LOG_IO(tag, lo, hi)       U1_Printf("[IO] %s 0x%02X,0x%02X\r\n", tag, lo, hi)
#else
#define VEC_LOG_ACTION(name, elapsed)  do {} while(0)
#define VEC_LOG_STATE(s)               do {} while(0)
#define VEC_LOG_DOOR(s)                do {} while(0)
#define VEC_LOG_CMD(cmd)               do {} while(0)
#define VEC_LOG_EVENT(evt)             do {} while(0)
#define VEC_LOG_IO(tag, lo, hi)        do {} while(0)
#endif

static const char* state_name(uint8_t s) {
    switch (s) {
        case V_STATE_LOCK:      return "LOCK";
        case V_STATE_IDLE:      return "IDLE";
        case V_STATE_READY:     return "READY";
        case V_STATE_RUNNING:   return "RUNNING";
        case V_STATE_EMERGENCY: return "EMERGENCY";
        case V_STATE_COMPLETE:  return "COMPLETE";
        default:                return "INIT";
    }
}

static const char* cmd_name(Vector_Cmd_t c) {
    switch (c) {
        case VCMD_CYLINDER_OPEN:  return "CYL_OPEN";
        case VCMD_CYLINDER_CLOSE: return "CYL_CLOSE";
        case VCMD_LOCK:           return "LOCK";
        case VCMD_UNLOCK:         return "UNLOCK";
        case VCMD_LED_OFF:        return "LED_OFF";
        case VCMD_LED_GREEN:      return "LED_GREEN";
        case VCMD_LED_RED:        return "LED_RED";
        case VCMD_LED_YELLOW:     return "LED_YELLOW";
        case VCMD_ESTOP:          return "ESTOP";
        case VCMD_DOOR_READY:     return "DOOR_READY";
        default:                  return "?";
    }
}

static const char* event_name(EventID_t e) {
    switch (e) {
        case EV_POWER_BTN:   return "POWER_BTN";
        case EV_DOOR1_BTN:   return "DOOR1_BTN";
        case EV_DOOR2_BTN:   return "DOOR2_BTN";
        case EV_LASER_ANY:   return "LASER";
        case EV_ESTOP_BTN:   return "ESTOP_BTN";
        case EV_SCPI_CYLINDER: return "SCPI_CYL";
        case EV_SCPI_LOCK:   return "SCPI_LOCK";
        case EV_SCPI_LED:    return "SCPI_LED";
        default:             return "?";
    }
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
    static uint32_t lock_press_tick = 0, lock_release_tick = 0;
    static uint8_t  lock_released = 1;
    static uint32_t door_ready_tick = 0, door_open_confirm_tick = 0, door_close_confirm_tick = 0;
    static uint32_t release_start_tick = 0;
    static uint32_t door_close_start_tick = 0, door_close_done_tick = 0, door_open_start_tick = 0;
    static uint32_t door_close_default_ms = 2500, air_last_check_tick = 0;
    static uint8_t  door_close_timing = 0, door_close_from_full = 0, door_close_time_learned = 0;
    static uint8_t  poweron_position_ok = 0;
    static uint8_t  door_up_cnt = 0, door_down_cnt = 0, door_up_db = 0, door_down_db = 0;
    static uint8_t  system_status = V_STATE_INIT;
    static uint8_t  rs485_err_cnt = 0;

    uint8_t in_buf[2] = {0}, out_buf[2] = {0};
    bool io_ok = IO_Read(5, 2, in_buf);
    io_ok = IO_Read(5, 1, out_buf) && io_ok;
    SetRS485_Ok(io_ok);
    if (!io_ok) {
        if (++rs485_err_cnt >= 40) { U1_Printf("[RS485] COMM ERROR\r\n"); rs485_err_cnt = 0; }
        return;
    }

    uint8_t in_01_08 = in_buf[0], in_09_16 = in_buf[1];
    uint8_t out_01_08 = out_buf[0], out_09_16 = out_buf[1];
    VEC_LOG_IO("IN",  in_01_08, in_09_16);
    VEC_LOG_IO("OUT", out_01_08, out_09_16);

    /* 消抖 */
    if (in_01_08 & 0x01) { if (++door_up_cnt >= DOOR_DEBOUNCE_CNT) door_up_db = 1; door_down_cnt = 0; }
    else { door_up_cnt = 0; door_up_db = 0; }
    if (in_01_08 & 0x02) { if (++door_down_cnt >= DOOR_DEBOUNCE_CNT) door_down_db = 1; door_up_cnt = 0; }
    else { door_down_cnt = 0; door_down_db = 0; }
    if (door_up_db)   in_01_08 |= 0x01; else in_01_08 &= ~0x01;
    if (door_down_db) in_01_08 |= 0x02; else in_01_08 &= ~0x02;

    /* IO 镜像 */
    Vector_IOState_t io;
    memset(&io, 0, sizeof(io));
    io.door_state = (in_01_08 & 0x01) ? 1 : ((in_01_08 & 0x02) ? 0 : 2);
    io.door_moving = (out_01_08 & 0x03) ? 1 : 0;
    io.sensor_summary = in_01_08;
    io.led_state = (out_01_08 >> 4) & 0x07;
    io.lock_state = (out_01_08 & 0x80) ? 1 : 0;
    RamVector_UpdateLocalIO(&io);

    uint32_t now = GetTim1Ms();

    /* 急停优先 */
    bool estop = (Flash_GetEstopType() == 1)
        ? ((in_09_16 & 0x08) != 0) : ((in_09_16 & 0x08) == 0);
    if (estop) {
        VEC_LOG_EVENT(EV_ESTOP_BTN);
        system_status = V_STATE_EMERGENCY;
        if (!(in_01_08 & 0x01)) CmdExec_Execute(VCMD_CYLINDER_OPEN);
        CmdExec_Execute(VCMD_LOCK);
        CmdExec_Execute(VCMD_LED_RED);
        door_close_done_tick = 0;
    }
    /* 激光超时 */
    else if ((in_01_08 & 0xE0) || (in_09_16 & 0x01)) {
        uint32_t de = door_close_start_tick ? (now - door_close_start_tick) : 0;
        if (door_close_time_learned && door_close_timing && door_close_start_tick &&
            !(in_01_08 & 0x02) && (de > door_close_default_ms * 2 / 3)) {
            VEC_LOG_EVENT(EV_LASER_ANY);
            system_status = V_STATE_EMERGENCY;
            CmdExec_Execute(VCMD_CYLINDER_OPEN);
            CmdExec_Execute(VCMD_LOCK);
            CmdExec_Execute(VCMD_LED_RED);
            door_close_start_tick = 0; door_close_timing = 0; door_close_done_tick = 0;
        }
    }
    /* 正常流转 */
    else {
        if (system_status == V_STATE_EMERGENCY) {
            if (((in_01_08 & 0xE0) != 0xE0) && ((in_09_16 & 0x01) != 0x01))
                { system_status = V_STATE_LOCK; door_close_done_tick = 0; }
        }
        if (system_status == V_STATE_INIT) system_status = V_STATE_LOCK;

        /* Lock ↔ Unlock */
        if (in_09_16 & 0x10) {
            if (!lock_press_tick) lock_press_tick = now;
        } else { lock_press_tick = 0; lock_released = 1; }
        if ((in_09_16 & 0x10) && ((now - lock_press_tick) >= LOCK_PRESS_MS) && lock_released &&
            (!lock_release_tick || (now - lock_release_tick) >= LOCK_IDLE_MS)) {
            if (out_01_08 & 0x80) { CmdExec_Execute(VCMD_LOCK); system_status = V_STATE_LOCK; VEC_LOG_ACTION("LOCK", now - lock_press_tick); }
            else { CmdExec_Execute(VCMD_UNLOCK); system_status = V_STATE_IDLE; VEC_LOG_ACTION("UNLOCK", now - lock_press_tick); }
            lock_press_tick = 0; lock_released = 0; lock_release_tick = now;
        }

        /* Idle */
        if (system_status == V_STATE_IDLE) {
            if (!(out_01_08 & 0x80)) system_status = V_STATE_LOCK;
            if (!poweron_position_ok) {
                if (in_01_08 & 0x01) { CmdExec_Execute(VCMD_CYLINDER_OPEN); poweron_position_ok = 1; }
                else if (in_01_08 & 0x02) { CmdExec_Execute(VCMD_CYLINDER_CLOSE); poweron_position_ok = 1; system_status = V_STATE_COMPLETE; }
            }
            if (in_01_08 & 0x01) system_status = (out_01_08 & 0x40) ? V_STATE_READY : V_STATE_IDLE;
            else if (in_01_08 & 0x02) system_status = V_STATE_COMPLETE;
            if ((in_09_16 & 0x06) && !(out_01_08 & 0x02)) {
                if (!door_ready_tick) door_ready_tick = now;
            } else { door_ready_tick = 0; }
            if (door_ready_tick && ((now - door_ready_tick) >= DOOR_READY_MS) && !release_start_tick) {
                CmdExec_Execute(VCMD_DOOR_READY);
                system_status = V_STATE_READY; door_ready_tick = 0; poweron_position_ok = 1;
            }
        }

        /* Ready */
        if (system_status == V_STATE_READY) {
            if (!(out_01_08 & 0x40)) system_status = V_STATE_IDLE;
            if ((out_01_08 & 0x02) && !(in_01_08 & 0x02)) system_status = V_STATE_RUNNING;
            uint8_t drs = 0;
            if ((in_09_16 & 0x06) && !(out_01_08 & 0x02) && !release_start_tick) {
                if (!door_close_confirm_tick) door_close_confirm_tick = now;
            } else { door_close_confirm_tick = 0; }
            if (door_close_confirm_tick && ((now - door_close_confirm_tick) >= DOOR_CLOSE_CONFIRM_MS)) drs = 1;
            if ((in_09_16 & 0x06) == 0x06 && drs && !release_start_tick) {
                if (!(out_01_08 & 0x02)) {
                    CmdExec_Execute(VCMD_CYLINDER_CLOSE);
                    door_close_confirm_tick = 0; release_start_tick = now;
                    system_status = V_STATE_RUNNING;
                    door_close_start_tick = now; door_close_timing = 1;
                    door_close_from_full = (in_01_08 & 0x01) ? 1 : 0;
                    air_last_check_tick = 0;
                    VEC_LOG_ACTION("CLOSE_START", 0);
                }
            }
        }

        /* Running */
        if (system_status == V_STATE_RUNNING) {
            if ((out_01_08 & 0x02) && (in_01_08 & 0x02)) {
                CmdExec_Execute(VCMD_LED_GREEN);
                system_status = V_STATE_COMPLETE;
                door_close_timing = 0; door_open_confirm_tick = 0;
                if (door_close_from_full) { door_close_default_ms = now - door_close_start_tick; door_close_time_learned = 1; }
                door_close_done_tick = now; door_close_from_full = 0;
                VEC_LOG_ACTION("CLOSE_DONE", door_close_default_ms);
            }
        }

        /* Complete */
        if (system_status == V_STATE_COMPLETE) {
            if ((out_01_08 & 0x01) && (in_01_08 & 0x01)) {
                CmdExec_Execute(VCMD_LED_OFF); system_status = V_STATE_IDLE; door_open_start_tick = 0;
                VEC_LOG_ACTION("OPEN_DONE", now - door_open_start_tick);
            }
            uint8_t dcs = 0;
            if ((in_09_16 & 0x06) && !(out_01_08 & 0x01) && (in_01_08 & 0x02)) {
                if (!door_open_confirm_tick) door_open_confirm_tick = now;
            } else { door_open_confirm_tick = 0; }
            if (door_open_confirm_tick && ((now - door_open_confirm_tick) >= DOOR_OPEN_CONFIRM_MS)) dcs = 1;
            if ((in_09_16 & 0x06) && dcs && !release_start_tick) {
                if (!(out_01_08 & 0x01)) {
                    CmdExec_Execute(VCMD_CYLINDER_OPEN);
                    door_open_start_tick = now; door_open_confirm_tick = 0; release_start_tick = now;
                VEC_LOG_ACTION("OPEN_START", 0);
                }
            }
        }

        /* 气压检测 */
        if ((in_01_08 & 0x02) && door_close_done_tick) {
            uint32_t el = now - door_close_done_tick;
            if (el >= 3000 && (!air_last_check_tick || ((now - air_last_check_tick) >= 1500))) {
                air_last_check_tick = now;
            }
        }
    }

    /* 释放检测 */
    if (!(in_09_16 & 0x06)) {
        if (release_start_tick && ((now - release_start_tick) >= RELEASE_DELAY_MS)) release_start_tick = 0;
    } else { if (release_start_tick) release_start_tick = now; }

    static uint8_t last_state = 0xFF;
    if (system_status != last_state) {
        VEC_LOG_STATE(state_name(system_status));
        last_state = system_status;
    }
    RamVector_SetState((Vector_SysState_t)system_status);
    RamVector_Heartbeat();
}
