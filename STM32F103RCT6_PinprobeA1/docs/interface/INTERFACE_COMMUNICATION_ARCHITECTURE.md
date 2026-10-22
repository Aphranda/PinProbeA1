# 通信与接口架构

Status: Active
Domain: INTERFACE
Canonical: `docs/interface/INTERFACE_COMMUNICATION_ARCHITECTURE.md`
Related: `docs/interface/SCPI_COMMANDS.md`, `docs/interface/INTERFACE_TODO.md`, `docs/arch/ARCH_PRODUCT_ARCHITECTURE.md`, `docs/hardware/HARDWARE_IO_CONSTRAINTS.md`, `docs/control/CONTROL_STATE_MACHINE_ARCHITECTURE.md`
Last updated: 2026-10-22

本文档定义 PinProbeA1 的两条物理通信链路、帧边界、超时与错误处理，以及上位机交互口径。
事实源：`Core/Src/usart.c`、`Core/Src/stm32f1xx_it.c`、`Hardware/modbus/src/RS485.c`、
`Hardware/BsmRelay/Src/BsmRelay.c`。

## 一、链路总览

| 链路 | 外设 | 对端 | 协议 | 用途 | 并发 |
|---|---|---|---|---|---|
| 主机链路 | `USART1` + DMA1（TX/RX） | PC（USB CDC 虚拟串口） | SCPI（ASCII 行协议） | 命令、查询、日志回读 | 单任务 `SCPITask` |
| IO 链路 | `USART3` + DMA1（TX/RX） | 开关量 IO 模块（ModBus 从机） | ModBus RTU | 16 路输入读取、16 路输出写入 | 单任务 `ModBusTask` |

两条链路互不转发：SCPI 命令不直接写 IO，一律经命令槽由 `cmd_exec` 下发；
IO 采样结果只进入 `RAM_Vector.io_state[0]`，不上行透传到 SCPI 之外的第三方。

## 二、主机链路（SCPI over USB CDC）

| 项 | 值 |
|---|---|
| 参数 | 115200 8N1，无硬件流控 |
| 接收 | DMA 空闲中断（IDLE）切块，缓冲指针 `Uart1_BuffIsReady` + 长度 `Uart1_RxLength` |
| 组帧 | 以 `\n` 结尾的行；`SCPITask` 要求长度 > 3 才交给 `SCPI_Input` |
| 轮询 | `SCPITask` 10 ms 周期；用临界区摘取缓冲，避免解析半帧 |
| 输出 | SCPI 应答 + 可选实时日志（`AppLog_PumpRealtime(4)`，每轮最多 4 条） |
| 错误 | libscpi 错误队列（`SYSTem:ERRor?`），负错误码语义见 `docs/interface/SCPI_COMMANDS.md` |

要点：

- SCPI 解析在低优先级任务执行，不阻塞 25 ms 状态机节拍与 50 ms IO 轮询。
- 日志与应答共用 USART1 发送通道，靠 `Uart1_IsTxBusy()` 互斥；日志被丢弃计入 `realtime_drop_count`。
- 命令字面量、参数域和查询项以 `docs/interface/SCPI_COMMANDS.md` 为准；本文档不重复列表。

## 三、IO 链路（ModBus RTU）

| 项 | 值 |
|---|---|
| 从机地址 | `0x01`（固定，代码内硬编码） |
| 参数 | 115200 8N1 |
| 发送超时 | `RS485_TX_TIMEOUT_MS` = 100 ms |
| 校验 | CRC16（ModBus 多项式，低字节先发） |
| 轮询周期 | 约 50 ms（SysTimer 二分频） |
| 每轮动作 | 读输入 2 字节 → 读输出 2 字节 → 更新 `io_state[0]` → `CmdExec_ExecuteAll()` |

使用到的功能码：

| 功能码 | 用途 | 调用点 | 请求要点 |
|---|---|---|---|
| `0x01` | 读输出线圈状态（当前输出镜像） | `IO_Read(5, 1, out_buf)` | 起始 0，数量 16 |
| `0x02` | 读离散输入（限位、按钮、激光、DUT） | `IO_Read(5, 2, in_buf)` | 起始 0，数量 16 |
| `0x03` | 读保持寄存器（模块诊断量） | `ReadHoldingRegister()` | 起始地址可变，读 1 个 |
| `0x05` | 写单线圈（继电器动作） | `WriteIO()` | 值 `0xFF00` 置位 / `0x0000` 复位 |

校验与重试：

- 应答必须回显从机地址、功能码和寄存器/线圈地址，否则判失败。
- 失败一次即本轮采样作废，`SetRS485_Ok(false)`，链路状态由 `ModBusTask` 统一裁决。
- 连续失败 `RS485_FAIL_THRESHOLD`(10 次，约 250 ms) 才升级为告警事件 `APPLOG_EVT_RS485_FAULT`。
- 恢复需要连续 `IO_RECOVERY_STABLE_SAMPLES`(10 帧) 一致，避免抖动误判。

## 四、链路可信状态与消费方

| 状态 | 产生条件 | 消费方行为 |
|---|---|---|
| `VECTOR_IO_LINK_OK` | 采样有效且已稳定 | 状态机正常迁移、动作正常下发 |
| `VECTOR_IO_LINK_RECOVERING` | 全 0 帧，或首次恢复未达 10 帧 | 暂停动作下发，保留最后可信状态 |
| `VECTOR_IO_LINK_FAULT` | 读失败（超时/CRC/回显不符） | 动作降级、告警、保持安全输出 |

安全例外（急停、激光、断电回锁）在链路故障时仍然生效：故障本身即视为不安全条件。

## 五、上位机交互口径

| 项 | 内容 |
|---|---|
| 上位机 | `Tools/pinprobe_gui.py`（PyInstaller 打包为 `PinProbeA1箱体控制程序.exe`） |
| 连接 | 串口枚举 + 手动选择；波特率固定 115200 |
| 命令 | 与 `docs/interface/SCPI_COMMANDS.md` 一一对应，界面动作不额外引入私有命令 |
| 反馈 | 查询轮询（状态、告警、IO）+ 命令即时应答；异常弹窗依据 `SYSTem:ERRor?` |
| 交付件 | `docs/reports/guide/` 下的中英文操作说明 |

## 六、未决问题

| 编号 | 问题 | 去向 |
|---|---|---|
| `IF-Q1` | 当前 SCPI 交付版（20260924）无 Markdown 源，交付链断裂。 | `docs/interface/INTERFACE_TODO.md` `INTERFACE-01` |
| `IF-Q2` | ModBus 从机地址、波特率硬编码，`Flash_Config_t` 中虽有 `device_id`/`rs485_baudrate` 字段但未被 IO 链路消费。 | `docs/interface/INTERFACE_TODO.md` `INTERFACE-03` |
| `IF-Q3` | 帧层缺少统一的「半双工总线占用」保护说明：当前靠单任务串行化隐式保证，需在新增总线主设备时复核。 | `docs/interface/INTERFACE_TODO.md` |
