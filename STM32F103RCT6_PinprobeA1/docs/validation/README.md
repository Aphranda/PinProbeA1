# 验证与证据域

Status: Active
Domain: VALIDATION
Canonical: `docs/validation/README.md`
Related: `docs/README.md`, `docs/PROJECT_TODO.md`, `docs/control/CONTROL_TODO.md`
Last updated: 2026-10-22

本目录存放验证方法、审查结论与证据：代码审查基线、回归验证、压力测试记录、实机联调证据。
它是 `docs/PROJECT_TODO.md` 中「已完成 / 已实现待验」判定的证据来源。

## 当前主线

| 当前路径 | 定位 | 使用规则 |
|---|---|---|
| `VALIDATION_CODE_REVIEW_20260613.md` | 全量静态代码审查报告（Core / Hardware / .eide / MDK-ARM），含问题清单与建议。 | 最近一次审查基线；结论变更需新报告，不就地重写。 |
| `VALIDATION_RISK_REVIEW_20260612.md` | 代码框架风险审查报告：分层、并发、资源与可维护性风险。 | 风险评审快照。 |
| `VALIDATION_TASK_PROGRESS.md` | 验证域任务进度与证据记录（`VALIDATION-TASK-*`）。 | 新验证证据追加于此；域级证据稳定后可拆分为 `<DOMAIN>_TASK_PROGRESS.md`。 |

## 证据分级

| 等级 | 含义 | 可用于 |
|---|---|---|
| `E1` 静态审查 | 只读代码/文档得出的结论，未编译未实机 | 缺陷线索、风险登记 |
| `E2` 构建验证 | 编译通过 + 目标文件产物 | 「已实现」判定 |
| `E3` 台架验证 | 单机功能自发自收、串口日志可复现 | 「已实现待验」→「台架通过」 |
| `E4` 实机联调 | 客户机构完整动作序列、上下位机联动 | 「已完成」判定 |
| `E5` 现场长期 | 现场连续运行与回归 | 发布放行 |

未达 `E4` 的功能不得在 `docs/PROJECT_TODO.md` 标注「已完成」。

## 常用验证手段

| 手段 | 工具 | 记录方式 |
|---|---|---|
| 编译与产物核对 | EIDE `build` 任务、`unify_builder.exe` | ROM/RAM 占用增量写入任务记录 |
| 命令面验证 | 上位机 `Tools/pinprobe_gui.py`、串口终端 | 命令 + 应答 + 错误码三元组 |
| 异常注入 | 拔插 USB、断开 RS485、遮挡激光、按急停 | 对应 `AppLog` 事件编号 |
| 压力测试 | 连续开关门循环 | 循环次数 + 失败点 |
| 日志回读 | `SYSTem:ERRor?`、AppLog 查询命令 | 时间戳序列 |

## 边界

- 验证域记录「验证了什么、怎么验证、结果如何」，不定义应实现的行为（归各业务域）。
- 审查报告是快照：新结论另立文件，不追溯修改旧报告。
- 证据必须可复现：写明命令、环境、期望结果与实际结果。
