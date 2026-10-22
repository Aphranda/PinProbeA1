# 接口域

Status: Active
Domain: INTERFACE
Canonical: `docs/interface/README.md`
Related: `docs/README.md`, `docs/arch/ARCH_PRODUCT_ARCHITECTURE.md`, `docs/control/CONTROL_STATE_MACHINE_ARCHITECTURE.md`
Last updated: 2026-10-22

本目录是对外接口的唯一入口：SCPI 命令表、USB CDC 虚拟串口链路、RS485/ModBus RTU 开关量链路，
以及上位机与设备的交互边界。命令「能不能做」由控制域定义，接口域只定义「怎么问、怎么答」。

## 当前主线

| 当前路径 | 定位 | 使用规则 |
|---|---|---|
| `SCPI_COMMANDS.md` | 内部命令表：以 `Hardware/libscpi/port/scpi-def.c` 为事实源整理的已实现命令、参数域与查询项。 | 接口域 canonical；命令实现变更后必须同步。 |
| `INTERFACE_COMMUNICATION_ARCHITECTURE.md` | 物理链路与协议边界：USART1/USB CDC、USART3/RS485 ModBus RTU、帧长与超时。 | 链路参数 canonical。 |
| `INTERFACE_TODO.md` | 接口域待办（交付版 SCPI 源回归、错误码一致性、ModBus 扩展）。 | 只维护未完成事项。 |

## 交付件与相关文档

| 当前路径 | 定位 |
|---|---|
| `../reports/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260924.html` | 当前中文 SCPI 交付版（客户联调参考）。 |
| `../reports/scpi/PinProbe A1 Box Control SCPI Command Reference 20260924.html` | 当前英文 SCPI 交付版。 |
| `../reports/guide/PinProbeA1箱体控制程序操作说明_客户版_20260728.md` | 上位机操作说明（含 SCPI 使用示例）。 |
| `../hardware/HARDWARE_IO_CONSTRAINTS.md` | 输入/输出位映射（查询返回值的物理含义）。 |
| `../legacy/scpi/` | 历史 SCPI 版本（0630/0727/20260728/20260811/20260911/20260912），仅作追溯。 |

## 边界

- 接口域定义命令语法、参数域、返回格式与错误码映射，不定义动作执行顺序（归 `docs/control/`）。
- 交付版 HTML/PDF 属于渲染产物，存放于 `docs/reports/scpi/`，不得作为命令定义的唯一来源。
- 历史版本一律冻结于 `docs/legacy/scpi/`，不得据其推断当前行为。
