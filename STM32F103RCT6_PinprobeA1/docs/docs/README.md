# 文档治理域

Status: Active
Domain: DOCS
Canonical: `docs/docs/README.md`
Related: `docs/README.md`, `docs/docs/DOCS_NAMING_STRUCTURE_PLAN.md`, `docs/docs/DOCS_DOMAIN_STRUCTURE_PLAN.md`, `docs/docs/DOCS_REGISTRY.md`
Last updated: 2026-10-22

本目录是文档治理域入口，定义 `docs/` 的命名规则、目录结构、canonical 归属、迁移记录和门禁检查。
治理规则只约束「文档怎么写、放在哪、谁说了算」，不描述产品或固件行为——行为归各业务域。

## 当前主线

| 当前路径 | 定位 | 使用规则 |
|---|---|---|
| `DOCS_NAMING_STRUCTURE_PLAN.md` | 文件名格式、领域前缀、类型后缀、元数据字段、引用规则和评审规则。 | 新增或改名文档前先读本文；规则变更需同步 `Tools/docs_check.py`。 |
| `DOCS_DOMAIN_STRUCTURE_PLAN.md` | 目录结构、域归属映射、canonical 归属、新文档落点判断顺序、迁移 gate。 | 判断「这份文档该放哪个目录」时的唯一依据。 |
| `DOCS_MIGRATION_TODO.md` | `Doc/` → `docs/` 迁移的逐文件映射、命名决策和残留待办（`DOCS-MIG-*`）。 | 追溯旧路径、处理迁移残留时使用。 |
| `DOCS_REGISTRY.md` | canonical 归属表（`REG-*`）+ 代码事实源对照 + 登记规则。 | 文档互相冲突时的判定台账。 |
| `DOCS_TASK_PROGRESS.md` | 文档体系自身的实施与验证证据。 | 每次治理变更追加 `DOCS-TASK-<DATE>-<NNN>`。 |
| `DOCS_EXECUTION_CONSTRAINTS.md` | 跨会话/跨 worker 的执行约束：设计门禁、证据闭环、文档-代码一致性。 | 新任务启动前读一次，避免越界改动。 |

## 阅读顺序

1. `DOCS_NAMING_STRUCTURE_PLAN.md`：先确认能写什么类型的文档、怎么命名。
2. `DOCS_DOMAIN_STRUCTURE_PLAN.md`：确认落点目录与 canonical 归属。
3. `DOCS_REGISTRY.md`：确认自己是否在改 canonical 文档、是否与别的域重叠。
4. `DOCS_EXECUTION_CONSTRAINTS.md`：确认改动门禁与证据要求。
5. `DOCS_MIGRATION_TODO.md`：确认历史路径与残留待办（仅在追溯时）。

## 门禁命令

```powershell
# 默认检查：元数据 + 索引覆盖 + 引用有效性 + 冲突标记（命名只警告）
python Tools\docs_check.py

# 新增文件评审：命名不合规直接失败
python Tools\docs_check.py --strict-names

# 旧路径残留扫描
rg -n "Doc/" docs README.md -g "*.md"
```

## 边界

- 本文档域不定义固件行为、SCPI 语义、状态机或硬件参数，这些归 `docs/arch/`、`docs/control/`、`docs/interface/`、`docs/hardware/`。
- 文档治理不下沉到 `docs/reports/` 的排版与模板实现，模板与生成链归 `docs/tools/TOOLS_DOC_TOOLCHAIN_PLAYBOOK.md`。
- 历史资料（`docs/legacy/`）不受命名与元数据规则强制约束，保持冻结原文。
