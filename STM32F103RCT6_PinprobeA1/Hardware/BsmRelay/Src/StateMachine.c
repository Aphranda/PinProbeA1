#include "StateMachine.h"
#include "BsmRelay.h"
#include "cmsis_os.h"
#include "tim.h"

// 基于TIM1(1ms)的精确延时定义，替代原有的循环计数
#define LOCK_PRESS_MS        300   // 锁定按钮按下确认时间
#define LOCK_IDLE_MS         1000  // 锁定按钮松开冷却时间

#define DOOR_BUTTON_MS       300   // 门按钮按下确认时间
#define DOOR_READY_MS        300   // 关门准备确认时间
#define DOOR_CLOSE_CONFIRM_MS 700  // 关门动作确认时间（需同时按两个按钮）

#define DOOR_OPEN_CONFIRM_MS 300   // 开门动作确认时间
#define RELEASE_DELAY_MS     200   // 按钮释放防抖时间
#define AIR_STABILIZE_MS     3000  // 气压稳定延时：关门后气压需约2秒才能达到最大值
#define AIR_CHECK_INTERVAL_MS 1500 // 气压检测间隔
#define Debug

// 动作时间打印宏：定义 ACTION_TIMING 以启用动作耗时输出
#define ACTION_TIMING
#ifdef ACTION_TIMING
#define LOG_ACTION_TIME(name, elapsed_ms) \
    U1_Printf("[%s] %u ms\r\n", name, (unsigned int)(elapsed_ms))
#else
#define LOG_ACTION_TIME(name, elapsed_ms)  do {} while(0)
#endif

// 外部变量声明
extern scpi_choice_def_t cylinder_source[];
extern scpi_choice_def_t lock_source[];
extern scpi_choice_def_t led_source[];

// 全局变量定义
uint32_t lock_press_tick = 0;         // 锁定按钮按下时刻(ms)，0=未按下
uint32_t lock_release_tick = 0;       // 锁定按钮松开时刻(ms)
uint8_t  lock_released = 1;           // 锁定按钮是否已松开，初始为1允许首次操作

uint32_t door_ready_tick = 0;         // 门按钮按下时刻(ms)，用于Idle状态
uint32_t door_open_confirm_tick = 0;  // 开门按钮确认计时
uint32_t door_close_confirm_tick = 0; // 关门按钮确认计时
uint32_t release_start_tick = 0;      // 按钮释放开始时刻(ms)，0=释放完成

uint8_t system_status = SYS_INIT;     // 系统当前状态
uint8_t system_old_status;            // 系统上一次状态

uint8_t door_status = Door_Mid;       // 门当前状态
uint8_t door_old_status;              // 门上一次状态

uint32_t door_close_start_tick = 0;   // 关门开始时刻(ms)，0=未开始关门
uint32_t door_close_done_tick = 0;    // 关门完成时刻(ms)，气压稳定延时起点
uint32_t door_open_start_tick = 0;    // 开门开始时刻(ms)，0=未开始开门
uint32_t door_close_default_ms = 2000;// 关门默认时间(ms)，运行时由实际关门时间更新
uint8_t door_close_timing = 0;        // 关门计时标志

uint32_t air_last_check_tick = 0;     // 上次气压检测时刻(ms)

// 状态机主入口，周期性调用
uint8_t StateMachine_Input()
{
    // 读取输入输出状态
    uint8_t* I_status = InputIO_Read(CHECK_NUM);
    uint8_t in_01_08 = I_status[0];
    uint8_t in_09_16 = I_status[1];
    uint8_t* O_status = OutputIO_Read(CHECK_NUM);
    uint8_t out_01_08 = O_status[0];
    uint8_t out_09_16 = O_status[1];

    system_old_status = system_status;
    door_old_status = door_status;

    // 各状态动作处理
    Init_Action(in_01_08, in_09_16, out_01_08, out_09_16);
    Lock_Action(in_01_08, in_09_16, out_01_08, out_09_16);
    Idle_Action(in_01_08, in_09_16, out_01_08, out_09_16);
    Ready_Action(in_01_08, in_09_16, out_01_08, out_09_16);
    Running_Action(in_01_08, in_09_16, out_01_08, out_09_16);
    Complete_Action(in_01_08, in_09_16, out_01_08, out_09_16);
    Emerge_Action(in_01_08, in_09_16, out_01_08, out_09_16);
    Release_detection(in_01_08, in_09_16, out_01_08, out_09_16);
    Door_detection(in_01_08, in_09_16, out_01_08, out_09_16);

    // 状态变化时打印
    if(system_status != system_old_status)
    {
        showStatus();
        system_old_status = system_status;
    }

    if(door_status != door_old_status)
    {
        showDoorStatus();
        door_old_status = door_status;
    }

    // 关门计时已改用TIM1时间戳，无需每周期累加
    return 0;
}

// 初始化动作：系统上电后进入锁定状态
uint8_t Init_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16)
{
    if(system_status == SYS_INIT)
    {
        system_status = Lock; // 初始化完成，进入锁定状态
    }
    return 0;
}

// 锁定状态动作及切换
uint8_t Lock_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16)
{
    // 状态迁移：动作状态->次级状态
    if(system_status == Lock)
    {
        if((out_01_08&power_out))
        {
            system_status = Idle; // 锁定状态中，长按解锁按钮，触发上电开关，进入空闲状态。
        }
        else if(!out_01_08&power_out)
        {
            system_status = Lock; // 锁定状态中，解锁未触发，进入锁定状态
        }
    }

    // 动作执行->任何状态下都能使用LOCK，进入锁定状态。
    uint32_t now = GetTim1Ms();
    uint8_t lock_ready_status = 0;

    if(in_09_16&(power_button>>8)) // 启动按钮按下
    {
        if(lock_press_tick == 0)
        {
            lock_press_tick = now; // 记录首次按下时刻
        }
        if((now - lock_press_tick) >= LOCK_PRESS_MS) // 按下保持时间达到要求
        {
            lock_ready_status = 1;
        }
    }
    else
    {
        lock_press_tick = 0;  // 按钮松开，重置按下计时
        lock_released = 1;    // 标记按钮已松开，允许下次操作
    }

    // 按钮保持足够时间 + 已松开过 + 冷却时间已过 → 切换锁定/空闲状态
    if((in_09_16&(power_button>>8)) && (lock_ready_status == 1) && lock_released &&
       (lock_release_tick == 0 || (now - lock_release_tick) >= LOCK_IDLE_MS))
    {
        if(out_01_08&power_out)
        {
            Lock_Write(lock_source[1]); // 若系统处于解锁状态，按下按钮后，系统進入锁定状态
            LED_Write(led_source[0]);   // 清除所有LED灯
            system_status = Lock;
            LOG_ACTION_TIME("LOCK", now - lock_press_tick);
        }
        else
        {
            Lock_Write(lock_source[0]); // 若系统处于锁定状态，按下按钮后，系统进行空闲状态
            LED_Write(led_source[0]);   // 清除所有LED灯
            system_status = Idle;
            LOG_ACTION_TIME("UNLOCK", now - lock_press_tick);
        }
        lock_ready_status = 0;
        lock_press_tick = 0;
        lock_released = 0;         // 清除松开标记，防止长按重复触发
        lock_release_tick = now;   // 开始冷却计时
    }
    return 0;
}

// 空闲状态动作及切换
uint8_t Idle_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16)
{
    uint32_t now = GetTim1Ms();

    // 判断当前状态
    if(system_status == Idle)
    {
        if(!(out_01_08&power_out))
        {
            system_status = Lock;           // 当流转至空闲状态，并且解锁使能关闭，系统进入锁定状态
        }

        if(in_01_08&door_sensor_up)         // 系统开门完成，判定状态前提
        {
            if (out_01_08&led_yellow)
            {
                system_status = Ready;      // 当开门，并且黄灯亮起，系统进入准备状态
            }
            else
            {
                system_status = Idle;       // 当开门，且黄灯熄灭，系统进入空闲状态
            }
        }
        else if (in_01_08&door_sensor_down) // 系统关门完成
        {
            system_status = Complete;
        }

        // 动作跳转执行：短按两个关门按钮，准备关门
        if((in_09_16&(door_button1>>8))||(in_09_16&(door_button2>>8)))
        {
            if(!(out_01_08&door_close))      // 若不在执行关闭动作，且门处于打开状态，进行动作跳转准备
            {
                if(door_ready_tick == 0)
                {
                    door_ready_tick = now;   // 记录首次按下时刻
                }
            }
        }
        else
        {
            door_ready_tick = 0;             // 按钮松开，重置
        }

        if((door_ready_tick != 0) && ((now - door_ready_tick) >= DOOR_READY_MS) && (release_start_tick == 0))
        {
            LED_Write(led_source[3]); // 黄灯亮起，表示门动作准备就绪
            system_status = Ready;  // door action ready
            door_ready_tick = 0;
        }
    }
    return 0;
}

// 准备状态动作及切换
uint8_t Ready_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16)
{
    uint32_t now = GetTim1Ms();

    // 判断当前状态
    if(system_status == Ready)
    {
        if(!(out_01_08&led_yellow))
        {
            system_status = Idle; // 在准备状态时，黄灯熄灭，回到空闲状态
        }
        else
        {
            // 关门动作执行中
            if((out_01_08&door_close)&&(!(in_01_08&door_sensor_down)))
            {
                system_status = Running; // 黄灯亮起，在执行关门动作，且还未关门完成时，系统处于运动状态
            }
            else if((out_01_08&door_open)&&(in_01_08&door_sensor_up))
            {
                system_status = Ready; // 黄灯亮起，执行开门动作，并且开门完成时，系统处于准备状态
            }
        }

        // 动作跳转执行：短按两个关门按钮，准备关门
        uint8_t door_ready_status = 0;
        if((in_09_16&(door_button1>>8))||(in_09_16&(door_button2>>8)))
        {
            if(!(out_01_08&door_close)&&(release_start_tick == 0))
            {
                if(door_close_confirm_tick == 0)
                {
                    door_close_confirm_tick = now;
                }
            }
            if((door_close_confirm_tick != 0) && ((now - door_close_confirm_tick) >= DOOR_CLOSE_CONFIRM_MS))
            {
                door_ready_status = 1;             // door action ready
            }
        }
        else
        {
            door_close_confirm_tick = 0;           // 按钮松开，重置
        }

        // 同时按下两个关门按钮，且准备就绪，开始关门
        if(((in_09_16&(door_button1>>8))&&(in_09_16&(door_button2>>8)))&&(door_ready_status ==1)&&(release_start_tick == 0))
        {
            if(!(out_01_08&door_close))
            {
                Cylinder_Write(1, cylinder_source[0]); // 执行关门动作
                door_close_confirm_tick = 0;
                door_ready_status = 0;
                release_start_tick = now;              // 开始释放计时
                system_status = Running;

                door_close_start_tick = now;           // 记录关门开始时刻
                door_close_timing = 1;                 // 开始关门计时
                air_last_check_tick = 0;               // 复位气压检测
                LOG_ACTION_TIME("CLOSE_START", 0);
            }
        }
    }
    return 0;
}

// 运动状态（关门中）动作及切换
uint8_t Running_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16)
{
    if(system_status == Running)
    {
        if(out_01_08&door_close) // 气缸回缩
        {
            if(in_01_08&door_sensor_down)   // 触发后限位传感
            {
                LED_Write(led_source[1]);   // 绿灯亮起，关门完毕
                system_status = Complete;   // 气缸到达后限位
                door_status = Door_Closed;  // 门状态更新为关闭
                door_close_timing = 0;      // 停止关门计时
                door_close_default_ms = GetTim1Ms() - door_close_start_tick; // 记录关门实际耗时(ms)
                door_close_done_tick = GetTim1Ms(); // 记录关门完成时刻，气压稳定延时起点
                LOG_ACTION_TIME("CLOSE_DONE", door_close_default_ms);
            }
            else{
                system_status = Running;    // 气缸回缩中
            }
        }
    }
    return 0;
}

/**
 * @brief  关门完成状态动作及切换
 * @param  in_01_08   输入IO的低8位
 * @param  in_09_16   输入IO的高8位
 * @param  out_01_08  输出IO的低8位
 * @param  out_09_16  输出IO的高8位
 * @retval 0
 *
 * 该函数在系统处于Complete（关门完成）状态时调用，主要处理：
 * 1. 检查是否需要开门（气缸伸出），并根据前限位传感器状态切换到Idle状态。
 * 2. 检查是否有开门请求（按钮按下），并在满足条件时执行开门动作。
 */
uint8_t Complete_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16)
{
    uint32_t now = GetTim1Ms();

    if(system_status == Complete)
    {
        // 1. 检查气缸是否正在伸出（开门动作）
        if(out_01_08 & door_open) // 如果气缸正在伸出
        {
            // 检查前限位传感器（门已完全打开）
            if(in_01_08 & door_sensor_up)
            {
                LED_Write(led_source[0]);   // 关灯，表示门已打开
                system_status = Idle;       // 切换到Idle（空闲）状态
                door_status = Door_Opened;  // 门状态更新为打开
                if(door_open_start_tick != 0) {
                    LOG_ACTION_TIME("OPEN_DONE", GetTim1Ms() - door_open_start_tick);
                    door_open_start_tick = 0;
                }
            }
        }

        uint8_t door_complete_status = 0; // 标志：是否满足开门动作的条件

        // 2. 检查是否有开门请求（任意一个开门按钮被按下）
        if((in_09_16 & (door_button1 >> 8)) || (in_09_16 & (door_button2 >> 8)))
        {
            // 如果当前没有执行开门动作且门已关到位
            if(!(out_01_08 & door_open) && (in_01_08 & door_sensor_down))
            {
                if(door_open_confirm_tick == 0)
                {
                    door_open_confirm_tick = now;
                }
            }
            // 如果按钮按下时间达到要求（防抖）
            if((door_open_confirm_tick != 0) && ((now - door_open_confirm_tick) >= DOOR_OPEN_CONFIRM_MS))
            {
                door_complete_status = 1; // 标记可以执行开门动作
            }
        }
        else
        {
            door_open_confirm_tick = 0;    // 按钮松开，重置
        }

        // 3. 满足开门条件且按钮已释放足够时间，执行开门动作
        if(((in_09_16 & (door_button1 >> 8)) || (in_09_16 & (door_button2 >> 8))) &&
            (door_complete_status == 1) && (release_start_tick == 0))
        {
            if(!(out_01_08 & door_open)) // 如果当前没有执行开门动作
            {
                Cylinder_Write(1, cylinder_source[1]);  // 执行开门动作（气缸伸出）
                door_open_start_tick = now;             // 记录开门开始时刻
                door_open_confirm_tick = 0;             // 清零按钮计时
                door_complete_status = 0;               // 清除动作标志
                release_start_tick = now;               // 设置释放计时
                LOG_ACTION_TIME("OPEN_START", 0);
            }
        }
    }
    return 0;
}

// 紧急状态检测与处理
uint8_t Emerge_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16)
{
    uint32_t now = GetTim1Ms();
    uint32_t door_elapsed = (door_close_start_tick != 0) ? (now - door_close_start_tick) : 0;

    // 如果当前系统状态为紧急状态
    if(system_status == Emergency)
    {
        // 检查所有激光传感器是否恢复正常（即没有异常信号）
        if(((in_01_08&laser_sensor1) != 0x20)||((in_01_08&laser_sensor2) != 0x40)||((in_01_08&laser_sensor3) != 0x80)||((in_09_16&(laser_sensor4>>8) != 0x01)))
        {
            system_status = Lock; // 恢复到锁定状态
            if(in_01_08&door_sensor_up)
            {
                LED_Write(led_source[0]); // 关灯，表示门已打开
            }
        }
    }

    // 检查急停按钮是否被按下（stop_button），如果被按下则立即进入紧急状态
    if((in_09_16&(stop_button>>8))!=0x08)
    {
        U1_Printf("E-STOP_Emerge_Action:%u\r\n",(unsigned int)door_elapsed);
        system_status = Emergency;                    // 设置系统状态为紧急
        Cylinder_Write(1,cylinder_source[1]);         // 执行气缸紧急动作（通常为开门）
        Lock_Write(lock_source[1]);                   // 执行锁紧急动作（通常为解锁）
        LED_Write(led_source[2]);                     // 点亮红灯表示紧急状态
    }
    // 检查激光传感器（laser_sensor1~4）是否有异常触发
    else if(((in_01_08&laser_sensor1)==0x20)||((in_01_08&laser_sensor2)==0x40)||((in_01_08&laser_sensor3)==0x80)||((in_09_16&(laser_sensor4>>8) == 0x01)))
    {
        // 只在关门进行中检测激光超时：关门已完成(door_close_timing==0)时不触发，避免关门后限位瞬断误报
        if(door_close_timing && (door_close_start_tick != 0) && !(in_01_08&door_sensor_down)&&(door_elapsed > (door_close_default_ms*2/3)))
        {
            U1_Printf("Door_Emerge_Action1:%u\r\n",(unsigned int)door_elapsed);
            U1_Printf("Door_Emerge_Action2:%u\r\n",(unsigned int)door_close_default_ms);
            system_status = Emergency;                // 设置系统状态为紧急
            Cylinder_Write(1,cylinder_source[1]);     // 执行气缸紧急动作（通常为开门）
            Lock_Write(lock_source[1]);               // 执行锁紧急动作（通常为解锁）
            LED_Write(led_source[2]);                 // 点亮红灯表示紧急状态
            door_close_start_tick = 0;                // 重置关门计时
            door_close_timing = 0;                    // 停止关门计时
        }
    }

    // 检查气压传感器：从关门完成时刻起算，等待气压稳定后再周期性检测
    // 气压稳定延时：关门后气路气压需约2秒才能达到最大值，稳定期间不检测，避免误报
    if((in_01_08 & door_sensor_down) && (door_close_done_tick != 0))
    {
        uint32_t elapsed = now - door_close_done_tick;

        // 先等待气压稳定延时，再开始检测
        if(elapsed >= AIR_STABILIZE_MS)
        {
            // 稳定后每隔AIR_CHECK_INTERVAL_MS检测一次气压传感器
            if((air_last_check_tick == 0) || ((now - air_last_check_tick) >= AIR_CHECK_INTERVAL_MS))
            {
                if(((in_01_08 & laser_sensor1) != 0x20))
                {
                    U1_Printf("Intake air pressure too low, elapsed:%u ms\r\n", (unsigned int)elapsed);
                }
                air_last_check_tick = now;
            }
        }
    }


    return 0;
}

// 按钮释放检测，防止误触发
// release_start_tick: 0=释放完成可操作, 非0=正在等待释放
// 两个门按钮同时松开且持续RELEASE_DELAY_MS后，清除释放计时
uint8_t Release_detection(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16)
{
    uint32_t now = GetTim1Ms();

    if((!(in_09_16&(door_button1>>8)))&&(!(in_09_16&(door_button2>>8))))
    {
        // 两个按钮都已松开
        if(release_start_tick != 0)
        {
            if((now - release_start_tick) >= RELEASE_DELAY_MS)
            {
                release_start_tick = 0; // 释放完成
            }
        }
    }
    else
    {
        // 仍有按钮按下，重置释放计时
        if(release_start_tick != 0)
        {
            release_start_tick = now;
        }
    }
    return 0;
}

// 门状态检测
uint8_t Door_detection(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16)
{
    if((out_01_08&door_close)&&(in_01_08&door_sensor_down))
    {
        door_status = Door_Closed;
    }
    else if((out_01_08&door_open)&&(in_01_08&door_sensor_up))
    {
        door_status = Door_Opened;
    }
    return 0;
}

// 系统状态打印（调试用）
uint8_t showStatus()
{
    #ifdef Debug
    switch (system_status)
    {
    case 0:
        U1_Printf("LOCK\r\n");
        break;
    case 1:
        U1_Printf("IDLE\r\n");
        break;
    case 2:
        U1_Printf("READY\r\n"); // READY
        break;
    case 3:
        U1_Printf("RUNNING\r\n");
        break;
    case 4:
        U1_Printf("EMERGENCY\r\n");
        break;
    case 5:
        U1_Printf("COMPLETE\r\n"); //COMPLETE
        break;
    default:
        break;
    }
    #endif // DEBUG
    return 0;
}

// 门状态打印（调试用）
uint8_t showDoorStatus(){
    osDelay(50);
    switch (door_status)
    {
        case 0:
            U1_Printf("CLOSED\r\n");
            break;
        case 1:
            U1_Printf("OPENED\r\n");
            break;
        case 2:
            U1_Printf("MID\r\n");
            break;
        default:
            break;
    }
    return 0;
}
