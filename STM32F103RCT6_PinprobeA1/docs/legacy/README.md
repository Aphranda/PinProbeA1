# 历史资料

Status: Active
Domain: LEGACY
Canonical: `docs/legacy/README.md`
Related: `docs/README.md`, `docs/docs/DOCS_MIGRATION_TODO.md`, `docs/docs/DOCS_DOMAIN_STRUCTURE_PLAN.md`
Last updated: 2026-10-22

本目录存放冻结的历史资料：历史版本 SCPI 指令表、最初版指令说明、串口日志、日常总结，
以及从上游工程迁入的架构原文。这些文件**不代表当前行为**，不得据其修订现行设计。

## 子目录

| 目录 | 内容 | 数量 | 说明 |
|---|---|---:|---|
| `scpi/` | 历史 SCPI 指令说明与 Reference（中文/英文，带日期版本） | 21 | 0630 → 20260912 各版，含「固件实现版」「mode_error201」「mode_impl」变体。当前交付版见 `docs/reports/scpi/`。 |
| `control/` | 最初版下位机指令说明与链路图 | 2 | `LEGACY_PINPROBEA1_COMMAND_LIST.md`（原 `PinprobeA1.md`）+ `PinProbeLink.png`。 |
| `arch/` | 上游 DHRT100 的 HAOFV 架构原文快照 | 1 | `LEGACY_HAOFV_ARCHITECTURE.md`：RP2350/TDMA/VDC/DPLL 体系，**非本产品架构**；本产品架构见 `docs/arch/ARCH_PRODUCT_ARCHITECTURE.md`。 |
| `logs/` | 串口/上位机导出日志 | 2 | `SaveWindows2026_6_16_23-33-06.TXT`、`SaveWindows2026_6_16_23-38-58.TXT`。 |
| （根） | 日常工作总结 | 1 | `LEGACY_TASK_PROGRESS_20260611.md`：2026-06-11 架构重构记录（RS485 迁入 `Hardware/ModBus` 等）。 |

## SCPI 历史版本清单

| 文件 | 说明 |
|---|---|
| `scpi/PinProbe A1 箱体控制 SCPI 指令说明0630.pdf` | 2026-06-30 首发版 |
| `scpi/PinProbe A1 箱体控制 SCPI 指令说明0727.html/.md/.pdf` | 2026-07-27 三件套 |
| `scpi/PinProbe A1 箱体控制 SCPI 指令说明20260728.md/.pdf` | 标注日期 2026-08-11 的中文版 |
| `scpi/PinProbe A1 箱体控制 SCPI 指令说明20260811.html/.pdf` | 2026-08-11 版 |
| `scpi/PinProbe A1 箱体控制 SCPI 指令说明20260911.html/.pdf` | 2026-09-11 版（与 20260811 内容一致） |
| `scpi/PinProbe A1 箱体控制 SCPI 指令说明20260912.html/.pdf` | 2026-09-12 版 |
| `scpi/PinProbe A1 箱体控制 SCPI 指令说明20260912_固件实现版.pdf` | 固件实现口径变体 |
| `scpi/PinProbe A1 Box Control SCPI Command Reference 20260728.html` | 英文 2026-07-28 版 |
| `scpi/PinProbe A1 Box Control SCPI Command Reference 20260912.html/.md/.pdf` | 英文 2026-09-12 版（最后一版带 md 源） |
| `scpi/PinProbe A1 Box Control SCPI Command Reference.md/.pdf` | 英文早期无日期版（2026-08-11 内容） |
| `scpi/PinProbeA1_SCPI_20260912_mode_error201.pdf` | 控制模式错误码专题变体 |
| `scpi/PinProbeA1_SCPI_20260912_mode_impl.pdf` | 控制模式实现专题变体 |

## 使用规则

- **只读**：不得修改内容。发现需要修订的内容，应在对应业务域新建 canonical 文档并引用历史文件。
- 文件命名保留原样（含中文与日期），本目录豁免命名与元数据门禁；引用失效属预期（多为上游路径）。
- 需要「某版本当时的命令定义」时，从这里取；需要「当前命令定义」时，读 `docs/interface/SCPI_COMMANDS.md`。
- 新增历史资料时，必须在本文档相应小节的清单中登记，写明来源与冻结原因。
