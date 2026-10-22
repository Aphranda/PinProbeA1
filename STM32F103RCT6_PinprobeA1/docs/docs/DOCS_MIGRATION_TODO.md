# 文档体系迁移待办

Status: Active
Domain: DOCS
Canonical: `docs/docs/DOCS_MIGRATION_TODO.md`
Related: `docs/README.md`, `docs/docs/DOCS_NAMING_STRUCTURE_PLAN.md`, `docs/docs/DOCS_DOMAIN_STRUCTURE_PLAN.md`, `docs/docs/DOCS_TASK_PROGRESS.md`
Last updated: 2026-10-22

本文档记录 `Doc/` → `docs/` 域目录化迁移的执行结果、逐文件映射、命名决策和残留待办。

迁移由一次性脚本 `Script/docs_migrate.ps1` 执行：已跟踪文件用 `git mv` 保留历史，
交付件与 gitignore 命中的文件用 `Move-Item`。执行结果：93 个文件全部落位，`Doc/` 目录已删除。

## 迁移结论

| 项目 | 结果 |
|---|---|
| 迁移文件数 | 93（跳过 0） |
| 目录改名 | `Doc/` → `docs/` |
| 核心 Markdown 英文规范化 | 12 个（见下方映射） |
| 客户交付件改名 | 0（`reports/`、`release/deliverables/` 保持原文件名） |
| 历史资料归档 | SCPI 历史版本 21 个 + 最初版指令表 + 串口日志 2 个 → `legacy/` |
| 门禁 | `python Tools\docs_check.py` 通过（见 `docs/docs/DOCS_TASK_PROGRESS.md`） |

## 逐文件映射

| 旧路径 | 新路径 |
|---|---|
| `Doc/HAOFV_ARCHITECTURE.md` | `docs/legacy/arch/LEGACY_HAOFV_ARCHITECTURE.md` |
| `Doc/时间分析.md` | `docs/control/CONTROL_TIMING_ANALYSIS.md` |
| `Doc/PinProbe A1 SCPI 指令表（内部）.md` | `docs/interface/SCPI_COMMANDS.md` |
| `Doc/PinProbeA1_基于HAOFV的CAN多机分布式架构.md` | `docs/multinode/MULTINODE_CAN_HAOFV_ARCHITECTURE.md` |
| `Doc/PinProbeA1_多机架构设计方案_RAM反射内存.md` | `docs/multinode/MULTINODE_REFLECTIVE_MEMORY_ARCHITECTURE.md` |
| `Doc/PinProbeA1_OTA固件升级与CAN多机分发方案.md` | `docs/ota/OTA_CAN_DISTRIBUTION_ARCHITECTURE.md` |
| `Doc/PinProbeA1_TPS7A2033失效初步分析报告_20261010.md` | `docs/hardware/HARDWARE_TPS7A2033_FAILURE_ANALYSIS.md` |
| `Doc/PinProbeA1_USB5V输入防护优化报告_20261010.md` | `docs/hardware/HARDWARE_USB5V_PROTECTION_DESIGN.md` |
| `Doc/PinProbeA1_USB自动拔插异常报告_A0.md` | `docs/hardware/HARDWARE_USB_REPLUG_A0_ANALYSIS.md` |
| `Doc/PinProbeA1_USB自动拔插异常报告_B0.md` | `docs/hardware/HARDWARE_USB_REPLUG_B0_ANALYSIS.md` |
| `Doc/BSM系列产品说明书.pdf` | `docs/hardware/datasheets/BSM系列产品说明书.pdf` |
| `Doc/参考资料/TI_TPS7A20_TPS7A2033_SBVS338H_datasheet.pdf` | `docs/hardware/datasheets/TI_TPS7A20_TPS7A2033_SBVS338H_datasheet.pdf` |
| `Doc/图/5V供电路径.svg` | `docs/hardware/diagrams/5V供电路径.svg` |
| `Doc/图/主机供电回路.svg` | `docs/hardware/diagrams/主机供电回路.svg` |
| `Doc/图/从机供电回路.svg` | `docs/hardware/diagrams/从机供电回路.svg` |
| `Doc/失效照片/image.png` | `docs/hardware/evidence/TPS7A2033_failure_vin.png` |
| `Doc/失效照片/PCB.png` | `docs/hardware/evidence/PCB.png` |
| `Doc/失效照片/Netlist_IO-PPA-CH318TUSBEXT-4F4-HASL-V1.0_2026-10-10.txt` | `docs/hardware/netlist/Netlist_IO-PPA-CH318TUSBEXT-4F4-HASL-V1.0_2026-10-10.txt` |
| `Doc/代码审查报告_20260613.md` | `docs/validation/VALIDATION_CODE_REVIEW_20260613.md` |
| `Doc/代码审查报告_风险分析.md` | `docs/validation/VALIDATION_RISK_REVIEW_20260612.md` |
| `Doc/Git分支与发布计划.md` | `docs/release/RELEASE_BRANCH_PLAN.md` |
| `Doc/PinProbeA1客户外发包_20260728.zip` | `docs/release/deliverables/PinProbeA1客户外发包_20260728.zip` |
| `Doc/PinProbeA1客户外发包_20260728/PinProbe A1 箱体控制 SCPI 指令说明20260728.pdf` | `docs/release/deliverables/PinProbeA1客户外发包_20260728/PinProbe A1 箱体控制 SCPI 指令说明20260728.pdf` |
| `Doc/PinProbeA1客户外发包_20260728/PinProbeA1箱体控制程序.exe` | `docs/release/deliverables/PinProbeA1客户外发包_20260728/PinProbeA1箱体控制程序.exe` |
| `Doc/PinProbeA1客户外发包_20260728/PinProbeA1箱体控制程序操作说明_20260728.pdf` | `docs/release/deliverables/PinProbeA1客户外发包_20260728/PinProbeA1箱体控制程序操作说明_20260728.pdf` |
| `Doc/PinProbe A1 箱体控制 SCPI 指令说明20260924.html` | `docs/reports/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260924.html` |
| `Doc/PinProbe A1 箱体控制 SCPI 指令说明20260924.pdf` | `docs/reports/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260924.pdf` |
| `Doc/PinProbe A1 Box Control SCPI Command Reference 20260924.html` | `docs/reports/scpi/PinProbe A1 Box Control SCPI Command Reference 20260924.html` |
| `Doc/PinProbe A1 Box Control SCPI Command Reference 20260924.pdf` | `docs/reports/scpi/PinProbe A1 Box Control SCPI Command Reference 20260924.pdf` |
| `Doc/PinProbe A1 状态机与动作控制流程图20260915.html` | `docs/reports/control/PinProbe A1 状态机与动作控制流程图20260915.html` |
| `Doc/PinProbe A1 状态机与动作控制流程图20260915.pdf` | `docs/reports/control/PinProbe A1 状态机与动作控制流程图20260915.pdf` |
| `Doc/flowcharts/01-state-machine.svg` | `docs/reports/control/flowcharts/01-state-machine.svg` |
| `Doc/flowcharts/02-door-usb-sequence.svg` | `docs/reports/control/flowcharts/02-door-usb-sequence.svg` |
| `Doc/flowcharts/03-action-update.svg` | `docs/reports/control/flowcharts/03-action-update.svg` |
| `Doc/flowcharts/a4-01-state-machine.svg` | `docs/reports/control/flowcharts/a4-01-state-machine.svg` |
| `Doc/flowcharts/a4-02-door-usb.svg` | `docs/reports/control/flowcharts/a4-02-door-usb.svg` |
| `Doc/flowcharts/a4-03-action-update.svg` | `docs/reports/control/flowcharts/a4-03-action-update.svg` |
| `Doc/flowcharts/generate.mjs` | `docs/reports/control/flowcharts/generate.mjs` |
| `Doc/flowcharts/generate-a4.mjs` | `docs/reports/control/flowcharts/generate-a4.mjs` |
| `Doc/flowcharts/README.md` | `docs/reports/control/flowcharts/README.md` |
| `Doc/PinProbe A1控制箱 嵌入式整体方案与架构报告.html` | `docs/reports/arch/PinProbe A1控制箱 嵌入式整体方案与架构报告.html` |
| `Doc/PinProbe A1控制箱 嵌入式整体方案与架构报告.pdf` | `docs/reports/arch/PinProbe A1控制箱 嵌入式整体方案与架构报告.pdf` |
| `Doc/PinProbeA1_TPS7A2033失效初步分析报告_20261010.html` | `docs/reports/hardware/PinProbeA1_TPS7A2033失效初步分析报告_20261010.html` |
| `Doc/PinProbeA1_TPS7A2033失效初步分析报告_20261012.pdf` | `docs/reports/hardware/PinProbeA1_TPS7A2033失效初步分析报告_20261012.pdf` |
| `Doc/PinProbeA1_USB5V输入防护优化报告_20261010.html` | `docs/reports/hardware/PinProbeA1_USB5V输入防护优化报告_20261010.html` |
| `Doc/PinProbeA1_USB5V输入防护优化报告_20261010.pdf` | `docs/reports/hardware/PinProbeA1_USB5V输入防护优化报告_20261010.pdf` |
| `Doc/PinProbeA1_USB自动拔插异常报告_A0.html` | `docs/reports/hardware/PinProbeA1_USB自动拔插异常报告_A0.html` |
| `Doc/PinProbeA1_USB自动拔插异常报告_A0.pdf` | `docs/reports/hardware/PinProbeA1_USB自动拔插异常报告_A0.pdf` |
| `Doc/PinProbeA1_USB自动拔插异常报告_B0.html` | `docs/reports/hardware/PinProbeA1_USB自动拔插异常报告_B0.html` |
| `Doc/PinProbeA1_USB自动拔插异常报告_B0.pdf` | `docs/reports/hardware/PinProbeA1_USB自动拔插异常报告_B0.pdf` |
| `Doc/PinProbeA1 Box Control Program Operation Guide Customer 20260728.html` | `docs/reports/guide/PinProbeA1 Box Control Program Operation Guide Customer 20260728.html` |
| `Doc/PinProbeA1 Box Control Program Operation Guide.pdf` | `docs/reports/guide/PinProbeA1 Box Control Program Operation Guide.pdf` |
| `Doc/PinProbeA1箱体控制程序操作说明_客户版_20260727.md` | `docs/reports/guide/PinProbeA1箱体控制程序操作说明_客户版_20260727.md` |
| `Doc/PinProbeA1箱体控制程序操作说明_客户版_20260728.md` | `docs/reports/guide/PinProbeA1箱体控制程序操作说明_客户版_20260728.md` |
| `Doc/PinProbeA1箱体控制程序操作说明_客户版_20260728.html` | `docs/reports/guide/PinProbeA1箱体控制程序操作说明_客户版_20260728.html` |
| `Doc/PinProbeA1箱体控制程序操作说明_客户版_20260728.pdf` | `docs/reports/guide/PinProbeA1箱体控制程序操作说明_客户版_20260728.pdf` |
| `Doc/PinProbeA1箱体控制程序操作说明_20260728.pdf` | `docs/reports/guide/PinProbeA1箱体控制程序操作说明_20260728.pdf` |
| `Doc/assets/pinprobe_gui_main.png` | `docs/reports/guide/assets/pinprobe_gui_main.png` |
| `Doc/assets/pinprobe_gui_ota.png` | `docs/reports/guide/assets/pinprobe_gui_ota.png` |
| `Doc/assets/烧录区域.png` | `docs/reports/guide/assets/烧录区域.png` |
| `Doc/assets/软件主体.png` | `docs/reports/guide/assets/软件主体.png` |
| `Doc/model/AI生成PPT操作手册.html` | `docs/tools/AI生成PPT操作手册.html` |
| `Doc/model/template.html` | `docs/assets/template/template.html` |
| `Doc/model/logo.png` | `docs/assets/template/logo.png` |
| `Doc/model/logo.svg` | `docs/assets/template/logo.svg` |
| `Doc/待办事项.md` | `docs/PROJECT_TODO.md` |
| `Doc/PinprobeA1.md` | `docs/legacy/control/LEGACY_PINPROBEA1_COMMAND_LIST.md` |
| `Doc/PinProbeLink.png` | `docs/legacy/control/PinProbeLink.png` |
| `Doc/PinProbe A1 箱体控制 SCPI 指令说明0630.pdf` | `docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明0630.pdf` |
| `Doc/PinProbe A1 箱体控制 SCPI 指令说明0727.html` | `docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明0727.html` |
| `Doc/PinProbe A1 箱体控制 SCPI 指令说明0727.md` | `docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明0727.md` |
| `Doc/PinProbe A1 箱体控制 SCPI 指令说明0727.pdf` | `docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明0727.pdf` |
| `Doc/PinProbe A1 箱体控制 SCPI 指令说明20260728.md` | `docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260728.md` |
| `Doc/PinProbe A1 箱体控制 SCPI 指令说明20260728.pdf` | `docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260728.pdf` |
| `Doc/PinProbe A1 箱体控制 SCPI 指令说明20260811.html` | `docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260811.html` |
| `Doc/PinProbe A1 箱体控制 SCPI 指令说明20260811.pdf` | `docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260811.pdf` |
| `Doc/PinProbe A1 箱体控制 SCPI 指令说明20260911.html` | `docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260911.html` |
| `Doc/PinProbe A1 箱体控制 SCPI 指令说明20260911.pdf` | `docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260911.pdf` |
| `Doc/PinProbe A1 箱体控制 SCPI 指令说明20260912.html` | `docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260912.html` |
| `Doc/PinProbe A1 箱体控制 SCPI 指令说明20260912.pdf` | `docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260912.pdf` |
| `Doc/PinProbe A1 箱体控制 SCPI 指令说明20260912_固件实现版.pdf` | `docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260912_固件实现版.pdf` |
| `Doc/PinProbe A1 Box Control SCPI Command Reference 20260728.html` | `docs/legacy/scpi/PinProbe A1 Box Control SCPI Command Reference 20260728.html` |
| `Doc/PinProbe A1 Box Control SCPI Command Reference 20260912.html` | `docs/legacy/scpi/PinProbe A1 Box Control SCPI Command Reference 20260912.html` |
| `Doc/PinProbe A1 Box Control SCPI Command Reference 20260912.md` | `docs/legacy/scpi/PinProbe A1 Box Control SCPI Command Reference 20260912.md` |
| `Doc/PinProbe A1 Box Control SCPI Command Reference 20260912.pdf` | `docs/legacy/scpi/PinProbe A1 Box Control SCPI Command Reference 20260912.pdf` |
| `Doc/PinProbe A1 Box Control SCPI Command Reference.md` | `docs/legacy/scpi/PinProbe A1 Box Control SCPI Command Reference.md` |
| `Doc/PinProbe A1 Box Control SCPI Command Reference.pdf` | `docs/legacy/scpi/PinProbe A1 Box Control SCPI Command Reference.pdf` |
| `Doc/PinProbeA1_SCPI_20260912_mode_error201.pdf` | `docs/legacy/scpi/PinProbeA1_SCPI_20260912_mode_error201.pdf` |
| `Doc/PinProbeA1_SCPI_20260912_mode_impl.pdf` | `docs/legacy/scpi/PinProbeA1_SCPI_20260912_mode_impl.pdf` |
| `Doc/SaveWindows2026_6_16_23-33-06.TXT` | `docs/legacy/logs/SaveWindows2026_6_16_23-33-06.TXT` |
| `Doc/SaveWindows2026_6_16_23-38-58.TXT` | `docs/legacy/logs/SaveWindows2026_6_16_23-38-58.TXT` |
| `Doc/今日工作总结_20260611.md` | `docs/legacy/LEGACY_TASK_PROGRESS_20260611.md` |
| `Doc/untitled.642.png` | `docs/archive/设备渲染示意图.png` |

## 迁移后的命名决策

| 决策 | 说明 |
|---|---|
| `HAOFV_ARCHITECTURE.md` | 该文档是上游 DHRT100（RP2350/TDMA/VDC/DPLL）的 HAOFV 架构原文快照，不是 PinProbeA1 架构。迁入 `docs/legacy/arch/LEGACY_HAOFV_ARCHITECTURE.md` 作为冻结参考；本产品架构见 `docs/arch/ARCH_PRODUCT_ARCHITECTURE.md`，多机演进见 `docs/multinode/MULTINODE_CAN_HAOFV_ARCHITECTURE.md`。 |
| SCPI 指令表 | 内部表迁为 `docs/interface/SCPI_COMMANDS.md`（事实源 `Hardware/libscpi/port/scpi-def.c`）；历史中英版本全部归 `docs/legacy/scpi/`；当前交付版 20260924 只保留 HTML/PDF 于 `docs/reports/scpi/`。 |
| 客户交付件 | 操作说明、SCPI 说明、流程图、失效分析报告的 HTML/PDF 一律保留原中文名，只更换目录，保证交付路径可追溯。 |
| 失效证据归位 | `失效照片/image.png` → `hardware/evidence/TPS7A2033_failure_vin.png`；`图/*.svg` → `hardware/diagrams/`；网表 → `hardware/netlist/`；正文图片引用已同步更新。 |
| 待办事项 | `待办事项.md` → `docs/PROJECT_TODO.md`，作为跨域项目板；各域 TODO 引用其条目，不重复维护状态。 |
| 流程图工具链 | `Doc/flowcharts/` 整体迁入 `docs/reports/control/flowcharts/`，保持生成脚本与输出产物的相对路径不变（`generate-a4.mjs` 输出到上级目录）。 |
| 领域前缀补充 | 迁移后新增 `SCPI` 领域前缀（用于 `docs/interface/SCPI_COMMANDS.md` 这类命令面专项文档），已同步 `docs/docs/DOCS_NAMING_STRUCTURE_PLAN.md` 与 `Tools/docs_check.py` 的 `ALLOWED_PREFIXES`。 |
| 历史命名微调 | `LEGACY_DAILY_SUMMARY_20260611.md` 的 `_SUMMARY` 不在允许后缀内，迁移后改名为 `LEGACY_TASK_PROGRESS_20260611.md`（内容未改动）。 |

## 残留待办

### P0 必须先收口

- [ ] DOCS-MIG-01：当前 SCPI 交付版（`docs/reports/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260924.html` 与英文版）没有对应的 Markdown 源，交付链已断。需要在 `docs/reports/scpi/` 建立可重建的 md 源，并与 `docs/interface/SCPI_COMMANDS.md` 核对命令一致性。
- [ ] DOCS-MIG-02：`docs/reports/control/flowcharts/` 的两个生成脚本迁移后需执行一次，确认 SVG 与 A4 HTML 输出路径仍正确（HTML 目标为 `docs/reports/control/`）。

### P1 应该完成

- [ ] DOCS-MIG-03：为 `control/`、`interface/`、`multinode/`、`ota/`、`storage/` 补齐 `*_TASK_PROGRESS.md`，把 `docs/PROJECT_TODO.md` 已完成条目的证据迁入对应域。
- [ ] DOCS-MIG-04：把 `docs/PROJECT_TODO.md` 的 P1–P4 条目拆成各域 `*_TODO.md` 执行清单，项目板只保留跨域门禁与优先级。
- [x] DOCS-MIG-05：`docs/hardware/` 缺少正式 IO 约束表（16 路输入/输出、RS485 地址、继电器映射）。**已完成**：由 `Hardware/BsmRelay/Inc/BsmRelay.h`、`state_vector.c` 的 `IN_*`/`OUT_*` 宏与 `flash.h` 的 LED IO 默认值反向整理，产出 `docs/hardware/HARDWARE_IO_CONSTRAINTS.md`（含 4 项待核实 `HW-IO-Q1..Q4`）。

### P2 可以延后

- [ ] DOCS-MIG-06：`docs/reports/guide/` 中英文操作说明版本号未统一，英文版 `PinProbeA1 Box Control Program Operation Guide.pdf` 无 md 源。
- [x] DOCS-MIG-07：`docs/archive/设备渲染示意图.png`（原 `untitled.642.png`）用途已确认为设备渲染图；待决定放入 `docs/assets/` 还是 `docs/reports/guide/assets/`；`docs/archive/README.md` 已更新。
- [ ] DOCS-MIG-08：根 `README.md` 的构建/烧录/出厂镜像段落可下沉为 `docs/tools/` 下的构建操作手册（拟建），根 README 只保留快速入口。

## 核对命令

```powershell
# 旧路径残留
rg -n "Doc/" docs README.md Script Tools -g "*.md" -g "*.ps1" -g "*.py"

# 文档门禁
python Tools\docs_check.py
python Tools\docs_check.py --strict-names

# 迁移提交前空白检查
git diff --check
```