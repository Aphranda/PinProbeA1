#include "StateMachine.h"
#include "BsmRelay.h"
#include "cmsis_os.h"

#define Lock_Delay_num 3
#define Door_Delay_num 3
#define Debug

extern scpi_choice_def_t cylinder_source[];
extern scpi_choice_def_t lock_source[];
extern scpi_choice_def_t led_source[];

uint16_t lock_num = 0;
uint16_t lock_idle = Lock_Delay_num; // LOCK切换的按压时间

uint16_t door_ready_num = 0;
uint16_t door_open_num = 0;
uint16_t door_close_num = 0;
uint16_t Release_flag = 0;
uint8_t system_status = SYS_INIT;
uint8_t system_old_status;

uint8_t door_status = Door_Mid;
uint8_t door_old_status;

uint8_t StateMachine_Input()
{

    // init Status

    // global check
    uint8_t* I_status = InputIO_Read(CHECK_NUM);
    uint8_t in_01_08 = I_status[0];
    uint8_t in_09_16 = I_status[1];
    uint8_t* O_status = OutputIO_Read(CHECK_NUM);
    uint8_t out_01_08 = O_status[0];
    uint8_t out_09_16 = O_status[1];
    
    system_old_status = system_status;
    door_old_status = door_status;

    Init_Action(in_01_08, in_09_16, out_01_08, out_09_16);
    Lock_Action(in_01_08, in_09_16, out_01_08, out_09_16);
    Idle_Action(in_01_08, in_09_16, out_01_08, out_09_16);
    Ready_Action(in_01_08, in_09_16, out_01_08, out_09_16);
    Running_Action(in_01_08, in_09_16, out_01_08, out_09_16);
    Complete_Action(in_01_08, in_09_16, out_01_08, out_09_16);
    Emerge_Action(in_01_08, in_09_16, out_01_08, out_09_16);
    Release_detection(in_01_08, in_09_16, out_01_08, out_09_16);
    Door_detection(in_01_08, in_09_16, out_01_08, out_09_16);
    
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
    return 0;
}

uint8_t Init_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16)
{
    if(system_status == SYS_INIT) 
    {
        system_status = Lock; // 初始化完成，进入锁定状态
    }
    return 0;
}

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


    if((in_09_16&(power_button>>8)) && (lock_ready_status == 1)&&(lock_idle <= 0)) // power button must be idle, jump action.
    {
        if(out_01_08&power_out)
        {
            Lock_Write(lock_source[1]); // 若系统处于解锁状态，按下按钮后，系统进入锁定状态
            LED_Write(led_source[0]);   // 清除所有LED灯
            system_status = Lock;
        }
        else
        {
            Lock_Write(lock_source[0]); // 若系统处于锁定状态，按下按钮后，系统进行空闲状态，
            LED_Write(led_source[0]);   // 清除所有LED灯
            system_status = Idle;
        }
        lock_ready_status = 0;
        lock_num = 0;
        lock_idle = Lock_Delay_num;
    }
    return 0;
}

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
        
        // 动作跳转执行
        if((in_09_16&(door_button1>>8))||(in_09_16&(door_button2>>8)))  // 同时短按两个关门按钮
        {
            if(!(out_01_08&door_close))      // 若不在执行关闭动作，且门处于打开状态，进行动作跳转准备
            {
                door_ready_num ++;
            }
        }
        if((door_ready_num >= Door_Delay_num)&&(Release_flag<=0))       // door action ready num
        {
            LED_Write(led_source[3]);
            system_status = Ready;  // door action ready
            door_ready_num = 0;  
        }
    }
    return 0;
}

uint8_t Ready_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16)
{
    // 判断当前状态
    if(system_status == Ready)
    {
        if(!(out_01_08&led_yellow)) 
        {
            system_status = Idle;                                       // 在准备状态时，黄灯熄灭，回到空闲状态
        }
        else 
        {
            if((out_01_08&door_close)&&(!(in_01_08&door_sensor_down))) 
            {
                system_status = Running;                                // 黄灯亮起，在执行关门动作，且还未关门完成时，系统处于运动状态
            }
            else if((out_01_08&door_open)&&(in_01_08&door_sensor_up))
            {
                system_status = Ready;                                  // 黄灯亮起，执行开门动作，并且开门完成时，系统处于准备状态
            }
        }

        // 动作跳转执行
        uint8_t door_ready_status = 0;
        if((in_09_16&(door_button1>>8))||(in_09_16&(door_button2>>8)))
        {
            if(!(out_01_08&door_close)&&(Release_flag<=0))
            {
                door_close_num ++;
            }
            if(door_close_num >= Door_Delay_num*2)                   // door action read num
            {
                door_ready_status = 1;                              // door action ready
            }
        }

        if(((in_09_16&(door_button1>>8))&&(in_09_16&(door_button2>>8)))&&(door_ready_status ==1)&&(Release_flag<=0))
        {
            if(!(out_01_08&door_close))
            {
                Cylinder_Write(1, cylinder_source[0]); // door closing
                door_close_num = 0;
                door_ready_status = 0;
                Release_flag = Door_Delay_num;
                system_status = Running;
            }
        }
    }
    return 0;
}

uint8_t Running_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16)
{
    if(system_status == Running)
    {
        if(out_01_08&door_close) //气缸回缩
        {
            if(in_01_08&door_sensor_down)   // 触发后限位传感
            {
                // LED_Write(led_source[1]);   // 绿灯亮起，关门完毕，进入软件测试流程
                system_status = Complete;   // 气缸到达后限位
            }
            else{
                system_status = Running;        // 气缸回缩中
            }
            
        }
    }
    return 0;
}

uint8_t Complete_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16)
{
    if(system_status == Complete)
    {
        if(out_01_08&door_open) // 气缸伸出
        {
            if(in_01_08&door_sensor_up) // 触发前限位传感
            {
                LED_Write(led_source[0]);
                system_status = Idle; // 气缸到达前限位
            }
        }

        uint8_t door_complete_status = 0;
        // ready action door
        if((in_09_16&(door_button1>>8))||(in_09_16&(door_button2>>8)))
        {
            if(!(out_01_08&door_open)&&(in_01_08&door_sensor_down)) //open door
            {
                door_open_num ++;
            }
            if(door_open_num >= Door_Delay_num/2)                   // door action read num
            {
                door_complete_status = 1;                              // door action ready
            }
        }
        if(((in_09_16&(door_button1>>8))||(in_09_16&(door_button2>>8)))&& (door_complete_status ==1)&&(Release_flag<=0))
        {
            if(!(out_01_08&door_open))
            {
                Cylinder_Write(1, cylinder_source[1]);  // door opening
                door_open_num = 0;
                door_complete_status = 0;
                Release_flag = Door_Delay_num;
            }
        }
    }
    return 0;
}

uint8_t Emerge_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16)
{
    if(system_status == Emergency)
    {
        if(((in_01_08&laser_sensor1) != 0x20)||((in_01_08&laser_sensor2) != 0x40)||((in_01_08&laser_sensor3) != 0x80)||((in_09_16&(laser_sensor4>>8) != 0x01)))
        {
            system_status = Lock;
            if(in_01_08&door_sensor_up)
            {
                LED_Write(led_source[0]);
            }
        }

    }

    if((in_09_16&(stop_button>>8))!=0x08)
    {
        system_status = Emergency;
        Cylinder_Write(1,cylinder_source[1]);
        Lock_Write(lock_source[1]);
        LED_Write(led_source[2]);
    }
    else if(((in_01_08&laser_sensor1)==0x20)||((in_01_08&laser_sensor2)==0x40)||((in_01_08&laser_sensor3)==0x80)||((in_09_16&(laser_sensor4>>8) ==
     0x01)))
    {
        if(!(in_01_08&door_sensor_down)&&!(in_01_08&door_sensor_up)) // 门未关上
        {
            system_status = Emergency;
            Cylinder_Write(1,cylinder_source[1]);
            Lock_Write(lock_source[1]);
            LED_Write(led_source[2]);
        }
    }
    return 0;
}

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