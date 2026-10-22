# 日志系统架构

Status: Active
Domain: STORAGE
Canonical: `docs/storage/STORAGE_LOG_ARCHITECTURE.md`
Related: `docs/storage/STORAGE_FLASH_ARCHITECTURE.md`, `docs/control/CONTROL_STATE_MACHINE_ARCHITECTURE.md`, `docs/interface/INTERFACE_COMMUNICATION_ARCHITECTURE.md`
Last updated: 2026-10-22

本文档定义 AppLog 结构化诊断日志：表结构、编码空间、写入路径、实时泵出与丢弃策略、回读方式。
事实源：`Hardware/AppLog/Inc/app_log.h`、`Hardware/AppLog/Src/app_log.c`、`app_log_uart.c`。

## 一、设计取向

- 日志是**结构化记录**，不是自由文本：每条记录 16 B 定长，字段语义固定，便于上位机解析与统计。
- 日志独立于 `RAM_Vector_t`：位于 `RW_APPLOG`（`0x2000A400`，1280 B），不占用向量表 1024 B 布局。
- 日志不可作为状态判定输入：业务逻辑不得反向解析日志来推导状态。
- 日志默认不落盘：掉电后仅保留 RAM 内最近 76 条（W25Q128 的 Event Log 分区已预留，见
  `docs/storage/STORAGE_FLASH_ARCHITECTURE.md`）。

## 二、表结构

| 项 | 值 |
|---|---|
| 基址 | `0x2000A400` |
| 容量 | `0x500`（1280 B） |
| 记录容量 | 76 条 × 16 B = 1216 B，其余为表头与尾部魔数 |
| 表头魔数 | `PLOG`（`0x504C4F47`） |
| 表尾魔数 | `END\0`（`0x454E4400`） |
| 版本 | 1 |
| 环形策略 | `write_index` / `read_index` 双指针，写满覆盖最旧记录并累加 `drop_count` |

`AppLog_Table_t` 关键字段：`version`、`capacity`、`write_index`、`read_index`、`read_count`、
`realtime_index`、`realtime_count`、`next_seq`、`node_id`、`realtime_enabled`、
`drop_count`、`realtime_drop_count`。

`AppLog_Record_t`（16 B，编译期断言）：

| 字段 | 类型 | 含义 |
|---|---|---|
| `tick` | `uint32_t` | 记录时刻（系统 tick，ms） |
| `seq` | `uint16_t` | 全局递增序号，用于检测丢失 |
| `node_id` | `uint8_t` | 节点号（单机为 0） |
| `level` | `uint8_t` | 级别，见 2.1 |
| `module` | `uint8_t` | 模块，见 2.2 |
| `event_id` | `uint8_t` | 事件/动作编号，见 2.3 |
| `arg0`/`arg1` | `uint16_t` | 参数（耗时 ms、旧/新状态、IO 位图等） |
| `reserved` | `uint16_t` | 保留 |

### 2.1 级别（`AppLog_Level_t`）

| 值 | 名称 | 用途 |
|---:|---|---|
| 1 | `INFO` | 正常动作与状态迁移 |
| 2 | `WARN` | 可恢复异常（链路恢复中、重试） |
| 3 | `ERROR` | 需要处置的故障（链路失效、超时、冲突） |

### 2.2 模块（`AppLog_Module_t`）

| 值 | 名称 | 覆盖范围 |
|---:|---|---|
| 1 | `STATE` | 状态迁移（旧状态、新状态、停留耗时） |
| 2 | `ACTION` | 动作起止（关门/开门/锁定/LED） |
| 3 | `EVENT` | 外部事件（急停、激光、按钮、SCPI） |
| 4 | `IO` | IO 原始位图（`in_lo/in_hi/out_lo/out_hi`） |

### 2.3 编号空间

**动作编号（`AppLog_ActionId_t`）**：`LOCK`、`UNLOCK`、`CLOSE_START`、`CLOSE_DONE`、`OPEN_START`、
`OPEN_DONE`、`CLOSE_PHASE`、`CLOSE_MARK`、`CYLINDER_OPEN`、`CYLINDER_CLOSE`、
`LED_OFF`、`LED_GREEN`、`LED_RED`、`LED_YELLOW`。

**事件编号（`AppLog_EventId_t`）**：`RS485_FAULT`、`RS485_RECOVERED`、`ESTOP`、`LASER`、
`RISK_PRESSURE`、`AIR_LOW`、`POWER_BTN`、`DOOR_BTN`、`SCPI_LOCK`、`SCPI_CYLINDER`、`SCPI_LED`、
`IO_WRITE_FAIL`、`USB_INSERT_FAIL`、`USB_RETRACT_FAIL`、`DUT_NOT_INPLACE`。

新增编号只能追加，不得改变既有值（否则历史日志失去可读性）。

## 三、写入路径

| 接口 | 用途 | 典型字段 |
|---|---|---|
| `AppLog_State(old, new, elapsed_ms)` | 状态迁移 | `module=STATE`，`arg0/arg1` 为旧/新状态 |
| `AppLog_Action(action_id, elapsed_ms, arg)` | 动作起止与耗时 | `module=ACTION` |
| `AppLog_Event(event_id, arg0, arg1)` | 事件 | `module=EVENT` |
| `AppLog_TimedEvent(event_id, elapsed_ms, arg)` | 带耗时的事件 | `module=EVENT` |
| `AppLog_IO(in_lo, in_hi, out_lo, out_hi)` | IO 位图快照 | `module=IO`，按需开启 |

写入应当是**非阻塞**的：调用方不做 IO 等待，记录只入 RAM 环形表。

## 四、实时泵出与丢弃策略

- `AppLog_PumpRealtime(max)` 由 `SCPITask` 每 10 ms 调用一次，每轮最多 4 条，经 `AppLog_UartSink`
  格式化为 ≤128 B 行发到 USART1。
- 发送忙（`Uart1_IsTxBusy()`）时本轮跳过并累加 `realtime_drop_count`，不阻塞任务。
- 实时输出是**可选**功能，由 `AppLog_SetRealtime(bool)` 控制；生产环境默认关闭，避免与 SCPI 应答抢带宽。
- 环形表写满时覆盖最旧记录并累加 `drop_count`；`AppLog_GetStatus()` 可查询丢弃计数，
  它是判断「日志是否够用」的唯一依据。

## 五、回读

| 接口 | 说明 |
|---|---|
| `AppLog_Read(&record)` | 按 `read_index` 顺序取一条（消费式） |
| `AppLog_GetStatus(&status)` | 待读条数、容量、序号、丢弃计数、实时开关 |
| `AppLog_Format(record, buf, size)` | 单条格式化（供串口/上位机显示） |
| `AppLog_Clear()` | 清空（仅清 RAM 表，不动持久化） |

SCPI 侧的日志查询命令见 `docs/interface/SCPI_COMMANDS.md`；查询本身不改变记录内容。

## 六、诊断开关

`vector_debug_flags`（见 `docs/control/CONTROL_STATE_MACHINE_ARCHITECTURE.md` 第七节）控制
运行时打印四类信息；它与 AppLog 是两个独立通道：

| 通道 | 内容 | 用途 |
|---|---|---|
| AppLog | 结构化记录，可回读、可计数 | 现场取证与统计 |
| `vector_debug_flags` | 即时打印文本 | 专项排查（尤其 `io` 刷屏） |

同时开启两者会造成串口输出膨胀，排查时应只开其一。

## 七、未决问题

| 编号 | 问题 | 去向 |
|---|---|---|
| `LOG-Q1` | Event Log 分区未启用落盘，掉电后证据丢失。 | `docs/PROJECT_TODO.md` |
| `LOG-Q2` | `AppLog` 记录容量 76 条，一次完整关门序列可能覆盖早期记录，需要按事件优先级分级保留策略。 | `docs/PROJECT_TODO.md` |
| `LOG-Q3` | 缺少「事件编号 ↔ 告警灯闪烁模式」对照表。 | `docs/control/CONTROL_TODO.md` `CONTROL-06` |
