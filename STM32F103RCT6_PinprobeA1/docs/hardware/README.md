# 硬件域

Status: Active
Domain: HARDWARE
Canonical: `docs/hardware/README.md`
Related: `docs/README.md`, `docs/arch/ARCH_PRODUCT_ARCHITECTURE.md`, `docs/interface/INTERFACE_COMMUNICATION_ARCHITECTURE.md`
Last updated: 2026-10-22

本目录是板级硬件事实与失效分析的唯一入口：IO 位映射与外围模块接口、供电与 USB 输入防护设计、
失效机理分析、网表与数据手册。

## 当前主线

| 当前路径 | 定位 | 使用规则 |
|---|---|---|
| `HARDWARE_IO_CONSTRAINTS.md` | 16 路输入 / 16 路输出的位映射、传感器语义、ModBus 访问口径。 | IO 位定义 canonical（代码宏在 `state_vector.c` / `BsmRelay.h`，两者必须一致）。 |
| `HARDWARE_USB5V_PROTECTION_DESIGN.md` | USB 5V 输入防护方案：钳位器件 + 保险丝取向，不串电阻。 | 接口防护设计 canonical（当前有效方案）。 |
| `HARDWARE_TPS7A2033_FAILURE_ANALYSIS.md` | TPS7A2033 输入侧碳化失效的机理判定与处置要求。 | 失效结论 canonical。 |
| `HARDWARE_USB_REPLUG_A0_ANALYSIS.md` | USB 自动拔插异常 A0 版：含代码定位、设计门禁与待办关联。 | 现场问题分析（A0 口径）。 |
| `HARDWARE_USB_REPLUG_B0_ANALYSIS.md` | 同一异常的 B0 版：面向客户的机构/时序解释。 | 现场问题分析（B0 口径）。 |

## 证据与参考资料

| 当前路径 | 定位 |
|---|---|
| `diagrams/主机供电回路.svg`、`diagrams/从机供电回路.svg`、`diagrams/5V供电路径.svg` | 供电路径图（失效分析引用）。 |
| `evidence/TPS7A2033_failure_vin.png` | 失效板 VIN 侧碳化形貌照片。 |
| `evidence/PCB.png` | 失效板 PCB 照片。 |
| `netlist/Netlist_IO-PPA-CH318TUSBEXT-4F4-HASL-V1.0_2026-10-10.txt` | 供电支路网表（失效分析依据）。 |
| `datasheets/TI_TPS7A20_TPS7A2033_SBVS338H_datasheet.pdf` | TPS7A20 系列数据手册。 |
| `datasheets/BSM系列产品说明书.pdf` | 开关量 IO 模块/继电器模块说明书。 |
| `../reports/hardware/` | 失效分析与防护优化报告的 HTML/PDF 交付件。 |

## 边界

- 硬件域记录「板子是什么样、失效为什么会发生」，不定义动作顺序（归 `docs/control/`）
  与协议帧（归 `docs/interface/`）。
- IO 位映射以代码宏为事实源；本文档是镜像，变更必须在同一次提交内同步。
- 失效分析结论不得作为设计变更的唯一依据：改动前需要复现验证或厂商确认。
