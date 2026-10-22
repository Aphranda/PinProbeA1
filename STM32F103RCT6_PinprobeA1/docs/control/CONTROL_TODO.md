# 控制域待办

Status: Active
Domain: CONTROL
Canonical: `docs/control/CONTROL_TODO.md`
Related: `docs/PROJECT_TODO.md`, `docs/control/CONTROL_STATE_MACHINE_ARCHITECTURE.md`, `docs/control/CONTROL_TIMING_ANALYSIS.md`, `docs/interface/INTERFACE_TODO.md`
Last updated: 2026-10-22

本清单只维护控制域**未完成**事项。跨域优先级与门禁状态以 `docs/PROJECT_TODO.md` 为准，
两边编号一一对应，不在此重复状态历史；证据写入 `docs/validation/VALIDATION_TASK_PROGRESS.md`
（后续可拆出 `CONTROL_TASK_PROGRESS.md`）。

## P0 必须完成

- [ ] `CONTROL-01` 控制模式权限分级的实机联调：在 `LOCAL` / `REMOTE` / `MIXED` 三模式下逐项验证
  物理按键与 SCPI 的允许/拒绝矩阵，记录拒绝错误码。对应项目板 `P0.4`、`P0.5`。
  验收：三模式 × 六类入口的完整矩阵表 + 串口日志证据。
- [ ] `CONTROL-02` 锁定/解锁与断电回锁回归：验证 `LOCK_IDLE_MS` 冷却、断电后输出复位、
  重新上电必须再次解锁。验收：断电-上电 5 次循环无异常。
- [ ] `CONTROL-03` USB 自动拔插异常纠偏（O1/O2）：门开到位后按 `HARDWARE_USB_REPLUG_A0_ANALYSIS.md`
  的顺序实施「先加诊断日志 → 再改纠偏条件」。验收：A/B 两组复现测试均通过。

## P1 应该完成

- [ ] `CONTROL-04` 动作耗时实测表：门开/关、USB 插/拔各 10 次的耗时分布（P50/P95/max），
  与 `USB_MOVE_TIMEOUT_MS` 等超时常量对照，确认余量。
- [ ] `CONTROL-05` 状态漂移自查：把 `system_status` 推导规则与 `CONTROL_STATE_MACHINE_ARCHITECTURE.md`
  第一节逐条对照，发现不一致时以代码为准并勘误文档。
- [ ] `CONTROL-06` 告警灯语义固化：红灯/黄灯闪烁模式与 `AppLog` 事件编号建立对照表（见 `docs/storage/STORAGE_LOG_ARCHITECTURE.md`）。

## P2 可以延后

- [ ] `CONTROL-07` 把 `state_vector.c` 的常量块（`IN_*` / `OUT_*` / 超时宏）抽成与 IO 表同源的头部，
  消除「代码宏」与 `docs/hardware/HARDWARE_IO_CONSTRAINTS.md` 两份定义之间的漂移风险。
- [ ] `CONTROL-08` 建立 `CONTROL_TASK_PROGRESS.md`，把控制域的实机验证证据从
  `docs/validation/VALIDATION_TASK_PROGRESS.md` 迁入。
