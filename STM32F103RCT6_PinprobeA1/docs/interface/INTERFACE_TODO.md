# 接口域待办

Status: Active
Domain: INTERFACE
Canonical: `docs/interface/INTERFACE_TODO.md`
Related: `docs/PROJECT_TODO.md`, `docs/interface/SCPI_COMMANDS.md`, `docs/interface/INTERFACE_COMMUNICATION_ARCHITECTURE.md`, `docs/docs/DOCS_MIGRATION_TODO.md`
Last updated: 2026-10-22

本清单只维护接口域**未完成**事项；跨域优先级见 `docs/PROJECT_TODO.md`。

## P0 必须完成

- [ ] `INTERFACE-01` SCPI 交付版源文件回归（对应 `DOCS-MIG-01`）：为
  `docs/reports/scpi/` 的 20260924 中英文交付版建立 Markdown 源，并纳入 `Tools/md_to_gts_a4_pages.py`
  的生成流程。验收：从 md 重新生成 HTML，内容与现有 20260924 版逐节一致（允许排版差异）。
- [ ] `INTERFACE-02` 命令一致性核对：把 `SCPI_COMMANDS.md`（事实源 `scpi-def.c`）与 20260924 交付版
  逐条比对，输出「交付版多出 / 代码多出 / 参数域不一致」三张差异表。验收：差异表清零或登记为待办。

## P1 应该完成

- [ ] `INTERFACE-03` IO 链路参数去硬编码：让 `ModBusTask` 使用 `Flash_Config_t.device_id` 与
  `rs485_baudrate`，并补 `SYSTem:` 查询项。验收：改配置后重启生效，默认值行为不变。
- [ ] `INTERFACE-04` 错误码使用清单：统计 `scpi-def.c` 各命令实际返回的错误码，与交付版说明对照，
  标出「文档写了但代码不会返回」的条目。
- [ ] `INTERFACE-05` 半双工总线占用约束：在 `INTERFACE_COMMUNICATION_ARCHITECTURE.md` 补一节，
  说明新增总线主设备时必须保持串行化的理由和检查方法。

## P2 可以延后

- [ ] `INTERFACE-06` 英文交付版与中文交付版术语对照表（`LOCK/UNLOCK`、`CYLinder`、`USB` 等），
  避免中英文说明出现语义偏差。
- [ ] `INTERFACE-07` 建立 `INTERFACE_TASK_PROGRESS.md`，记录联调与协议冻结证据。
