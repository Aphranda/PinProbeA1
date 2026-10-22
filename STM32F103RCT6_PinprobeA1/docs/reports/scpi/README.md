# SCPI 交付件

Status: Active
Domain: REPORTS
Canonical: `docs/reports/scpi/README.md`
Related: `docs/reports/README.md`, `docs/interface/SCPI_COMMANDS.md`, `docs/interface/INTERFACE_TODO.md`
Last updated: 2026-10-22

## 当前交付版

| 文件 | 语言 | 说明 |
|---|---|---|
| `PinProbe A1 箱体控制 SCPI 指令说明20260924.html` | 中文 | 客户联调用 SCPI 指令说明（A4 竖版，含封面与目录） |
| `PinProbe A1 箱体控制 SCPI 指令说明20260924.pdf` | 中文 | 同上打印版 |
| `PinProbe A1 Box Control SCPI Command Reference 20260924.html` | 英文 | 英文客户版 |
| `PinProbe A1 Box Control SCPI Command Reference 20260924.pdf` | 英文 | 同上打印版 |

## 源文件状态

**当前交付版没有对应的 Markdown 源文件。** 20260924 版是在更早的 md 基础上扩充后直接维护的，
因此无法通过 `docs/tools/TOOLS_DOC_TOOLCHAIN_PLAYBOOK.md` 的链路重建。

| 项 | 状态 |
|---|---|
| 最近带 md 源的版本 | 英文 `20260912`（已移入 `docs/legacy/scpi/`）、中文 `20260728`（已移入 `docs/legacy/scpi/`） |
| 20260924 与 md 源的命令覆盖差异 | 20260924 明显更全（中文版 `CONFigure:` 出现 94 次 vs md 源 69 次），不能直接回退使用旧 md |
| 处理方式 | 重建 md 源并纳入生成链，见 `docs/interface/INTERFACE_TODO.md` `INTERFACE-01` |

## 使用规则

- 交付前置检查：封面日期、命令表与 `docs/interface/SCPI_COMMANDS.md` 一致性、PDF 分页。
- 历史版本一律从 `docs/legacy/scpi/` 取，用于追溯「某一版当时是怎么写的」，不得作为当前行为依据。
- 本目录 Markdown（本 README 除外）豁免文档治理元数据门禁。
