#ifndef __STATEMACHINE_H
#define __STATEMACHINE_H

#include <stdint.h>
#include "BsmRelay.h"
#include "cmsis_os.h"
// 状态枚举定义
typedef enum {
    SYS_INIT = 0,
    LOCK,
    IDLE,
    READY,
    RUNNING,
    COMPLETE,
    EMERGENCY,
    STATE_COUNT
} SystemState;

// 门状态枚举
typedef enum {
    DOOR_CLOSED = 0,
    DOOR_OPENED = 1,
    DOOR_MID = 2
} DoorState;

// 事件枚举
typedef enum {
    EVENT_NONE = 0,
    EVENT_POWER_ON,
    EVENT_UNLOCK,
    EVENT_DOOR_READY,
    EVENT_DOOR_CLOSE_START,
    EVENT_DOOR_CLOSED,
    EVENT_DOOR_OPEN_START,
    EVENT_DOOR_OPENED,
    EVENT_EMERGENCY,
    EVENT_COUNT
} SystemEvent;

// 状态机配置参数
#define LOCK_DELAY_COUNT 3
#define DOOR_DELAY_COUNT 3
#define DOOR_CLOSE_DEFAULT_TIME 40

// 状态机上下文结构体
typedef struct {
    SystemState currentState;
    SystemState previousState;
    DoorState doorStatus;
    
    // 计时相关
    uint16_t doorCloseTimer;
    uint16_t doorCloseDefaultTime;
    uint8_t doorCloseTiming;
    
    // 按钮防抖
    uint16_t lockPressCount;
    uint16_t lockReleaseCount;
    uint16_t doorReadyCount;
    uint16_t doorOpenCount;
    uint16_t doorCloseCount;
    uint16_t releaseFlag;
} StateMachineContext;

// 初始化函数
void StateMachine_Init(void);

// 主处理函数
void StateMachine_Process(void);

// 获取当前状态
SystemState StateMachine_GetCurrentState(void);

// 获取门状态
DoorState StateMachine_GetDoorState(void);

// 强制设置状态(用于调试)
void StateMachine_SetState(SystemState newState);

#endif /* __STATEMACHINE_H */
