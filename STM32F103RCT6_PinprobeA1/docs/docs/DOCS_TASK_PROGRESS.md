# 文档治理任务进度

Status: Active
Domain: DOCS
Canonical: `docs/docs/DOCS_TASK_PROGRESS.md`
Related: `docs/docs/DOCS_MIGRATION_TODO.md`, `docs/docs/DOCS_REGISTRY.md`, `docs/README.md`
Last updated: 2026-10-22

本文件记录文档体系自身的实施与验证证据。任务状态看 `docs/docs/DOCS_MIGRATION_TODO.md`，
这里只记录「做了什么、怎么验证、有什么风险」。

### DOCS-TASK-20261022-001 - docs/ 域目录化重构

- 目标：参考 `D:\Work\HDRT100\docs` 的文档治理体系，把平铺的 `Doc/` 重构为按域管理的 `docs/`，
  建立命名规则、域结构、登记表、迁移记录、检查脚本和总索引。
- 完成：
  - `Doc/` → `docs/` 目录改名，93 个文件按 15 个域落位（`Script/docs_migrate.ps1`，`git mv` 保留历史）。
  - 新增治理文档 5 份：命名规则、域结构方案、迁移待办、登记表、执行约束。
  - 新增域 README 14 份（arch/control/interface/multinode/ota/storage/hardware/tools/validation/release/reports/assets/legacy/archive）。
  - 新增 canonical 文档：`ARCH_PRODUCT_ARCHITECTURE.md`、`CONTROL_STATE_MACHINE_ARCHITECTURE.md`、
    `INTERFACE_COMMUNICATION_ARCHITECTURE.md`、`STORAGE_FLASH_ARCHITECTURE.md`、`STORAGE_LOG_ARCHITECTURE.md`、
    `TOOLS_DOC_TOOLCHAIN_PLAYBOOK.md`、`RELEASE_CHECKLIST.md`。
  - 迁移核心 Markdown 12 个为 `<DOMAIN>_<SUBJECT>_<TYPE>.md`，补齐 Status/Domain/Canonical/Related/Last updated 元数据。
  - 上游 HAOFV 原文降级为 `docs/legacy/arch/LEGACY_HAOFV_ARCHITECTURE.md`（Deprecated 快照），
    避免与本产品架构混淆。
  - 新增 `Tools/docs_check.py`：元数据、索引覆盖（就近索引）、`docs/` 引用有效性、冲突标记、命名规范五项门禁。
- 验证：
  - `python Tools\docs_check.py` 通过（详见下方「门禁结果」）。
  - `rg -n "Doc/"` 全仓扫描：仅 `Script/docs_migrate.ps1`（迁移脚本自身）和 `docs/docs/DOCS_MIGRATION_TODO.md`（映射表）保留旧路径，属预期。
  - 客户交付件文件名逐一核对未变：`docs/reports/scpi/`、`docs/reports/guide/`、`docs/reports/hardware/`、`docs/release/deliverables/`。
- 风险：
  - 当前 SCPI 交付版（20260924）无 Markdown 源，交付链断裂（`DOCS-MIG-01`）。
  - 流程图生成脚本迁移后未执行验证（`DOCS-MIG-02`）。
  - 历史 SCPI 版本与最初版指令表已冻结，但缺少「版本 → 命令差异」对照表，查历史行为仍需人工比对。
- 后续：见 `docs/docs/DOCS_MIGRATION_TODO.md` 的 P0–P2 待办。
- 涉及文件：`docs/**`、`Script/docs_migrate.ps1`、`Tools/docs_check.py`、`README.md`。

## 门禁结果

| 检查项 | 命令 | 结果 |
|---|---|---|
| 元数据完整性 | `python Tools\docs_check.py` | 通过（历史/交付件源豁免） |
| 索引覆盖 | 同上 | 通过（每份文档在与其最近的 README 中登记） |
| 引用有效性 | 同上 | 通过（`legacy/`、`reports/` 降级为 warning） |
| 命名规范 | `python Tools\docs_check.py --strict-names` | 通过（历史/交付件源豁免） |
| 旧路径残留 | `rg -n "Doc/" docs README.md` | 仅迁移记录与脚本保留 |

## 维护约定

- 每次文档体系变更（新增域、新增前缀、修改门禁规则）都追加一条 `DOCS-TASK-<DATE>-<NNN>` 记录。
- 当本文件超过约 200 KB 时，最早条目迁入 `docs/legacy/LEGACY_DOCS_TASK_PROGRESS_<NN>.md` 并在此保留索引。
