#include "StateMachine.h"
#include "BsmRelay.h"
#include "cmsis_os.h"
// 外部变量声明
extern scpi_choice_def_t cylinder_source[];
extern scpi_choice_def_t lock_source[];
extern scpi_choice_def_t led_source[];
// 外部打印函数声明
extern void U1_Printf(const char *format, ...);
// 状态机上下文
static StateMachineContext smContext = {
    .currentState = SYS_INIT,
    .previousState = SYS_INIT,
    .doorStatus = DOOR_MID,
    .doorCloseTimer = 0,
    .doorCloseDefaultTime = DOOR_CLOSE_DEFAULT_TIME,
    .doorCloseTiming = 0,
    .lockPressCount = 0,
    .lockReleaseCount = LOCK_DELAY_COUNT,
    .doorReadyCount = 0,
    .doorOpenCount = 0,
    .doorCloseCount = 0,
    .releaseFlag = 0
};
// 私有函数声明
static SystemEvent ReadInputEvents(void);
static void HandleStateTransition(SystemEvent event);
static void ExecuteStateActions(SystemState state);
static void ShowSystemStatus(void);
static void ShowDoorStatus(void);
static void HandleStateChange(void);
static void HandleTiming(void);
static void HandleEmergencyEnter(void);
// 状态转移动作函数
static void InitToLock(void);
static void LockToIdle(void);
static void IdleToReady(void);
static void ReadyToRunning(void);
static void RunningToComplete(void);
static void CompleteToIdle(void);
static void AnyToEmergency(void);
static void EmergencyToLock(void);
static uint8_t CheckEmergencyRecover(void);
// 状态转移表
typedef struct {
    SystemState currentState;
    SystemEvent event;
    SystemState nextState;
    void (*action)(void);
} StateTransition;

/*************************************************
 * 状态转移表 - 核心逻辑
 *************************************************/
static const StateTransition stateTable[] = {
    /* 当前状态       触发事件               下一状态        状态转移动作函数 */
    
    // 系统初始化完成后进入锁定状态
    {SYS_INIT,    EVENT_POWER_ON,      LOCK,        InitToLock},
    
    // 正常操作流程
    {LOCK,        EVENT_UNLOCK,        IDLE,        LockToIdle},
    {IDLE,        EVENT_DOOR_READY,    READY,       IdleToReady},
    {READY,       EVENT_DOOR_CLOSE_START, RUNNING,  ReadyToRunning},
    {RUNNING,     EVENT_DOOR_CLOSED,   COMPLETE,    RunningToComplete},
    {COMPLETE,    EVENT_DOOR_OPEN_START, IDLE,      CompleteToIdle},
    
    // 从任何状态都可以进入紧急状态
    {LOCK,        EVENT_EMERGENCY,     EMERGENCY,   AnyToEmergency},
    {IDLE,        EVENT_EMERGENCY,     EMERGENCY,   AnyToEmergency},
    {READY,       EVENT_EMERGENCY,     EMERGENCY,   AnyToEmergency},
    {RUNNING,     EVENT_EMERGENCY,     EMERGENCY,   AnyToEmergency},
    {COMPLETE,    EVENT_EMERGENCY,     EMERGENCY,   AnyToEmergency},
    
    // 从紧急状态恢复
    {EMERGENCY,   EVENT_POWER_ON,      LOCK,        EmergencyToLock},

};

/*************************************************
 * 公共接口函数实现
 *************************************************/

/**
 * @brief 初始化状态机
 * @note 系统启动时调用，重置所有状态变量
 */
void StateMachine_Init(void) {
    smContext.currentState = SYS_INIT;
    smContext.previousState = SYS_INIT;
    smContext.doorStatus = DOOR_MID;
    
    // 重置所有计数器
    smContext.lockPressCount = 0;
    smContext.lockReleaseCount = LOCK_DELAY_COUNT;
    smContext.doorReadyCount = 0;
    smContext.doorOpenCount = 0; 
    smContext.doorCloseCount = 0;
    smContext.releaseFlag = 0;
    
    // 初始化硬件输出
    Lock_Write(lock_source[1]); // 初始锁定
    LED_Write(led_source[0]);   // 关闭所有LED
}

/**
 * @brief 状态机主处理函数
 * @note 需要周期性调用(如每10ms调用一次)
 */
void StateMachine_Process(void) {
    // 1. 读取并处理所有输入事件
    SystemEvent currentEvent = ReadInputEvents();
    
    // 2. 根据当前状态和事件处理状态转移
    HandleStateTransition(currentEvent);
    
    // 3. 执行当前状态的持续动作
    ExecuteStateActions(smContext.currentState);
    
    // 4. 处理状态变化时的操作
    HandleStateChange();
    
    // 5. 处理计时相关逻辑
    HandleTiming();
}

/**
 * @brief 获取当前系统状态
 * @return 当前系统状态枚举值
 */
SystemState StateMachine_GetCurrentState(void) {
    return smContext.currentState;
}

/**
 * @brief 获取当前门状态
 * @return 当前门状态枚举值
 */
DoorState StateMachine_GetDoorState(void) {
    return smContext.doorStatus;
}

/*************************************************
 * 私有函数实现 - 状态机核心逻辑
 *************************************************/

/**
 * @brief 读取并解析所有输入事件
 * @return 检测到的事件枚举值
 */
static SystemEvent ReadInputEvents(void) {
    uint8_t* inputs = InputIO_Read(CHECK_NUM);
    uint8_t in_01_08 = inputs[0];
    uint8_t in_09_16 = inputs[1];
    
    /* 事件检测优先级从高到低 */
    
    // 1. 急停按钮检测 (最高优先级)
    if((in_09_16 & (STOP_BUTTON >> 8)) != 0x08) {
        return EVENT_EMERGENCY;
    }
    
    // 2. 激光安全传感器检测
    if(((in_01_08 & LASER_SENSOR1 ) == 0x20) || 
       ((in_01_08 & LASER_SENSOR2) == 0x40) ||
       ((in_01_08 & LASER_SENSOR3) == 0x80) ||
       ((in_09_16 & (LASER_SENSOR4 >> 8)) == 0x01)) {
        // 如果门未完全关闭且超时，触发紧急事件
        if(!(in_01_08 & DOOR_SENSOR_DOWN) && 
           (smContext.doorCloseTimer > (smContext.doorCloseDefaultTime * 2 / 3))) {
            return EVENT_EMERGENCY;
        }
    }
    
    // 3. 解锁按钮检测
    if(in_09_16 & (POWER_BUTTON >> 8)) {
        smContext.lockPressCount++;
        if(smContext.lockPressCount >= LOCK_DELAY_COUNT && 
           smContext.lockReleaseCount <= 0) {
            return EVENT_UNLOCK;
        }
    } else {
        if(smContext.lockReleaseCount > 0) {
            smContext.lockReleaseCount--;
        }
    }
    
    // 4. 门按钮检测
    uint8_t doorBtn1 = in_09_16 & (DOOR_BUTTON1 >> 8);
    uint8_t doorBtn2 = in_09_16 & (DOOR_BUTTON2 >> 8);
    
    // 关门准备检测
    if(doorBtn1 || doorBtn2) {
        smContext.doorReadyCount++;
        if(smContext.doorReadyCount >= DOOR_DELAY_COUNT && 
           smContext.releaseFlag <= 0) {
            return EVENT_DOOR_READY;
        }
    }
    
    // 关门启动检测 (需要两个按钮同时按下)
    if(doorBtn1 && doorBtn2 && smContext.doorCloseCount >= (DOOR_DELAY_COUNT * 2)) {
        return EVENT_DOOR_CLOSE_START;
    }
    
    // 开门启动检测
    if((doorBtn1 || doorBtn2) && smContext.doorOpenCount >= (DOOR_DELAY_COUNT / 2)) {
        return EVENT_DOOR_OPEN_START;
    }
    
    // 门状态检测
    if(in_01_08 & DOOR_SENSOR_DOWN) {
        return EVENT_DOOR_CLOSED;
    }
    
    if(in_01_08 & DOOR_SENSOR_UP) {
        return EVENT_DOOR_OPENED;
    }
    
    return EVENT_NONE;
}

/**
 * @brief 处理状态转移
 * @param event 触发事件
 */
static void HandleStateTransition(SystemEvent event) {
    if(event == EVENT_NONE) return;
    
    // 遍历状态表查找匹配的转移
    for(uint8_t i = 0; i < sizeof(stateTable)/sizeof(StateTransition); i++) {
        const StateTransition *trans = &stateTable[i];
        
        if(trans->currentState == smContext.currentState && 
           trans->event == event) {
            // 执行状态转移动作
            if(trans->action != NULL) {
                trans->action();
            }
            
            // 更新状态
            smContext.previousState = smContext.currentState;
            smContext.currentState = trans->nextState;
            
            break;
        }
    }
}

/**
 * @brief 执行当前状态的持续动作
 * @param state 当前状态
 */
static void ExecuteStateActions(SystemState state) {
    uint8_t* outputs = OutputIO_Read(CHECK_NUM);
    uint8_t out_01_08 = outputs[0];
    
    switch(state) {
        case RUNNING:
            // 关门动作中持续检查门状态
            if(out_01_08 & DOOR_CLOSE) {
                Cylinder_Write(1, cylinder_source[0]);
            }
            break;
            
        case COMPLETE:
            // 完成状态保持门关闭
            if(!(out_01_08 & DOOR_CLOSE)) {
                LED_Write(led_source[1]); // 绿灯表示门已关
            }
            break;
            
        case EMERGENCY:
            // 紧急状态持续检查恢复条件
            if(CheckEmergencyRecover()) {
                HandleStateTransition(EVENT_POWER_ON);
            }
            break;
            
        default:
            // 其他状态不需要持续动作
            break;
    }
}

/**
 * @brief 处理状态变化时的特殊操作
 */
static void HandleStateChange(void) {
    if(smContext.currentState != smContext.previousState) {
        // 状态变化时记录日志
        ShowSystemStatus();
        
        // 特殊状态进入处理
        if(smContext.currentState == EMERGENCY) {
            // 紧急状态特殊处理
            HandleEmergencyEnter();
        }
    }
}

/**
 * @brief 处理所有计时相关逻辑
 */
static void HandleTiming(void) {
    // 关门计时处理
    if(smContext.doorCloseTiming) {
        smContext.doorCloseTimer++;
        
        // 检查关门超时
        if(smContext.doorCloseTimer > smContext.doorCloseDefaultTime) {
            smContext.doorCloseTiming = 0;
            HandleStateTransition(EVENT_EMERGENCY);
        }
    }
    
    // 按钮释放检测
    if(smContext.releaseFlag > 0) {
        smContext.releaseFlag--;
    }
}

/*************************************************
 * 状态转移动作函数实现
 *************************************************/

/**
 * @brief 从初始化状态转移到锁定状态
 */
static void InitToLock(void) {
    Lock_Write(lock_source[1]); // 锁定
    LED_Write(led_source[0]);   // 关闭LED
    U1_Printf("System initialized and locked\r\n");
}

/**
 * @brief 从锁定状态转移到空闲状态
 */
static void LockToIdle(void) {
    Lock_Write(lock_source[0]); // 解锁
    LED_Write(led_source[0]);   // 关闭LED
    smContext.lockPressCount = 0;
    smContext.lockReleaseCount = LOCK_DELAY_COUNT;
    U1_Printf("System unlocked, entering idle mode\r\n");
}

/**
 * @brief 从空闲状态转移到准备状态
 */
static void IdleToReady(void) {
    LED_Write(led_source[3]); // 黄灯亮
    smContext.doorReadyCount = 0;
    U1_Printf("Door operation prepared\r\n");
}

/**
 * @brief 从准备状态转移到运行状态
 */
static void ReadyToRunning(void) {
    Cylinder_Write(1, cylinder_source[0]); // 启动关门
    smContext.doorCloseCount = 0;
    smContext.releaseFlag = DOOR_DELAY_COUNT;
    smContext.doorCloseTimer = 0;
    smContext.doorCloseTiming = 1; // 开始关门计时
    U1_Printf("Door closing started\r\n");
}

/**
 * @brief 从运行状态转移到完成状态
 */
static void RunningToComplete(void) {
    LED_Write(led_source[1]); // 绿灯亮
    smContext.doorCloseTiming = 0; // 停止计时
    smContext.doorCloseDefaultTime = smContext.doorCloseTimer; // 记录实际关门时间
    U1_Printf("Door closed successfully\r\n");
}

/**
 * @brief 从完成状态转移到空闲状态
 */
static void CompleteToIdle(void) {
    Cylinder_Write(1, cylinder_source[1]); // 启动开门
    smContext.doorOpenCount = 0;
    smContext.releaseFlag = DOOR_DELAY_COUNT;
    U1_Printf("Door opening started\r\n");
}

/**
 * @brief 任何状态转移到紧急状态
 */
static void AnyToEmergency(void) {
    Cylinder_Write(1, cylinder_source[1]); // 紧急开门
    Lock_Write(lock_source[1]);            // 紧急锁定
    LED_Write(led_source[2]);              // 红灯闪烁
    smContext.doorCloseTimer = 0;
    smContext.doorCloseTiming = 0;
    U1_Printf("EMERGENCY TRIGGERED!\r\n");
}

/**
 * @brief 从紧急状态恢复到锁定状态
 */
static void EmergencyToLock(void) {
    LED_Write(led_source[0]); // 关闭报警灯
    U1_Printf("Emergency recovered, system locked\r\n");
}

/*************************************************
 * 辅助函数实现
 *************************************************/

/**
 * @brief 检查紧急状态恢复条件
 * @return 1可恢复，0不可恢复
 */
static uint8_t CheckEmergencyRecover(void) {
    uint8_t* inputs = InputIO_Read(CHECK_NUM);
    uint8_t in_01_08 = inputs[0];
    uint8_t in_09_16 = inputs[1];
    
    // 检查所有安全传感器是否正常
    if(((in_01_08 & LASER_SENSOR1 ) != 0x20) &&
       ((in_01_08 & LASER_SENSOR2) != 0x40) &&
       ((in_01_08 & LASER_SENSOR3) != 0x80) &&
       ((in_09_16 & (LASER_SENSOR4 >> 8)) != 0x01)) {
        return 1;
    }
    
    return 0;
}

/**
 * @brief 进入紧急状态的特殊处理
 */
static void HandleEmergencyEnter(void) {
    // 保存当前状态到Flash等非易失存储器
    // 触发外部报警等
}

/**
 * @brief 显示系统状态(调试用)
 */
static void ShowSystemStatus(void) {
#ifdef DEBUG
    const char* stateNames[] = {
        "SYS_INIT", "LOCK", "IDLE", "READY", 
        "RUNNING", "COMPLETE", "EMERGENCY"
    };
    U1_Printf("[State] %s\r\n", stateNames[smContext.currentState]);
#endif
}

/**
 * @brief 显示门状态(调试用)
 */
static void ShowDoorStatus(void) {
#ifdef DEBUG
    const char* doorStates[] = {"CLOSED", "OPENED", "MID"};
    U1_Printf("[Door] %s\r\n", doorStates[smContext.doorStatus]);
#endif
}
