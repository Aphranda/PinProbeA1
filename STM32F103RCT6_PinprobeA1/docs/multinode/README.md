# 多机分布式域

Status: Active
Domain: MULTINODE
Canonical: `docs/multinode/README.md`
Related: `docs/README.md`, `docs/arch/ARCH_PRODUCT_ARCHITECTURE.md`, `docs/ota/OTA_CAN_DISTRIBUTION_ARCHITECTURE.md`
Last updated: 2026-10-22

本目录是 CAN 多机能力的唯一入口：节点模型与寻址、RAM 反射内存向量表的多机语义、
命令槽分发与 ACK/NACK、心跳与 stale 判定、集群状态机。所有内容都属于**演进方向**，
单机基线的行为以 `docs/arch/ARCH_PRODUCT_ARCHITECTURE.md` 与 `docs/control/` 为准。

## 当前主线

| 当前路径 | 定位 | 使用规则 |
|---|---|---|
| `MULTINODE_CAN_HAOFV_ARCHITECTURE.md` | 基于 HAOFV 规则的 CAN 多机分布式架构：分层、节点模型、Vector 字段 owner、CAN Vector Sync 协议、Deployment Gate、M1–M5 路线。 | 多机架构 canonical（HAOFV 化视角）。 |
| `MULTINODE_REFLECTIVE_MEMORY_ARCHITECTURE.md` | 多机协同设计方案：RAM 反射内存 1024 B 布局、W25Q128 快照、CAN 帧格式、ModBus 从站寄存器映射、任务与内存预算。 | 多机数据结构 canonical（布局视角）。 |
| `MULTINODE_TODO.md` | 多机域待办与门禁（位率确认、健康监控、Deployment Gate）。 | 只维护未完成事项。 |

## 两份多机文档的关系

| 维度 | `MULTINODE_CAN_HAOFV_ARCHITECTURE.md` | `MULTINODE_REFLECTIVE_MEMORY_ARCHITECTURE.md` |
|---|---|---|
| 关注点 | owner 边界、协议语义、门禁与路线 | 内存布局、快照、帧格式、寄存器映射 |
| 冲突时 | 结构/语义以 HAOFV 为主 | 字段偏移与容量以反射内存方案为主 |
| 共同事实 | `RAM_Vector_t` 布局与 `Hardware/RamVector/Inc/ram_vector.h` 一致 | 同上 |

两份文档对同一字段给出不同偏移或容量时，以 `ram_vector.h` 的 `_Static_assert` 结果为准，
并登记漂移项到 `docs/PROJECT_TODO.md`。

## 相关文档

| 当前路径 | 定位 |
|---|---|
| `../arch/ARCH_PRODUCT_ARCHITECTURE.md` | 单机基线与多机演进边界、RAM/Flash 预算。 |
| `../ota/OTA_CAN_DISTRIBUTION_ARCHITECTURE.md` | CAN 多机 OTA 分发（与多机命令通道共用总线）。 |
| `../storage/STORAGE_FLASH_ARCHITECTURE.md` | W25Q128 分区（快照区归属的定义方）。 |
| `../legacy/arch/LEGACY_HAOFV_ARCHITECTURE.md` | 上游 HAOFV 原文快照，多机规则的来源参考（非本产品契约）。 |

## 边界

- 多机域不修改单机行为；单机路径必须能在多机代码存在时保持既有语义。
- CAN 总线同时承载多机同步与 OTA 分发，任何新增功能码必须先更新协议表再实现。
- 多机方案不得削弱安全例外：急停与故障广播优先级高于任何同步帧。
