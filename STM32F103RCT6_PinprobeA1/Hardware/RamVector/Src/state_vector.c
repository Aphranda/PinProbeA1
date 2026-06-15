/*
 * state_vector.c — 纯逻辑状态机 (不直接读写 RS485)
 *
 * IO 数据来自 RamVector (ModBusTask 异步更新)
 * 事件 → PostCmd → ModBusTask 执行
 *
 * ════════════════════════════════════════════════════════════════════════════
 *  架构概述
 * ════════════════════════════════════════════════════════════════════════════
 *
 * 本模块是 PinProbe A1 箱体控制的核心状态机, 每 25ms 由 SysTimer 触发一次。
 * 它不直接操作 RS485 — 而是从 RamVector 读 IO 镜像, 将动作意向写入
 * RamVector 命令槽, 由 ModBusTask 统一执行。
 *
 * 状态机包含两层:
 *   Layer 1: IO 观测 → 状态自动纠偏
 *     物理世界发生变化后 (门限位到位、断电等), 状态机自动跟随。
 *     例如: power_out 断开 → 无论当前什么状态, 无条件回 Lock。
 *
 *   Layer 2: 事件驱动 → PostCmd
 *     按钮/急停/激光等事件触发动作, 写入 RamVector 命令槽。
 *     安全事件 (急停/激光) 优先级最高, 直接 PostCmd 不经过状态判断。
 *
 * ── 7 个状态 ──────────────────────────────────────────────────────────────
 *
 *   INIT ───────────────────────────────► LOCK   上电起始, 立即转 Lock
 *   LOCK ←────────────────────────────── 任何状态断电、急停恢复后
 *   LOCK ──► IDLE                       解锁 (power_out=1)
 *   IDLE ──► READY                      按关门按钮 + 黄灯亮
 *   IDLE ──► COMPLETE                   门已在下限位
 *   READY ──► RUNNING                  双按钮确认 → 关门开始
 *   RUNNING ──► COMPLETE               下限位到位 (或风险模式气压确认)
 *   COMPLETE ──► IDLE                  开门到位
 *   ANY ──► EMERGENCY                  急停 / 激光防夹触发
 *   EMERGENCY ──► LOCK                 传感器恢复
 *
 * ── 3 个命令通道 (全部通过 RamVector, 带优先级仲裁) ──────────────────────
 *
 *   通道      │ 优先级 0 (观测) │ 优先级 1 (用户) │ 优先级 2 (安全)
 *   ──────────┼────────────────┼─────────────────┼─────────────────
 *   Lock      │              — │ 按钮/SCPI 锁解锁  │ 急停锁
 *   Cylinder  │              — │ 按钮/SCPI 开关门  │ 急停开门
 *   LED       │ Emergency 红灯  │ 按钮/SCPI 灯色    │ RS485故障黄灯
 *
 * ════════════════════════════════════════════════════════════════════════════
 */

#include "state_vector.h"
#include "ram_vector.h"
#include "tim.h"
#include "flash.h"
#include <string.h>

/* ── 运行时调试开关 (由 SCPI CONFigure:DEBUg:xxx 控制, 默认全关) ── */
VectorDebugFlags_t vector_debug_flags = {false, false, false, false};

/* 调试打印宏 (仅对应 debug flag 开启时才输出) */
#define VEC_ACTION(n,e) do { if (vector_debug_flags.action) \
    Uart1_Printf("[%s] %u ms\r\n", n, (unsigned int)(e)); } while(0)

#define VEC_EVENT(e)    do { if (vector_debug_flags.event) \
    Uart1_Printf("[EVENT] %s\r\n", (e)); } while(0)

/* 状态码 → 可读名称 */
static const char* state_name(uint8_t s) {
    switch(s){case 0:return"LOCK";case 1:return"IDLE";case 2:return"READY";
    case 3:return"RUNNING";case 4:return"EMERGENCY";case 5:return"COMPLETE";default:return"INIT";}
}

/* ════════════════════════════════════════════════════════════════════════════
 *  IO 位定义
 *
 *  RS485 IO 扩展板通过 ModBus 返回 2+2 字节原始数据:
 *    IN[0]  (in_lo):  传感器 bit0~bit7   — 限位、激光、气压
 *    IN[1]  (in_hi):  传感器 bit8~bit15 — 按钮、急停、激光4
 *    OUT[0] (out_lo): 执行器 bit0~bit7  — 门方向、LED、电源
 *    OUT[1] (out_hi): 执行器 bit8~bit15 — 预留(当前未使用)
 *
 *  命名对齐 BsmRelay.h 的 inputIO / outputIO 枚举。
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── 输入低字节 IN[0] (传感器) ── */
#define IN_DOOR_UP      0x01   /* 门上限位: 门已完全打开 */
#define IN_DOOR_DOWN    0x02   /* 门下限位: 门已完全关闭 */
#define IN_LASER1       0x20   /* 激光1 / 气压传感器: 检测进气压力是否正常 */
#define IN_LASER2       0x40   /* 激光2: 防夹手检测 */
#define IN_LASER3       0x80   /* 激光3: 防夹手检测 */
#define IN_LASER_ANY    (IN_LASER1 | IN_LASER2 | IN_LASER3)  /* 低字节全部激光 */

/* ── 输入高字节 IN[1] (按钮/急停) ── */
#define IN_LASER4       0x01   /* 激光4: 防夹手检测 (高字节 bit0) */
#define IN_DOOR_BTN1    0x02   /* 门按钮 1 (关门/开门) */
#define IN_DOOR_BTN2    0x04   /* 门按钮 2 (关门/开门, 需双按) */
#define IN_DOOR_BTN_ANY  (IN_DOOR_BTN1 | IN_DOOR_BTN2)
#define IN_ESTOP_BTN    0x08   /* 急停按钮 (NC常闭或NO常开, Flash可配) */
#define IN_POWER_BTN    0x10   /* 电源按钮: 长按切换锁定/解锁 */

/* ── 输出低字节 OUT[0] (执行器) ── */
#define OUT_DOOR_OPEN   0x01   /* 气缸伸出 → 门上升 (开门) */
#define OUT_DOOR_CLOSE  0x02   /* 气缸回缩 → 门下降 (关门) */
#define OUT_DOOR_MOVING (OUT_DOOR_OPEN | OUT_DOOR_CLOSE)
#define OUT_LED_GREEN   0x10   /* 绿灯: 关门完成/正常 */
#define OUT_LED_RED     0x20   /* 红灯: 急停/故障 */
#define OUT_LED_YELLOW  0x40   /* 黄灯: 准备关门 / RS485故障 */
#define OUT_POWER       0x80   /* 电源锁: 0=锁定(断电) 1=解锁(上电) */

/* ── 速查宏: 把位运算封装为布尔语义 ── */
#define IS_DOOR_UP(v)     ((v) & IN_DOOR_UP)       /* 门在上限位? */
#define IS_DOOR_DOWN(v)   ((v) & IN_DOOR_DOWN)     /* 门在下限位? */
#define IS_ANY_LASER(v,h) (((v) & IN_LASER_ANY) || ((h) & IN_LASER4))  /* 任意激光被遮挡? */
#define IS_ANY_BTN(h)     ((h) & IN_DOOR_BTN_ANY)   /* 任意门按钮按下? */
#define IS_BOTH_BTN(h)    (((h) & IN_DOOR_BTN_ANY) == IN_DOOR_BTN_ANY)  /* 两个门按钮同时按下? */
#define IS_ESTOP(h)       ((h) & IN_ESTOP_BTN)      /* 急停按钮触点闭合? (NO模式) */
#define IS_POWER_BTN(h)   ((h) & IN_POWER_BTN)      /* 电源按钮按下? */
#define IS_UNLOCKED(o)    ((o) & OUT_POWER)          /* 系统已解锁? */
#define IS_DOOR_OPENING(o)  ((o) & OUT_DOOR_OPEN)    /* 气缸正在伸出? */
#define IS_DOOR_CLOSING(o)  ((o) & OUT_DOOR_CLOSE)   /* 气缸正在回缩? */
#define IS_LED_YELLOW(o)  ((o) & OUT_LED_YELLOW)     /* 黄灯亮? */

/* ── 时序参数 (ms), 基于 TIM1 1ms 时钟 ── */
#define LOCK_PRESS_MS         300   /* 电源按钮需按住 300ms 才生效 (防误触) */
#define LOCK_IDLE_MS         1000   /* 锁定/解锁后 1s 内不响应再次按下 */
#define DOOR_READY_MS         200   /* 按关门按钮 200ms → 亮黄灯进入 Ready */
#define DOOR_CLOSE_CONFIRM_MS 500   /* Ready 下双按 500ms → 开始关门 (防误触) */
#define DOOR_OPEN_CONFIRM_MS  200   /* Complete 下单按 200ms → 开始开门 */
#define RELEASE_DELAY_MS      200   /* 按钮全部松开后等 200ms 才允许下次操作 */
#define DOOR_DEBOUNCE_CNT       3   /* 连续 3 次采样一致才确认 (3×25ms=75ms) */
#define RS485_FAIL_THRESHOLD   10   /* RS485 连续失败 10 次 (~250ms) 触发告警 */

/*
 * 消抖宏: 连续 DOOR_DEBOUNCE_CNT 次读到高电平才确认, 任一次低电平就复位。
 * 用于过滤门限位和按钮的瞬态抖动。
 */
#define DEBOUNCE_UP(cnt, db, raw, mask)   do { \
    if ((raw) & (mask)) { if (++(cnt) >= DOOR_DEBOUNCE_CNT) (db) = 1; } \
    else { (cnt) = 0; (db) = 0; } \
} while(0)

void StateVector_Input(void)
{
    /*
     * ── 持久状态变量 (static, 跨调用保持) ──────────────────────────────
     *
     * 锁控:
     *   lock_press_tick    电源按钮首次按下时刻 (0=未按下)
     *   lock_release_tick  上次锁定/解锁动作完成时刻 (用于冷却)
     *   lock_released      按钮是否已松开 (防止按住不放反复触发)
     *
     * 门控时序:
     *   door_ready_tick          Idle 下按关门按钮的时刻
     *   door_close_confirm_tick  Ready 下按按钮的确认计时
     *   door_open_confirm_tick   Complete 下按按钮的确认计时
     *   release_start_tick      按钮释放计时起点 (0=已释放, 可接受新操作)
     *
     * 关门过程:
     *   door_close_start_tick    本次关门开始时刻 (0=未在关门)
     *   door_close_done_tick     关门完成时刻 (气压检测的稳定延时起点)
     *   door_open_start_tick     本次开门开始时刻 (用于 OPEN_DONE 耗时)
     *   door_close_default_ms    学习的关门时间 (首次全行程关门后更新)
     *   door_close_timing        是否正在计关门时间
     *   door_close_from_full     本次关门是否从上限位开始 (用于决定是否学习)
     *   door_close_time_learned  是否已完成至少一次关门学习
     *   air_last_check_tick      上次气压检测时刻
     *   m_23 / m_100 / m_300     关门时间里程碑已打印标志 (调试用, 各印一次)
     *
     * 位置确认:
     *   poweron_position_ok      上电后是否已完成位置确认
     *
     * 消抖:
     *   door_up_cnt / door_up_db     门上限位消抖计数/消抖后值
     *   door_down_cnt / door_down_db 门下限位消抖计数/消抖后值
     *   btn1_cnt / btn1_db           按钮1消抖计数/消抖后值
     *   btn2_cnt / btn2_db           按钮2消抖计数/消抖后值
     *
     * 系统:
     *   system_status   当前状态 (V_STATE_LOCK / IDLE / READY / ...)
     *   rs485_err_cnt   RS485 连续失败计数 (成功清零)
     *   rs485_fault     RS485 故障锁 (true=已触发保护, 禁止自动操作)
     */

    static uint32_t lock_press_tick, lock_release_tick;
    static uint8_t  lock_released = 1;
    static uint32_t door_ready_tick, door_open_confirm_tick, door_close_confirm_tick;
    static uint32_t release_start_tick;
    static uint32_t door_close_start_tick, door_close_done_tick, door_open_start_tick;
    static uint32_t door_close_default_ms = 2500, air_last_check_tick;
    static uint8_t  door_close_timing, door_close_from_full, door_close_time_learned;
    static uint8_t  poweron_position_ok;
    static uint8_t  door_up_cnt, door_down_cnt, door_up_db, door_down_db;
    static uint8_t  btn1_cnt, btn2_cnt, btn1_db, btn2_db;
    static uint8_t  system_status = V_STATE_INIT;
    static uint8_t  rs485_err_cnt;
    static bool     rs485_fault;
    static bool     m_23, m_100, m_300;

    /* ══════════════════════════════════════════
     *  步骤 1: 读取 IO 镜像
     *
     *  ModBusTask 每周期更新 RamVector 中的 IO 数据,
     *  本状态机只读缓存, 不触发 RS485 通讯。
     * ══════════════════════════════════════════ */
    const Vector_IOState_t* vio = RamVector_GetLocalIO();
    uint8_t in_lo  = vio->raw_in_lo;    /* IN[0]: 传感器 (限位/激光/气压) */
    uint8_t in_hi  = vio->raw_in_hi;    /* IN[1]: 按钮 (门/急停/电源) */
    uint8_t out_lo = vio->raw_out_lo;   /* OUT[0]: 执行器 (门/LED/电源) */
    uint8_t out_hi = vio->raw_out_hi;   /* OUT[1]: 预留, 当前未使用 */
    (void)out_hi;
    (void)door_close_time_learned;      /* 调试/风险模式引用, 消除警告 */

    bool io_ok = (vio->rs485_ok != 0);

    /* ══════════════════════════════════════════
     *  步骤 1b: RS485 故障保护
     *
     *  连续 RS485_FAIL_THRESHOLD 次通讯失败 → 点亮黄灯告警,
     *  停止所有自动操作 (按钮和 SCPI 命令仍可通过 RamVector 写入,
     *  但本状态机不执行任何 PostCmd)。
     *  通讯恢复后自动清除故障态。
     * ══════════════════════════════════════════ */
    if (!io_ok) {
        if (++rs485_err_cnt >= RS485_FAIL_THRESHOLD) {
            if (!rs485_fault) {                         /* 首次触发, 仅打印一次 */
                Uart1_Printf("[RS485] FAULT\r\n");
                RamVector_PostLED(VCMD_LED_YELLOW, CMD_PRIO_SAFETY);
                rs485_fault = true;
                rs485_err_cnt = 0;                      /* 防溢出, 保持故障态 */
            }
        }
        return;                                         /* IO 不可靠, 跳过本轮 */
    }
    if (rs485_fault) {                                  /* 故障恢复 */
        Uart1_Printf("[RS485] RECOVERED\r\n");
        RamVector_PostLED(VCMD_LED_OFF, CMD_PRIO_SAFETY);
        rs485_fault = false;
    }
    rs485_err_cnt = 0;

    /* IO 变化时输出 (调试, 合并为一次 printf 避免非阻塞丢帧) */
    if (vector_debug_flags.io)
    { static uint8_t li0=0xFF,li1=0xFF,lo0=0xFF,lo1=0xFF;
      if(in_lo!=li0||in_hi!=li1||out_lo!=lo0||out_hi!=lo1){
          Uart1_Printf("[IO] IN:0x%02X,0x%02X OUT:0x%02X,0x%02X\r\n",
                       in_lo,in_hi,out_lo,out_hi);
          li0=in_lo;li1=in_hi;lo0=out_lo;lo1=out_hi;
      }
    }

    /* ══════════════════════════════════════════
     *  步骤 2: 输入消抖
     *
     *  门限位开关和按钮在切换瞬间会产生数 ms 的抖动 (bounce),
     *  连续采样 DOOR_DEBOUNCE_CNT 次确认后才更新消抖后的值,
     *  避免状态机在抖动期间反复跳转。
     *
     *  消抖后的值覆盖原始字节中对应的位, 后续逻辑全部使用消抖值。
     * ══════════════════════════════════════════ */
    DEBOUNCE_UP(door_up_cnt,   door_up_db,   in_lo, IN_DOOR_UP);
    DEBOUNCE_UP(door_down_cnt, door_down_db, in_lo, IN_DOOR_DOWN);
    DEBOUNCE_UP(btn1_cnt,      btn1_db,      in_hi, IN_DOOR_BTN1);
    DEBOUNCE_UP(btn2_cnt,      btn2_db,      in_hi, IN_DOOR_BTN2);

    /* 用消抖结果更新相应位: 抖掉 → 0, 稳定高 → 1 */
    if (door_up_db)   in_lo |= IN_DOOR_UP;   else in_lo &= ~IN_DOOR_UP;
    if (door_down_db) in_lo |= IN_DOOR_DOWN; else in_lo &= ~IN_DOOR_DOWN;
    if (btn1_db)      in_hi |= IN_DOOR_BTN1; else in_hi &= ~IN_DOOR_BTN1;
    if (btn2_db)      in_hi |= IN_DOOR_BTN2; else in_hi &= ~IN_DOOR_BTN2;

    uint32_t now = GetTim1Ms();     /* 当前时间 (TIM1 ms 计数器) */
    /*
     * 急停按钮: 支持 NC (常闭) 和 NO (常开) 两种接线方式, Flash 可配置。
     * 后续触发与恢复都使用同一个有效态, 避免 NC/NO 逻辑不一致。
     */
    bool estop_active = (Flash_GetEstopType() == 1)
        ? IS_ESTOP(in_hi)           /* NO 模式: bit=1 → 触发 */
        : !IS_ESTOP(in_hi);         /* NC 模式: bit=0 → 触发 (默认) */

    /* ══════════════════════════════════════════════════════════════════════
     *  步骤 3: Layer 1 — IO 观测 → 状态自动纠偏
     *
     *  不依赖任何按钮事件, 纯粹从物理 IO 推断系统当前应该处于什么状态。
     *  这保证了断电恢复、传感器瞬断恢复后状态机能自动回到正确的状态,
     *  不会卡死在中间态。
     *
     *  状态转移图:
     *
     *    [上电] → INIT → LOCK
     *                ↑
     *    power_out=0 —┘  (任何非紧急/非初始状态, 断电就回 Lock)
     *
     *    LOCK ── power_out=1 ──→ IDLE
     *      ↑                      │
     *      │      door_up         ├─→ READY  (黄灯亮 + 上限位)
     *      │      door_down       └─→ COMPLETE (门在下限位)
     *      │
     *    READY ── 黄灯灭 ──→ IDLE
     *      └── door_close + !door_down ──→ RUNNING
     *
     *    RUNNING ── door_down到位 ──→ COMPLETE (绿灯)
     *                (或风险模式: 气压+超时确认)
     *
     *    COMPLETE ── door_open + door_up到位 ──→ IDLE (关灯)
     *
     *    EMERGENCY ── 激光清 + 急停清 ──→ LOCK
     * ══════════════════════════════════════════════════════════════════════ */

    /* INIT → LOCK: 上电初始转入锁定 */
    if (system_status == V_STATE_INIT) system_status = V_STATE_LOCK;

    /*
     * 断电回 Lock: power_out=0 表示系统被锁定(断电),
     * 无论之前是什么状态 (除了 EMERGENCY 和 INIT), 立即回 Lock。
     * 这是安全底线 — 断电时所有动作必须停止。
     */
    if (!IS_UNLOCKED(out_lo) && system_status != V_STATE_EMERGENCY && system_status != V_STATE_INIT)
        system_status = V_STATE_LOCK;

    /* Lock → Idle: 解锁信号 (power_out=1) 出现即进入空闲, 可以操作 */
    if (system_status == V_STATE_LOCK && IS_UNLOCKED(out_lo))
        system_status = V_STATE_IDLE;

    /* Idle 状态: 判断当前门位置 */
    if (system_status == V_STATE_IDLE) {
        /*
         * 门在上限位 (或人工确认过位置) → Idle 或 Ready
         *   Ready 条件: 黄灯已亮 (说明用户已按过关门按钮)
         *   否则保持 Idle
         *
         * 门在下限位 → Complete (说明门关着, 可能是断电恢复)
         *
         * 门在中间 → 停留 Idle, 等用户操作 (不自动动作, 安全考虑)
         */
        if (IS_DOOR_UP(in_lo) || poweron_position_ok)
            system_status = IS_LED_YELLOW(out_lo) ? V_STATE_READY : V_STATE_IDLE;
        else if (IS_DOOR_DOWN(in_lo))
            system_status = V_STATE_COMPLETE;
    }

    /* Ready 状态: 黄灯亮, 等待关门指令 */
    if (system_status == V_STATE_READY) {
        if (!IS_LED_YELLOW(out_lo)) system_status = V_STATE_IDLE;  /* 黄灯被清除 → 回 Idle */
        if (IS_DOOR_CLOSING(out_lo) && !IS_DOOR_DOWN(in_lo)) system_status = V_STATE_RUNNING;  /* 开始关门 */
    }

    /* Running 状态: 关门进行中 */
    if (system_status == V_STATE_RUNNING) {

        /* 调试: 追踪关门阶段 (TOP → MID → BOTTOM) */
        if (vector_debug_flags.action)
        {   static uint8_t close_phase = 0; /* 0=上, 1=中, 2=下 */
            uint8_t new_phase;
            if (IS_DOOR_UP(in_lo))          new_phase = 0;
            else if (IS_DOOR_DOWN(in_lo))   new_phase = 2;
            else                            new_phase = 1;
            if (new_phase != close_phase) {
                Uart1_Printf("[CLOSE] phase %d @ %u ms\r\n",
                    new_phase, (unsigned int)(now - door_close_start_tick));
                close_phase = new_phase;
            }
        }

        /* 调试: 关门时间里程碑 (各只印一次) */
        if (vector_debug_flags.action)
        {   uint32_t el = now - door_close_start_tick;
            if (!m_23 && el > door_close_default_ms * 2 / 3)
                { Uart1_Printf("[CLOSE] 2/3 @ %u ms, laser active\r\n", (unsigned int)el); m_23 = true; }
            if (!m_100 && el > door_close_default_ms)
                { Uart1_Printf("[CLOSE] 100%% @ %u ms, expected done\r\n", (unsigned int)el); m_100 = true; }
            if (!m_300 && el > door_close_default_ms * 3)
                { Uart1_Printf("[CLOSE] 300%% @ %u ms, TIMEOUT\r\n", (unsigned int)el); m_300 = true; }
        }

        /*
         * 关门完成判定 — 两条路径满足其一即可:
         *
         *   [正常路径] limit_ok: 下限位传感器触发 (门已关到底)
         *
         *   [风险模式] risk_ok: 气压传感器确认 + 超过学习时间
         *     当限位开关故障时, Flash_GetRiskMode()=true 启用此路径。
         *     条件: 关门指令在执行 + 进气压力正常 + 已超过学习的关门时间。
         *
         * 完成动作:
         *   - 绿灯亮 (表示安全)
         *   - 如果本次从上限位开始关门, 学习实际关门耗时 (用于下次超时判断)
         *   - 记录关门完成时刻 (气压检测的延时起点)
         */
        bool limit_ok  = IS_DOOR_CLOSING(out_lo) && IS_DOOR_DOWN(in_lo);
        bool risk_ok   = Flash_GetRiskMode()
                      && IS_DOOR_CLOSING(out_lo)
                      && (in_lo & IN_LASER1)    /* 气压传感器正常 (未低压报警) */
                      && door_close_start_tick
                      && ((now - door_close_start_tick) > door_close_default_ms);
        if (limit_ok || risk_ok) {
            if (risk_ok && !limit_ok)
                Uart1_Printf("[RISK] door close confirmed by pressure, limit failed\r\n");
            RamVector_PostLED(VCMD_LED_GREEN, CMD_PRIO_USER);
            door_close_timing = 0; door_open_confirm_tick = 0;
            /* 从上限位开始的关门才学习时间 (中间位置的时间不准确) */
            if (door_close_from_full) {
                uint32_t t = now - door_close_start_tick;
                if (t >= 1000) {                        /* < 1s 不学, 太快不靠谱 */
                    door_close_default_ms = t;
                    door_close_time_learned = 1;
                }
            }
            door_close_done_tick = now; door_close_from_full = 0;
            VEC_ACTION("CLOSE_DONE", door_close_default_ms);
            system_status = V_STATE_COMPLETE;
        }
    }

    /* Complete → Idle: 开门到位 (气缸在伸出 + 上限位触发) */
    if (system_status == V_STATE_COMPLETE) {
        if (IS_DOOR_OPENING(out_lo) && IS_DOOR_UP(in_lo)) {
            RamVector_PostLED(VCMD_LED_OFF, CMD_PRIO_USER);
            VEC_ACTION("OPEN_DONE", now - door_open_start_tick);
            door_open_start_tick = 0;
            door_close_done_tick  = 0;
            poweron_position_ok   = 0;                  /* 完成一周期, 复位人工确认 */
            system_status = V_STATE_IDLE;
        }
    }

    /*
     * Emergency → Lock:
     *   急停恢复条件: 所有激光传感器恢复正常 + 急停按钮已复位
     *   恢复后进入 Lock 而非之前的状态, 确保安全。
     *   红灯在 Emergency 期间由事件处理段 (步骤4) 维持, 这里只判断退出。
     */
    if (system_status == V_STATE_EMERGENCY) {
        RamVector_PostLED(VCMD_LED_RED, CMD_PRIO_OBSERVE);
        if (!IS_ANY_LASER(in_lo, in_hi) && !estop_active) {
            system_status = V_STATE_LOCK;
            door_close_done_tick = 0;
        }
    }

    /* ══════════════════════════════════════════
     *  步骤 4: 安全事件 — 急停 / 激光防夹
     *
     *  安全事件优先级最高, 直接 PostCmd (CMD_PRIO_SAFETY),
     *  不经过状态判断。处理完后 return, 本轮不再执行按钮事件。
     * ══════════════════════════════════════════ */

    bool estop = estop_active;
    bool laser_emergency = false;

    if (estop) {
        uint32_t et = door_close_start_tick ? (now - door_close_start_tick) : 0;
        if (vector_debug_flags.event)
            Uart1_Printf("[EVENT] ESTOP @ %u ms (close el:%u)\r\n", (unsigned int)now, (unsigned int)et);

        system_status = V_STATE_EMERGENCY;
        RamVector_PostLED(VCMD_LED_RED, CMD_PRIO_SAFETY);     /* 红灯 */
        RamVector_PostLock(VCMD_LOCK, CMD_PRIO_SAFETY);       /* 锁定 */
        if (!IS_DOOR_UP(in_lo))                                /* 门未在顶 → 开门 */
            RamVector_PostCylinder(VCMD_CYLINDER_OPEN, CMD_PRIO_SAFETY);
        door_close_done_tick = 0;
    }

    /*
     * 激光防夹: 关门过程中任意激光被遮挡 → 紧急停止 + 开门释放
     *
     *   触发窗口: 关门开始后, 经过 2/3 的预期关门时间才开始检测。
     *   这是因为门体在关门初期会经过激光束, 属于正常现象。
     *
     *   触发动作: 红灯 + 锁 + 开门 (释放被夹物体)。
     *
     *   激光包括:
     *     IN[0] bit5-7: 激光1 (气压) / 激光2 / 激光3
     *     IN[1] bit0:   激光4
     *   任意一个被遮挡 (低字节激光为 1 表示被遮, 气压激光正常时为 1) 即触发。
     */
    if (IS_ANY_LASER(in_lo, in_hi)) {
        uint32_t de = door_close_start_tick ? (now - door_close_start_tick) : 0;
        /*
         * 仅在以下条件全部满足时才触发:
         *   1. door_close_timing: 正在关门
         *   2. door_close_start_tick: 有关门开始时间
         *   3. !IS_DOOR_DOWN: 门还没关到底 (到底后激光不触发)
         *   4. de > 2/3 预期时间: 已过激光有效窗口
         */
        if (door_close_timing && door_close_start_tick &&
            !IS_DOOR_DOWN(in_lo) && (de > door_close_default_ms * 2 / 3)) {
            if (vector_debug_flags.event)
                Uart1_Printf("[EVENT] LASER @ %u ms\r\n", (unsigned int)de);
            laser_emergency = true;
            system_status = V_STATE_EMERGENCY;
            RamVector_PostLED(VCMD_LED_RED, CMD_PRIO_SAFETY);
            RamVector_PostLock(VCMD_LOCK, CMD_PRIO_SAFETY);
            RamVector_PostCylinder(VCMD_CYLINDER_OPEN, CMD_PRIO_SAFETY);
            door_close_start_tick = 0; door_close_timing = 0; door_close_done_tick = 0;
        }
    }

    /* ══════════════════════════════════════════
     *  步骤 5: 按钮事件 → PostCmd
     *
     *  仅在非急停/非激光紧急时处理按钮。
     *  按钮事件通过 RamVector 命令槽发出, 由 ModBusTask 执行。
     * ══════════════════════════════════════════ */
    if (!estop && !laser_emergency) {

        /*
         * ── 电源按钮: 长按 LOCK_PRESS_MS 切换 锁定/解锁 ──
         *
         * 机制: 按住 300ms 触发, 松开后需等 1000ms (冷却) 才能再次触发。
         * 当前解锁 → 执行锁定; 当前锁定 → 执行解锁 + 关灯。
         */
        if (IS_POWER_BTN(in_hi)) {
            if (!lock_press_tick) lock_press_tick = now;    /* 记录首次按下时刻 */
        } else {
            lock_press_tick = 0; lock_released = 1;          /* 松开: 重置计时, 允许下次触发 */
        }
        if (IS_POWER_BTN(in_hi) && ((now - lock_press_tick) >= LOCK_PRESS_MS) && lock_released &&
            (!lock_release_tick || (now - lock_release_tick) >= LOCK_IDLE_MS)) {
            if (IS_UNLOCKED(out_lo)) {
                RamVector_PostLock(VCMD_LOCK, CMD_PRIO_USER); VEC_ACTION("LOCK", now - lock_press_tick);
            } else {
                RamVector_PostLock(VCMD_UNLOCK, CMD_PRIO_USER);
                RamVector_PostLED(VCMD_LED_OFF, CMD_PRIO_USER);
                VEC_ACTION("UNLOCK", now - lock_press_tick);
            }
            lock_press_tick = 0; lock_released = 0; lock_release_tick = now;  /* 冷却开始 */
        }

        /*
         * ── 上电后首次解锁: 位置确认 ──
         *
         * 上电时不知道门在什么位置, 需要确认一次:
         *   - 门在上限位 → 输出开门信号压紧
         *   - 门在下限位 → 输出关门信号压紧
         *   - 门在中间 → 不自动操作, 等人按按钮 (安全, 防止意外夹伤)
         */
        if (system_status == V_STATE_IDLE && !poweron_position_ok) {
            if (IS_DOOR_UP(in_lo)) {
                RamVector_PostCylinder(VCMD_CYLINDER_OPEN,  CMD_PRIO_USER); poweron_position_ok = 1;
            } else if (IS_DOOR_DOWN(in_lo)) {
                RamVector_PostCylinder(VCMD_CYLINDER_CLOSE, CMD_PRIO_USER); poweron_position_ok = 1;
            }
        }

        /*
         * ── Idle 状态: 按关门按钮 → 亮黄灯 (进入 Ready) ──
         *
         * 操作: 任意一个门按钮按下 + 不在关门动作中 → 200ms 后亮黄灯。
         * 黄灯表示"准备关门", 下一步需要双按确认才会真正关门。
         */
        if (system_status == V_STATE_IDLE) {
            if (IS_ANY_BTN(in_hi) && !IS_DOOR_CLOSING(out_lo)) {
                if (!door_ready_tick) door_ready_tick = now;
            } else {
                door_ready_tick = 0;                            /* 按钮松开或条件不满足 → 重置 */
            }
            if (door_ready_tick && ((now - door_ready_tick) >= DOOR_READY_MS) && !release_start_tick) {
                RamVector_PostLED(VCMD_LED_YELLOW, CMD_PRIO_USER);  /* 黄灯 = 准备就绪 */
                door_ready_tick = 0; poweron_position_ok = 1;
            }
        }

        /*
         * ── Ready 状态: 双按钮确认 → 关门 ──
         *
         * 安全设计: 必须同时按下两个门按钮 (双手操作, 防止单手被夹),
         * 持续 500ms 确认后才真正执行关门。
         * 关门从上限位开始 → 记录为 "全行程关门" (用于学习关门时间)。
         */
        if (system_status == V_STATE_READY) {
            /* 第一阶段: 任意按钮按下 → 开始 500ms 倒计时 (不是双按就直接重置) */
            if (IS_ANY_BTN(in_hi) && !IS_DOOR_CLOSING(out_lo) && !release_start_tick) {
                if (!door_close_confirm_tick) door_close_confirm_tick = now;
            } else {
                door_close_confirm_tick = 0;
            }
            /* 第二阶段: 双按 + 500ms 到 → 执行关门 */
            if (IS_BOTH_BTN(in_hi) && door_close_confirm_tick &&
                ((now - door_close_confirm_tick) >= DOOR_CLOSE_CONFIRM_MS) && !release_start_tick) {
                if (!IS_DOOR_CLOSING(out_lo)) {
                    RamVector_PostCylinder(VCMD_CYLINDER_CLOSE, CMD_PRIO_USER);
                    door_close_confirm_tick = 0; release_start_tick = now;
                    door_close_start_tick = now; door_close_timing = 1;
                    door_close_from_full = IS_DOOR_UP(in_lo) ? 1 : 0;  /* 记录起点 */
                    air_last_check_tick = 0;
                    m_23 = m_100 = m_300 = false;                       /* 重置里程碑 */
                    VEC_ACTION("CLOSE_START", 0);
                }
            }
        }

        /*
         * ── Complete 状态: 按按钮 → 开门 ──
         *
         * 门已关闭, 单按钮确认 200ms 即可开门 (相对于关门更安全, 不需要双按)。
         */
        if (system_status == V_STATE_COMPLETE) {
            /* 确认阶段: 按下 + 不在开门中 + 门已关 → 计时 */
            if (IS_ANY_BTN(in_hi) && !IS_DOOR_OPENING(out_lo) && IS_DOOR_DOWN(in_lo)) {
                if (!door_open_confirm_tick) door_open_confirm_tick = now;
            } else {
                door_open_confirm_tick = 0;
            }
            /* 确认时间到 → 开门 */
            if (IS_ANY_BTN(in_hi) && door_open_confirm_tick &&
                ((now - door_open_confirm_tick) >= DOOR_OPEN_CONFIRM_MS) && !release_start_tick) {
                if (!IS_DOOR_OPENING(out_lo)) {
                    RamVector_PostCylinder(VCMD_CYLINDER_OPEN, CMD_PRIO_USER);
                    door_open_start_tick = now; door_open_confirm_tick = 0; release_start_tick = now;
                    VEC_ACTION("OPEN_START", 0);
                }
            }
        }

        /*
         * ── 关门后气压监测 ──
         *
         * 关门完成后等待 3s 让气压稳定, 之后每 1.5s 检测一次。
         * 气压过低 (IN_LASER1=0) 时打印警告, 不触发急停。
         * 这是状态监控而非安全保护, 仅用于诊断气路问题。
         */
        if (IS_DOOR_DOWN(in_lo) && door_close_done_tick) {
            uint32_t el = now - door_close_done_tick;
            if (el >= 3000 && (!air_last_check_tick || ((now - air_last_check_tick) >= 1500))) {
                if (!(in_lo & IN_LASER1))
                    Uart1_Printf("Intake air pressure too low, elapsed:%u ms\r\n", (unsigned int)el);
                air_last_check_tick = now;
            }
        }
    }

    /*
     * ── 按钮释放检测 ──
     *
     * 两个门按钮全部松开 + 持续 RELEASE_DELAY_MS → release_start_tick=0,
     * 表示"一次完整操作已结束", 可以接受下一次操作。
     * 防止按钮在释放过程中的抖动被误判为新的一次按下。
     */
    if (!IS_ANY_BTN(in_hi)) {
        if (release_start_tick && ((now - release_start_tick) >= RELEASE_DELAY_MS))
            release_start_tick = 0;                             /* 释放完成 */
    } else {
        if (release_start_tick) release_start_tick = now;       /* 还有按钮按着, 重置释放计时 */
    }

    /* ══════════════════════════════════════════
     *  步骤 6: 状态变化输出 + 同步到 RamVector
     * ══════════════════════════════════════════ */
    if (vector_debug_flags.state)
    { static uint8_t  last_state = 0xFF;
      static uint32_t state_enter_tick;
      if (system_status != last_state) {
          if (last_state != 0xFF)
              Uart1_Printf("[STATE] %s -> %s (%u ms)\r\n",
                  state_name(last_state), state_name(system_status),
                  (unsigned int)(now - state_enter_tick));
          else
              Uart1_Printf("[STATE] %s\r\n", state_name(system_status));
          last_state = system_status;
          state_enter_tick = now;
      }
    }

    RamVector_SetState((Vector_SysState_t)system_status);
    RamVector_Heartbeat();
}
