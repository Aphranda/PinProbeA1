# 架构域

Status: Active
Domain: ARCH
Canonical: `docs/arch/README.md`
Related: `docs/README.md`, `docs/docs/DOCS_DOMAIN_STRUCTURE_PLAN.md`, `docs/control/CONTROL_STATE_MACHINE_ARCHITECTURE.md`, `docs/multinode/MULTINODE_CAN_HAOFV_ARCHITECTURE.md`
Last updated: 2026-10-22

本目录是 PinProbeA1 的总体架构入口：单板固件分层、RTOS 任务与资源边界、状态事实归属、
单机基线与多机演进的关系。具体控制逻辑、命令语义、Flash 布局、硬件参数分别下沉到
`docs/control/`、`docs/interface/`、`docs/storage/`、`docs/hardware/`。

## 当前主线

| 当前路径 | 定位 | 使用规则 |
|---|---|---|
| `ARCH_PRODUCT_ARCHITECTURE.md` | 产品总体架构：MCU/RTOS 任务表、RAM/Flash 分区、状态事实流向、单机与多机边界、发布门禁。 | 架构域唯一 canonical；不写具体 IO 位表与命令字。 |

## 相关上游资料

| 当前路径 | 定位 |
|---|---|
| `../legacy/arch/LEGACY_HAOFV_ARCHITECTURE.md` | 上游 DHRT100 的 HAOFV 顶层架构原文快照（RP2350/TDMA/VDC/DPLL），**非本产品架构**，仅作多机演进的参考输入。 |
| `../multinode/MULTINODE_CAN_HAOFV_ARCHITECTURE.md` | HAOFV 思想在 PinProbeA1 上的实际落地方案（CAN 多机）。 |
| `../multinode/MULTINODE_REFLECTIVE_MEMORY_ARCHITECTURE.md` | RAM 反射内存向量表方案，对应 HAOFV 的 System Vector Blackboard。 |

## 当前阅读顺序

1. `ARCH_PRODUCT_ARCHITECTURE.md`：先确认固件分层、RTOS 任务职责和资源预算。
2. `../control/CONTROL_STATE_MACHINE_ARCHITECTURE.md`：确认状态机与动作编排如何消费命令与 IO。
3. `../interface/SCPI_COMMANDS.md`：确认对外命令面。
4. `../multinode/MULTINODE_CAN_HAOFV_ARCHITECTURE.md`：需要多机能力时，再读演进方案。

## 边界

- 架构域说明 owner、层次、资源仲裁和长期原则，不冻结 PCB pin map、器件选型与网表事实。
- 上游 HAOFV 原文中的 RP2350/TDMA/VDC/DPLL 机制不自动适用于本产品；引用时必须先确认本产品是否具备对应硬件基础。
- 任何架构级改动（新增任务、改变 RAM 分区、引入新总线主设备）必须在本文档更新后再改代码。
