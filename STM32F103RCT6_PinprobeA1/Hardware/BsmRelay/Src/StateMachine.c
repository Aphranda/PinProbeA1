/**
 * @file    StateMachine.c
 * @brief   系统主状态机实现
 * @note    
 *          架构说明：
 *          - 使用函数指针分发表（StateDispatchTable），主循环按当前状态
 *            直接索引到对应的处理函数，避免逐一遍历所有状态。
 *          - 状态处理分为 enter（状态进入时执行一次）和 run（周期执行）两个阶段，
 *            由 StateMachine_Input 中的 transition 检测自动触发。
 *          - 紧急状态监测（急停/激光传感器）是跨状态切面，在调度前统一检测。
 *          调用周期：50ms（由 ModBusTask 控制）
 */

#include "StateMachine.h"
#include "BsmRelay.h"
#include "cmsis_os.h"

/* ======================== 宏定义 ======================== */

/// 锁定动作消抖计数（约 3 个周期 = 150ms）
#define LOCK_DEBOUNCE_NUM      3

/// 关门按钮消抖计数（约 3 个周期 = 150ms）
#define DOOR_DEBOUNCE_NUM      3

/// 按钮释放等待计数
#define RELEASE_WAIT_NUM       3

/// 气压稳定延时周期数（关门后约 2 秒，40 周期）
#define AIR_STABILIZE_CYCLES   60

/// 气压检测周期间隔（稳定后每 30 周期检测一次）
#define AIR_CHECK_INTERVAL     30

/// 关门超时百分比（超过此时间触发紧急）
#define DOOR_TIMEOUT_RATIO     0.667f

//#define STATEMACHINE_DEBUG

/* ======================== 外部引用 ======================== */

extern scpi_choice_def_t cylinder_source[];
extern scpi_choice_def_t lock_source[];
extern scpi_choice_def_t led_source[];

/* ======================== 状态机全局变量 ======================== */

MachineState system_status = SYS_INIT;  ///< 系统当前状态（类型安全）
MachineState system_old_status;         ///< 系统上一周期状态

uint8_t door_status = Door_Mid;         ///< 门当前状态
uint8_t door_old_status;                ///< 门上一周期状态

uint16_t door_close_timer = 0;          ///< 关门计时器（单位：调用周期）
uint16_t door_close_default_time = 40;  ///< 关门默认参考时间（单位：调用周期）
uint8_t  door_close_timing = 0;         ///< 关门计时标志

/* ======================== 局部状态变量 ======================== */

static uint16_t lock_press_cnt = 0;     ///< 启动按钮按下计数（消抖）
static uint16_t lock_release_cnt = LOCK_DEBOUNCE_NUM; ///< 启动按钮释放等待计数
static uint8_t  lock_ready = 0;         ///< 锁定动作就绪标志

static uint16_t door_ready_cnt = 0;     ///< 关门准备按钮计数
static uint16_t door_open_cnt = 0;      ///< 开门按钮计数
static uint16_t door_close_cnt = 0;     ///< 关门执行按钮计数
static uint16_t release_flag = 0;       ///< 按钮释放等待计数（防止误触发）

static uint16_t air_check_cnt = 0;      ///< 气压检测周期计数
static uint8_t  state_entered = 0;      ///< 当前状态是否已执行 enter 动作

/* ======================== 前向声明 ======================== */

static void StateMachine_Dispatch(const IOStatus *io);
static void Emergency_Monitor(const IOStatus *io);
static void State_Enter(MachineState state);

static void sys_init_enter(const IOStatus *io);
static void lock_run(const IOStatus *io);
static void idle_run(const IOStatus *io);
static void ready_run(const IOStatus *io);
static void running_run(const IOStatus *io);
static void complete_run(const IOStatus *io);
static void emergency_run(const IOStatus *io);

static void release_detection(const IOStatus *io);
static void door_detection(const IOStatus *io);
static void air_pressure_monitor(const IOStatus *io);

/* ======================== 状态分发表 ======================== */

/**
 * @brief 状态处理函数指针类型
 * @param io 当前周期 IO 状态快照
 */
typedef void (*StateHandler)(const IOStatus *io);

/**
 * @brief 状态分发表
 * 
 * 索引 = MachineState 枚举值。
 * 每个状态可定义 enter（进入时调用一次）和 run（每周期调用）两个处理函数。
 * enter == NULL 表示不需要进入动作。
 */
static const StateHandler state_entry_table[] = {
    [Lock]      = NULL,              /* Lock 由 lock_run 内部处理进入动作 */
    [Idle]      = NULL,
    [Ready]     = NULL,
    [Running]   = NULL,
    [Emergency] = NULL,
    [Complete]  = NULL,
    [SYS_INIT]  = sys_init_enter,
};

static const StateHandler state_dispatch_table[] = {
    [Lock]      = lock_run,
    [Idle]      = idle_run,
    [Ready]     = ready_run,
    [Running]   = running_run,
    [Emergency] = emergency_run,
    [Complete]  = complete_run,
    [SYS_INIT]  = NULL,             /* SYS_INIT 由 sys_init_enter 处理，无需 run */
};

/* ======================== 状态机主入口 ======================== */

/**
 * @brief  状态机主入口，由 ModBusTask 每 50ms 周期调用
 * @retval 0 成功
 */
uint8_t StateMachine_Input(void)
{
    // 1. 统一读取 IO 状态
    uint8_t *I_status = InputIO_Read(CHECK_NUM);
    uint8_t *O_status = OutputIO_Read(CHECK_NUM);

    IOStatus io = {
        .in_01_08  = I_status[0],
        .in_09_16  = I_status[1],
        .out_01_08 = O_status[0],
        .out_09_16 = O_status[1],
    };

    // 2. 记录旧状态（用于变化检测）
    system_old_status = system_status;
    door_old_status = door_status;

    // 3. 跨状态切面：紧急监测（优先级最高）
    Emergency_Monitor(&io);

    // 4. 状态分发：通过函数指针表直接调度
    StateMachine_Dispatch(&io);

    // 5. 跨状态辅助检测
    release_detection(&io);
    door_detection(&io);

    // 6. 关门计时
    if (door_close_timing) {
        door_close_timer++;
    }

    // 7. 状态变化打印
    if (system_status != system_old_status) {
        showStatus();
    }
    if (door_status != door_old_status) {
        showDoorStatus();
    }

    return 0;
}

/* ======================== 状态分发核心 ======================== */

/**
 * @brief 根据当前状态进行分发调度
 * @param io IO 状态快照
 * 
 * 检测到状态变化时，先调用新状态的 enter 函数（如果存在），
 * 然后每周期调用状态的 run 函数。
 */
static void StateMachine_Dispatch(const IOStatus *io)
{
    MachineState current = system_status;

    // 越界保护
    if (current > SYS_INIT) {
        system_status = Lock;
        return;
    }

    // 检测状态切换 → 触发 enter
    if (system_status != system_old_status) {
        state_entered = 0;
    }

    // 进入动作（状态切换后执行一次）
    if (!state_entered) {
        if (state_entry_table[current] != NULL) {
            state_entry_table[current](io);
        }
        state_entered = 1;
    }

    // 周期动作
    if (state_dispatch_table[current] != NULL) {
        state_dispatch_table[current](io);
    }
}

/* ======================== 紧急监测（跨状态切面） ======================== */

/**
 * @brief 紧急状态监测——在任何状态下都优先执行
 * @param io IO 状态快照
 * 
 * 监测条件（满足任一即触发）：
 * 1. 急停按钮被按下
 * 2. 关门过程中激光传感器被遮挡（且关门时间超过 2/3 参考时间）
 */
static void Emergency_Monitor(const IOStatus *io)
{
    uint8_t in_09_16 = io->in_09_16;
    uint8_t in_01_08 = io->in_01_08;

    // 急停按钮检测
    if ((in_09_16 & (stop_button >> 8)) != 0x08) {
        U1_Printf("E-STOP_Emerge_Action:%u\r\n", door_close_timer);
        system_status = Emergency;
        Cylinder_Write(1, cylinder_source[1]);    // 开门
        Lock_Write(lock_source[1]);                // 解锁
        LED_Write(led_source[2]);                  // 红灯
        return;
    }

    // 激光传感器检测（关门过程中）
    uint8_t laser_triggered = 
        ((in_01_08 & laser_sensor1) == 0x20) ||
        ((in_01_08 & laser_sensor2) == 0x40) ||
        ((in_01_08 & laser_sensor3) == 0x80) ||
        ((in_09_16 & (laser_sensor4 >> 8)) == 0x01);

    if (laser_triggered) {
        // 关门过程中且超时阈值到达
        if (!(in_01_08 & door_sensor_down) &&
            (door_close_timer > (uint16_t)(door_close_default_time * DOOR_TIMEOUT_RATIO))) {
            U1_Printf("Door_Emerge_Action:%u\r\n", door_close_timer);
            system_status = Emergency;
            Cylinder_Write(1, cylinder_source[1]); // 开门
            Lock_Write(lock_source[1]);             // 解锁
            LED_Write(led_source[2]);               // 红灯
            door_close_timer = 0;
            door_close_timing = 0;
            return;
        }
    }
}

/* ======================== 状态处理函数 ======================== */

/**
 * @brief SYS_INIT 进入动作——直接切换到 Lock 状态
 */
static void sys_init_enter(const IOStatus *io)
{
    (void)io;
    system_status = Lock;
}

/* ================================================================
 * Lock —— 锁定状态
 * 
 * 进入条件：系统初始化完成 / 解锁关闭 / 启动按钮切换
 * 退出条件：解锁成功（power_out 有效）
 * 
 * 此状态同时处理启动按钮的切换逻辑（锁定 ↔ 解锁）。
 * ================================================================ */

static void lock_run(const IOStatus *io)
{
    uint8_t in_09_16 = io->in_09_16;
    uint8_t out_01_08 = io->out_01_08;

    // ---- 状态切换逻辑 ----
    if (out_01_08 & power_out) {
        // 解锁成功 → 进入空闲
        system_status = Idle;
    }

    // ---- 启动按钮切换逻辑（任何状态下可触发锁定/解锁切换） ----
    if (in_09_16 & (power_button >> 8)) {
        lock_press_cnt++;
        if (lock_press_cnt >= LOCK_DEBOUNCE_NUM) {
            lock_ready = 1;
        }
    } else {
        if (lock_release_cnt > 0) {
            lock_release_cnt--;
        }
    }

    // 按钮按下确认 + 已释放 → 执行锁定/解锁切换
    if (lock_ready && (lock_release_cnt == 0) && (in_09_16 & (power_button >> 8))) {
        if (out_01_08 & power_out) {
            // 当前解锁 → 切换到锁定
            Lock_Write(lock_source[1]);   // LOCKED
            LED_Write(led_source[0]);     // LED OFF
            system_status = Lock;
        } else {
            // 当前锁定 → 切换到解锁
            Lock_Write(lock_source[0]);   // UNLOCK
            LED_Write(led_source[0]);     // LED OFF
            system_status = Idle;
        }
        lock_ready = 0;
        lock_press_cnt = 0;
        lock_release_cnt = LOCK_DEBOUNCE_NUM;
    }
}

/* ================================================================
 * Idle —— 空闲状态
 * 
 * 进入条件：解锁成功 / 开门完成 / 黄灯熄灭
 * 退出条件：解锁关闭 → Lock
 *          门关到位 → Complete
 *          关门按钮确认 → Ready
 * ================================================================ */

static void idle_run(const IOStatus *io)
{
    uint8_t in_01_08 = io->in_01_08;
    uint8_t in_09_16 = io->in_09_16;
    uint8_t out_01_08 = io->out_01_08;

    // ---- 状态切换 ----
    if (!(out_01_08 & power_out)) {
        system_status = Lock;
        return;
    }

    if (in_01_08 & door_sensor_up) {
        // 门已打开
        system_status = (out_01_08 & led_yellow) ? Ready : Idle;
        return;
    }

    if (in_01_08 & door_sensor_down) {
        system_status = Complete;
        return;
    }

    // ---- 关门准备动作 ----
    if ((in_09_16 & (door_button1 >> 8)) || (in_09_16 & (door_button2 >> 8))) {
        if (!(out_01_08 & door_close)) {
            door_ready_cnt++;
        }
    }

    // 按钮按够次数 + 已释放 → 亮黄灯，进入 Ready
    if ((door_ready_cnt >= DOOR_DEBOUNCE_NUM) && (release_flag == 0)) {
        LED_Write(led_source[3]);       // 黄灯
        system_status = Ready;
        door_ready_cnt = 0;
    }
}

/* ================================================================
 * Ready —— 准备状态
 * 
 * 进入条件：门打开 + 黄灯亮
 * 退出条件：黄灯熄灭 → Idle
 *          关门执行 → Running
 * ================================================================ */

static void ready_run(const IOStatus *io)
{
    uint8_t in_01_08 = io->in_01_08;
    uint8_t in_09_16 = io->in_09_16;
    uint8_t out_01_08 = io->out_01_08;

    // ---- 状态切换 ----
    if (!(out_01_08 & led_yellow)) {
        system_status = Idle;
        return;
    }

    // 关门执行中
    if ((out_01_08 & door_close) && !(in_01_08 & door_sensor_down)) {
        system_status = Running;
        return;
    }

    // 开门执行中 + 开门到位
    if ((out_01_08 & door_open) && (in_01_08 & door_sensor_up)) {
        system_status = Ready;  // 维持
        return;
    }

    // ---- 关门执行动作（双按钮同时按下确认） ----
    uint8_t door_ready_status = 0;

    if ((in_09_16 & (door_button1 >> 8)) || (in_09_16 & (door_button2 >> 8))) {
        if (!(out_01_08 & door_close) && (release_flag == 0)) {
            door_close_cnt++;
        }
        if (door_close_cnt >= DOOR_DEBOUNCE_NUM * 2) {
            door_ready_status = 1;
        }
    }

    // 同时按下两个按钮 → 执行关门
    if ((in_09_16 & (door_button1 >> 8)) &&
        (in_09_16 & (door_button2 >> 8)) &&
        door_ready_status && (release_flag == 0)) {
        if (!(out_01_08 & door_close)) {
            Cylinder_Write(1, cylinder_source[0]); // CLOSE
            door_close_cnt = 0;
            door_ready_status = 0;
            release_flag = RELEASE_WAIT_NUM;
            system_status = Running;

            // 关门计时开始
            door_close_timer = 0;
            door_close_timing = 1;
            air_check_cnt = 0;
        }
    }
}

/* ================================================================
 * Running —— 关门运行状态
 * 
 * 进入条件：关门命令已发出
 * 退出条件：门关到位（door_sensor_down）→ Complete
 * ================================================================ */

static void running_run(const IOStatus *io)
{
    uint8_t in_01_08 = io->in_01_08;
    uint8_t out_01_08 = io->out_01_08;

    if (out_01_08 & door_close) {
        if (in_01_08 & door_sensor_down) {
            // 关门到位
            LED_Write(led_source[1]);               // 绿灯
            system_status = Complete;
            door_status = Door_Closed;
            door_close_timing = 0;
            door_close_default_time = door_close_timer; // 记录实际关门时间
        }
        // else: 仍在运动中，维持 Running
    }
}

/* ================================================================
 * Complete —— 关门完成状态
 * 
 * 进入条件：门已关闭到位
 * 退出条件：开门动作执行 + 开门到位 → Idle
 * ================================================================ */

static void complete_run(const IOStatus *io)
{
    uint8_t in_01_08 = io->in_01_08;
    uint8_t in_09_16 = io->in_09_16;
    uint8_t out_01_08 = io->out_01_08;

    // ---- 状态切换（开门执行中） ----
    if (out_01_08 & door_open) {
        if (in_01_08 & door_sensor_up) {
            LED_Write(led_source[0]);   // 关灯
            system_status = Idle;
            door_status = Door_Opened;
        }
        return;
    }

    // ---- 开门请求 ----
    uint8_t door_complete_ready = 0;

    if ((in_09_16 & (door_button1 >> 8)) || (in_09_16 & (door_button2 >> 8))) {
        if (!(out_01_08 & door_open) && (in_01_08 & door_sensor_down)) {
            door_open_cnt++;
        }
        if (door_open_cnt >= DOOR_DEBOUNCE_NUM / 2) {
            door_complete_ready = 1;
        }
    }

    // 满足条件 + 按钮释放 → 执行开门
    if (((in_09_16 & (door_button1 >> 8)) || (in_09_16 & (door_button2 >> 8))) &&
        door_complete_ready && (release_flag == 0)) {
        if (!(out_01_08 & door_open)) {
            Cylinder_Write(1, cylinder_source[1]); // OPEN
            door_open_cnt = 0;
            door_complete_ready = 0;
            release_flag = RELEASE_WAIT_NUM;
        }
    }
}

/* ================================================================
 * Emergency —— 紧急状态
 * 
 * 进入条件：急停按钮 / 激光传感器异常
 * 退出条件：所有安全传感器恢复正常 → Lock
 * ================================================================ */

static void emergency_run(const IOStatus *io)
{
    uint8_t in_01_08 = io->in_01_08;
    uint8_t in_09_16 = io->in_09_16;

    // 检查所有激光传感器是否已恢复正常
    uint8_t all_clear =
        ((in_01_08 & laser_sensor1) != 0x20) &&
        ((in_01_08 & laser_sensor2) != 0x40) &&
        ((in_01_08 & laser_sensor3) != 0x80) &&
        ((in_09_16 & (laser_sensor4 >> 8)) != 0x01);

    if (all_clear) {
        system_status = Lock;
        if (in_01_08 & door_sensor_up) {
            LED_Write(led_source[0]);   // 关灯
        }
    }
}

/* ======================== 辅助检测函数 ======================== */

/**
 * @brief 按钮释放检测——防止短时间内重复触发
 * @param io IO 状态快照
 */
static void release_detection(const IOStatus *io)
{
    uint8_t in_09_16 = io->in_09_16;
    if (!(in_09_16 & (door_button1 >> 8)) && !(in_09_16 & (door_button2 >> 8))) {
        if (release_flag > 0) {
            release_flag--;
        }
    }
}

/**
 * @brief 门状态检测
 * @param io IO 状态快照
 */
static void door_detection(const IOStatus *io)
{
    uint8_t in_01_08 = io->in_01_08;
    uint8_t out_01_08 = io->out_01_08;

    if ((out_01_08 & door_close) && (in_01_08 & door_sensor_down)) {
        door_status = Door_Closed;
    } else if ((out_01_08 & door_open) && (in_01_08 & door_sensor_up)) {
        door_status = Door_Opened;
    }
}

/**
 * @brief 气压监测——关门完成后等待气压稳定，周期性检测进气压力
 * @param io IO 状态快照
 * 
 * 关门后需约 2 秒（60 周期）等待气压稳定，稳定后每 30 周期检测一次。
 * 检测结果仅输出日志，不触发状态切换。
 */
static void air_pressure_monitor(const IOStatus *io)
{
    uint8_t in_01_08 = io->in_01_08;

    if ((in_01_08 & door_sensor_down) && (door_close_timer != 0)) {
        air_check_cnt++;

        if (air_check_cnt > AIR_STABILIZE_CYCLES) {
            uint16_t post_stabilize = air_check_cnt - AIR_STABILIZE_CYCLES;

            if (post_stabilize >= AIR_CHECK_INTERVAL) {
                if ((in_01_08 & laser_sensor1) != 0x20) {
                    U1_Printf("Intake air pressure too low, check_num:%u\r\n", air_check_cnt);
                }
                // 保留稳定延时基数
                air_check_cnt = AIR_STABILIZE_CYCLES;
            }
        }
    }
}

/* ======================== 兼容接口：保留原始函数 ======================== */

/**
 * @brief 以下函数保留原始的基于 IO 参数的签名，供外部可能存在的调用使用。
 *        内部已不再依赖这些函数，状态切换逻辑已移至上述 static 处理函数。
 */

uint8_t Init_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16) {
    (void)in_01_08; (void)in_09_16; (void)out_01_08; (void)out_09_16;
    if (system_status == SYS_INIT) system_status = Lock;
    return 0;
}

uint8_t Lock_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16) {
    IOStatus io = { .in_01_08 = in_01_08, .in_09_16 = in_09_16, .out_01_08 = out_01_08, .out_09_16 = out_09_16 };
    if (system_status == Lock) lock_run(&io);
    return 0;
}

uint8_t Idle_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16) {
    IOStatus io = { .in_01_08 = in_01_08, .in_09_16 = in_09_16, .out_01_08 = out_01_08, .out_09_16 = out_09_16 };
    if (system_status == Idle) idle_run(&io);
    return 0;
}

uint8_t Ready_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16) {
    IOStatus io = { .in_01_08 = in_01_08, .in_09_16 = in_09_16, .out_01_08 = out_01_08, .out_09_16 = out_09_16 };
    if (system_status == Ready) ready_run(&io);
    return 0;
}

uint8_t Running_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16) {
    IOStatus io = { .in_01_08 = in_01_08, .in_09_16 = in_09_16, .out_01_08 = out_01_08, .out_09_16 = out_09_16 };
    if (system_status == Running) running_run(&io);
    return 0;
}

uint8_t Complete_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16) {
    IOStatus io = { .in_01_08 = in_01_08, .in_09_16 = in_09_16, .out_01_08 = out_01_08, .out_09_16 = out_09_16 };
    if (system_status == Complete) complete_run(&io);
    return 0;
}

uint8_t Emerge_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16) {
    IOStatus io = { .in_01_08 = in_01_08, .in_09_16 = in_09_16, .out_01_08 = out_01_08, .out_09_16 = out_09_16 };
    Emergency_Monitor(&io);
    if (system_status == Emergency) emergency_run(&io);
    return 0;
}

uint8_t Release_detection(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16) {
    IOStatus io = { .in_01_08 = in_01_08, .in_09_16 = in_09_16, .out_01_08 = out_01_08, .out_09_16 = out_09_16 };
    release_detection(&io);
    return 0;
}

uint8_t Door_detection(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16) {
    IOStatus io = { .in_01_08 = in_01_08, .in_09_16 = in_09_16, .out_01_08 = out_01_08, .out_09_16 = out_09_16 };
    door_detection(&io);
    return 0;
}

/* ======================== 调试打印 ======================== */

uint8_t showStatus(void)
{
#ifdef STATEMACHINE_DEBUG
    switch (system_status) {
    case Lock:      U1_Printf("LOCK\r\n");      break;
    case Idle:      U1_Printf("IDLE\r\n");      break;
    case Ready:     U1_Printf("READY\r\n");     break;
    case Running:   U1_Printf("RUNNING\r\n");   break;
    case Emergency: U1_Printf("EMERGENCY\r\n"); break;
    case Complete:  U1_Printf("COMPLETE\r\n");  break;
    default:        break;
    }
#endif
    return 0;
}

uint8_t showDoorStatus(void)
{
    osDelay(50);
    switch (door_status) {
    case Door_Closed: U1_Printf("CLOSED\r\n"); break;
    case Door_Opened: U1_Printf("OPENED\r\n"); break;
    case Door_Mid:    U1_Printf("MID\r\n");    break;
    default:          break;
    }
    return 0;
}
