# 操作说明交付件

Status: Active
Domain: REPORTS
Canonical: `docs/reports/guide/README.md`
Related: `docs/reports/README.md`, `docs/interface/INTERFACE_COMMUNICATION_ARCHITECTURE.md`, `docs/tools/README.md`
Last updated: 2026-10-22

## 当前交付版

| 文件 | 语言 | 说明 |
|---|---|---|
| `PinProbeA1箱体控制程序操作说明_客户版_20260728.md` | 中文 | 客户版操作说明源 Markdown（含界面截图） |
| `PinProbeA1箱体控制程序操作说明_客户版_20260728.html` | 中文 | A4 竖版交付件 |
| `PinProbeA1箱体控制程序操作说明_客户版_20260728.pdf` | 中文 | 打印版 |
| `PinProbeA1箱体控制程序操作说明_20260728.pdf` | 中文 | 内部/客户包内同名 PDF（内容为同一版的内部别名） |
| `PinProbeA1 Box Control Program Operation Guide Customer 20260728.html` | 英文 | 英文客户版 |
| `PinProbeA1 Box Control Program Operation Guide.pdf` | 英文 | 英文打印版（无 md 源） |

## 历史版本

| 文件 | 说明 |
|---|---|
| `PinProbeA1箱体控制程序操作说明_客户版_20260727.md` | 上一版中文说明（保留在交付目录，避免图片相对路径失效） |

## 图片资源

`assets/` 下的截图被上述 Markdown 以相对路径 `assets/...` 引用：

| 文件 | 用途 |
|---|---|
| `assets/pinprobe_gui_main.png` | 上位机主界面 |
| `assets/pinprobe_gui_ota.png` | 固件升级界面 |
| `assets/软件主体.png` | 软件主体/封面图 |
| `assets/烧录区域.png` | 烧录区域说明图 |

移动本目录时必须连同 `assets/` 一起移动，否则 md 中的图片引用全部失效。

## 使用规则

- 截图随上位机界面变更而失效，界面改版后必须重新截图并更新 md，再重新生成 HTML/PDF。
- 英文版当前无 md 源，改版时需同步补齐（见 `docs/docs/DOCS_MIGRATION_TODO.md` `DOCS-MIG-06`）。
- 本目录 Markdown（本 README 除外）豁免文档治理元数据门禁。
