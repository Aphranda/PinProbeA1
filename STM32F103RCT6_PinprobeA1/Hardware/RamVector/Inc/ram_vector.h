/*
 * ram_vector.h
 *
 *  RAM 向量表 — 事件/命令/状态三区解耦
 *
 *  布局与主线 PinProbeA1 完全一致 (1024B @ 0x2000A000)
 *  单机: 只用 cmd + io_state[0], 其余字段预留
 *  多机: 启用 node_status + CAN 同步, 无需改布局
 */

#ifndef APP_RAMVECTOR_INC_RAM_VECTOR_H_
#define APP_RAMVECTOR_INC_RAM_VECTOR_H_

#include "main.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ===== 基地址 ===== */
#define RAM_VECTOR_BASE_ADDR    0x2000A000UL
#define RAM_VECTOR_SIZE         1024U
#define RAM_VECTOR_MAX_NODES    8U

/* ========================================================================== */
/*              命令码 — 所有动作统一编号                                      */
/* ========================================================================== */

typedef enum {
    VCMD_NONE               = 0x0000,

    /* 气缸 1 (门) */
    VCMD_CYLINDER_OPEN      = 0x0100,
    VCMD_CYLINDER_CLOSE     = 0x0101,
    /* 气缸 2 (USB) */
    VCMD_CYLINDER2_OPEN     = 0x0102,
    VCMD_CYLINDER2_CLOSE    = 0x0103,

    /* 锁定/解锁 */
    VCMD_LOCK               = 0x0200,
    VCMD_UNLOCK             = 0x0201,

    /* LED */
    VCMD_LED_OFF            = 0x0300,
    VCMD_LED_GREEN          = 0x0301,
    VCMD_LED_RED            = 0x0302,
    VCMD_LED_YELLOW         = 0x0303,

    /* 急停 (传感器直接注入) */
    VCMD_ESTOP              = 0x0400,

    /* 状态就绪 (Idle→Ready 过渡) */
    VCMD_DOOR_READY         = 0x0500,

    /* 系统级 */
    VCMD_SYS_RESET          = 0x0600,
} Vector_Cmd_t;

/* ===== 命令优先级 ===== */
#define CMD_PRIO_OBSERVE  0   /* 状态观测 (最低) */
#define CMD_PRIO_USER     1   /* 按钮 / SCPI */
#define CMD_PRIO_SAFETY   2   /* 急停 / 激光 / 故障 (最高) */

/* ===== 命令槽 (带优先级仲裁) ===== */
typedef struct __attribute__((packed)) {
    uint16_t cmd;
    uint8_t  priority;
} CmdSlot_t;

/* ========================================================================== */
/*              节点状态 (每节点 8B)                                           */
/* ========================================================================== */

typedef struct __attribute__((packed)) {
    uint8_t  online;            /* 在线标志 */
    uint8_t  state;             /* 系统状态 */
    uint8_t  hw_ver;            /* 硬件版本 */
    uint8_t  fw_ver_minor;      /* 固件次版本 */
    uint8_t  fw_ver_major;      /* 固件主版本 */
    uint8_t  error_code;        /* 最后错误码 */
    uint16_t heartbeat;         /* 心跳计数 */
} Vector_NodeStatus_t;          /* 8 字节 */

/* ========================================================================== */
/*              系统状态 (与 MachineState 对齐)                                */
/* ========================================================================== */

typedef enum {
    V_STATE_LOCK            = 0,
    V_STATE_IDLE            = 1,
    V_STATE_READY           = 2,
    V_STATE_RUNNING         = 3,
    V_STATE_EMERGENCY       = 4,
    V_STATE_COMPLETE        = 5,
    V_STATE_INIT            = 6,
} Vector_SysState_t;

/* ========================================================================== */
/*              事件 ID — 所有输入源统一编号                                   */
/* ========================================================================== */

typedef enum {
    EV_NONE             = 0,
    /* 按钮 */
    EV_POWER_BTN        = 1,
    EV_DOOR1_BTN        = 2,
    EV_DOOR2_BTN        = 3,
    /* 传感器 */
    EV_LASER_ANY        = 10,
    EV_ESTOP_BTN        = 11,
    EV_DOOR_UP_LIMIT    = 12,
    EV_DOOR_DOWN_LIMIT  = 13,
    /* SCPI 命令 */
    EV_SCPI_CYLINDER    = 20,
    EV_SCPI_LOCK        = 21,
    EV_SCPI_LED         = 22,
} EventID_t;

/* ========================================================================== */
/*              IO 状态镜像 (每节点 16B, 与主线对齐)                            */
/* ========================================================================== */

typedef struct __attribute__((packed)) {
    uint8_t  cylinder_cmd[2];       /* 气缸命令 */
    uint8_t  cylinder_state[2];     /* 气缸状态 */
    uint8_t  lock_cmd;              /* 锁定命令 */
    uint8_t  lock_state;            /* 锁定状态 */
    uint8_t  led_cmd;               /* LED 命令 */
    uint8_t  led_state;             /* LED 状态 */
    uint8_t  door_state;            /* 门状态 */
    uint8_t  door_moving;           /* 门运动中 */
    uint8_t  sensor_summary;        /* 传感器摘要 */
    uint8_t  raw_in_lo;             /* 输入低字节 (IN[0]) */
    uint8_t  raw_in_hi;             /* 输入高字节 (IN[1]) */
    uint8_t  raw_out_lo;            /* 输出低字节 (OUT[0]) */
    uint8_t  raw_out_hi;            /* 输出高字节 (OUT[1]) */
    uint8_t  rs485_ok;              /* RS485 通讯状态 */
} Vector_IOState_t;

/* ========================================================================== */
/*              主向量表 (1024B, 与主线完全对齐)                               */
/* ========================================================================== */

typedef struct __attribute__((packed)) {
    /* 帧头 (16B) */
    uint32_t magic;
    uint32_t version;
    uint32_t timestamp;
    uint32_t crc32;

    /* 命令区 (16B) — 三通道独立, 带优先级仲裁 */
    CmdSlot_t cmd_lock;      /* LOCK / UNLOCK */
    CmdSlot_t cmd_cylinder;  /* CYLINDER OPEN / CLOSE */
    CmdSlot_t cmd_led;       /* LED OFF / GREEN / RED / YELLOW */

    /* 路由 (8B) — 单机为0, 多机启用 */
    uint8_t  node_mask;
    uint8_t  source_node;
    uint8_t  priority;
    uint8_t  ack_flags;
    uint32_t ack_timestamp;

    /* 全局状态 (2B) */
    uint8_t  global_state;
    uint8_t  error_code;

    /* 节点状态 (64B) — 心跳+在线+版本 */
    Vector_NodeStatus_t node_status[RAM_VECTOR_MAX_NODES];

    /* IO 状态 (128B) */
    Vector_IOState_t io_state[RAM_VECTOR_MAX_NODES];

    /* 扩展参数 (786B) — 预留 */
    uint8_t  ext_params[786];

    /* 帧尾 (4B) */
    uint32_t end_magic;
} RAM_Vector_t;

/* 编译期校验 */
#ifndef __CC_ARM
_Static_assert(sizeof(RAM_Vector_t) == RAM_VECTOR_SIZE,
               "RAM_Vector_t must be exactly 1024 bytes");
#endif

/* ===== API ===== */

void        RamVector_Init(uint8_t node_id);
RAM_Vector_t* RamVector_Get(void);

/* 命令注入 — 三通道独立, 带优先级仲裁 */
void RamVector_PostLock(Vector_Cmd_t cmd, uint8_t prio);
void RamVector_PostCylinder(Vector_Cmd_t cmd, uint8_t prio);
void RamVector_PostLED(Vector_Cmd_t cmd, uint8_t prio);
Vector_Cmd_t RamVector_GetLockCmd(void);
Vector_Cmd_t RamVector_GetCylinderCmd(void);
Vector_Cmd_t RamVector_GetLEDCmd(void);
void RamVector_ClearCmd(void);

/* IO 状态更新 (执行层/IO读取后调用) */
void        RamVector_UpdateLocalIO(const Vector_IOState_t *io);
const Vector_IOState_t* RamVector_GetLocalIO(void);

/* 状态同步 */
void        RamVector_SetState(Vector_SysState_t s);
Vector_SysState_t RamVector_GetState(void);

/* 心跳 (WatchDog/监控用) */
void        RamVector_Heartbeat(void);
uint16_t    RamVector_GetHeartbeat(void);

#endif /* APP_RAMVECTOR_INC_RAM_VECTOR_H_ */
