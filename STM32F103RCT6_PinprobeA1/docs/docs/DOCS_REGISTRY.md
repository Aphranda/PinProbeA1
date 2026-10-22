# 文档与契约登记表

Status: Active
Domain: DOCS
Canonical: `docs/docs/DOCS_REGISTRY.md`
Related: `docs/README.md`, `docs/docs/DOCS_NAMING_STRUCTURE_PLAN.md`, `docs/docs/DOCS_DOMAIN_STRUCTURE_PLAN.md`
Last updated: 2026-10-22

本表是「哪个文档对哪类事实有最终解释权」的唯一台账。出现冲突时，按本表的 canonical 归属判定，
冲突方一律视为待勘误或待废弃。登记表只登记文档身份和边界，不复制文档内容。

## 一、Canonical 归属表

| 编号 | 领域 | canonical 文档 | 独占事实（其他文档不得另行定义） |
|---|---|---|---|
| `REG-ARCH-01` | 总体架构 | `docs/arch/ARCH_PRODUCT_ARCHITECTURE.md` | 单板固件分层、RTOS 任务职责、RAM/Flash 预算口径、单机与多机边界 |
| `REG-CONTROL-01` | 控制 | `docs/control/CONTROL_STATE_MACHINE_ARCHITECTURE.md` | 主状态机状态集与迁移条件、动作编排顺序、控制模式权限矩阵、安全例外 |
| `REG-CONTROL-02` | 控制 | `docs/control/CONTROL_TIMING_ANALYSIS.md` | 定时基准、任务周期、动作超时与状态机时间预算 |
| `REG-IF-01` | 接口 | `docs/interface/SCPI_COMMANDS.md` | 已实现 SCPI 命令名、参数域、错误码映射（事实源 `Hardware/libscpi/port/scpi-def.c`） |
| `REG-IF-02` | 接口 | `docs/interface/INTERFACE_COMMUNICATION_ARCHITECTURE.md` | USB CDC 与 RS485/ModBus 链路参数、帧格式、链路可信状态语义 |
| `REG-MN-01` | 多机 | `docs/multinode/MULTINODE_CAN_HAOFV_ARCHITECTURE.md` | CAN 多机分层、主从职责、节点寻址与心跳策略 |
| `REG-MN-02` | 多机 | `docs/multinode/MULTINODE_REFLECTIVE_MEMORY_ARCHITECTURE.md` | `RAM_Vector_t` 布局语义、命令槽仲裁、节点状态与 IO 镜像字段 |
| `REG-OTA-01` | OTA | `docs/ota/OTA_CAN_DISTRIBUTION_ARCHITECTURE.md` | OTA 状态机、A/B 槽位、Manifest、boot flag、多机分发时序 |
| `REG-STO-01` | 存储 | `docs/storage/STORAGE_FLASH_ARCHITECTURE.md` | 内部 Flash 配置页布局与版本、W25Q128 分区表、快照区归属 |
| `REG-STO-02` | 存储 | `docs/storage/STORAGE_LOG_ARCHITECTURE.md` | AppLog 表结构、级别/模块/事件编码、回读与丢弃策略 |
| `REG-HW-01` | 硬件 | `docs/hardware/HARDWARE_USB5V_PROTECTION_DESIGN.md` | USB 5V 输入防护器件选型与钳位方案（当前有效设计取向） |
| `REG-HW-02` | 硬件 | `docs/hardware/HARDWARE_TPS7A2033_FAILURE_ANALYSIS.md` | TPS7A2033 输入侧失效机理判定与处置要求 |
| `REG-TOOLS-01` | 工具 | `docs/tools/TOOLS_DOC_TOOLCHAIN_PLAYBOOK.md` | 文档生成链（md → A4 HTML → PDF）、模板与校验脚本用法 |
| `REG-VAL-01` | 验证 | `docs/validation/VALIDATION_CODE_REVIEW_20260613.md` | 静态审查基线结论与未闭环风险清单 |
| `REG-REL-01` | 发布 | `docs/release/RELEASE_BRANCH_PLAN.md` | 分支职责、版本命名、产物归档规则 |
| `REG-DOCS-01` | 治理 | `docs/docs/DOCS_NAMING_STRUCTURE_PLAN.md` | 文件名格式、领域前缀、类型后缀、元数据字段 |
| `REG-DOCS-02` | 治理 | `docs/docs/DOCS_DOMAIN_STRUCTURE_PLAN.md` | 目录结构、新文档落点判断顺序、迁移 gate |
| `REG-DOCS-03` | 治理 | `docs/docs/DOCS_REGISTRY.md` | 本登记表自身：canonical 归属判定规则 |

## 二、代码事实源对照

文档不能替代代码，以下事实以代码为准，文档只做镜像与解释：

| 事实 | 代码事实源 | 对应文档 |
|---|---|---|
| SCPI 命令表 | `Hardware/libscpi/port/scpi-def.c` | `docs/interface/SCPI_COMMANDS.md` |
| RAM 向量表布局 | `Hardware/RamVector/Inc/ram_vector.h` | `docs/multinode/MULTINODE_REFLECTIVE_MEMORY_ARCHITECTURE.md` |
| 状态机与动作编排 | `Hardware/RamVector/Src/state_vector.c`、`cmd_exec.c` | `docs/control/CONTROL_STATE_MACHINE_ARCHITECTURE.md` |
| RTOS 任务与优先级 | `Core/Src/freertos.c`、`App/Src/app_tasks.c` | `docs/arch/ARCH_PRODUCT_ARCHITECTURE.md` |
| 链接脚本与 RAM 分区 | `MDK-ARM/STM32F103RCT6_PinprobeA1.sct` | `docs/storage/STORAGE_FLASH_ARCHITECTURE.md` |
| Flash 配置结构 | `Hardware/Flash/Inc/flash.h` | `docs/storage/STORAGE_FLASH_ARCHITECTURE.md` |
| W25Q128 / OTA 分区 | `Hardware/OTA/Inc/ota_manifest.h` | `docs/ota/OTA_CAN_DISTRIBUTION_ARCHITECTURE.md` |
| AppLog 表结构 | `Hardware/AppLog/Inc/app_log.h` | `docs/storage/STORAGE_LOG_ARCHITECTURE.md` |
| 上位机命令映射 | `Tools/pinprobe_gui.py` | `docs/tools/README.md` |

## 三、登记规则

1. 新增 canonical 文档时，必须在本表新增一行，写明「独占事实」。
2. 两份文档对同一事实给出不同数值时，按本表判定 canonical；非 canonical 一侧必须在 5 个工作日内勘误或标注 `Deprecated`。
3. 事实源在代码、镜像在文档的条目（第二节），代码改动后同一次提交内更新文档；未同步时以代码为准，并在 `docs/PROJECT_TODO.md` 登记漂移项。
4. 交付件（`docs/reports/`）不作为事实源，只是某个时点的渲染产物；其内容与 canonical 文档冲突时以 canonical 为准。
5. 历史资料（`docs/legacy/`）永久冻结，不得据其修订现行设计；如需复活历史方案，先在对应域写 `_DESIGN.md` 并登记本表。
