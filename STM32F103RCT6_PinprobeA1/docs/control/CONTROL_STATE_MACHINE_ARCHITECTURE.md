# 状态机与动作控制架构

Status: Active
Domain: CONTROL
Canonical: `docs/control/CONTROL_STATE_MACHINE_ARCHITECTURE.md`
Related: `docs/control/CONTROL_TIMING_ANALYSIS.md`, `docs/control/CONTROL_TODO.md`, `docs/arch/ARCH_PRODUCT_ARCHITECTURE.md`, `docs/interface/SCPI_COMMANDS.md`, `docs/hardware/HARDWARE_IO_CONSTRAINTS.md`
Last updated: 2026-10-22

本文档定义 PinProbeA1 的主状态机、动作编排顺序、控制模式权限矩阵与安全例外。
事实源：`Hardware/RamVector/Src/state_vector.c`（状态与迁移）、`cmd_exec.c`（动作执行）、
`App/Src/app_tasks.c`（节拍与链路判定）。本文档与代码冲突时以代码为准，并登记漂移项。

## 一、状态集

`system_status` 取值与 `Vector_SysState_t` 对齐：

| 状态 | 值 | 含义 | 进入条件 | 退出条件 |
|---|---:|---|---|---|
| `V_STATE_INIT` | 6 | 上电初始化，尚未建立 IO 基线 | 复位后首次 `StateVector_Input()` | 首次完成 IO 采样后转 `LOCK` |
| `V_STATE_LOCK` | 0 | 断电锁定态（电源锁输出为 0） | 上电；`OUT_POWER` 丢失；`EMERGENCY` 复位后 | 电源按钮长按或 SCPI 解锁 → `IDLE` |
| `V_STATE_IDLE` | 1 | 空闲待命，门为开位 | 解锁完成；门在下限位对应 `COMPLETE` 的反向判定；黄灯被清除 | 按门按钮 → `READY`；关门动作开始 → `RUNNING` |
| `V_STATE_READY` | 2 | 关门预备（黄灯亮），等待确认 | 门按钮按住 `DOOR_READY_MS`(200 ms) | 黄灯清除 → `IDLE`；双按确认或 SCPI 关门 → `RUNNING` |
| `V_STATE_RUNNING` | 3 | 动作执行中（门/USB 行程） | 关门/开门动作开始 | 行程到位 → `COMPLETE`；超时/冲突 → 告警；急停 → `EMERGENCY` |
| `V_STATE_COMPLETE` | 5 | 动作完成（门在下限位，USB 已插入） | 关门行程完成 | 开门动作开始 → `RUNNING`；解锁丢失 → `LOCK` |
| `V_STATE_EMERGENCY` | 4 | 急停/激光防护触发 | 急停按钮有效或任意激光遮挡 | 触发源消失后转 `LOCK`（需重新解锁） |

约束：

- `EMERGENCY` 是唯一不经用户确认就能进入的状态，且进入后必须经 `LOCK` 再回 `IDLE`，不允许直接恢复运行。
- 状态由 IO 镜像与动作输出共同推导（`system_status` 在 `StateVector_Input()` 内按当前输出/输入校正），
  不是纯事件驱动迁移表；因此任何「状态与 IO 不一致」都会在同一周期被纠偏。

## 二、动作编排顺序

### 2.1 关门序列（`IDLE` → `READY` → `RUNNING` → `COMPLETE`）

```text
门按钮按住 200 ms
  → 黄灯亮，进入 READY
双按确认 500 ms（或 SCPI 关门请求）
  → USB 插入（OUT_USB_IN），等待 IS_USB_INSERTED
  → 门下降（OUT_DOOR_CLOSE）
  → 门下限位有效 → COMPLETE（黄灯灭、绿灯常亮）
```

- USB 行程短，超过 `USB_MOVE_TIMEOUT_MS`(2000 ms) 未到位判 `USB_FAIL_TIMEOUT`；
  传感器冲突判 `USB_FAIL_SENSOR_CONFLICT`；双输出冲突判 `USB_FAIL_DUAL_OUTPUT`。
- 关门请求有两个入口：物理按钮（`CLOSE_PENDING_READY`）与 SCPI（`CLOSE_PENDING_SCPI`），
  由 `StateVector_TakeDoorCloseRequest()` 串行仲裁，避免重复下发。
- `DOOR_CLOSE_ARMED_HOLD_MS`(1000 ms) 内保持请求有效，超时作废。

### 2.2 开门序列（`COMPLETE` → `RUNNING` → `IDLE`）

```text
门按钮单击 200 ms（或 SCPI 开门）
  → USB 回退（OUT_USB_OUT），等待 IS_USB_RETRACTED
  → 门上升（OUT_DOOR_OPEN）
  → 门上限位有效 → IDLE
```

### 2.3 动作优先级与仲裁

命令槽三分（`RAM_Vector_t.cmd_lock` / `cmd_cylinder` / `cmd_led`），每槽带优先级：

| 优先级 | 宏 | 来源 |
|---:|---|---|
| 0 | `CMD_PRIO_OBSERVE` | 状态观测派生动作（最低） |
| 1 | `CMD_PRIO_USER` | 物理按钮、SCPI |
| 2 | `CMD_PRIO_SAFETY` | 急停、激光、故障注入（最高） |

- 高优先级可覆盖低优先级未执行命令；同优先级不覆盖，后到命令等待当前动作完成。
- `RamVector_TakeCmds()` 一次性取走快照供执行层使用；`RamVector_ClearNonSafetyCylinderCmd()`
  只清普通气缸命令，不动安全命令。

## 三、控制模式与权限矩阵

三种模式（`ControlMode_t`）：`LOCAL=1`、`REMOTE=2`、`MIXED=0`（上电默认）。

| 入口或能力 | `LOCAL` | `REMOTE` | `MIXED` |
|---|---|---|---|
| 物理按键执行普通门/USB 动作 | 允许 | 禁止 | 允许 |
| SCPI 执行普通门/USB 动作 | 禁止 | 允许 | 允许 |
| 状态、告警、错误队列、日志查询 | 允许 | 允许 | 允许 |
| IO、传感器、版本、诊断查询 | 允许 | 允许 | 允许 |
| 急停、激光防护、RS485 故障保护 | 始终有效 | 始终有效 | 始终有效 |
| 断电回锁及其他安全状态机 | 始终有效 | 始终有效 | 始终有效 |
| 锁定/解锁、必要维护操作 | 独立安全/维护权限 | 独立安全/维护权限 | 独立安全/维护权限 |

实现要点：

- 门禁函数为 `ControlMode_AllowsScpiAction()` / `ControlMode_AllowsPhysicalAction()`，
  所有普通动作入口必须先过门禁，再入命令槽。
- 模式切换本身是受控动作：`ControlMode_Set()` 记录迁移，SCPI 侧负责持久化，状态机不写 Flash。
- 被拒绝的动作必须返回明确错误码，不允许静默丢弃。

## 四、安全例外（不得削弱）

| 例外 | 触发源 | 行为 | 恢复 |
|---|---|---|---|
| 急停 | `IN_ESTOP_BTN`（常开/常闭可配） | 立即进 `EMERGENCY`，断电回锁 | 触发源消失 → `LOCK`，需人工解锁 |
| 激光防护 | 任意激光遮挡（`IN_LASER1..4`） | `EMERGENCY`，停止动作并报告 | 遮挡消失后重新解锁 |
| RS485 故障保护 | 连续失败 `RS485_FAIL_THRESHOLD`(10 次，约 250 ms) | 链路判 `FAULT`，动作降级并告警 | 连续 `IO_RECOVERY_STABLE_SAMPLES`(10 帧) 一致后恢复 `OK` |
| 断电回锁 | `OUT_POWER` 丢失 | 强制 `LOCK`，门/USB 输出复位 | 重新解锁 |

## 五、IO 链路可信状态

`Vector_IOState_t.rs485_ok` 复用为链路可信度，不改变 RAM 布局：

| 值 | 宏 | 语义 | 状态机行为 |
|---:|---|---|---|
| 0 | `VECTOR_IO_LINK_FAULT` | 读失败（超时/CRC 错） | 不执行新动作，保留最后可信状态，累计告警 |
| 1 | `VECTOR_IO_LINK_OK` | 采样有效且已稳定 | 正常迁移与执行 |
| 2 | `VECTOR_IO_LINK_RECOVERING` | 全 0 帧或首次恢复中 | 暂停动作下发，等待稳定采样 |

`ModBusTask` 的判定顺序：读失败 → 全 0 帧 → 未就绪时累计连续一致采样（10 帧）→ `OK`。
全 0 帧被视为「模块未上电/未响应」，不当作有效开关量。

## 六、去抖与超时预算

| 常量 | 值 | 用途 |
|---|---:|---|
| `DOOR_DEBOUNCE_CNT` | 3（3×25 ms = 75 ms） | 输入采样去抖 |
| `LOCK_PRESS_MS` | 300 ms | 电源按钮长按确认 |
| `LOCK_IDLE_MS` | 1000 ms | 锁定/解锁后忽略重复按压 |
| `DOOR_READY_MS` | 200 ms | 门按钮 → `READY` |
| `DOOR_CLOSE_CONFIRM_MS` | 500 ms | `READY` 下双按确认关门 |
| `DOOR_OPEN_CONFIRM_MS` | 200 ms | `COMPLETE` 下单按开门 |
| `RELEASE_DELAY_MS` | 200 ms | 按钮全松后的冷却 |
| `USB_MOVE_TIMEOUT_MS` | 2000 ms | USB 行程超时判故障 |
| `LED_ALERT_RED_BLINK_*` | 125 ms × 6 边沿 | 红色告警闪烁 |
| `LED_ALERT_YELLOW_BLINK_INTERVAL_MS` | 250 ms | 黄色告警闪烁 |

时间预算与任务周期的关系见 `docs/control/CONTROL_TIMING_ANALYSIS.md`。

## 七、调试开关

`vector_debug_flags` 由 SCPI `CONFigure:DEBUg:*` 控制，分四路：
`state`（状态切换与耗时）、`action`（动作起止）、`event`（急停/激光）、`io`（原始 IO 值刷屏）。
`io` 会产生大量输出，只在专项排查时开启，生产环境保持关闭。

## 八、未决问题

| 编号 | 问题 | 去向 |
|---|---|---|
| `CONTROL-Q1` | 控制模式权限分级已完成设计冻结与固件实现，实机联调未完成，无法标「已完成」。 | `docs/PROJECT_TODO.md` P0.4/P0.5 |
| `CONTROL-Q2` | USB 自动拔插异常（门开到位后 USB 未自动回退）的 O1/O2 纠偏未实现。 | `docs/hardware/HARDWARE_USB_REPLUG_A0_ANALYSIS.md`、`docs/PROJECT_TODO.md` |
| `CONTROL-Q3` | 状态推导与命令执行都在轮询路径上，缺少「动作耗时」实测表。 | `docs/validation/VALIDATION_TASK_PROGRESS.md` |
