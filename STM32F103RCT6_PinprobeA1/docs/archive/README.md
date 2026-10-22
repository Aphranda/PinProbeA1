# 归档

Status: Active
Domain: ARCHIVE
Canonical: `docs/archive/README.md`
Related: `docs/README.md`, `docs/docs/DOCS_MIGRATION_TODO.md`
Last updated: 2026-10-22

本目录存放不构成任何文档主线的一次性产出、废弃素材与待判定资产。与 `docs/legacy/` 的区别：
`legacy/` 是被替代但仍可追溯的历史文档，`archive/` 是「留下来但暂时说不清用途」的东西。

## 当前内容

| 路径 | 说明 | 处置建议 |
|---|---|---|
| `设备渲染示意图.png` | 设备渲染示意图（3.11 MB / 3263282 B，文件时间 2026-07-28）。迁移前的文件名为 `untitled.642.png`，2026-10-22 迁移后由使用者确认用途并改名。 | 若用于交付件封面或方案报告，转入 `docs/assets/`（通用资源）或 `docs/reports/guide/assets/`（操作说明）；确认后删除本表条目。 |

## 迁移记录

`Doc/` → `docs/` 迁移的完整逐文件映射、命名决策与残留待办不在本目录，而在
`docs/docs/DOCS_MIGRATION_TODO.md`（仍在执行的迁移台账应放在治理域，而不是归档域）。
迁移脚本本身保留在 `Script/docs_migrate.ps1`，作为可复现证据。

## 使用规则

- 本目录不新增文档；只收留无法归入任何业务域且不应删除的资产。
- 每项资产必须在本表登记来源与处置建议，长期未处置项应在季度清理时删除。
- 本目录豁免命名与元数据门禁（本 README 除外）。
