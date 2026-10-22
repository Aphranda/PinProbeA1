# 静态资源

Status: Active
Domain: ASSETS
Canonical: `docs/assets/README.md`
Related: `docs/README.md`, `docs/tools/TOOLS_DOC_TOOLCHAIN_PLAYBOOK.md`, `docs/reports/README.md`
Last updated: 2026-10-22

本目录存放交付件生成链与文档共用的静态资源（模板、Logo）。业务证据（失效照片、网表）
不放这里，归 `docs/hardware/`；上位机截图归 `docs/reports/guide/assets/`。

## 内容

| 路径 | 定位 | 使用方 |
|---|---|---|
| `template/template.html` | GTS A4 竖版模板（含 `gts-logo` 分组、封面、页眉页脚样式） | `Tools/md_to_gts_a4_pages.py --template` |
| `template/logo.png` | 位图 Logo（PPT/封面用） | 交付件封面、PPT |
| `template/logo.svg` | 矢量 Logo（打印用） | 交付件封面 |

## 规则

- 模板是**共享依赖**：任何改动都会影响全部 HTML 交付件，改完必须重新生成并目视检查封面与分页。
- 模板中的 `gts-logo` 分组是生成脚本的硬依赖，删除会导致生成失败。
- Logo 替换需同时更新 png 与 svg 两份，避免打印版与屏幕版不一致。
- 本目录不存放客户专属或项目专属图片；新增通用资源前先确认是否属于某个业务域。
