# PinProbeA1 USB自动拔插异常报告 A0
Status: Active
Domain: HARDWARE
Canonical: `docs/hardware/HARDWARE_USB_REPLUG_A0_ANALYSIS.md`
Related: `docs/hardware/HARDWARE_USB_REPLUG_B0_ANALYSIS.md`, `docs/control/CONTROL_TODO.md`, `docs/PROJECT_TODO.md`
Last updated: 2026-10-22

文档更新时间：2026-10-10 ｜ 版本 A0 ｜ 口径：含代码定位、设计门禁与待办关联 ｜ 依据：上位机连续开关门测试日志（T+32098.629～T+32145.304）、现场机构说明（门末段缓冲棉条）与当前固件源码核对。

:::chips
现象 | 门开到位后 USB 未自动回退
工况 | 上位机指令连续开关门
判定 | 回退命令未投递（非执行失败）
根因定位 | 自动拔出仅挂 IDLE，流程停在 RUNNING
:::

:::note
**结论：** 机械与传感链路正常（最后一次未被打断的关门 3850 ms 正常到底）。自动回退的唯一投递点在 `state_vector.c:822-838`，其准入条件为 `system_status == V_STATE_IDLE`（`:831`）；指令连续开关门时流程整段停在 `RUNNING`（`stay=57550ms` 反推 T+32084.754 进入），该分支一次都没有执行，因此**从未投递**，且无任何日志或告警。
:::

:::todo
**唯一待确认项：** 按"开门 → 等门到底（日志出现 `CLOSE_DONE`）→ 再开门"复现一次并保留开门后的原始日志。出现 `OPEN_DONE`/`COMPLETE -> IDLE` 仍不回退 → 查 IDLE 下的其它 guard；不出现 → 与本文结论一致，直接进入第 4 章实现。
:::

## 一、现象与判定

| 项目 | 内容 |
| --- | --- |
| 现象 | 门开到位后 USB 保持插入；全段无 `CYLINDER_OPEN cyl=2`、无 `USB_RETRACT_FAIL`、无状态回退日志 |
| 工况 | 压测预设"开关门循环"：`CONFigure:CYLInder1 OPEN` / `CLOSE` 交替，周期约 3.0 s；随后人工补发一次开门 |
| 排除 | !!USB 路故障!!：该路全程未被驱动过（`cmd_exec` 侧无下发电平变化）；!!门/气缸故障!!：最后一次关门 3850 ms 正常到底 |
| 判定 | 回退**未投递**，属流程准入问题，不是投递后的执行失败 |

## 二、证据链（同一份日志可逐条复核）

| 观测项 | 日志值 | 推导 |
| --- | --- | --- |
| 流程停留 | `RUNNING -> COMPLETE stay=57550ms` | 进入 RUNNING = 32142.304 − 57.550 = **T+32084.754 s** |
| 日志窗口 | T+32098.629 → T+32142.304（44 s） | 整段落在**同一次** RUNNING 内，期间无其它状态迁移 |
| 门在上限位 | `CLOSE_PHASE ... phase=0` 出现 8 次 | 该日志只在 `system_status == V_STATE_RUNNING` 分支打印（`:842-855`）→ 门到顶时状态是 RUNNING |
| 门关到底 | `phase=2` 仅 1 次 + `CLOSE_DONE duration=3850ms` | 全行程 3850 ms（末段为缓冲棉条减速段）；**只有最后一次到过下限位** |
| 指令周期 | `CLOSE` → `OPEN` = 3.0 s（6 次全部一致） | 3.0 s < 3.85 s，**必然**在约 74% 处被打断 |
| 学习值 | `point=2/3` 在 `el=2625 ms` 触发 | `door_close_default_ms ≈ 3.9 s`，与实测 3.85 s 一致（学习的先决条件是"从上限位出发且真正到底"，循环期间未更新） |
| USB / 安全 | 0 条 `cyl=2`；无 `EMERGENCY`、无 `LASER` | 回退未投递；安全链路未介入 |
| 气压 | `AIR_LOW low_after=3000ms` | 与"防夹窗口内未触发急停"互证（`IS_ANY_LASER` 含气压位） |

<!-- page-break -->

## 三、根因分析

**触发条件：** 门体末段装配缓冲棉条，门到位前减速缓冲，末段限位触发偏慢，全行程 3850 ms（现场机构说明）。指令周期 3.0 s 短于全行程，6 次关门全部在约 74% 处被下一条开门打断，**打断点落在缓冲段内**。

**直接原因：** 自动回退的唯一投递点 `state_vector.c:822-838` 要求 `RamVector_GetCylinderCmd() == VCMD_NONE`（`:824`）、`system_status == V_STATE_IDLE`（`:831`）、`IS_DOOR_UP`、`!USB_IO_RETRACT_OK` 等。流程整段处于 RUNNING，`:831` 恒假 → 分支从未执行。状态查询显示"USB 已插入"与此不矛盾：气缸镜像在 `:791-806` 有一条只看上下位传感的兜底判断。

**结构性缺口（本次要闭环的部分）：**

| 编号 | 缺口 | 代码位置 | 后果 |
| --- | --- | --- | --- |
| G1 | RUNNING 无位置出口；步骤 3 纠偏块只覆盖 INIT/LOCK/IDLE/READY | `:583-602`、`:886-920` | 门被开回上限位后无处可去，流程长期停留且静默 |
| G2 | COMPLETE → IDLE 要求"开门输出 + 上限位**同拍**"，比 IDLE 自身判定（只看上限位）更严 | `:924-934` vs `:593` | 开门那一拍未被观测到时永久停在 COMPLETE |
| G3 | 未投递/未到位无日志无告警；F1 在门开到位时会补投安全优先级回退，F2 无对应重试 | `:767-790`、`:696/731/757` | 现场只能看到"USB 没动"，且一次失败后不再尝试 |
| G4 | 入口不对称：门 CLOSE 走 `VCMD_DOOR_CLOSE_REQ` 由状态机编排，门 OPEN 为裸 `VCMD_CYLINDER_OPEN` | `scpi-def.c:349` vs `:351` | 指令驱动开关门必然把状态机推入 RUNNING 而无人拉回 |

## 四、纠偏方案（实现口径）

插在步骤 3 纠偏块之后（`state_vector.c:602` 之后，所需变量在该处均已可用）：

```text
if ((system_status == V_STATE_RUNNING || system_status == V_STATE_COMPLETE) &&
    IS_DOOR_UP(in_lo) && !IS_DOOR_CLOSING(out_lo) &&
    !IS_LED_YELLOW_STATE(led_state) && !door_ready_tick &&
    !door_close_request_pending && !close_pending_after_usb &&
    !door_close_armed_after_usb) {
    if (door_close_timing) { door_close_timing = 0; door_close_start_tick = 0; }
    door_close_done_tick = 0;
    poweron_position_ok  = 0;
    m_23 = m_100 = m_300 = false;
    system_status = V_STATE_IDLE;   /* 同拍后续 :822-838 即可投递 USB 回退 */
}
```

设计约束：

1. **必须带 `!IS_DOOR_CLOSING(out_lo)`**：正常关门起始那一小段 `IS_DOOR_UP` 仍为真（本机实测 350 ms 后才离开上限位，`:613` 的 `door_close_start_tick` 赋值也特意带了 `!IS_DOOR_UP`），否则每次关门都会被自己打断。
2. **veto 列表必须与 `:825-830` 的 `ready_intent` 对齐**（黄灯 / 按钮 / `door_ready_tick` / `close_pending_after_usb` / `door_close_armed_after_usb` / `door_close_request_pending`），否则会与"关门待生效保护"（`:1019-1024`，最多 1 s）打架。
3. **判定以位置为主、不要求开门输出同拍**：这正是 G2 的修法，兼容裸命令开门、输出被脉冲清零、掉气后门体上浮三种场景。
4. **放在 `:812` 之前**：纠偏当拍落到 IDLE，同拍后面的 `:822-838` 就会投递 `VCMD_CYLINDER2_OPEN`，拔 USB 自动恢复，无需另加 USB 逻辑。
5. **O3/O4 的落点**：未投递时打一条一次性诊断（输出被哪一条 guard 短路）+ `USB_RETRACT_FAIL` 后允许在门上限位有效时重试一次；`usb_fault` 目前会把 `:822` 整段永久关掉，恢复只能靠到位观测。

<!-- page-break -->

## 五、验证与复现

| 观测点 | 期望 |
| --- | --- |
| 复现 A：关门到位（出 `CLOSE_DONE`）后再开门 | 出现 `OPEN_DONE` + `COMPLETE -> IDLE`；加纠偏前若二者都无，则 G2 命中 |
| 复现 B：关门途中开门（复刻压测节奏） | 加纠偏后出现 `RUNNING -> IDLE` 紧跟 `CYLINDER_OPEN cyl=2`；未加前两者都不出现 |
| 关键字 grep | `OPEN_DONE`、`COMPLETE -> IDLE`、`RUNNING -> IDLE`、`CYLINDER_OPEN cyl=2`、`USB_RETRACT_FAIL`、`EMERGENCY` |
| 状态查询 | 门开到位时 `READ:SYSTem:STATe?` = `IDLE`；`READ:CYLInder2:STATe?` 数秒内 `OPENED` |
| IO 核对 | `READ:IO:ALL?`：OUT IO1(开门)/IO3(USB 插入)/IO4(USB 回退)、IN USB 上/下位 |
| 前置条件 | `READ:DEBUg:STATe?` / `READ:DEBUg:ACTion?` 必须为 ON，否则 `OPEN_DONE` 与状态行本来就不打印 |

复现脚本侧只需把压测预设改成"发 `CLOSE` 后等 `READ:CYLInder1:STATe?` 到 `CLOSED`（或日志出 `CLOSE_DONE`）再发 `OPEN`"，用于分离"节奏问题"与"流程问题"。本项对应待办 `P2.4`（USB 气缸状态闭环实机验证）：本次为其首次现场暴露，A1～A4 可作为该工作包的验收基线。

## 六、附带发现（待确认，安全相关）

1. **气压位与防夹位共用聚合判断**：`IS_ANY_LASER`（`:116`）把 `IN_LASER1`（气压）与激光 2/3/4 一起 OR。本次因气压位为 0 才没有在关门中段误触发；气压位为正常电平时该聚合恒真，而正常关门本来就要走到接近 100% 才到底 → **需结合接线与配置核对后再决定是否拆分聚合条件**，不并入本次整改。
2. **风险模式兜底依赖气压位**：`risk_ok`（`:887-891`）要求 `pressure_sensor_level = in_lo & IN_LASER1` 为 1，气压位为 0 时即使限位失效也兜不住。
3. **防夹窗口与缓冲段的关系**：窗口起点为 `de > door_close_default_ms * 2/3`（`:999-1000`），受末段缓冲影响，该起点对应的实际行程位置比几何 2/3 更靠后，需按机构实测复核覆盖区间。

## 七、落地顺序与门禁

`docs/PROJECT_TODO.md`（P0.1）明确规定：未通过设计门禁前不修改 `scpi-def.c`、`state_vector.c` 与 `RamVector` 控制入口。本章方案全部落在 `state_vector.c` 步骤 3，建议顺序：① 设计确认（纠偏条件与安全边界复核，不改安全链路）→ ② 只加第 4.5 条诊断日志先取现场数据 → ③ 实现 O1/O2 纠偏 → ④ 按第 5 章 A/B 两组复现验证 → ⑤ 再补 O3/O4 告警与重试。B0 版见《PinProbeA1 USB自动拔插异常报告 B0》。

:::bar
本次为**流程动作未投递**：唯一投递点 `state_vector.c:822-838` 被 `:831` 的 IDLE 条件短路，根因是 RUNNING 无位置出口 + 回退只挂 IDLE；机械与传感链路正常，整改不触碰安全链路。
:::
