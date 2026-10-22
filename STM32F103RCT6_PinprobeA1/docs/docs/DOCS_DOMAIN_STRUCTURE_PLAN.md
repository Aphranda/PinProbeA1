# PinProbeA1 文档域目录化管理方案

Status: Active
Domain: DOCS
Canonical: `docs/docs/DOCS_DOMAIN_STRUCTURE_PLAN.md`
Related: `docs/README.md`, `docs/docs/DOCS_NAMING_STRUCTURE_PLAN.md`, `docs/docs/DOCS_MIGRATION_TODO.md`, `docs/arch/ARCH_PRODUCT_ARCHITECTURE.md`
Last updated: 2026-10-22

本文档定义 `docs/` 的域目录结构、域归属规则、新文档落点判断顺序和迁移 gate。
参考 `D:\Work\HDRT100\docs` 的治理方式：根 `README.md` 作为唯一总入口，子目录按稳定领域分组，
每个领域保留 canonical 主文档、TODO、任务进度和归档入口。

## 目标

- 让文档结构与固件/产品结构一致：控制状态机、SCPI 接口、CAN 多机、OTA、存储日志、硬件失效分析各有清晰入口。
- 结束 `Doc/` 平铺目录（中英文混名 + 92 个文件同级存放 + 版本靠文件名日期区分）的历史。
- 保留 `docs/README.md` 作为唯一总索引：新读者按域即可读完，不依赖全文搜索。
- 迁移过程保护引用、交付件和生成脚本，不改变任何客户可交付文件名。

## 目标目录

```text
docs/
  README.md            总索引（唯一入口）
  PROJECT_TODO.md      跨域项目板：优先级、门禁与验收基线
  docs/                文档治理：命名规则、域结构、迁移表、登记表、治理进度
  arch/                产品总体架构（单机基线 + 多机演进方向）
  control/             状态机、动作控制、控制模式与权限、IO 链路可信状态、时序分析
  interface/           SCPI 命令表、USB CDC 串口、RS485/ModBus、上位机交互边界
  multinode/           CAN 多机分布式：节点寻址、RAM 反射内存向量表、心跳与同步
  storage/             内部 Flash 配置、W25Q128 布局、AppLog 日志与快照
  ota/                 OTA、Bootloader、A/B 槽位、出厂合并镜像与多机分发
  hardware/            IO 约束、供电与接口防护、网表、数据手册、失效分析
  tools/               PC 工具（箱体控制程序）、构建/烧录脚本、文档生成链
  validation/          代码审查、回归验证、压力测试与证据
  release/             分支与发布计划、发布门禁、出厂/客户交付包
  reports/             当前交付件输出：SCPI 指令说明、操作说明、流程图、失效分析报告
  assets/              模板、Logo、通用静态资源
  legacy/              历史版本 SCPI 表、最初版报告、串口日志、上游迁移原文
  archive/             归档说明、一次性脚本产出与废弃素材
```

目录命名使用小写英文；文档文件名遵循 `<DOMAIN>_<SUBJECT>_<TYPE>.md`；客户交付件保留中文原名。

## 对外能力与文档域映射

| 对外能力 / 模块 | 文档目录 | 说明 |
|---|---|---|
| `SYSTem` / `*IDN?` / 版本与序列号 | `interface/`, `storage/` | 接口契约在 `interface/`，配置持久化在 `storage/` |
| `CONFigure:*` / `READ:*` | `interface/`, `control/` | 命令语义与错误码归接口，动作执行与状态归控制 |
| 门/USB 动作、锁定、LED、急停 | `control/` | 状态机、动作编排、控制模式权限矩阵 |
| RS485 开关量链路（16 路 IO） | `interface/`, `control/` | 协议与链路参数归接口，IO 链路可信状态与执行归控制 |
| CAN 多机 / 节点寻址 / 反射内存 | `multinode/` | 节点表、命令槽、心跳、快照同步 |
| OTA / Bootloader / 出厂镜像 | `ota/` | A/B 槽位、Manifest、boot flag、多机分发 |
| Flash 配置 / W25Q128 / 日志 | `storage/` | 分区表、配置结构、AppLog 环形区 |
| 供电与 USB 防护 / 失效分析 | `hardware/` | 失效机理、改进方案、证据照片、datasheet |
| 上位机工具 | `tools/` | `pinprobe_gui.py`、构建与烧录、文档生成链 |
| 发布与交付 | `release/`, `reports/` | 分支计划/门禁在 `release/`，实际交付件在 `reports/` |

## Canonical 归属

| 域 | canonical 主文档 | 说明 |
|---|---|---|
| `arch/` | `ARCH_PRODUCT_ARCHITECTURE.md` | 单板固件架构、任务与资源边界、多机演进方向。 |
| `control/` | `CONTROL_STATE_MACHINE_ARCHITECTURE.md` | 7 状态主状态机、动作编排、模式权限与安全例外。 |
| `interface/` | `SCPI_COMMANDS.md` | 以 `scpi-def.c` 为事实源的内部命令表。 |
| `interface/` | `INTERFACE_COMMUNICATION_ARCHITECTURE.md` | USB CDC、RS485/ModBus、帧与错误处理边界。 |
| `multinode/` | `MULTINODE_CAN_HAOFV_ARCHITECTURE.md` | CAN 多机的 HAOFV 化分层与主从职责。 |
| `multinode/` | `MULTINODE_REFLECTIVE_MEMORY_ARCHITECTURE.md` | RAM 反射内存向量表与多机同步模型。 |
| `ota/` | `OTA_CAN_DISTRIBUTION_ARCHITECTURE.md` | OTA 阶段、槽位、Manifest 与 CAN 分发。 |
| `storage/` | `STORAGE_FLASH_ARCHITECTURE.md` | 内部 Flash 配置页 + W25Q128 分区。 |
| `storage/` | `STORAGE_LOG_ARCHITECTURE.md` | AppLog 表结构、级别/模块/事件编码与回读。 |
| `hardware/` | `HARDWARE_USB5V_PROTECTION_DESIGN.md` | 供电与接口防护设计取向（当前有效方案）。 |
| `tools/` | `TOOLS_DOC_TOOLCHAIN_PLAYBOOK.md` | Markdown → A4 HTML → PDF 的可重复生成链。 |
| `validation/` | `VALIDATION_CODE_REVIEW_20260613.md` | 最近一次全量静态审查基线。 |
| `release/` | `RELEASE_BRANCH_PLAN.md` | 分支职责、版本命名与产物归档规则。 |
| `docs/` | `DOCS_NAMING_STRUCTURE_PLAN.md` | 命名与元数据规则。 |

## 迁移批次（已执行）

| 批次 | 内容 | 状态 |
|---|---|---|
| Phase 0 | 建立目标结构、映射表与检查脚本 | 完成（2026-10-22） |
| Phase 1 | `Doc/` → `docs/` 目录改名，92 个文件按域落位 | 完成（2026-10-22） |
| Phase 2 | 核心 Markdown 英文规范命名 + 元数据补齐 | 完成（2026-10-22） |
| Phase 3 | 历史版本 SCPI 表、最初版报告、串口日志归入 `legacy/` | 完成（2026-10-22） |
| Phase 4 | SCPI 交付版 Markdown 源回归（当前交付版无对应 md 源） | 待办，见 `docs/interface/INTERFACE_TODO.md` |

文件级映射表与残留引用核对见 `docs/docs/DOCS_MIGRATION_TODO.md`。

## 迁移与移动 gate

任何目录级移动必须满足：

- 先提出迁移表：旧路径、新路径、引用影响。
- 同一任务内更新所有 Markdown、README、脚本内引用（`rg -n "Doc/|docs/"`）。
- 客户交付件（`reports/`、`release/deliverables/`）文件名不得改动，只允许更换所在目录。
- 运行 `python Tools\docs_check.py` 必须通过；新增文件评审使用 `--strict-names`。
- 使用 `git diff --check` 检查空白与换行问题。
- 不混合功能代码改动和大规模文档移动。

## 新文档落点规则

新增文档按以下顺序判断目录：

1. 是否是产品总体架构、任务/资源边界、跨域主线：`arch/`。
2. 是否影响状态机、动作编排、控制模式、IO 链路判定：`control/`。
3. 是否是对外命令、串口/USB 协议、RS485/ModBus、上位机交互：`interface/`。
4. 是否涉及 CAN 节点、反射内存、心跳与多机同步：`multinode/`。
5. 是否涉及 OTA、Bootloader、镜像与分发：`ota/`。
6. 是否涉及 Flash 配置、W25Q128 分区、AppLog、快照：`storage/`。
7. 是否是 IO 约束、供电器件、网表、失效分析：`hardware/`。
8. 是否是 PC 工具、构建/烧录脚本、文档生成链：`tools/`。
9. 是否是审查、回归、压力测试与证据：`validation/`。
10. 是否是分支/版本/交付约束：`release/`。
11. 是否是 HTML/PDF 交付输出：`reports/`（源 Markdown 归原域）。
12. 是否是历史原文或外部迁入资料：`legacy/`。
13. 是否是废弃素材或一次性脚本产出：`archive/`。

## 推荐阅读树

```text
PinProbeA1
├─ 1. 总体架构
│  ├─ arch/ARCH_PRODUCT_ARCHITECTURE.md
│  └─ PROJECT_TODO.md
├─ 2. 控制主链路
│  ├─ control/CONTROL_STATE_MACHINE_ARCHITECTURE.md
│  ├─ control/CONTROL_TIMING_ANALYSIS.md
│  └─ control/CONTROL_TODO.md
├─ 3. 对外接口
│  ├─ interface/SCPI_COMMANDS.md
│  ├─ interface/INTERFACE_COMMUNICATION_ARCHITECTURE.md
│  └─ interface/INTERFACE_TODO.md
├─ 4. 多机与升级
│  ├─ multinode/MULTINODE_CAN_HAOFV_ARCHITECTURE.md
│  ├─ multinode/MULTINODE_REFLECTIVE_MEMORY_ARCHITECTURE.md
│  └─ ota/OTA_CAN_DISTRIBUTION_ARCHITECTURE.md
├─ 5. 存储与日志
│  ├─ storage/STORAGE_FLASH_ARCHITECTURE.md
│  └─ storage/STORAGE_LOG_ARCHITECTURE.md
├─ 6. 硬件与可靠性
│  ├─ hardware/HARDWARE_USB5V_PROTECTION_DESIGN.md
│  └─ hardware/HARDWARE_TPS7A2033_FAILURE_ANALYSIS.md
├─ 7. 工具、验证与发布
│  ├─ tools/TOOLS_DOC_TOOLCHAIN_PLAYBOOK.md
│  ├─ validation/VALIDATION_CODE_REVIEW_20260613.md
│  └─ release/RELEASE_BRANCH_PLAN.md
└─ 8. 历史与交付
   ├─ reports/            当前交付件
   └─ legacy/             历史版本与上游原文
```
