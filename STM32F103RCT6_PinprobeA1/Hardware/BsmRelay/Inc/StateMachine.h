#ifndef __STATEMACHINE_H
#define __STATEMACHINE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/**
 * @brief 系统主状态枚举
 * @note  枚举值不可随意更改——scpi-def.c 中的 sys_source[] 索引与之绑定
 */
typedef enum {
    Lock = 0,           // 锁定状态，系统不可操作
    Idle = 1,           // 空闲状态，已解锁但未操作
    Ready = 2,          // 准备状态，黄灯亮起，门动作待触发
    Running = 3,        // 运行状态，关门动作执行中
    Emergency = 4,      // 紧急状态，急停或传感器异常
    Complete = 5,       // 关门完成状态
    SYS_INIT = 6,       // 系统初始化（上电后短暂停留）
} MachineState;

/**
 * @brief LED 状态枚举
 */
typedef enum {
    twinkle = 0,        // 闪烁
    Green = 1,          // 绿灯
    Red = 2,            // 红灯
    Yellow = 3,         // 黄灯
    Led_OFF = 4,        // 关闭
} LEDState;

/**
 * @brief 急停按钮状态
 */
typedef enum {
    PressDown = 0,      // 按下
    PressUp = 1,        // 弹起
} EStopButtonState;

/**
 * @brief 启动按钮状态
 */
typedef enum {
    PressFall = 0,      // 下降沿触发
    PressRise = 1,      // 上升沿触发
} StartButtonState;

/**
 * @brief 安全传感器状态
 */
typedef enum {
    Trigger = 0,        // 触发（不安全）
    UnTriggered = 1,    // 未触发（安全）
} SafeSensorState;

/**
 * @brief 气缸（门）状态
 */
typedef enum {
    Door_Closed = 0,    // 门关闭
    Door_Opened = 1,    // 门打开
    Door_Mid = 2,       // 门在中间
} CylinderState;

/**
 * @brief 输入输出状态快照
 * 
 * 每次状态机轮询时统一读取，通过该结构体在各状态处理函数间传递，
 * 避免重复 IO 读取。
 */
typedef struct {
    uint8_t in_01_08;   // 输入 IO 低 8 位
    uint8_t in_09_16;   // 输入 IO 高 8 位
    uint8_t out_01_08;  // 输出 IO 低 8 位
    uint8_t out_09_16;  // 输出 IO 高 8 位
} IOStatus;

/**
 * @brief 状态动作映射（保留，外部 SCPI 可能使用）
 */
typedef struct Action_Map {
    MachineState Machine;
    LEDState led;
    EStopButtonState eStopButton;
    StartButtonState startButton;
    SafeSensorState safeSensor;
    CylinderState cylinder;
} MachineAction;

/* ========== 状态机全局变量 ========== */
extern MachineState system_status;      ///< 系统当前状态（类型安全）
extern uint8_t door_status;             ///< 门当前状态
extern uint16_t door_close_timer;       ///< 关门计时器
extern uint8_t door_close_timing;       ///< 关门计时标志

/* ========== 核心 API ========== */
uint8_t StateMachine_Input(void);

/* ========== 状态处理函数（内部使用，为兼容保留导出） ========== */
uint8_t Init_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16);
uint8_t Lock_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16);
uint8_t Idle_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16);
uint8_t Ready_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16);
uint8_t Running_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16);
uint8_t Complete_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16);
uint8_t Emerge_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16);
uint8_t Release_detection(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16);
uint8_t Door_detection(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16);

/* ========== 调试接口 ========== */
uint8_t showStatus(void);
uint8_t showDoorStatus(void);

#endif /* __STATEMACHINE_H */
