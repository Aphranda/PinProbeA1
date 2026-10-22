# 硬件失效分析与防护报告

Status: Active
Domain: REPORTS
Canonical: `docs/reports/hardware/README.md`
Related: `docs/reports/README.md`, `docs/hardware/README.md`, `docs/hardware/HARDWARE_TPS7A2033_FAILURE_ANALYSIS.md`, `docs/hardware/HARDWARE_USB5V_PROTECTION_DESIGN.md`
Last updated: 2026-10-22

## 当前交付版

| 文件 | 对应 canonical 文档 | 说明 |
|---|---|---|
| `PinProbeA1_TPS7A2033失效初步分析报告_20261010.html` | `docs/hardware/HARDWARE_TPS7A2033_FAILURE_ANALYSIS.md` | TPS7A2033 输入侧碳化失效分析（2026-10-10 版式） |
| `PinProbeA1_TPS7A2033失效初步分析报告_20261012.pdf` | 同上 | 2026-10-12 修订打印版（内容以 md 为准） |
| `PinProbeA1_USB5V输入防护优化报告_20261010.html` | `docs/hardware/HARDWARE_USB5V_PROTECTION_DESIGN.md` | USB 5V 输入防护优化（钳位 + 保险丝方案） |
| `PinProbeA1_USB5V输入防护优化报告_20261010.pdf` | 同上 | 打印版 |
| `PinProbeA1_USB自动拔插异常报告_A0.html` / `.pdf` | `docs/hardware/HARDWARE_USB_REPLUG_A0_ANALYSIS.md` | USB 自动拔插异常分析 A0（含代码定位与门禁） |
| `PinProbeA1_USB自动拔插异常报告_B0.html` / `.pdf` | `docs/hardware/HARDWARE_USB_REPLUG_B0_ANALYSIS.md` | 同一异常 B0（机构/时序口径） |

## 引用资源

报告中的图与照片来自 `docs/hardware/`：

| 资源 | 路径 |
|---|---|
| 主机供电回路图 | `docs/hardware/diagrams/主机供电回路.svg` |
| 从机供电回路图 | `docs/hardware/diagrams/从机供电回路.svg` |
| 5V 供电路径图 | `docs/hardware/diagrams/5V供电路径.svg` |
| 失效形貌照片 | `docs/hardware/evidence/TPS7A2033_failure_vin.png` |
| PCB 照片 | `docs/hardware/evidence/PCB.png` |
| 网表 | `docs/hardware/netlist/Netlist_IO-PPA-CH318TUSBEXT-4F4-HASL-V1.0_2026-10-10.txt` |

## 使用规则

- 报告结论以 `docs/hardware/` 的 md 为准；HTML/PDF 只做呈现。
- 失效照片、网表等证据不可裁剪、不可重命名后替换，保持可追溯。
- 本目录 Markdown（本 README 除外）豁免文档治理元数据门禁。
