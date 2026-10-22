# 待办事项
Status: Active
Domain: PROJECT
Canonical: `docs/PROJECT_TODO.md`
Related: `docs/README.md`, `docs/control/CONTROL_TODO.md`, `docs/interface/INTERFACE_TODO.md`, `docs/multinode/MULTINODE_TODO.md`, `docs/ota/OTA_TODO.md`, `docs/validation/VALIDATION_TASK_PROGRESS.md`
Last updated: 2026-10-22

> 2026-09-12 重整：按“当前主线门禁 → 分层工作包 → 验收基线”组织。当前 `P0` 主线为控制模式权限分级；设计冻结、固件实现和编译验证已完成，实机联调待验。

## 一、优先级与执行规则

| 优先级 | 目标 | 进入条件 |
|---|---|---|
| `P0` | 安全与控制边界，当前为控制模式主线 | 必须先完成设计冻结、实现和死锁回归 |
| `P1` | 运行基础、并发与协议 | `P0` 验收通过后进入 |
| `P2` | 功能闭环、诊断与现场验证 | `P1` 相关接口稳定后进入 |
| `P3` | OTA 与硬件扩展 | 单机控制闭环稳定后进入 |
| `P4` | 工程质量与发布 | 主要功能冻结后集中处理 |
| `M` | 多机阶段 | 单机版本稳定后统一规划 |

执行规则：

1. 每个工作包都必须同时写清楚“实现内容”和“完成判据”，不能只记录文件名。
2. `P0.1` 是代码前设计门禁；未通过前不修改 `scpi-def.c`、`state_vector.c` 和 `RamVector` 控制入口。
3. `P0` 工作包按 `P0.1 → P0.2 → P0.3 → P0.4 → P0.5` 顺序推进。
4. 普通动作和安全动作分开处理：模式只限制普通门/USB控制入口，不得削弱急停、激光防夹、RS485故障保护和断电回锁。
5. 变更完成后先做静态审查和编译，再做通信、并发、异常和实机验证；未通过验收的事项不能标记为完成。

## 二、当前主线：控制模式权限分级（P0）

### 2.1 模式功能契约与权限边界

| 入口或能力 | `LOCAL` | `REMOTE` | `MIXED` |
|---|---|---|---|
| 物理按键执行普通门/USB动作 | 允许 | 禁止 | 允许 |
| SCPI执行普通门/USB动作 | 禁止 | 允许 | 允许 |
| 状态、告警、错误队列、日志查询 | 允许 | 允许 | 允许 |
| IO、传感器、版本和诊断查询 | 允许 | 允许 | 允许 |
| 急停、激光防夹、RS485故障保护 | 始终有效 | 始终有效 | 始终有效 |
| 断电回锁及其他安全状态机 | 始终有效 | 始终有效 | 始终有效 |
| 锁定/解锁、必要维护操作 | 独立安全/维护权限 | 独立安全/维护权限 | 独立安全/维护权限 |

模式定义：

- `LOCAL`：本地手动控制模式。普通门/USB动作只能由面板或物理按键发起，SCPI普通动作必须被拒绝；查询和安全保护不受影响。
- `REMOTE`：远程软件控制模式。普通门/USB动作只能由SCPI发起，物理按键普通动作必须被屏蔽；查询和安全保护不受影响。
- `MIXED`：兼容模式。物理按键和SCPI均可执行普通动作，保持当前系统行为；上电默认进入该模式。

### 2.2 P0 工作包

| 编号 | 阶段 | 状态 | 工作内容 | 完成判据 | 主要位置 |
|---|---|---|---|---|---|
| `P0.1` | 设计冻结 | 已完成 | 固化三模式定义、权限矩阵、查询例外、安全动作例外、锁定/解锁独立权限，以及模式切换时的动作处理规则；确认指令为 `CONFigure:MODE`、`READ:MODE?`、`CONFigure:MODE:ENABle`、`READ:MODE:ENABle?` | 内部指令表、客户说明和本待办中的模式语义一致；明确所有拒绝条件、返回错误和恢复路径；完成代码前并发/死锁设计审查 | `docs/interface/SCPI_COMMANDS.md`、`docs/reports/scpi/` |
| `P0.2` | 运行时架构 | 已完成 | 建立统一的模式状态访问接口；区分当前运行模式与 Flash 中的 `mode_enable`；明确 SCPI任务、StateVector任务和ModBusTask的读写边界；设计原子快照、切换时序和命令槽清理策略 | 模式状态不会被并发读写破坏；临界区内不执行Flash、RS485或可能阻塞的操作；不存在“持锁等待任务、任务反向等待锁”的闭环 | `scpi-def.c`, `state_vector.c`, `ram_vector.c/h`, `cmd_exec.c` |
| `P0.3` | 指令与配置实现 | 已完成 | 实现四条模式指令；当前模式不保存到Flash，上电默认`MIXED`；`mode_enable`掉电保存；保存失败时运行时配置回滚；禁止形成`REMOTE + mode_enable OFF`；始终保留`MODE LOCAL`和`MODE:ENABle ON`恢复路径 | 合法参数、非法参数、重复设置、Flash异常和配置读取均有确定结果；通信中断或保存失败不会造成控制权丢失 | `scpi-def.c`, `Hardware/Flash/Inc/flash.h`, `Hardware/Flash/Src/flash.c` |
| `P0.4` | 权限入口实现 | 已完成 | 在SCPI普通气缸动作写入`RamVector`前进行模式检查；在StateVector物理按键入口进行模式检查；拒绝时不写入、不遗留pending请求；模式切换时清理旧普通动作，已开始的安全动作不被取消 | `LOCAL`、`REMOTE`、`MIXED`三种模式的SCPI/物理按键行为符合矩阵；查询始终可用；安全事件始终可执行 | `scpi-def.c`, `state_vector.c`, `ram_vector.c/h`, `cmd_exec.c` |
| `P0.5` | 并发与死锁回归 | 已完成 | 验证模式切换、动作进行中切换、按钮长按、SCPI连续发送、通信中断、急停、激光触发、RS485故障和断电回锁；验证切回`LOCAL`后需按钮释放并重新操作 | 已通过编译、静态审查和实机验证；无死锁、无旧命令重放、无普通命令越权，安全动作保持有效 | 固件、`Tools`、实机 |

模式控制主线的最小验收矩阵：

```text
MIXED → 当前物理按键和 SCPI 普通动作行为不变
MIXED → LOCAL → 物理按键可用，SCPI 普通动作拒绝
MIXED → REMOTE → SCPI 普通动作可用，物理按键普通动作拒绝
LOCAL → mode_enable OFF → 可设置；REMOTE → mode_enable OFF → 不允许形成
LOCAL/REMOTE/MIXED → 告警、日志、状态、IO、传感器和版本查询均可读取
动作进行中切换 → 不取消已开始的安全动作，不重放旧普通动作
通信中断/急停/激光/RS485故障/断电回锁 → 仍保留恢复控制权的路径
```

## 三、P1 — 运行基础、并发与协议

| 编号 | 工作内容 | 完成判据 | 主要位置 |
|---|---|---|---|
| `P1.1` | USART3/RS485独占事务：维护命令与ModBusTask互斥；link fault时命令不得先take后丢 | 事务所有权清晰；故障时返回明确错误；普通动作和维护命令不会互相覆盖或永久等待 | `main.c`, `scpi-def.c`, `cmd_exec.c` |

## 四、P2 — 功能闭环、诊断与现场验证

| 编号 | 工作内容 | 完成判据 | 主要位置 |
|---|---|---|---|
| `P2.1` | SCPI命令错误反馈全覆盖检查 | 参数错误、范围错误、状态冲突、模式拒绝、通信失败和Flash失败都能进入统一错误队列；错误不会被静默吞掉 | `scpi-def.c` |
| `P2.2` | IO板型号识别：读取ModBus寄存器地址0，与预期型号比对并纳入POST | 型号异常能被记录、上报并阻止不匹配硬件进入正常控制流程 | `RS485.c`, `main.c` |
| `P2.3` | 出厂测试模式：一键执行IO、传感器和通讯自测 | 每项测试有超时、结果、错误记录和结束状态；失败后不会遗留气缸动作 | 新模块 |
| `P2.4` | USB气缸状态闭环实机验证 | 验证USB插入/回退、到位判断、READY/COMPLETE流程、超时日志以及带/不带USB气缸的出厂配置 | `state_vector.c`, `ram_vector.c/h`, `scpi-def.c`, 实机 |
| `P2.5` | SCPI、物理按键、自动流程和异常保护的组合回归 | 覆盖门/USB手动动作、自动流程、状态返回、锁定、告警和模式切换，不出现状态假象或重复动作 | 固件、`Tools`、实机 |
| `P2.6` | 开门后USB未自动弹出：IDLE位置纠偏（2026-10-10 固件已改，**待客户现场验证**） | 现象：关门途中被开门打断（压测周期3.0 s < 关门全行程3.85 s），门回上限位后状态停在`RUNNING`，USB自动回退只在`IDLE`投递，因此从未弹出。改动：步骤3在READY判定后加一段IO观测纠偏——`RUNNING/COMPLETE` + 门上限位 + 非下限位 + 开门输出有效 + 无关门输出 → 关灯、补`OPEN_DONE`、清关门计时 → `IDLE`，同拍由既有USB自动绑定投递回退；不改RUNNING语义、开门沿、USB逻辑和安全链路。完成判据：① 关门途中开门，门到上限位出现`RUNNING -> IDLE`紧跟`CYLINDER_OPEN cyl=2`，`READ:CYLInder2:STATe?` 2 s内为`OPENED`；② 关门到底后开门出现`OPEN_DONE`、`COMPLETE -> IDLE`并回退；③ 正常按钮/SCPI关门仍以`CLOSE_DONE`结束、中途无`-> IDLE`；④ 激光/急停仍进入`EMERGENCY`；⑤ 原压测脚本≥100次无卡死、无`USB_*_FAIL`。前置：`READ:DEBUg:STATe?`/`ACTion?`为ON。详见《PinProbeA1 USB自动拔插异常报告 A0》 | `state_vector.c`, 客户现场 |
| `P2.7` | 门动作状态语义与监测补全（P2.6现场通过后再做） | ① `RUNNING`按客户说明覆盖开门动作（开门沿进入RUNNING并作废本次关门计时，回开途中防夹窗口不再保持）；② 关门/开门行程超时告警；③ `usb_fault`锁存及SCPI关门pending导致回退永久被挡的恢复路径，静默置故障分支补日志；④ `IN_LASER1`气压位在`IS_ANY_LASER`/`risk_ok`/`AIR_LOW`中电平语义统一（需现场接线确认）；⑤ USB位置传感消抖、PULSE输入USB方向掩码核对。每项单独设计评审、编译与实机回归 | `state_vector.c`, `input_state.c`, 实机 |

## 五、P3 — OTA 与硬件扩展

| 编号 | 工作内容 | 完成判据 | 主要位置 |
|---|---|---|---|
| `P3.1` | OTA固件升级：W25Q128暂存、SCPI传输、Bootloader写入；后续支持CAN分发 | 升级过程有完整性校验、超时、取消和失败恢复；APP搬运失败时不会因喂狗或状态错误卡死 | 新模块、`Bootloader/Src/boot_main.c` |
| `P3.2` | OTA诊断状态一致性 | App确认运行后正确更新持久化状态；`SYSTem:OTA:BOOT?`与实际boot flags一致 | `ota_manager.c`, `scpi-def.c` |
| `P3.3` | OTA分块接收严格校验 | 非末尾块必须为固定块长，末尾块等于剩余长度；非法短写不会被标记为已接收 | `ota_manager.c` |
| `P3.4` | OTA重复块处理 | 相同offset重复块必须比较payload；不一致时返回错误并要求重新开始或重写 | `ota_manager.c` |
| `P3.5` | Bootloader擦除策略 | 写入新APP前清理完整APP分区至配置区边界，避免旧尾部残留 | `boot_main.c` |
| `P3.6` | 掉电检测 | PVD触发后停止气缸、进入安全状态并保存必要状态；恢复后不得自动重放普通动作 | `main.c`, `state_vector.c` |

## 六、P4 — 工程质量与发布

| 编号 | 工作内容 | 完成判据 | 主要位置 |
|---|---|---|---|
| `P4.1` | 启用散列文件：`useCustomScatterFile: true` | Debug/Release均能正确链接、下载和启动 | `.eide/eide.yml` |
| `P4.2` | 编译warning清零与MISRA-C基础检查 | 目标配置无新增warning；关键控制和Flash代码完成静态检查记录 | 各模块 |

---

## 七、多机阶段（CAN 组网时统一做）

| # | 内容 | 说明 |
|---|------|------|
| M1 | CAN 多机组网（向量表已预留 1024B, CAN 驱动已有） | 节点发现、心跳同步、版本冲突解决 |
| M2 | Flash 故障日志：最近 N 条事件（急停/通讯异常/超时/复位） | W25Q128 大容量更适合 |
| M3 | Flash 配置校验：所有字段合法性检查 | 与 Flash 日志一并规划分区 |
| M4 | Flash 操作计数：气缸/锁动作持久化 | 预防性维护参考 |
| M5 | W25Q128 分区：固件暂存区、快照区、日志区、配置区 | OTA + 快照 + 日志统一管理 |
| M5.1 | 主机 OTA 镜像 CAN 分发：主机先校验本地 W25Q128 镜像，再分发给目标节点 | 子节点本地暂存、校验、READY 后统一 COMMIT |
| M6 | 向量表 CAN 同步：version 驱动, CRC32 校验, 冲突诊断 | 主线已实现，单机启用 |
| M7 | 设备总寿命累计 (跨复位持久化至 Flash/备份寄存器) | 与 Flash 日志一并规划 |

---

## 已完成基线 ✓

以下内容已经完成，保留在此作为后续开发必须保持的架构基线。

- TIM1 毫秒计时、WatchDog 独立线程
- LOCK 防重复触发、气压检测修正、激光急停保护
- RS485 TX 超时、CRC16 查表、帧长校验、WaitRx
- rs485_io_ok 隔离、门限位消抖
- WriteIO 返回 bool、SCPI 读写全部走向量表
- RamVector 1024B 对齐主线、node 0=单机 1-8=多机
- ModBusTask RS485 独占、StateVectorTask 纯逻辑
- RS485 模块 → Hardware/ModBus
- UART1/3 DMA 双缓冲审查、U1_Printf 非阻塞、命名规范
- SysTimer 精确节拍替代 osDelay
- 风险模式、防夹手时序、E-STOP 常开/常闭可配
- IDN 字段可配（Flash 持久化）
- VECTOR_DEBUG 动作/状态/事件日志
- 流程文档 + 待办文档
- 上电自检（RCC、Flash、RS485，含PC串口启动提示）
- 复位原因捕获（RCC->CSR，POR/IWDG/NRST/软件复位）
- RS485故障保护（连续失败后停止自动操作并告警）
- 气缸写入先断对向线圈，再合目标线圈
- `SYSTem:VERSion?`（固件版本和Git hash）
- `RamVector` IO整帧快照读取，状态机和SCPI查询改读局部副本
- `RamVector` 命令槽原子Take/Clear，执行保持在临界区外
- `SYSTem:UPTime?`（运行秒数）
- `WriteIO()` 返回值汇总检查，失败写入AppLog
- `ReadIO()` 发送失败直接返回
