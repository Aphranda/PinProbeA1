# 文档与执行约束

Status: Active
Domain: DOCS
Canonical: `docs/docs/DOCS_EXECUTION_CONSTRAINTS.md`
Related: `docs/PROJECT_TODO.md`, `docs/docs/DOCS_REGISTRY.md`, `docs/control/CONTROL_STATE_MACHINE_ARCHITECTURE.md`
Last updated: 2026-10-22

本文档定义跨会话、跨执行者（人 / AI 助手 / 外部协作方）的长期执行约束。
约束来源于本项目的实际事故与门禁要求，不是风格偏好；违反时先停下补证据，再继续改动。

## 一、设计门禁（C1）

- `P0.1` 设计冻结是代码前置条件：未通过前不得修改 `Hardware/libscpi/port/scpi-def.c`、
  `Hardware/RamVector/Src/state_vector.c` 与 `RamVector` 控制入口。状态见 `docs/PROJECT_TODO.md`。
- 涉及安全链路（急停、激光防护、RS485 故障保护、断电回锁）的改动，必须先给出边界说明和复现步骤，
  不允许「顺手改一点」。
- 控制模式只限制普通门/USB 动作入口，不得削弱任何安全例外。

## 二、证据闭环（C2）

- 任何「已完成」判断必须附可复现证据：编译输出、串口日志、上位机截图、或实机测试记录。
- 未通过实机联调的事项只能标 `已实现待验`，不得标 `已完成`。
- 文档中的数值（超时、周期、引脚、地址、版本号）必须能在代码或网表中找到对应事实源；
  找不到时标注「待核实」，不得凭空填写。

## 三、文档-代码一致性（C3）

- 事实源在代码、镜像在文档的条目见 `docs/docs/DOCS_REGISTRY.md` 第二节；
  代码改动与文档更新放在同一次提交内。
- 交付件（`docs/reports/`）与 canonical 文档冲突时，以 canonical 为准，并登记漂移项到 `docs/PROJECT_TODO.md`。

## 四、改动边界（C4）

- 不混合功能改动与大规模文档/文件移动。
- 不删除历史资料；确需废弃时标 `Deprecated` 并说明替代文档。
- 不动客户交付件文件名（`docs/reports/`、`docs/release/deliverables/`），只允许更换所在目录。
- 构建产物、临时预览目录（`tmp/`、`build/`）不纳入版本管理，也不作为证据引用。

## 五、提交前自检（C5）

```powershell
git diff --check                  # 空白与换行
python Tools\docs_check.py        # 文档门禁
```

- 一次提交只解决一类问题，commit message 使用 `docs:` / `feat:` / `fix:` 前缀。
- 涉及文档迁移时，额外执行 `rg -n "Doc/"` 确认没有旧路径残留（迁移记录本身除外）。
