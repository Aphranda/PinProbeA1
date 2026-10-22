# 多机域待办

Status: Active
Domain: MULTINODE
Canonical: `docs/multinode/MULTINODE_TODO.md`
Related: `docs/PROJECT_TODO.md`, `docs/multinode/MULTINODE_CAN_HAOFV_ARCHITECTURE.md`, `docs/multinode/MULTINODE_REFLECTIVE_MEMORY_ARCHITECTURE.md`, `docs/ota/OTA_TODO.md`
Last updated: 2026-10-22

本清单只维护多机域**未完成**事项。多机整体属于 P3/M 阶段，前置条件是单机控制闭环稳定。

## P0 必须完成（进入多机开发前的门禁）

- [ ] `MULTINODE-01` CAN 位率确认：`Core/Src/can.c` 当前参数（Prescaler 6 / BS1 12TQ / BS2 1TQ / SJW 1TQ）
  在 APB1=36 MHz 下折算约 428.6 kbit/s，与常见 500 kbit/s 目标不一致。需确认目标位率并修正参数或更新设计。
  验收：示波器/逻辑分析仪实测位宽，与设计值一致。
- [ ] `MULTINODE-02` 节点健康监控落地：`defaultTask` 当前为占位，需扫描 `node_status[1..7]` 的
  `online` 与 `heartbeat`，实现离线判定与告警（对应 `ARCH-Q2`）。
- [ ] `MULTINODE-03` 两份多机文档的字段偏移对账：`MULTINODE_CAN_HAOFV_ARCHITECTURE.md` 与
  `MULTINODE_REFLECTIVE_MEMORY_ARCHITECTURE.md` 的 `RAM_Vector_t` 字段表与 `ram_vector.h` 三方核对，
  输出差异清单并勘误。

## P1 应该完成

- [ ] `MULTINODE-04` Deployment Gate 落地：把 `MULTINODE_CAN_HAOFV_ARCHITECTURE.md` 第 10 节的检查项
  转成可执行检查（脚本或启动自检），并给出 gate 状态查询入口。
- [ ] `MULTINODE-05` 节点寻址命令：补齐节点级 SCPI 命令（当前仅 4 类节点作用域命令可用，
  见 `docs/interface/SCPI_COMMANDS.md`），含目标节点掩码与广播规则。
- [ ] `MULTINODE-06` 快照双区切换验证：W25Q128 A/B 快照的写入、切换、掉电恢复流程实测
  （对应 `docs/storage/STORAGE_FLASH_ARCHITECTURE.md`）。

## P2 可以延后

- [ ] `MULTINODE-07` CAN 总线负载评估：多机同步 + OTA 分发的帧率上限与最坏时延预算。
- [ ] `MULTINODE-08` 建立 `MULTINODE_TASK_PROGRESS.md`，记录 M1–M5 各阶段证据。
