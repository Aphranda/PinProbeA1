# 工具域

Status: Active
Domain: TOOLS
Canonical: `docs/tools/README.md`
Related: `docs/README.md`, `docs/tools/TOOLS_DOC_TOOLCHAIN_PLAYBOOK.md`, `docs/release/RELEASE_BRANCH_PLAN.md`
Last updated: 2026-10-22

本目录登记本项目的配套工具、脚本与生成链：上位机工具、构建与烧录、文档生成，
以及工具自身的约束（谁能改、输出到哪、如何验证）。

## 当前主线

| 当前路径 | 定位 | 使用规则 |
|---|---|---|
| `TOOLS_DOC_TOOLCHAIN_PLAYBOOK.md` | Markdown → A4 HTML → PDF 的可重复生成链（`md_to_gts_a4_pages.py`、`html_to_pdf_edge.py`、`a4_layout_check.py`），含模板与校验方法。 | 文档交付件的唯一生成方式。 |
| `AI生成PPT操作手册.html` | AI 生成 PPT 的操作说明（外部经验沉淀）。 | 参考手册，不参与固件构建。 |

## 工具清单

| 工具 | 仓库路径 | 用途 | 输出 |
|---|---|---|---|
| 上位机控制程序 | `Tools/pinprobe_gui.py` | 串口连接、动作控制、状态监视、固件升级、截图与压力测试入口 | 打包为 `PinProbeA1箱体控制程序.exe`（PyInstaller，`Tools/PinProbeA1.spec`） |
| 出厂镜像合并 | `Tools/merge_ota_images.py`、`Script/build_factory_images.ps1` | 合并 Bootloader + APP 为出厂镜像 | `build/PinProbeA1_Factory/*.hex/.bin/.json` |
| Markdown → A4 HTML | `Tools/md_to_gts_a4_pages.py` | 按 GTS 模板生成 A4 竖版 HTML | `docs/reports/**/*.html` |
| HTML → PDF | `Tools/html_to_pdf_edge.py` | 借助 Edge 无头模式打印 PDF | `docs/reports/**/*.pdf` |
| A4 版式检查 | `Tools/a4_layout_check.py` | 估算分页溢出风险 | 控制台报告 |
| 流程图生成 | `docs/reports/control/flowcharts/generate.mjs`、`generate-a4.mjs` | 由源码结构生成 SVG 与三页 A4 HTML | `docs/reports/control/flowcharts/*.svg`、`docs/reports/control/*.html` |
| 压力测试 | `Tools/pinprobe_pressure_test.py`（由 GUI 调用） | 连续动作/异常注入 | 控制台与日志 |
| 图标生成 | `Tools/Doicon.py` | 生成 `Tools/icon_PPA.ico` | 图标资源 |
| 文档迁移（一次性） | `Script/docs_migrate.ps1` | `Doc/` → `docs/` 迁移，已执行完毕 | 保留为迁移证据 |

## 约束

- 工具不改固件行为：任何「GUI 里加一条私有命令」的做法无效，命令必须先在 `scpi-def.c` 实现并登记
  `docs/interface/SCPI_COMMANDS.md`。
- 工具生成的交付件输出目录固定为 `docs/reports/`，不得写回 `docs/legacy/`。
- 构建产物（`build/`、`tmp/`）不入版本管理，也不作为证据引用。
- 模板与 Logo 位于 `docs/assets/template/`，改动模板需重新生成受影响的全部交付件。
