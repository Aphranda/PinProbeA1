# PinProbeA1 产品架构

Status: Active
Domain: ARCH
Canonical: `docs/arch/ARCH_PRODUCT_ARCHITECTURE.md`
Related: `docs/control/CONTROL_STATE_MACHINE_ARCHITECTURE.md`, `docs/interface/SCPI_COMMANDS.md`, `docs/interface/INTERFACE_COMMUNICATION_ARCHITECTURE.md`, `docs/multinode/MULTINODE_REFLECTIVE_MEMORY_ARCHITECTURE.md`, `docs/storage/STORAGE_FLASH_ARCHITECTURE.md`, `docs/ota/OTA_CAN_DISTRIBUTION_ARCHITECTURE.md`
Last updated: 2026-10-22

本文档是 PinProbeA1 箱体控制固件的总体架构 canonical。它定义固件分层、RTOS 任务与优先级、
RAM/Flash/外设资源归属、状态事实流向，以及单机基线与多机演进的边界。
具体数值一律以代码为准，镜像关系见 `docs/docs/DOCS_REGISTRY.md` 第二节。

## 一、产品定位

| 项目 | 内容 |
|---|---|
| 产品 | PinProbe A1 箱体控制设备（单箱体，门/USB 动作、锁定、指示灯、安全联锁） |
| 主控 | STM32F103RCT6，Cortex-M3 @ 72 MHz，256 KB Flash，48 KB SRAM |
| RTOS | FreeRTOS + CMSIS-RTOS v2（`Core/Src/freertos.c`） |
| 对外接口 | USB CDC 虚拟串口上的 SCPI（USART1），RS485/ModBus RTU 开关量模块（USART3） |
| 上位机 | `Tools/pinprobe_gui.py`（打包为 `PinProbeA1箱体控制程序.exe`） |
| 升级 | USB 单机 OTA + Bootloader；CAN 多机分发见 `docs/ota/` |
| 演进 | 单机稳定基线 → CAN 多机（节点寻址、反射内存快照） |

## 二、固件分层

| 层 | 目录 | 职责 | 约束 |
|---|---|---|---|
| 应用编排层 | `App/Src/app_tasks.c` | 四个任务体、节拍分发、RS485 链路可信状态判定 | 不直接操作继电器，动作统一经命令槽 |
| 状态事实层 | `Hardware/RamVector/` | `RAM_Vector_t` 单一事实表：命令槽、系统状态、节点状态、IO 镜像 | 1024 B 定长、布局冻结、`_Static_assert` 校验 |
| 动作编排层 | `Hardware/RamVector/Src/state_vector.c`、`cmd_exec.c` | 主状态机、动作序列、IO 回读纠偏 | 只有本层可决定动作顺序 |
| 命令解析层 | `Hardware/libscpi/` | SCPI 词法/语法/错误码、命令树 `scpi-def.c` | 只产生意图，不直接驱动 IO |
| 外设驱动层 | `Hardware/BsmRelay`、`Hardware/modbus` | ModBus RTU 帧收发、IO 读写、CRC16 | 单总线串行访问，超时 100 ms |
| 持久化层 | `Hardware/Flash`、`Hardware/W25Q128`、`Hardware/AppLog` | 配置页、外部 Flash 分区、诊断日志环形表 | 唯一写者原则，见 `docs/storage/` |
| 升级层 | `Bootloader/`、`Hardware/OTA/` | Boot 选择、A/B 槽位、Manifest 校验、多机分发 | Boot 不依赖 RTOS |
| 板级初始化 | `Core/` | 时钟、GPIO、USART/DMA、CAN、TIM、IWDG、HAL 配置 | CubeMX 生成区不得手工破坏 |

依赖方向：`App` → `RamVector(state_vector/cmd_exec)` → `libscpi` / `BsmRelay` / `Flash` / `OTA` → `Core`。
反向依赖禁止：驱动层不得回调状态机，`libscpi` 不得直接写 GPIO。

## 三、RTOS 任务与时间基准

| 任务 | 入口 | 栈（字） | 优先级 | 周期/触发 | 职责 |
|---|---|---:|---|---|---|
| `defaultTask` | `StartDefaultTask` | 128×4 | Normal | 1 s | 集群健康监控占位（多机时扫描 `node_status[1..7]`） |
| `SCPI` | `SCPITask` | 1024×4 | Low | 10 ms 轮询 | 取出 USART1 接收块 → `SCPI_Input`，并泵出实时日志 |
| `ModBus` | `ModBusTask` | 512×4 | Normal1 | SysTimer/2 ≈ 50 ms | 读输入/输出镜像、更新链路可信状态、执行命令槽 |
| `WatchDog` | `WatchDogTask` | 128×4 | Low | 500 ms | 刷新 IWDG（溢出约 4 s） |
| `StateVector` | `StateVectorTask` | 512×4 | Normal | SysTimer ≈ 25 ms | `StateVector_Input()`：事件采集与状态迁移 |
| `SysTimer`（软件定时器） | `SysTimerCallback` | — | — | 25 ms 周期 | 置位 `ModBus`/`StateVector` 线程标志；ModBus 二分频 |

关键口径：

- 状态机调用周期约 25 ms（SysTimer 节拍），ModBus 轮询约 50 ms（二分频）；`docs/control/CONTROL_TIMING_ANALYSIS.md` 中的历史描述与本表冲突时以本文档和代码为准。
- 任务间不共享可变全局变量，跨任务事实一律经 `RAM_Vector_t` 与线程标志传递。
- `SCPITask` 用临界区摘取接收缓冲，长度 > 3 才解析，避免半帧误触发。

## 四、RAM 与 Flash 布局

链接脚本 `MDK-ARM/STM32F103RCT6_PinprobeA1.sct`：

| 区域 | 基址 | 大小 | 用途 |
|---|---:|---:|---|
| `ER_IROM1` | `0x08000000` | `0x0003F800`（254 KB） | 应用代码 + 只读数据（末 2 KB 留给配置页） |
| `RW_IRAM1` | `0x20000000` | `0x0000A000`（40 KB） | 常规 RW/ZI、FreeRTOS 堆与任务栈 |
| `RW_RAMVEC` | `0x2000A000` | `0x00000400`（1 KB） | `RAM_Vector_t` 单实例 |
| `RW_APPLOG` | `0x2000A400` | `0x00000500`（1.25 KB） | `AppLog_Table_t` 环形日志 |
| `RW_IRAM2` | `0x2000A900` | `0x00001700`（5.75 KB） | 预留余量 |

OTA 阶段分区（`Hardware/OTA/Inc/ota_manifest.h`）：

| 区域 | 地址 | 大小 | 说明 |
|---|---:|---:|---|
| Bootloader | `0x08000000` | ≤ `0x6000`（24 KB） | 无 RTOS，含 Boot 标志与回滚 |
| APP | `0x08006000` | ≤ `0x39800`（230 KB） | 应用镜像 |
| 配置页 | `0x0803F800` | `0x800`（2 KB） | `Flash_Config_t`，`magic=0xA5A5A5A5` |

外部 Flash（W25Q128，16 MB）分区的 canonical 说明见 `docs/storage/STORAGE_FLASH_ARCHITECTURE.md`。

## 五、外设与总线归属

| 外设 | 用途 | 关键参数 | 事实源 |
|---|---|---|---|
| `USART1` + DMA1 | USB CDC 虚拟串口：SCPI 输入、日志回读 | 115200 8N1 | `Core/Src/usart.c` |
| `USART3` + DMA1 | RS485/ModBus RTU 开关量模块 | 115200 8N1，发送超时 100 ms | `Core/Src/usart.c`、`Hardware/modbus/` |
| `CAN1` | 多机分布式总线（当前未接入业务） | Prescaler 6 / BS1 12TQ / BS2 1TQ / SJW 1TQ，AutoBusOff，自动唤醒、自动重传 | `Core/Src/can.c` |
| `SPI1` | W25Q128 外部 Flash | 见 `Core/Src/spi.c` | `Core/Src/spi.c` |
| `TIM1`（HAL 时基） | 1 ms 时基 | `Core/Src/stm32f1xx_hal_timebase_tim.c` | 同上 |
| `IWDG` | 看门狗 | 溢出约 4 s，500 ms 喂狗 | `Core/Src/iwdg.c` |

RS485 总线上只有一个外部从站（开关量 IO 模块，从机地址 `0x01`），
所有访问串行化在 `ModBusTask` 内，不允许多任务并发发帧。

## 六、状态事实流向

```text
按钮/传感器 ─┐
RS485 输入镜像 ├─→ StateVectorTask（事件采集 + 状态迁移，25 ms）
SCPI 命令 ────┘                     │
                                    ├─→ RAM_Vector.cmd_*（命令槽，带优先级）
                                    ├─→ RAM_Vector.global_state / error_code
                                    └─→ AppLog（事件/动作证据）
                                            │
                                   ModBusTask（≈50 ms）
                                            ├─→ cmd_exec：按固定顺序下发继电器
                                            ├─→ 回读 IO 镜像（raw_in/raw_out）
                                            └─→ 状态纠偏、链路可信状态更新
```

- `RAM_Vector_t` 是唯一共享事实表；命令槽三分（lock / cylinder / led）+ 优先级
  （`CMD_PRIO_OBSERVE` < `CMD_PRIO_USER` < `CMD_PRIO_SAFETY`）实现仲裁。
- 安全例外（急停、激光防护、RS485 故障保护、断电回锁）不经过控制模式门禁，任何模式下始终生效。
- 同一事实不得在多个模块各存一份副本；发现副本即为缺陷，登记到 `docs/PROJECT_TODO.md`。

## 七、单机基线与多机演进

| 维度 | 单机基线（当前主线） | 多机演进（规划） |
|---|---|---|
| 节点 | 单节点，`node_status[0]` 与 `io_state[0]` 有效 | 最多 8 节点（`RAM_VECTOR_MAX_NODES`） |
| 通信 | USB CDC（SCPI）+ RS485（IO 模块） | 增加 CAN 节点寻址、心跳、命令分发 |
| 快照 | 仅 RAM 内向量表 | W25Q128 A/B 快照 + 反射内存同步 |
| 版本 | `v0.9.0-standalone` 系列 | `v0.x-multinode` 系列，分支规则见 `docs/release/RELEASE_BRANCH_PLAN.md` |
| 文档 | 本目录 + `control/` + `interface/` | 追加 `docs/multinode/` 与 `docs/ota/` 的多机章节 |

多机方案的前提：单机控制闭环稳定、`state_vector.c` 动作序列冻结、OTA 单机路径可回滚。
未满足前提时不得在 `main` 上并行推进多机改动。

## 八、发布门禁

- 变更涉及安全链路（急停、激光、RS485 故障保护、断电回锁）时，必须通过 `docs/release/RELEASE_CHECKLIST.md` 的安全项。
- ROM/RAM 占用变化必须报告增量；`RW_IRAM1` 余量低于 2 KB 时禁止新增常驻缓冲。
- 架构级改动（新增任务、改 RAM 分区、引入新总线主设备）需先更新本文档，再改代码。

## 九、未决问题

| 编号 | 问题 | 影响 | 去向 |
|---|---|---|---|
| `ARCH-Q1` | `CAN1` 位时序参数换算为约 428.6 kbit/s（APB1=36 MHz），与常见的 500 kbit/s 目标不一致，需实机确认目标位率。 | 多机互通 | `docs/multinode/MULTINODE_TODO.md` |
| `ARCH-Q2` | `defaultTask` 目前是健康监控占位，未实现节点离线判定。 | 多机可用性 | `docs/multinode/MULTINODE_TODO.md` |
| `ARCH-Q3` | `RW_IRAM2` 5.75 KB 预留区未定义归属，多机缓冲与 OTA 下载缓冲的分配策略未定。 | 多机/OTA | `docs/PROJECT_TODO.md` |
