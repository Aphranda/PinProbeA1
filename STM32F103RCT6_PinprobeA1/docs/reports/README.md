# 报告输出域

Status: Active
Domain: REPORTS
Canonical: `docs/reports/README.md`
Related: `docs/README.md`, `docs/tools/TOOLS_DOC_TOOLCHAIN_PLAYBOOK.md`, `docs/interface/README.md`, `docs/control/README.md`, `docs/hardware/README.md`
Last updated: 2026-10-22

本目录存放**当前有效的交付件渲染产物**（HTML/PDF）及其源 Markdown。交付件不是事实源：
内容以各域 canonical 文档为准，本目录只保证「某一版交付出去的东西长什么样」。

## 子目录

| 目录 | 内容 | 当前交付版 |
|---|---|---|
| `scpi/` | SCPI 指令说明中英文交付件 | 20260924（中/英 HTML+PDF） |
| `guide/` | 上位机操作说明（客户版）与截图资源 | `PinProbeA1箱体控制程序操作说明_客户版_20260728`（md/html/pdf） |
| `control/` | 状态机与动作控制流程图 + 生成脚本与 SVG | `PinProbe A1 状态机与动作控制流程图20260915`（html/pdf） |
| `arch/` | 嵌入式整体方案与架构报告 | `PinProbe A1控制箱 嵌入式整体方案与架构报告`（html/pdf） |
| `hardware/` | 失效分析与防护优化报告 | `HARDWARE_TPS7A2033`、`USB5V`、`USB自动拔插 A0/B0`（html/pdf） |

## 规则

- 交付件只能由 `docs/tools/TOOLS_DOC_TOOLCHAIN_PLAYBOOK.md` 的链路生成，禁止手工改 HTML 后交付。
- 同主题只保留**当前版**在 `reports/`；历史版本移入 `docs/legacy/<topic>/`，不就地覆盖。
- 文件名保留客户可见的中文/英文原名（不改名、不改大小写、不改日期格式），便于交付追溯。
- 本目录下的 Markdown（除各子目录 README）豁免文档治理元数据与命名门禁，原因是它们会被 A4/PDF
  生成链直接渲染；但内容必须与对应域 canonical 文档一致。
- 每新增一份交付件，必须更新所在子目录的 `README.md` 与 `docs/release/RELEASE_CHECKLIST.md` 的交付件项。

## 子目录入口

| 入口 | 说明 |
|---|---|
| `scpi/README.md` | SCPI 交付件版本与源文件状态 |
| `guide/README.md` | 操作说明版本与截图资源 |
| `control/README.md` | 流程图交付件与生成脚本 |
| `arch/README.md` | 整体方案与架构报告 |
| `hardware/README.md` | 失效分析与防护优化报告 |
