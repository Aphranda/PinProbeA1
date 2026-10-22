# 文档命名与层级规则

Status: Active
Domain: DOCS
Canonical: `docs/docs/DOCS_NAMING_STRUCTURE_PLAN.md`
Related: `docs/README.md`, `docs/docs/DOCS_DOMAIN_STRUCTURE_PLAN.md`, `docs/docs/DOCS_MIGRATION_TODO.md`, `docs/docs/DOCS_REGISTRY.md`
Last updated: 2026-10-22

本文档定义 `docs/` 下 Markdown 文档的统一命名格式、层级关系、交叉引用规则和新增文件规则。
所有 Markdown 按 UTF-8（无 BOM）读取和写入。本规则参考 `D:\Work\HDRT100\docs` 的文档治理体系，
并按 PinProbeA1 的实际域划分裁剪。

## 基本原则

- `docs/README.md` 是文档总入口，新增文档后必须更新索引。
- 文件名使用 ASCII、全大写域名前缀、下划线分隔和明确类型后缀。
- 新文档不使用中文文件名；客户交付件（HTML/PDF/客户版 Markdown）保留原中文名，但必须由所在域 README 注明归属和当前版本。
- 一个文档只承担一种主职责：架构、设计、计划、TODO、进度、命令或发布检查。
- 设计文档不长期堆积任务日志；任务清单放入 `*_TODO.md`，验证记录放入 `*_TASK_PROGRESS.md`。
- 涉及 IO 映射、Flash 布局、SCPI 命令、协议帧、状态迁移的文档，应优先写成表格或固定字段，避免只用自然语言描述。

## 文件名格式

新文件统一使用：

```text
<DOMAIN>_<SUBJECT>_<TYPE>.md
```

示例：

```text
CONTROL_STATE_MACHINE_ARCHITECTURE.md
INTERFACE_SCPI_PROTOCOL_DESIGN.md
OTA_CAN_DISTRIBUTION_ARCHITECTURE.md
STORAGE_FLASH_ARCHITECTURE.md
HARDWARE_USB5V_PROTECTION_DESIGN.md
```

命名字段规则：

| 字段 | 规则 |
|---|---|
| `DOMAIN` | 领域前缀，全大写 ASCII，必须来自下方领域前缀表。 |
| `SUBJECT` | 主题名，全大写 ASCII 和下划线，表达具体对象。 |
| `TYPE` | 文档类型后缀，必须来自类型后缀表。 |

## 领域前缀

| 前缀 | 领域 | 对应目录 |
|---|---|---|
| `ARCH` | 顶层架构、跨域架构、产品总体架构 | `docs/arch/` |
| `CONTROL` | 状态机、动作控制、控制模式与权限、IO 链路可信状态 | `docs/control/` |
| `INTERFACE` | 通信链路、USB CDC、串口、RS485/ModBus、上位机交互 | `docs/interface/` |
| `SCPI` | SCPI 命令面专项（命令表、协议语义、交付版对照） | `docs/interface/` |
| `MULTINODE` | CAN 多机、RAM 反射内存、节点寻址与心跳 | `docs/multinode/` |
| `OTA` | OTA、Bootloader、A/B 与应用镜像、多机分发 | `docs/ota/` |
| `STORAGE` | 内部 Flash 配置、W25Q128、AppLog、快照 | `docs/storage/` |
| `HARDWARE` | IO 约束、供电/接口防护、网表、BOM、失效分析 | `docs/hardware/` |
| `TOOLS` | PC 工具（PinProbeA1 箱体控制程序）、构建与文档生成链 | `docs/tools/` |
| `VALIDATION` | 代码审查、回归验证、压力测试、证据 | `docs/validation/` |
| `RELEASE` | 分支与发布计划、发布门禁、交付包 | `docs/release/` |
| `DOCS` | 文档治理、命名规则、索引与登记表 | `docs/docs/` |
| `PROJECT` | 跨域项目板、优先级与验收基线 | `docs/` |
| `LEGACY` | 历史资料、外部迁入原文、冻结参考 | `docs/legacy/` |

新增领域前缀前，必须先确认能否归入已有领域；确认后同步更新本表、
`docs/docs/DOCS_DOMAIN_STRUCTURE_PLAN.md` 和 `Tools/docs_check.py` 的 `ALLOWED_PREFIXES`。

## 类型后缀

| 后缀 | 用途 |
|---|---|
| `_ARCHITECTURE.md` | 顶层或跨模块架构，说明边界、依赖方向和长期原则。 |
| `_DESIGN.md` | 具体功能、硬件电路、协议帧、状态机或数据结构设计。 |
| `_PLAN.md` | 阶段性实施计划、迁移计划、重构计划。 |
| `_TODO.md` | 可执行任务清单，按 P0/P1/P2 或阶段拆分。 |
| `_TASK_PROGRESS.md` | 任务进度、闭环验证、风险和决策记录。 |
| `_CHECKLIST.md` | 发布、评审、硬件冻结或验证门禁检查表。 |
| `_PLAYBOOK.md` | 可重复执行的操作手册（构建、烧录、文档生成、bring-up）。 |
| `_COMPARISON.md` | 方案、器件或开源项目对比。 |
| `_EVALUATION.md` | 可行性、可移植性、性能或风险评估。 |
| `_COMMANDS.md` | 命令、协议入口或用户可调用接口列表。 |
| `_ANALYSIS.md` | 波形、时序、性能、数据或问题分析。 |
| `_CONSTRAINTS.md` | 硬件、资源、接口或系统边界约束，说明不可绕过的限制和派生规则。 |
| `_REGISTRY.md` | 登记表，作为某类事实的唯一台账。 |
| `_REVIEW.md` | 评审快照（代码审查、方案评审、交叉核验），不作为契约。 |

不得随意发明近义后缀（`_NOTE.md`、`_DOC.md`、`_SPEC.md`）。确实需要新增后缀时，先更新本文档与检查脚本。

## 层级关系

逻辑层级如下：

```text
00 文档治理       docs/docs/
01 系统架构       docs/arch/
02 控制与状态机   docs/control/
03 接口与 SCPI    docs/interface/
04 多机分布式     docs/multinode/
05 存储与日志     docs/storage/
06 OTA 与启动     docs/ota/
07 硬件与失效分析 docs/hardware/
08 工具与文档链   docs/tools/
09 验证与证据     docs/validation/
10 发布与交付     docs/release/
11 报告输出       docs/reports/
12 静态资源       docs/assets/
13 历史资料       docs/legacy/
14 归档           docs/archive/
```

目录只表达文档归属，不替代文件名前缀。任何跨目录移动都必须更新全部引用。

## 文档元数据

除历史资料（`docs/legacy/`）、归档（`docs/archive/`）和交付件源（`docs/reports/` 下非 README 文件）
外，新文档标题下方必须包含固定元数据块：

```text
Status: Draft | Active | Frozen | Deprecated
Domain: <DOMAIN>
Canonical: `docs/<PATH>.md`
Related: `docs/<RELATED>.md`
Last updated: YYYY-MM-DD
```

| 字段 | 含义 |
|---|---|
| `Status` | `Draft` 草案，`Active` 当前有效，`Frozen` 已冻结只允许勘误，`Deprecated` 已废弃但仍保留。 |
| `Domain` | 所属领域，必须能映射到领域前缀。 |
| `Canonical` | 该文档自身的仓库路径，格式 `docs/<PATH>.md`。 |
| `Related` | 直接相关文档，多个文件用逗号分隔。 |
| `Last updated` | 最后一次有意义内容更新的日期（`YYYY-MM-DD`）。 |

历史资料和交付件源豁免元数据门禁的理由：交付件源会被 A4/PDF 生成链直接渲染，
插入元数据块会污染版面；历史资料保持冻结原文以便追溯。

## 新增文件规则

新增文档前按以下顺序判断：

1. 能否补充到现有 canonical 文档中。
2. 是否需要独立生命周期（独立 TODO、独立进度、独立硬件约束）。
3. 是否已有同域同类型文件，避免重复创建近义文档。
4. 文件名是否符合 `<DOMAIN>_<SUBJECT>_<TYPE>.md`。
5. 是否已加入所属域 `README.md` 与 `docs/README.md` 索引。
6. 是否在相关主文档中被引用（`Related` 或正文链接）。

推荐拆分方式：

| 场景 | 建议落点 |
|---|---|
| 长期原则、依赖边界、架构分层 | `_ARCHITECTURE.md` |
| 固件功能、硬件电路、协议帧、状态机 | `_DESIGN.md` |
| 还没开始实现，需分阶段推进 | `_PLAN.md` |
| 已决定要做，需 P0/P1/P2 拆任务 | `_TODO.md` |
| 已开始闭环，需要记录过程和结论 | `_TASK_PROGRESS.md` |
| 命令、接口、用户可调用入口 | `_COMMANDS.md` |
| 发布前必须逐项确认 | `_CHECKLIST.md` |
| 可重复执行的操作步骤 | `_PLAYBOOK.md` |

## TODO 与进度规则

`*_TODO.md` 用于还没完成的工作：

```text
## P0 必须完成
- [ ] P0-01 任务标题：验收标准。

## P1 应该完成
- [ ] P1-01 任务标题：验收标准。

## P2 可以延后
- [ ] P2-01 任务标题：验收标准。
```

`*_TASK_PROGRESS.md` 用于记录已经发生的工作：

```text
### <DOMAIN>-TASK-YYYYMMDD-NNN - 任务标题

- 目标：
- 完成：
- 验证：
- 风险：
- 后续：
- 涉及文件：
```

进度路由：

| 领域 | 进度入口 |
|---|---|
| 文档治理 | `docs/docs/DOCS_TASK_PROGRESS.md` |
| 验证与证据 | `docs/validation/VALIDATION_TASK_PROGRESS.md` |
| 跨域总体 | `docs/PROJECT_TODO.md`（状态板）+ 对应域 `*_TASK_PROGRESS.md`（证据） |
| 其他领域 | 新建或补齐 `docs/<DOMAIN>/<DOMAIN>_TASK_PROGRESS.md` |

设计文档中可以保留简短的“未决问题”，但不得把任务过程堆进设计正文。

## 交叉引用规则

- 引用其他文档时，使用相对仓库根的全路径写法 `docs/<PATH>.md`，便于全文检索和迁移核对。
- 同目录内的正文链接可以使用相对路径（`OTHER_DOC.md`）。
- 元数据 `Canonical` / `Related`、任务记录中的“涉及文件”、迁移记录表，一律使用 `docs/<PATH>.md`。
- 文件改名或移动时，必须用全文搜索确认引用已经同步更新。
- 禁止只写“见前文/见上文”而不提供文件名。
- `docs/legacy/` 内的历史原文允许保留失效引用（其引用的是上游工程路径），检查脚本对其降级为 warning。

引用与门禁命令：

```powershell
# 全文搜索旧路径残留
rg -n "Doc/|docs/" docs -g "*.md"

# 文档门禁检查
python Tools\docs_check.py
python Tools\docs_check.py --strict-names
```

默认检查覆盖：元数据完整性、索引覆盖（就近索引）、Markdown `docs/` 引用有效性、冲突标记、
命名规范（历史/交付件降级为 warning）。新增文件评审必须使用 `--strict-names`。

## 评审规则

新增或重构文档时，至少检查：

- 文件名符合规则，前缀/后缀在允许列表内。
- 顶部元数据齐全，`Canonical` 与实际路径一致。
- 已加入所属域 README 与总 README。
- 架构、TODO、进度文档互相可追踪。
- IO 表、SCPI 命令、Flash 布局与代码实现没有语义冲突。
- 文档采用 UTF-8 无 BOM，中文内容没有乱码。
