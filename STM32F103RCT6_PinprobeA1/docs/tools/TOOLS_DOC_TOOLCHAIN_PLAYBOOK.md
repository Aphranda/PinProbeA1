# 文档生成链操作手册

Status: Active
Domain: TOOLS
Canonical: `docs/tools/TOOLS_DOC_TOOLCHAIN_PLAYBOOK.md`
Related: `docs/tools/README.md`, `docs/docs/DOCS_NAMING_STRUCTURE_PLAN.md`, `docs/reports/README.md`
Last updated: 2026-10-22

本手册给出从 Markdown 源生成 A4 HTML 与 PDF 交付件的可重复步骤。
交付件（`docs/reports/`）只能是本链的产物，不允许手工编辑 HTML 后直接交付。

## 一、链路总览

```text
docs/<domain>/*.md  ──md_to_gts_a4_pages.py──▶  docs/reports/<topic>/*.html
                              │                        │
                              │                        └─html_to_pdf_edge.py──▶ *.pdf
                              └─ a4_layout_check.py（分页溢出预估，可选）
```

| 环节 | 脚本 | 输入 | 输出 |
|---|---|---|---|
| A4 HTML | `Tools/md_to_gts_a4_pages.py` | Markdown + `docs/assets/template/template.html` | `docs/reports/<topic>/*.html` |
| PDF | `Tools/html_to_pdf_edge.py` | HTML | `docs/reports/<topic>/*.pdf` |
| 版式检查 | `Tools/a4_layout_check.py` | HTML/内容 | 控制台溢出报告 |
| 流程图 | `docs/reports/control/flowcharts/generate.mjs`、`generate-a4.mjs` | 源码结构（脚本内描述） | `flowcharts/*.svg` + `docs/reports/control/*.html` |
| 旧版单页模板链 | `Tools/md_to_gts_a4_pdf.ps1` | Markdown + 模板 + 输出路径 | HTML/PDF（保留兼容，不推荐新用） |

## 二、生成 A4 HTML

```powershell
python Tools\md_to_gts_a4_pages.py `
  --markdown "docs\reports\scpi\<源文件>.md" `
  --template "docs\assets\template\template.html" `
  --html "docs\reports\scpi\<输出>.html" `
  --date 2026-10-22 `
  --doc-kind "SCPI Reference" `
  --footer-name "PinProbe A1 文档"
```

常用参数：

| 参数 | 必填 | 说明 |
|---|---|---|
| `--markdown` | 是 | Markdown 源路径 |
| `--template` | 是 | GTS 模板（`docs/assets/template/template.html`） |
| `--html` | 是 | HTML 输出路径 |
| `--date` | 否 | 封面日期，默认 `2026-07-27`，交付时务必显式指定当天日期 |
| `--doc-kind` | 否 | 封面文档类型标签 |
| `--cover-subtitle`、`--footer-name`、`--cover-label`、`--cover-label-sub` | 否 | 封面与页脚文案 |
| `--cover-image`、`--cover-image-alt` | 否 | 封面图（如 `docs/reports/guide/assets/软件主体.png`） |
| `--cover-info` | 否 | 封面信息行（可多组） |
| `--no-cover` | 否 | 省略封面页 |

注意：

- 模板中的 `gts-logo` 分组是必需的，缺失会直接抛错；改模板后必须重生成全部交付件。
- Markdown 中的 `:::chips` 等扩展语法由该脚本解析，不是标准 Markdown。
- 表格列宽与字体由模板 CSS 决定，调版式改模板而不是改输出 HTML。

## 三、生成 PDF

```powershell
python Tools\html_to_pdf_edge.py `
  --html "docs\reports\scpi\<输出>.html" `
  --pdf  "docs\reports\scpi\<输出>.pdf"
```

- 依赖本机 Microsoft Edge（脚本按两个固定路径探测）。
- 使用临时用户数据目录与无头打印，A4 竖版边距由 HTML 的 `@page` 决定。
- 生成后**必须**目视检查首页封面与末页，Edge 版本变化可能影响分页。

## 四、分页溢出检查

```powershell
python Tools\a4_layout_check.py --help
```

该脚本按 `md_to_gts_a4_pages.py` 的 CSS 模型估算每页内容高度，用于提前发现溢出。
模型是近似的：距预算 10 mm 以内的页面视为「必须人工复核」。它不能替代目视检查。

## 五、流程图生成

```powershell
cd docs\reports\control\flowcharts
node generate.mjs        # 生成 01/02/03 三张交互版 SVG
node generate-a4.mjs     # 生成 a4-01..03 打印版 SVG，并输出 ../PinProbe A1 状态机与动作控制流程图20260915.html
```

- `generate-a4.mjs` 的 HTML 输出目标是**上一级目录**（`docs/reports/control/`），依赖目录结构保持不变。
- SVG 内标注的源码行号（如 `state_vector.c:627`）是绘制时快照；代码变动后需重新生成，
  否则交付件与代码脱节（见 `docs/PROJECT_TODO.md` 的文档漂移项）。

## 六、交付前检查表

- [ ] 生成命令中的 `--date` 为交付当天日期。
- [ ] HTML 与 PDF 同源、同名、同目录。
- [ ] 首页封面、页眉、页脚、页码连续且无溢出。
- [ ] 表格没有跨页断行错误，图片没有拉伸变形。
- [ ] 交付件中的命令/参数与 `docs/interface/SCPI_COMMANDS.md` 一致。
- [ ] 交付件放回 `docs/reports/<topic>/`，旧版本移入 `docs/legacy/<topic>/`（禁止就地覆盖历史版本）。
- [ ] 更新所在域的 README 索引与 `docs/reports/README.md` 的「当前交付版」表。

## 七、已知问题

| 编号 | 问题 | 影响 |
|---|---|---|
| `TOOLS-Q1` | 当前 SCPI 交付版（20260924）无 Markdown 源，无法由本链重建。 | 交付件不可追溯，见 `docs/interface/INTERFACE_TODO.md` `INTERFACE-01`。 |
| `TOOLS-Q2` | `Tools/md_to_gts_a4_pdf.ps1`（PowerShell 旧链）与 `md_to_gts_a4_pages.py` 并存，行为不完全一致。 | 新交付件统一用 `.py` 链，`.ps1` 仅保留历史可复现性。 |
| `TOOLS-Q3` | 流程图生成脚本未纳入自动化检查，源码行号漂移不会报警。 | 需人工在代码变更后重生成。 |
