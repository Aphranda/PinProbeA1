# 控制域

Status: Active
Domain: CONTROL
Canonical: `docs/control/README.md`
Related: `docs/README.md`, `docs/arch/ARCH_PRODUCT_ARCHITECTURE.md`, `docs/interface/SCPI_COMMANDS.md`
Last updated: 2026-10-22

本目录是「箱体动作怎么执行」的唯一入口：主状态机、门/USB 动作编排、锁定与 LED、
控制模式权限矩阵、IO 链路可信状态判定与时间预算。

## 当前主线

| 当前路径 | 定位 | 使用规则 |
|---|---|---|
| `CONTROL_STATE_MACHINE_ARCHITECTURE.md` | 7 状态主状态机、迁移条件、动作编排顺序、模式权限矩阵、安全例外。 | 控制域 canonical；不写 SCPI 命令字面量（归 `docs/interface/`）。 |
| `CONTROL_TIMING_ANALYSIS.md` | 计时基准、任务周期、去抖/确认/超时预算（2026-06 版分析）。 | 时间口径与 `docs/arch/ARCH_PRODUCT_ARCHITECTURE.md` 冲突时以后者为准。 |
| `CONTROL_TODO.md` | 控制域待办（实机联调、纠偏、证据缺口），状态引用 `docs/PROJECT_TODO.md`。 | 只维护未完成事项，不写流水账。 |

## 相关交付件

| 当前路径 | 定位 |
|---|---|
| `../reports/control/PinProbe A1 状态机与动作控制流程图20260915.html` | 三页 A4 竖版流程图交付件（主状态机、门与 USB 时序、动作与 IO 更新闭环）。 |
| `../reports/control/flowcharts/README.md` | 流程图生成脚本说明与 SVG 索引。 |

## 相关参考

| 当前路径 | 定位 |
|---|---|
| `../arch/ARCH_PRODUCT_ARCHITECTURE.md` | 固件分层、任务优先级与状态事实流向（口径来源）。 |
| `../interface/SCPI_COMMANDS.md` | 触发动作的命令与参数域。 |
| `../interface/INTERFACE_COMMUNICATION_ARCHITECTURE.md` | RS485 链路可信状态的物理来源与判定。 |
| `../hardware/HARDWARE_IO_CONSTRAINTS.md` | 输入/输出位映射（动作与限位传感器的对应关系）。 |

## 边界

- 控制域定义「状态怎么迁移、动作按什么顺序下发」，不定义命令语法（`docs/interface/`）、不定义电气特性（`docs/hardware/`）。
- 安全例外（急停、激光防护、RS485 故障保护、断电回锁）不属于控制模式的管辖范围，任何模式必须放行。
- 控制模式只限制普通门/USB 动作入口，不得用来限制查询、告警读取与维护操作。
