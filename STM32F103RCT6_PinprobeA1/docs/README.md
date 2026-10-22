# PinProbeA1 文档索引

Status: Active
Domain: DOCS
Canonical: `docs/README.md`
Related: `docs/docs/DOCS_NAMING_STRUCTURE_PLAN.md`, `docs/docs/DOCS_DOMAIN_STRUCTURE_PLAN.md`, `docs/docs/DOCS_REGISTRY.md`
Last updated: 2026-10-22

本文档是 `docs/` 的唯一总入口。文档命名、层级、元数据和落点规则以
`docs/docs/DOCS_NAMING_STRUCTURE_PLAN.md` 与 `docs/docs/DOCS_DOMAIN_STRUCTURE_PLAN.md` 为准。
2026-10-22 完成了 `Doc/` 平铺目录到 `docs/` 域目录的迁移，迁移台账见
`docs/docs/DOCS_MIGRATION_TODO.md`。

## 域目录入口

| 目录 | 入口 | 作用 |
|---|---|---|
| `docs/` | `docs/README.md` | 本索引 + 跨域项目板 `PROJECT_TODO.md` |
| `docs/docs/` | `docs/docs/README.md` | 文档治理：命名规则、域结构、迁移台账、登记表、治理进度、执行约束 |
| `arch/` | `arch/README.md` | 产品总体架构：固件分层、RTOS 任务、RAM/Flash 预算、单机与多机边界 |
| `control/` | `control/README.md` | 状态机、动作编排、控制模式权限、时序预算 |
| `interface/` | `interface/README.md` | SCPI 命令表、USB CDC、RS485/ModBus 链路 |
| `multinode/` | `multinode/README.md` | CAN 多机、RAM 反射内存、节点与心跳 |
| `ota/` | `ota/README.md` | OTA、Bootloader、A/B 槽位、多机分发 |
| `storage/` | `storage/README.md` | 内部 Flash 配置、W25Q128 分区、AppLog |
| `hardware/` | `hardware/README.md` | IO 位映射、供电与 USB 防护、失效分析、网表与手册 |
| `tools/` | `tools/README.md` | 上位机工具、构建/烧录脚本、文档生成链 |
| `validation/` | `validation/README.md` | 代码审查、回归验证、证据分级 |
| `release/` | `release/README.md` | 分支与版本、发布门禁、客户交付包 |
| `reports/` | `reports/README.md` | 当前交付件（HTML/PDF）与其源 Markdown |
| `assets/` | `assets/README.md` | 模板与 Logo 等共享静态资源 |
| `legacy/` | `legacy/README.md` | 历史 SCPI 版本、最初版说明、串口日志、上游原文 |
| `archive/` | `archive/README.md` | 归档素材与待判定资产 |

## Canonical 主文档

| 领域 | canonical 主文档 | 说明 |
|---|---|---|
| ARCH/PRODUCT | `arch/ARCH_PRODUCT_ARCHITECTURE.md` | 固件分层、RTOS 任务表、RAM/Flash 分区、外设归属、状态事实流向、发布门禁。 |
| CONTROL/SM | `control/CONTROL_STATE_MACHINE_ARCHITECTURE.md` | 7 状态主状态机、动作编排顺序、模式权限矩阵、安全例外、去抖与超时预算。 |
| CONTROL/TIMING | `control/CONTROL_TIMING_ANALYSIS.md` | 计时基准与状态机时间分析（2026-06 版，冲突时以架构文档为准）。 |
| INTERFACE/SCPI | `interface/SCPI_COMMANDS.md` | 内部 SCPI 命令表，事实源 `Hardware/libscpi/port/scpi-def.c`。 |
| INTERFACE/LINK | `interface/INTERFACE_COMMUNICATION_ARCHITECTURE.md` | USB CDC 与 RS485/ModBus 链路参数、帧与错误处理。 |
| MULTINODE/CAN | `multinode/MULTINODE_CAN_HAOFV_ARCHITECTURE.md` | CAN 多机分层、节点模型、同步协议、Deployment Gate、M1–M5 路线。 |
| MULTINODE/REFMEM | `multinode/MULTINODE_REFLECTIVE_MEMORY_ARCHITECTURE.md` | 反射内存布局、快照、CAN 帧与寄存器映射。 |
| OTA | `ota/OTA_CAN_DISTRIBUTION_ARCHITECTURE.md` | OTA 状态机、A/B 槽位、Manifest、Bootloader 规则、CAN 分发。 |
| STORAGE/FLASH | `storage/STORAGE_FLASH_ARCHITECTURE.md` | 内部 Flash 配置页与 W25Q128 分区、唯一写者规则。 |
| STORAGE/LOG | `storage/STORAGE_LOG_ARCHITECTURE.md` | AppLog 表结构、编号空间、实时泵出与丢弃策略。 |
| HARDWARE/IO | `hardware/HARDWARE_IO_CONSTRAINTS.md` | 16 路输入/输出位映射与 ModBus 访问参数。 |
| HARDWARE/USB5V | `hardware/HARDWARE_USB5V_PROTECTION_DESIGN.md` | USB 5V 输入防护当前有效方案。 |
| HARDWARE/FAIL | `hardware/HARDWARE_TPS7A2033_FAILURE_ANALYSIS.md` | TPS7A2033 输入侧失效机理与处置要求。 |
| TOOLS/DOCCHAIN | `tools/TOOLS_DOC_TOOLCHAIN_PLAYBOOK.md` | Markdown → A4 HTML → PDF 生成链。 |
| VALIDATION/R1 | `validation/VALIDATION_CODE_REVIEW_20260613.md` | 最近一次全量静态审查基线。 |
| VALIDATION/R2 | `validation/VALIDATION_RISK_REVIEW_20260612.md` | 框架与并发风险评审。 |
| RELEASE/BRANCH | `release/RELEASE_BRANCH_PLAN.md` | 分支职责、版本命名、产物归档。 |
| RELEASE/CHECK | `release/RELEASE_CHECKLIST.md` | 发布前逐项检查表（含安全项）。 |
| DOCS/NAMING | `docs/docs/DOCS_NAMING_STRUCTURE_PLAN.md` | 命名、元数据、引用与评审规则。 |
| DOCS/STRUCT | `docs/docs/DOCS_DOMAIN_STRUCTURE_PLAN.md` | 目录结构、落点规则、迁移 gate。 |
| DOCS/REGISTRY | `docs/docs/DOCS_REGISTRY.md` | canonical 归属表与代码事实源对照，冲突判定台账。 |

## 任务与进度路由

| 领域 | 入口 | 说明 |
|---|---|---|
| 跨域项目板 | `docs/PROJECT_TODO.md` | 优先级 P0–P4 / M、门禁、验收基线；唯一状态事实源。 |
| 文档治理 | `docs/docs/DOCS_TASK_PROGRESS.md` | 文档体系实施与门禁证据（`DOCS-TASK-*`）。 |
| 文档迁移 | `docs/docs/DOCS_MIGRATION_TODO.md` | 迁移逐文件映射与 `DOCS-MIG-*` 残留待办。 |
| 控制域 | `docs/control/CONTROL_TODO.md` | 实机联调、纠偏、证据缺口（`CONTROL-*`）。 |
| 接口域 | `docs/interface/INTERFACE_TODO.md` | SCPI 交付源回归、命令一致性、ModBus 参数（`INTERFACE-*`）。 |
| 多机域 | `docs/multinode/MULTINODE_TODO.md` | 位率确认、健康监控、Deployment Gate（`MULTINODE-*`）。 |
| OTA 域 | `docs/ota/OTA_TODO.md` | 阶段 A–F 门禁（`OTA-*`）。 |
| 验证证据 | `docs/validation/VALIDATION_TASK_PROGRESS.md` | `VALIDATION-TASK-*` 证据记录。 |
| 其他领域 | 新建 `<DOMAIN>_TASK_PROGRESS.md` | 新闭环记录优先建立领域进度文件，不再堆进单一全局文件。 |

## 快速查找规则

- 查系统边界与任务/内存预算：先读 `arch/ARCH_PRODUCT_ARCHITECTURE.md`。
- 查某个动作为什么会这样执行：先读 `control/CONTROL_STATE_MACHINE_ARCHITECTURE.md`。
- 查命令字面量、参数域、错误码：先读 `interface/SCPI_COMMANDS.md`，再看 `reports/scpi/` 当前交付版。
- 查串口/ModBus 链路参数与帧：读 `interface/INTERFACE_COMMUNICATION_ARCHITECTURE.md`。
- 查 IO 位与继电器对应关系：读 `hardware/HARDWARE_IO_CONSTRAINTS.md`。
- 查 Flash/W25Q128 分区与写者：读 `storage/STORAGE_FLASH_ARCHITECTURE.md`。
- 查升级与回滚：读 `ota/OTA_CAN_DISTRIBUTION_ARCHITECTURE.md` 与 `ota/OTA_TODO.md`。
- 查多机规划：读 `multinode/README.md` 选择两份多机文档之一。
- 查现场失效与防护：读 `hardware/README.md`，再看对应分析与 `reports/hardware/` 交付件。
- 查文档怎么写、放哪、谁说了算：读 `docs/docs/` 下的命名规则、域结构与登记表。
- 查「现在到底还差什么」：读 `docs/PROJECT_TODO.md` 与各域 `*_TODO.md`。
- 查历史某版的命令定义：读 `legacy/scpi/`（只读，不代表当前行为）。

## 文件索引

### 00 文档治理（`docs/docs/`）

| 文件 | 定位 |
|---|---|
| `docs/docs/README.md` | 文档治理域入口与门禁命令。 |
| `docs/docs/DOCS_NAMING_STRUCTURE_PLAN.md` | 命名格式、领域前缀、类型后缀、元数据、引用与评审规则。 |
| `docs/docs/DOCS_DOMAIN_STRUCTURE_PLAN.md` | 目标目录、域映射、canonical 归属、迁移批次、落点规则、阅读树。 |
| `docs/docs/DOCS_MIGRATION_TODO.md` | `Doc/` → `docs/` 迁移结论、93 条逐文件映射、命名决策、残留待办。 |
| `docs/docs/DOCS_REGISTRY.md` | canonical 归属表（`REG-*`）、代码事实源对照、登记规则。 |
| `docs/docs/DOCS_TASK_PROGRESS.md` | 文档体系实施与门禁证据。 |
| `docs/docs/DOCS_EXECUTION_CONSTRAINTS.md` | 跨执行者的设计门禁、证据闭环、改动边界与提交前自检。 |
| `docs/PROJECT_TODO.md` | 跨域项目板：优先级、门禁与验收基线。 |

### 01 系统架构（`docs/arch/`）

| 文件 | 定位 |
|---|---|
| `docs/arch/README.md` | 架构域入口与阅读顺序。 |
| `docs/arch/ARCH_PRODUCT_ARCHITECTURE.md` | 产品总体架构 canonical。 |

### 02 控制（`docs/control/`）

| 文件 | 定位 |
|---|---|
| `docs/control/README.md` | 控制域入口。 |
| `docs/control/CONTROL_STATE_MACHINE_ARCHITECTURE.md` | 主状态机与动作控制 canonical。 |
| `docs/control/CONTROL_TIMING_ANALYSIS.md` | 计时基准与状态机时间分析。 |
| `docs/control/CONTROL_TODO.md` | 控制域待办。 |

### 03 接口（`docs/interface/`）

| 文件 | 定位 |
|---|---|
| `docs/interface/README.md` | 接口域入口。 |
| `docs/interface/SCPI_COMMANDS.md` | 内部 SCPI 命令表 canonical。 |
| `docs/interface/INTERFACE_COMMUNICATION_ARCHITECTURE.md` | 通信链路与协议边界 canonical。 |
| `docs/interface/INTERFACE_TODO.md` | 接口域待办。 |

### 04 多机分布式（`docs/multinode/`）

| 文件 | 定位 |
|---|---|
| `docs/multinode/README.md` | 多机域入口与两份文档的关系。 |
| `docs/multinode/MULTINODE_CAN_HAOFV_ARCHITECTURE.md` | CAN 多机 HAOFV 化架构 canonical。 |
| `docs/multinode/MULTINODE_REFLECTIVE_MEMORY_ARCHITECTURE.md` | 反射内存与快照布局 canonical。 |
| `docs/multinode/MULTINODE_TODO.md` | 多机域待办与门禁。 |

### 05 存储与日志（`docs/storage/`）

| 文件 | 定位 |
|---|---|
| `docs/storage/README.md` | 存储域入口与唯一写者原则。 |
| `docs/storage/STORAGE_FLASH_ARCHITECTURE.md` | 内部 Flash + W25Q128 分区 canonical。 |
| `docs/storage/STORAGE_LOG_ARCHITECTURE.md` | AppLog 架构 canonical。 |

### 06 OTA（`docs/ota/`）

| 文件 | 定位 |
|---|---|
| `docs/ota/README.md` | OTA 域入口与事实源。 |
| `docs/ota/OTA_CAN_DISTRIBUTION_ARCHITECTURE.md` | OTA 总体架构 canonical。 |
| `docs/ota/OTA_TODO.md` | OTA 域待办。 |

### 07 硬件（`docs/hardware/`）

| 文件 | 定位 |
|---|---|
| `docs/hardware/README.md` | 硬件域入口、证据与资料索引。 |
| `docs/hardware/HARDWARE_IO_CONSTRAINTS.md` | IO 位映射与 ModBus 访问参数。 |
| `docs/hardware/HARDWARE_USB5V_PROTECTION_DESIGN.md` | USB 5V 输入防护设计。 |
| `docs/hardware/HARDWARE_TPS7A2033_FAILURE_ANALYSIS.md` | TPS7A2033 失效分析。 |
| `docs/hardware/HARDWARE_USB_REPLUG_A0_ANALYSIS.md` | USB 自动拔插异常 A0 分析。 |
| `docs/hardware/HARDWARE_USB_REPLUG_B0_ANALYSIS.md` | USB 自动拔插异常 B0 分析。 |
| `docs/hardware/diagrams/` | 供电路径 SVG（3 个）。 |
| `docs/hardware/evidence/` | 失效形貌与 PCB 照片。 |
| `docs/hardware/netlist/` | IO-PPA-CH318TUSBEXT 网表。 |
| `docs/hardware/datasheets/` | TPS7A20 与 BSM 系列数据手册。 |

### 08 工具（`docs/tools/`）

| 文件 | 定位 |
|---|---|
| `docs/tools/README.md` | 工具清单与约束。 |
| `docs/tools/TOOLS_DOC_TOOLCHAIN_PLAYBOOK.md` | 文档生成链操作手册。 |
| `docs/tools/AI生成PPT操作手册.html` | AI 生成 PPT 参考手册（HTML）。 |

### 09 验证与证据（`docs/validation/`）

| 文件 | 定位 |
|---|---|
| `docs/validation/README.md` | 验证域入口与证据分级 E1–E5。 |
| `docs/validation/VALIDATION_CODE_REVIEW_20260613.md` | 全量静态代码审查报告。 |
| `docs/validation/VALIDATION_RISK_REVIEW_20260612.md` | 框架与并发风险审查报告。 |
| `docs/validation/VALIDATION_TASK_PROGRESS.md` | 验证证据记录。 |

### 10 发布与交付（`docs/release/`）

| 文件 | 定位 |
|---|---|
| `docs/release/README.md` | 发布域入口与版本命名速查。 |
| `docs/release/RELEASE_BRANCH_PLAN.md` | 分支与发布计划。 |
| `docs/release/RELEASE_CHECKLIST.md` | 发布检查表。 |
| `docs/release/deliverables/PinProbeA1客户外发包_20260728/` | 2026-07-28 客户交付包（冻结）。 |
| `docs/release/deliverables/PinProbeA1客户外发包_20260728.zip` | 同一交付包的压缩归档。 |

### 11 报告输出（`docs/reports/`）

| 文件 | 定位 |
|---|---|
| `docs/reports/README.md` | 报告输出域入口与交付件规则。 |
| `docs/reports/scpi/README.md` | SCPI 交付件版本与源文件状态。 |
| `docs/reports/scpi/*.html` / `*.pdf` | SCPI 指令说明 20260924 中英文交付件。 |
| `docs/reports/guide/README.md` | 操作说明版本与截图资源说明。 |
| `docs/reports/guide/*.md` / `*.html` / `*.pdf` | 客户版操作说明（中/英）与截图 `assets/`。 |
| `docs/reports/control/README.md` | 流程图交付件与生成链。 |
| `docs/reports/control/PinProbe A1 状态机与动作控制流程图20260915.html` / `.pdf` | 三页 A4 流程图交付件。 |
| `docs/reports/control/flowcharts/README.md` | 流程图内容说明与源码行号索引。 |
| `docs/reports/control/flowcharts/generate.mjs`、`generate-a4.mjs` | 流程图生成脚本。 |
| `docs/reports/control/flowcharts/*.svg` | 流程图 SVG（交互版与 A4 打印版各 3 张）。 |
| `docs/reports/arch/README.md` | 整体方案与架构报告说明。 |
| `docs/reports/arch/PinProbe A1控制箱 嵌入式整体方案与架构报告.html` / `.pdf` | 整体方案报告交付件。 |
| `docs/reports/hardware/README.md` | 失效分析与防护报告索引。 |
| `docs/reports/hardware/*.html` / `*.pdf` | TPS7A2033 / USB5V / USB 自动拔插 A0、B0 交付件。 |

### 12 静态资源（`docs/assets/`）

| 文件 | 定位 |
|---|---|
| `docs/assets/README.md` | 资源域入口与模板修改规则。 |
| `docs/assets/template/template.html` | GTS A4 竖版模板（生成链硬依赖）。 |
| `docs/assets/template/logo.png`、`logo.svg` | Logo 位图与矢量版。 |

### 13 历史资料（`docs/legacy/`）

| 文件 | 定位 |
|---|---|
| `docs/legacy/README.md` | 历史资料入口，含 SCPI 历史版本清单与使用规则。 |
| `docs/legacy/scpi/` | 21 个历史 SCPI 指令说明（中/英，0630 → 20260912 及变体）。 |
| `docs/legacy/control/LEGACY_PINPROBEA1_COMMAND_LIST.md` | 最初版下位机指令说明。 |
| `docs/legacy/control/PinProbeLink.png` | 最初版链路图。 |
| `docs/legacy/arch/LEGACY_HAOFV_ARCHITECTURE.md` | 上游 DHRT100 HAOFV 架构原文快照（非本产品架构）。 |
| `docs/legacy/logs/` | 串口/上位机导出日志（2 个 TXT）。 |
| `docs/legacy/LEGACY_TASK_PROGRESS_20260611.md` | 2026-06-11 架构重构日常总结。 |

### 14 归档（`docs/archive/`）

| 文件 | 定位 |
|---|---|
| `docs/archive/README.md` | 归档域入口与待判定资产清单。 |
| `docs/archive/设备渲染示意图.png` | 用途待判定的截图。 |

## 门禁

```powershell
python Tools\docs_check.py                 # 元数据 + 索引覆盖 + 引用 + 冲突标记
python Tools\docs_check.py --strict-names  # 另加命名规范硬失败
```

新增文档、改名或移动目录后必须运行上述命令，并同步更新本索引与所属域 README。
