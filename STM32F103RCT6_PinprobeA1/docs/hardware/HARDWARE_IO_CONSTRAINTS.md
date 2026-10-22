# IO 位映射与外围约束

Status: Active
Domain: HARDWARE
Canonical: `docs/hardware/HARDWARE_IO_CONSTRAINTS.md`
Related: `docs/hardware/README.md`, `docs/control/CONTROL_STATE_MACHINE_ARCHITECTURE.md`, `docs/interface/INTERFACE_COMMUNICATION_ARCHITECTURE.md`
Last updated: 2026-10-22

本文档记录 PinProbeA1 通过 RS485/ModBus 连接的开关量 IO 模块的位映射与语义。
事实源：`Hardware/BsmRelay/Inc/BsmRelay.h`（`inputIO`/`outputIO` 枚举）、
`Hardware/RamVector/Src/state_vector.c`（`IN_*`/`OUT_*` 宏）、`Hardware/Flash/Inc/flash.h`（可配置 LED IO 编号）。

## 一、编号约定

- ModBus 线圈/离散输入按 **1-based 编号**（IO 1 对应线圈地址 1）。
- 代码内部按 **bit 位** 表示：`bit = 编号 - 1`；16 路量分成 `lo`（bit0–7）与 `hi`（bit8–15）两个字节。
- 输入镜像 `raw_in_lo` / `raw_in_hi`，输出镜像 `raw_out_lo` / `raw_out_hi` 存于
  `RAM_Vector.io_state[0]`（`Vector_IOState_t`）。
- 位定义在两处出现（`BsmRelay.h` 枚举与 `state_vector.c` 宏）必须保持一致；发现不一致按「代码先行、文档跟随」处理并登记缺陷。

## 二、输入位映射（离散输入，功能码 `0x02`）

| 位 | 掩码（16 位） | 宏（`BsmRelay.h`） | 宏（`state_vector.c`） | 语义 | 有效电平 |
|---:|---:|---|---|---|---|
| bit0 | `0x0001` | `door_sensor_up` | `IN_DOOR_UP` | 门上限位：门已完全打开 | 1 = 到位 |
| bit1 | `0x0002` | `door_sensor_down` | `IN_DOOR_DOWN` | 门下限位：门已完全关闭 | 1 = 到位 |
| bit2 | `0x0004` | `door_sensor_mid` | —（未使用） | 门中位检测（预留） | 1 = 到位 |
| bit3 | `0x0008` | `usb_sensor_up` | `IN_USB_UP` | USB 已插到位 | 1 = 到位 |
| bit4 | `0x0010` | `usb_sensor_down` | `IN_USB_DOWN` | USB 已回退到位 | 1 = 到位 |
| bit5 | `0x0020` | `laser_sensor1` | `IN_LASER1` | 激光 1 / 气压传感器（`risk_mode` 时可改语义为进气压力正常） | 1 = 遮挡/异常 |
| bit6 | `0x0040` | `laser_sensor2` | `IN_LASER2` | 激光 2：防夹手 | 1 = 遮挡 |
| bit7 | `0x0080` | `laser_sensor3` | `IN_LASER3` | 激光 3：防夹手 | 1 = 遮挡 |
| bit8 | `0x0100` | `laser_sensor4` | `IN_LASER4`（hi bit0） | 激光 4：防夹手 | 1 = 遮挡 |
| bit9 | `0x0200` | `door_button1` | `IN_DOOR_BTN1`（hi bit1） | 门按钮 1 | 1 = 按下 |
| bit10 | `0x0400` | `door_button2` | `IN_DOOR_BTN2`（hi bit2） | 门按钮 2（双按确认用） | 1 = 按下 |
| bit11 | `0x0800` | `stop_button` | `IN_ESTOP_BTN`（hi bit3） | 急停按钮 | 由 `estop_type` 决定常开/常闭 |
| bit12 | `0x1000` | `power_button` | `IN_POWER_BTN`（hi bit4） | 电源按钮（长按 300 ms 切锁定/解锁） | 1 = 按下 |
| bit13 | `0x2000` | `dut_sensor` | `IN_DUT_SENSOR`（hi bit5） | DUT 到位传感器 | 1 = 到位 |
| bit14–15 | `0x4000`/`0x8000` | — | — | 预留 | — |

组合判据（`state_vector.c`）：

| 判据 | 表达式 | 用途 |
|---|---|---|
| 任意激光遮挡 | `IS_ANY_LASER(in_lo, in_hi)` = bit5/6/7 或 bit8 | 触发 `EMERGENCY` |
| USB 已插入 | `IS_USB_INSERTED` = `IN_USB_UP && !IN_USB_DOWN` | 关门序列判定 |
| USB 已回退 | `IS_USB_RETRACTED` = `IN_USB_DOWN && !IN_USB_UP` | 开门序列判定 |
| USB 传感器冲突 | `IS_USB_UP && IS_USB_DOWN` → `USB_FAIL_SENSOR_CONFLICT` | 故障判定 |
| 双按钮 | `IS_BOTH_BTN` = 两个门按钮同时按下 | `READY` 下关门确认 |
| 任意门按钮 | `IS_ANY_BTN` | `IDLE`/`COMPLETE` 下的动作触发 |
| 急停 | `IS_ESTOP(in_hi)` = bit11 | 安全例外 |
| 解锁状态 | `IS_UNLOCKED(out_lo)` = `OUT_POWER` | `LOCK`/`IDLE` 判定 |

## 三、输出位映射（线圈，功能码 `0x01` 读 / `0x05` 写）

| 位 | 掩码 | 宏（`BsmRelay.h`） | 宏（`state_vector.c`） | 语义 | 说明 |
|---:|---:|---|---|---|---|
| bit0 | `0x0001` | `door_open` | `OUT_DOOR_OPEN` | 门气缸伸出（门上升） | 与 bit1 互斥 |
| bit1 | `0x0002` | `door_close` | `OUT_DOOR_CLOSE` | 门气缸回缩（门下降） | 与 bit0 互斥 |
| bit2 | `0x0004` | `usb_pne_in` | `OUT_USB_IN` | USB 插入 | 对外语义 `CLOSE/CLOSED` |
| bit3 | `0x0008` | `usb_pne_out` | `OUT_USB_OUT` | USB 回退 | 对外语义 `OPEN/OPENED` |
| bit4 | `0x0010` | `led_green` | —（经 `LED_*`） | 绿灯 | 输出编号默认 5（`led_green_io`） |
| bit5 | `0x0020` | `led_red` | —（经 `LED_*`） | 红灯 | 输出编号默认 6（`led_red_io`） |
| bit6 | `0x0040` | `led_yellow` | —（经 `LED_*`） | 黄灯 | 输出编号默认 7（`led_yellow_io`） |
| bit7 | `0x0080` | `power_out` | `OUT_POWER` | 电源锁：0 = 锁定（断电），1 = 解锁（上电） | 断电回锁关键输出 |
| bit8 | `0x0100` | `power_led` | — | 电源指示灯 | — |
| bit9–15 | — | — | — | 预留 | — |

互斥约束：

- `OUT_DOOR_OPEN` 与 `OUT_DOOR_CLOSE` 不得同时置位（`OUT_DOOR_MOVING` 仅用于判定「门在动」）。
- `OUT_USB_IN` 与 `OUT_USB_OUT` 不得同时置位，同时置位判 `USB_FAIL_DUAL_OUTPUT`。
- `OUT_POWER` 为 0 时其余动作输出必须复位（断电回锁）。

## 四、LED 状态编码

`LED_DecodeState(out_bits)` 把输出位图折叠为单一状态码，`IS_LED_RED_STATE(s)` = 2、
`IS_LED_YELLOW_STATE(s)` = 4。SCPI 侧语义（`GREEN`/`RED`/`YELLOW`/`OFF`）见
`docs/interface/SCPI_COMMANDS.md`。

## 五、ModBus 访问参数

| 项 | 值 | 事实源 |
|---|---|---|
| 从机地址 | `0x01` | `RS485.c`（硬编码） |
| 波特率 | 115200 8N1 | `usart.c`（USART3） |
| 功能码 | `0x01` 读线圈、`0x02` 读离散输入、`0x03` 读保持寄存器、`0x05` 写单线圈 | `RS485.c` |
| 发送超时 | 100 ms | `RS485_TX_TIMEOUT_MS` |
| 轮询 | 约 50 ms，每轮读输入 + 读输出 | `app_tasks.c` |
| 重试 | 每轮最多 5 次（`IO_Read(checkNum=5, …)`） | `BsmRelay.h` `CHECK_NUM` |

协议层细节见 `docs/interface/INTERFACE_COMMUNICATION_ARCHITECTURE.md`。

## 六、待核实项

| 编号 | 项 | 说明 |
|---|---|---|
| `HW-IO-Q1` | 中位传感器（bit2）未接入状态机 | 是否保留、是否用于门行程异常判定待定。 |
| `HW-IO-Q2` | `laser_sensor1` 的 `risk_mode` 双语义 | 气压传感器与激光 1 共用同一位，切换条件需在 `docs/control/` 明确。 |
| `HW-IO-Q3` | `led_*_io` 默认值 5/6/7 与位 4/5/6 的 1-based 对应关系 | 已按 1-based 编号解释；需以实测写入线圈号复核。 |
| `HW-IO-Q4` | `power_led` 无状态机管理 | 目前仅由输出镜像反映。 |
