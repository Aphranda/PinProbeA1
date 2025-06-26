#include "StateMachine.h"
#include "BsmRelay.h"
#include "cmsis_os.h"

// 定义锁定和关门动作的延迟计数
#define Lock_Delay_num 3
#define Door_Delay_num 3
#define Debug

// 外部变量声明
extern scpi_choice_def_t cylinder_source[];
extern scpi_choice_def_t lock_source[];
extern scpi_choice_def_t led_source[];

// 全局变量定义
uint16_t lock_num = 0;                // 锁定动作计数
uint16_t lock_idle = Lock_Delay_num;  // 锁定动作松开计数

uint16_t door_ready_num = 0;          // 关门准备计数
uint16_t door_open_num = 0;           // 开门动作计数
uint16_t door_close_num = 0;          // 关门动作计数
uint16_t Release_flag = 0;            // 按钮释放标志
uint8_t system_status = SYS_INIT;     // 系统当前状态
uint8_t system_old_status;            // 系统上一次状态

uint8_t door_status = Door_Mid;       // 门当前状态
uint8_t door_old_status;              // 门上一次状态

uint16_t door_close_timer = 0;        // 关门计时器，单位为调用周期
uint16_t door_close_default_time = 40;// 关门默认时间，单位为调用周期
uint8_t door_close_timing = 0;        // 关门计时标志

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

    // 关门计时
    if(door_close_timing)
    {
        door_close_timer++; // 每次调用加1，单位为调用周期
    }
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
    uint8_t lock_ready_status = 0;
    if(in_09_16&(power_button>>8)) // 启动按钮按下
    {
        lock_num ++;
        if(lock_num>=Lock_Delay_num) // 启动按钮按下确认
        {
            lock_ready_status = 1;
        }
    }
    else{
        if(lock_idle>0)             // 启动按钮松开
        {
            lock_idle --;           // 启动按钮松开一段时间
        }
    }

    // 按钮松开后，切换锁定/空闲状态
    if((in_09_16&(power_button>>8)) && (lock_ready_status == 1)&&(lock_idle <= 0))
    {
        if(out_01_08&power_out)
        {
            Lock_Write(lock_source[1]); // 若系统处于解锁状态，按下按钮后，系统進入锁定状态
            LED_Write(led_source[0]);   // 清除所有LED灯
            system_status = Lock;
        }
        else
        {
            Lock_Write(lock_source[0]); // 若系统处于锁定状态，按下按钮后，系统进行空闲状态
            LED_Write(led_source[0]);   // 清除所有LED灯
            system_status = Idle;
        }
        lock_ready_status = 0;
        lock_num = 0;
        lock_idle = Lock_Delay_num;
    }
    return 0;
}

// 空闲状态动作及切换
uint8_t Idle_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16)
{
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
                door_ready_num ++;
            }
        }
        if((door_ready_num >= Door_Delay_num)&&(Release_flag<=0))       // door action ready num
        {
            LED_Write(led_source[3]); // 黄灯亮起，表示门动作准备就绪
            system_status = Ready;  // door action ready
            door_ready_num = 0;  
        }
    }
    return 0;
}

// 准备状态动作及切换
uint8_t Ready_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16)
{
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
            if(!(out_01_08&door_close)&&(Release_flag<=0))
            {
                door_close_num ++;
            }
            if(door_close_num >= Door_Delay_num*2) // 按钮按下次数达到要求
            {
                door_ready_status = 1;             // door action ready
            }
        }

        // 同时按下两个关门按钮，且准备就绪，开始关门
        if(((in_09_16&(door_button1>>8))&&(in_09_16&(door_button2>>8)))&&(door_ready_status ==1)&&(Release_flag<=0))
        {
            if(!(out_01_08&door_close))
            {
                Cylinder_Write(1, cylinder_source[0]); // 执行关门动作
                door_close_num = 0;
                door_ready_status = 0;
                Release_flag = Door_Delay_num;
                system_status = Running;
                
                door_close_timer = 0; // 重置计时器
                door_close_timing = 1; // 开始计时
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
                door_close_timing = 0;      // 停止关门计时
                door_close_default_time = door_close_timer; // 记录关门时间
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
    if(system_status == Complete)
    {
        // 1. 检查气缸是否正在伸出（开门动作）
        if(out_01_08 & door_open) // 如果气缸正在伸出
        {
            // 检查前限位传感器（门已完全打开）
            if(in_01_08 & door_sensor_up)
            {
                LED_Write(led_source[0]);   // 点亮绿灯，表示门已打开
                system_status = Idle;       // 切换到Idle（空闲）状态
            }
        }

        uint8_t door_complete_status = 0; // 标志：是否满足开门动作的条件

        // 2. 检查是否有开门请求（任意一个开门按钮被按下）
        if((in_09_16 & (door_button1 >> 8)) || (in_09_16 & (door_button2 >> 8)))
        {
            // 如果当前没有执行开门动作且门已关到位
            if(!(out_01_08 & door_open) && (in_01_08 & door_sensor_down))
            {
                door_open_num++; // 按钮按下次数累加
            }
            // 如果按钮按下次数达到要求（防抖或确认）
            if(door_open_num >= Door_Delay_num / 2)
            {
                door_complete_status = 1; // 标记可以执行开门动作
            }
        }

        // 3. 满足开门条件且按钮已释放，执行开门动作
        if(((in_09_16 & (door_button1 >> 8)) || (in_09_16 & (door_button2 >> 8))) && 
            (door_complete_status == 1) && (Release_flag <= 0))
        {
            if(!(out_01_08 & door_open)) // 如果当前没有执行开门动作
            {
                Cylinder_Write(1, cylinder_source[1]);  // 执行开门动作（气缸伸出）
                door_open_num = 0;                      // 清零按钮计数
                door_complete_status = 0;               // 清除动作标志
                Release_flag = Door_Delay_num;          // 设置释放标志，防止重复触发
            }
        }
    }
    return 0;
}

// 紧急状态检测与处理
uint8_t Emerge_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16)
{
    // 如果当前系统状态为紧急状态
    if(system_status == Emergency)
    {
        // 检查所有激光传感器是否恢复正常（即没有异常信号）
        if(((in_01_08&laser_sensor1) != 0x20)||((in_01_08&laser_sensor2) != 0x40)||((in_01_08&laser_sensor3) != 0x80)||((in_09_16&(laser_sensor4>>8) != 0x01)))
        {
            system_status = Lock; // 恢复到锁定状态
            if(in_01_08&door_sensor_up)
            {
                LED_Write(led_source[0]); // 点亮绿灯，表示门已打开
            }
        }
    }

    // 检查急停按钮是否被按下（stop_button），如果被按下则立即进入紧急状态
    if((in_09_16&(stop_button>>8))!=0x08)
    {
        system_status = Emergency;                    // 设置系统状态为紧急
        Cylinder_Write(1,cylinder_source[1]);         // 执行气缸紧急动作（通常为开门）
        Lock_Write(lock_source[1]);                   // 执行锁紧急动作（通常为解锁）
        LED_Write(led_source[2]);                     // 点亮红灯表示紧急状态
    }
    // 检查激光传感器（laser_sensor1~4）是否有异常触发
    else if(((in_01_08&laser_sensor1)==0x20)||((in_01_08&laser_sensor2)==0x40)||((in_01_08&laser_sensor3)==0x80)||((in_09_16&(laser_sensor4>>8) == 0x01)))
    {
        // 如果门未完全关闭，且关门计时超过默认时间的2/3，则进入紧急状态
        if(!(in_01_08&door_sensor_down)&&(door_close_timer>(door_close_default_time*2/3)))
        {
            system_status = Emergency;                // 设置系统状态为紧急
            Cylinder_Write(1,cylinder_source[1]);     // 执行气缸紧急动作（通常为开门）
            Lock_Write(lock_source[1]);               // 执行锁紧急动作（通常为解锁）
            LED_Write(led_source[2]);                 // 点亮红灯表示紧急状态
            door_close_timer = 0;                     // 重置关门计时器
            door_close_timing = 0;                    // 停止关门计时
        }
    }
    return 0;
}

// 按钮释放检测，防止误触发
uint8_t Release_detection(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16)
{
    if((!(in_09_16&(door_button1>>8)))&&(!(in_09_16&(door_button2>>8))))
    {
        if(Release_flag>0)
        {
            Release_flag --;
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