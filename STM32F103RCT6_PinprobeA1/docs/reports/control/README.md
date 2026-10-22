# 状态机与动作控制流程图交付件

Status: Active
Domain: REPORTS
Canonical: `docs/reports/control/README.md`
Related: `docs/reports/README.md`, `docs/control/CONTROL_STATE_MACHINE_ARCHITECTURE.md`, `docs/tools/TOOLS_DOC_TOOLCHAIN_PLAYBOOK.md`
Last updated: 2026-10-22

## 当前交付版

| 文件 | 说明 |
|---|---|
| `PinProbe A1 状态机与动作控制流程图20260915.html` | 三页 A4 竖版流程图：主状态机、门与 USB 动作时序、动作执行与 IO 更新闭环。流程图内嵌，无外部资源依赖。 |
| `PinProbe A1 状态机与动作控制流程图20260915.pdf` | 打印版 |

## 生成链

| 文件 | 说明 |
|---|---|
| `flowcharts/generate.mjs` | 生成交互版 SVG：`01-state-machine.svg`、`02-door-usb-sequence.svg`、`03-action-update.svg` |
| `flowcharts/generate-a4.mjs` | 生成打印版 SVG（`a4-01..03`）并输出本目录的 A4 HTML |
| `flowcharts/README.md` | 流程图内容说明与源码行号索引 |

```powershell
cd docs\reports\control\flowcharts
node generate.mjs
node generate-a4.mjs
```

`generate-a4.mjs` 的输出目标是上一级目录（本目录），依赖此相对结构，移动目录时必须同步改脚本。

## 使用规则

- 图中标注的源码行号与状态名为**绘制时快照**，代码改动后必须重新生成，否则会误导读者。
- 交付件对应的语义定义以 `docs/control/CONTROL_STATE_MACHINE_ARCHITECTURE.md` 为准；
  两者冲突时先修文档/代码，再重生成流程图。
- 本目录 Markdown（本 README 除外）豁免文档治理元数据门禁。
