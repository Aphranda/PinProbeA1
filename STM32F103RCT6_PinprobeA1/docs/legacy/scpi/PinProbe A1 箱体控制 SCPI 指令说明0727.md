# PinProbe A1 箱体控制 SCPI 指令说明

> 文档更新时间：`2026-07-28`。本文用于客户侧上位机联调、设备控制与维护参考。

---

## 设备信息

| 项目         | 默认值                                              | 说明                                          |
| ------------ | --------------------------------------------------- | --------------------------------------------- |
| 制造商       | `GTS`                                             | `*IDN?` 字段 1，可通过 `SYSTem:IDN1` 修改 |
| 型号         | `PINPROBEA1`                                      | `*IDN?` 字段 2，可通过 `SYSTem:IDN2` 修改 |
| 序列号       | `20250626`                                        | `*IDN?` 字段 3，可通过 `SYSTem:IDN3` 修改 |
| IDN 固件版本 | `V0.0.9`                                          | `*IDN?` 字段 4，可通过 `SYSTem:IDN4` 修改 |
| 编译固件版本 | `v1.0.0+<build-id>` | `SYSTem:VERSion?` 返回                      |

> `*IDN?` 返回当前 SCPI IDN 配置：`制造商,型号,序列号,IDN固件版本`。IDN 字段保存在设备配置中，可能与编译固件版本不同。

## 通信约定

| 项目         | 说明                                                                      |
| ------------ | ------------------------------------------------------------------------- |
| 串口波特率   | `115200`                                                                |
| 指令结束符   | `\r\n`                                                                  |
| 普通响应     | 单行文本                                                                  |
| 日志批量响应 | `READ:LOG:ALL?` 可能返回多行文本                                        |
| OTA 数据块   | 使用 SCPI definite-length binary block，当前单块 payload 最大`128` 字节 |

> SCPI 报错有三类来源：解析器错误队列、命令主动返回的 `ERR...` 文本，以及设备异常日志。上位机应同时处理指令响应和日志流，详见[报错与异常处理](#报错与异常处理)。

## 基础指令（IEEE 488.2）

| 指令      | 参数 | 响应                               | 说明                 |
| --------- | ---- | ---------------------------------- | -------------------- |
| `*CLS`  |      | 无                                 | 清除状态和错误队列   |
| `*IDN?` |      | `GTS,PINPROBEA1,20250626,V0.0.9` | 查询设备身份信息     |
| `*RST`  |      | 无                                 | 复位 SCPI 解析器状态 |
| `*STB?` |      | 状态字节                           | 查询状态字节         |
| `*WAI`  |      | 无                                 | 等待指令完成         |
| `*OPC?` |      | `1`                              | 查询操作完成状态     |

## 系统指令

| 指令                     | 参数 | 响应                                               | 说明                               |
| ------------------------ | ---- | -------------------------------------------------- | ---------------------------------- |
| `SYSTem:ERRor[:NEXT]?` |      | 错误码与描述                                       | 查询下一条错误信息                 |
| `SYSTem:ERRor:COUNt?`  |      | 错误数量                                           | 查询错误队列数量                   |
| `SYSTem:VERSion?`      |      | `v1.0.0+<build-id>`                              | 查询编译固件版本和构建标识         |
| `SYSTem:UPTime?`       |      | 秒数                                               | 查询系统运行时间                   |
| `SYSTem:REBoot`        |      | `OK`                                             | 延时约 200 ms 后软件复位           |
| `SYSTem:FLASH:ID?`     |      | `OK ID=<JEDEC> SR1=<xx> SR2=<xx>` 或 `ERR ...` | 查询 W25Q128 Flash ID 与状态寄存器 |

### 示例

```scpi
SYSTem:ERRor:NEXT?
SYSTem:ERRor:COUNt?
SYSTem:VERSion?
SYSTem:UPTime?
SYSTem:FLASH:ID?
SYSTem:REBoot
```

## 设备身份配置指令

| 指令             | 参数         | 响应   | 说明                                  |
| ---------------- | ------------ | ------ | ------------------------------------- |
| `SYSTem:IDN1`  | `<string>` | 无     | 配置制造商字段，并保存到 Flash        |
| `SYSTem:IDN1?` |              | 字符串 | 查询制造商字段                        |
| `SYSTem:IDN2`  | `<string>` | 无     | 配置型号字段，并保存到 Flash          |
| `SYSTem:IDN2?` |              | 字符串 | 查询型号字段                          |
| `SYSTem:IDN3`  | `<string>` | 无     | 配置序列号/日期字段，并保存到 Flash   |
| `SYSTem:IDN3?` |              | 字符串 | 查询序列号/日期字段                   |
| `SYSTem:IDN4`  | `<string>` | 无     | 配置 IDN 固件版本字段，并保存到 Flash |
| `SYSTem:IDN4?` |              | 字符串 | 查询 IDN 固件版本字段                 |

### 示例

```scpi
SYSTem:IDN1 "GTS"
SYSTem:IDN2 "PINPROBEA1"
SYSTem:IDN3 "20260727"
SYSTem:IDN4 "V0.0.9"
*IDN?
```

## 通信配置指令

| 指令                   | 参数       | 响应                          | 说明                                      |
| ---------------------- | ---------- | ----------------------------- | ----------------------------------------- |
| `CONFigure:BAUDrate` | `115200` | `115200 baudrate is enable` | 配置 BSM 通信波特率。当前仅支持`115200` |

### 示例

```scpi
CONFigure:BAUDrate 115200
```

## 箱门与 USB 控制指令

| 指令                      | 参数             | 响应                         | 说明                                  |
| ------------------------- | ---------------- | ---------------------------- | ------------------------------------- |
| `CONFigure:CYLInder1`     | `OPEN` / `CLOSE` | `OPEN` / `CLOSE`             | 打开或关闭箱门                        |
| `READ:CYLInder1:STATe?`   |                  | [执行器状态](#执行器状态)    | 查询箱门当前状态                      |
| `CONFigure:CYLInder2`     | `OPEN` / `CLOSE` | `OPEN` / `CLOSE`             | 手动控制 USB 气缸；`CLOSE`=插入，`OPEN`=拔出/回退 |
| `READ:CYLInder2:STATe?`   |                  | [执行器状态](#执行器状态)    | 查询 USB 插拔机构当前状态             |
| `CONFigure:USB:AUTO`      | `OFF` / `ON`     | `OFF` / `ON`                 | USB 自动拔插流程出厂配置项，禁止修改；出厂已按设备硬件配置完成 |
| `READ:USB:AUTO?`          |                  | `OFF` / `ON`                 | 查询 USB 自动拔插流程出厂配置，确认当前设备是否启用该流程 |

> USB 动作按连接状态定义：`CYLInder2 CLOSE` 表示插入/连接 USB，`CYLInder2 OPEN` 表示拔出/回退 USB。

### 执行器状态

| 返回值      | 说明                |
| ----------- | ------------------- |
| `CLOSE`   | 关闭指令已接收；USB 表示插入/连接 |
| `OPEN`    | 打开指令已接收；USB 表示拔出/回退 |
| `CLOSING` | 正在关闭；USB 表示正在插入 |
| `OPENING` | 正在打开；USB 表示正在拔出/回退 |
| `CLOSED`  | 已关闭；USB 表示已插入/已连接 |
| `OPENED`  | 已打开；USB 表示已拔出/已回退 |
| `CYL ERR` | 执行器错误          |

### USB 自动流程

`CONFigure:USB:AUTO ON` 后，设备会把 USB 插入、回退动作绑定到门流程：

| 场景 | 自动动作 |
| ---- | -------- |
| `READY` 状态下双按钮确认满 500 ms | 先执行 `CONFigure:CYLInder2 CLOSE` 插入 USB；确认 USB 插入到位后，才自动执行关门 |
| USB 插入未到位或传感异常 | 不执行关门；记录 `USB_INSERT_FAIL`。门仍在打开位置时会先执行 `CONFigure:CYLInder2 OPEN` 回退 USB，并红灯快闪提示 |
| `COMPLETE` 状态下按键开门 | 先自动开门；门打开到位并回到 `IDLE` 后，若 USB 未回退到位，再执行 `CONFigure:CYLInder2 OPEN` 回退 USB |
| USB 回退未到位或输出异常 | 记录 `USB_RETRACT_FAIL`，红灯快闪提示；必要时退回 `IDLE` 并等待按钮释放后才能重新进入关门准备 |
| 诊断兜底（正常流程不应出现） | 若关门完成后仍检测到 USB 未插到位，记录 `USB_INSERT_FAIL`，黄灯闪烁提示；USB 到位后自动恢复绿灯 |

> `CONFigure:USB:AUTO ON/OFF` 为出厂配置项，禁止修改；出厂时已按设备硬件配置完成。带 USB 气缸的设备配置为 `ON`，设备会自动接管 USB 插入/回退流程：关门前先插入 USB，插入到位后才关门；开门到位后自动回退 USB。不带 USB 气缸的设备配置为 `OFF`。客户侧建议使用 `READ:USB:AUTO?` 确认当前出厂配置。

### 示例

```scpi
CONFigure:CYLInder1 OPEN
CONFigure:CYLInder1 CLOSE
READ:CYLInder1:STATe?
CONFigure:CYLInder2 CLOSE
CONFigure:CYLInder2 OPEN
READ:CYLInder2:STATe?
READ:USB:AUTO?
```

## 锁定与指示灯指令

| 指令                 | 参数                    | 响应                    | 说明           |
| -------------------- | ----------------------- | ----------------------- | -------------- |
| `CONFigure:LOCK`   | `UNLOCK` / `LOCKED` | `UNLOCK` / `LOCKED` | 配置设备锁状态 |
| `READ:LOCK:STATe?` |                         | [锁状态](#锁状态)        | 查询锁状态     |

### 锁状态

| 参数/返回值  | 说明   |
| ------------ | ------ |
| `UNLOCK`   | 解锁   |
| `LOCKED`   | 锁定   |
| `LOCK ERR` | 锁错误 |

### 示例

```scpi
CONFigure:LOCK LOCKED
CONFigure:LOCK UNLOCK
READ:LOCK:STATe?
```

## LED 指示灯指令

| 指令                   | 参数                                       | 响应           | 说明                                             |
| ---------------------- | ------------------------------------------ | -------------- | ------------------------------------------------ |
| `CONFigure:LED`      | `OFF` / `GREEN` / `RED` / `YELLOW` | LED 状态       | 配置 LED 指示灯                                  |
| `READ:LED:STATe?`    |                                            | LED 状态       | 查询 LED 状态                                    |
| `CONFigure:LED:MAP`  | `<io5>,<io6>,<io7>`                      | `G,R,Y` 格式 | 配置 IO5/IO6/IO7 对应的 LED 颜色，并保存到 Flash |
| `READ:LED:MAP?`      |                                            | `G,R,Y` 格式 | 查询 IO5/IO6/IO7 对应的 LED 颜色                 |

### LED 状态

| 参数/返回值 | 说明     |
| ----------- | -------- |
| `OFF`     | LED 关闭 |
| `GREEN`   | 绿色 LED |
| `RED`     | 红色 LED |
| `YELLOW`  | 黄色 LED |
| `LED ERR` | LED 错误 |

### LED 映射参数

| 参数               | 说明             |
| ------------------ | ---------------- |
| `G` / `GREEN`  | 当前 IO 对应绿灯 |
| `R` / `RED`    | 当前 IO 对应红灯 |
| `Y` / `YELLOW` | 当前 IO 对应黄灯 |

> `CONFigure:LED:MAP` 三个参数不能重复，且必须同时包含绿、红、黄三种颜色。

### 示例

```scpi
CONFigure:LED GREEN
CONFigure:LED OFF
READ:LED:STATe?
CONFigure:LED:MAP G,R,Y
READ:LED:MAP?
```

## 系统状态查询指令

| 指令                   | 参数 | 响应                           | 说明                       |
| ---------------------- | ---- | ------------------------------ | -------------------------- |
| `READ:SYSTem:STATe?` |      | [系统状态](#系统状态返回值)     | 查询系统状态               |
| `READ:IO:ALL?`       |      | `IN:0xHH,0xHH OUT:0xHH,0xHH` | 查询全部原始输入和输出状态 |

### 系统状态返回值

| 返回值        | 说明             |
| ------------- | ---------------- |
| `LOCK`      | 系统处于锁定状态 |
| `IDLE`      | 系统空闲         |
| `READY`     | 系统就绪         |
| `RUNNING`   | 系统运行中       |
| `EMERGENCY` | 急停触发         |
| `COMPLETE`  | 系统操作完成     |
| `SYS ERR`   | 系统错误         |

### 示例

```scpi
READ:SYSTem:STATe?
READ:IO:ALL?
```

## 急停、风险模式与 Boot 诊断指令

| 指令                      | 参数             | 响应             | 说明                                         |
| ------------------------- | ---------------- | ---------------- | -------------------------------------------- |
| `CONFigure:ESTOP:TYPE`  | `NC` / `NO`  | `NC` / `NO`  | 配置急停输入类型，并保存到 Flash             |
| `READ:ESTOP:TYPE?`      |                  | `NC` / `NO`  | 查询急停输入类型                             |
| `CONFigure:RISK:MODE`   | `OFF` / `ON` | `OFF` / `ON` | 配置 Risk Mode，并保存到 Flash               |
| `READ:RISK:MODE?`       |                  | `OFF` / `ON` | 查询 Risk Mode                               |
| `CONFigure:BOOT:DIAG`   | `OFF` / `ON` | `OFF` / `ON` | 配置 Bootloader 诊断串口输出，并保存到 Flash |
| `READ:BOOT:DIAG?`       |                  | `OFF` / `ON` | 查询 Bootloader 诊断串口输出开关             |

### 参数说明

| 参数    | 说明     |
| ------- | -------- |
| `NC`  | 急停常闭 |
| `NO`  | 急停常开 |
| `OFF` | 关闭功能 |
| `ON`  | 开启功能 |

### 示例

```scpi
CONFigure:ESTOP:TYPE NC
READ:ESTOP:TYPE?
CONFigure:RISK:MODE ON
READ:RISK:MODE?
CONFigure:BOOT:DIAG ON
READ:BOOT:DIAG?
```

## OTA 固件升级指令

| 指令                   | 参数                                    | 响应                                          | 说明                                 |
| ---------------------- | --------------------------------------- | --------------------------------------------- | ------------------------------------ |
| `SYSTem:OTA:STATus?` |                                         | `STATE:... SIZE:... RECV:...`               | 查询 OTA 接收与校验状态              |
| `SYSTem:OTA:BOOT?`   |                                         | `FLASH:... FLAGS:... MAN:...`               | 查询 Bootloader 与 OTA Manifest 状态 |
| `SYSTem:OTA:BEGIN`   | `<size>,<crc32>,<version>,<image_id>` | `OK` 或 `ERR,<code>,<name>`               | 开始一次 OTA 上传                    |
| `SYSTem:OTA:DATA`    | `<offset>,#<n><len><data>`            | `OK,NEXT:<offset>` 或 `ERR,<code>,<name>` | 写入一块 OTA 数据                    |
| `SYSTem:OTA:END`     |                                         | `OK` 或 `ERR,<code>,<name>`               | 结束 OTA 数据接收                    |
| `SYSTem:OTA:VERify?` |                                         | 状态文本或错误文本                            | 校验 OTA 镜像                        |
| `SYSTem:OTA:COMMit`  |                                         | `OK` 或错误文本                             | 提交升级请求，成功后设备复位         |
| `SYSTem:OTA:ABORt`   |                                         | `OK` 或错误文本                             | 中止当前 OTA 流程                    |

### OTA 状态响应

`SYSTem:OTA:STATus?` 返回格式：

```text
STATE:<state> SIZE:<bytes> RECV:<bytes> BLOCKS:<recv>/<total> NEXT:<offset> CRC:0x<crc32> VER:0x<version> IMG:<id> ERR:<code>
```

`SYSTem:OTA:BOOT?` 返回格式：

```text
FLASH:<0|1> FLAGS:<0|1> MAN:<0|1> SEQ:<n> ACT:<n> PEND:<slot> PREV:<slot> BSTATE:<n> ATT:<n>/<max> BERR:<code> MSTATE:<state> SIZE:<bytes> CRC:0x<crc32> VER:0x<version> IMG:<id>
```

### OTA 示例

```scpi
SYSTem:OTA:STATus?
SYSTem:OTA:BEGIN 4096,305419896,65536,1
SYSTem:OTA:DATA 0,#3128<128 bytes binary payload>
SYSTem:OTA:END
SYSTem:OTA:VERify?
SYSTem:OTA:COMMit
```

## 报错与异常处理

### SCPI 解析器错误

参数缺失、参数非法、未知指令、Flash 保存失败或底层硬件通信失败时，固件会向 SCPI 错误队列写入错误。错误队列容量为 `17` 条，可用以下方式读取：

```scpi
SYSTem:ERRor:COUNt?
SYSTem:ERRor:NEXT?
*CLS
```

| 场景 | 典型表现 | 处理建议 |
| ---- | -------- | -------- |
| 未知指令 | 串口可能主动输出 `**ERROR: -113, "Undefined header"`；错误队列可读到 `-113,"Undefined header"` | 停止发送该指令，检查命令拼写和层级 |
| 缺少参数 | `-109,"Missing parameter"` | 补齐必填参数 |
| 参数值非法 | `-224,"Illegal parameter value"` 或 `-222,"Data out of range"` | 检查枚举值、LED 映射是否重复、数值范围是否越界 |
| Flash 保存失败 | `-320,"Storage fault"` | 配置可能未持久化；建议重试并读取对应查询指令确认 |
| UART/RS485 硬件或通信失败 | `-240,"Hardware error"` 或 `-360,"Communication error"` | 查询 `READ:LOG:STATus?`、`READ:IO:ALL?`，并检查 IO 扩展板链路 |

> SCPI 错误队列使用出队读取；`SYSTem:ERRor:NEXT?` 每读一次弹出一条。`*CLS` 会清除状态和错误队列。

### 命令主动返回的 `ERR...`

部分命令不通过 SCPI 错误队列报错，而是在普通响应中返回 `ERR...`：

| 指令 | 报错格式 | 说明 |
| ---- | -------- | ---- |
| `SYSTem:FLASH:ID?` | `ERR INIT=<name>` 或 `ERR ID=<name>` | W25Q128 初始化或读取 JEDEC ID 失败 |
| OTA 指令 | `ERR,<code>,<name>` | OTA 流程错误，错误码见下表 |

### OTA 错误码

| 代码 | 名称 | 含义 |
| ---- | ---- | ---- |
| `0` | `OK` | 成功 |
| `1` | `BUSY` | OTA 流程忙 |
| `2` | `BAD_STATE` | 当前状态不允许执行该 OTA 步骤 |
| `3` | `BAD_PARAM` | 参数错误 |
| `4` | `FLASH_FAIL` | Flash 操作失败 |
| `5` | `SIZE_OVERFLOW` | 镜像大小超出允许范围 |
| `6` | `CRC_FAIL` | CRC 校验失败 |
| `7` | `CAN_TIMEOUT` | CAN 分发超时 |
| `8` | `CAN_NACK` | CAN 节点拒绝 |
| `9` | `NODE_NOT_READY` | 节点未就绪 |
| `10` | `BOOT_FAIL` | Bootloader 升级失败 |
| `11` | `APP_NOT_CONFIRMED` | App 未确认 |
| `12` | `BLOCK_MISSING` | OTA 数据块缺失 |
| `13` | `BAD_MANIFEST` | Manifest 无效 |
| `14` | `UNSUPPORTED_IMAGE` | 镜像类型不支持 |

### 运行异常处理

运行时安全事件和执行器异常主要通过设备日志上报。上位机建议按以下顺序处理：

1. 指令响应以 `ERR` 或 `ERROR` 开头时，立即读取 `SYSTem:ERRor:NEXT?` 直到返回无错误。
2. 定期读取 `READ:LOG:STATus?`；若 `PEND` 非零，读取 `READ:LOG:NEXT?` 或 `READ:LOG:ALL?`。
3. 发现 `[ERROR]` 时停止自动测试流程，采集 `READ:SYSTem:STATe?`、`READ:IO:ALL?`、`READ:CYLInder1:STATe?`、`READ:CYLInder2:STATe?`。
4. 发现 `[WARN]` 时结合事件名判断是否可继续；USB 插拔失败、RS485 故障、IO 写入失败建议人工确认后再继续。

| 异常 | 日志级别 | 固件动作 | 恢复建议 |
| ---- | -------- | -------- | -------- |
| 急停 `ESTOP` | `[ERROR]` | 进入 `EMERGENCY`，红灯，系统开门并锁定 | 急停复位且激光清除后，重新按下启动键解锁恢复 |
| 激光防夹 `LASER` | `[ERROR]` | 关门后段检测到遮挡时进入 `EMERGENCY`，红灯，系统开门并锁定 | 清除遮挡后，重新按下启动键解锁恢复 |
| RS485 故障 `RS485_FAULT` | `[WARN]` | IO 链路标记不可用，状态机暂停本轮自动操作并请求黄灯告警 | 等待 `RS485_RECOVERED`，再读取 IO 和系统状态确认 |
| Risk Mode 压力事件 `RISK_PRESSURE` | `[WARN]` | Risk Mode 下用气压和学习时间完成关门判定 | 检查门下限位或气压传感状态 |
| 气压低 `AIR_LOW` | `[WARN]` | 仅记录诊断日志，不触发急停 | 等待 `clear_after` 记录或检查气路 |
| IO 写入失败 `IO_WRITE_FAIL` | `[WARN]` | 对应执行器命令可能未真正输出 | 检查 IO 板通信、电源和执行器线路 |
| USB 插入失败 `USB_INSERT_FAIL` | `[WARN]` | 可能回退 USB、红灯快闪或在 `COMPLETE` 中黄灯闪烁 | 查询 `READ:CYLInder2:STATe?`，检查 USB 插入气缸和到位传感 |
| USB 回退失败 `USB_RETRACT_FAIL` | `[WARN]` | 红灯快闪，必要时退回 `IDLE` | 查询 `READ:CYLInder2:STATe?`，检查 USB 回退气缸和到位传感 |

## 调试开关指令

| 指令                       | 参数             | 响应             | 说明                            |
| -------------------------- | ---------------- | ---------------- | ------------------------------- |
| `CONFigure:DEBUg:STATe`  | `OFF` / `ON` | `OFF` / `ON` | 运行时开启/关闭状态迁移调试输出 |
| `READ:DEBUg:STATe?`      |                  | `OFF` / `ON` | 查询状态迁移调试输出开关        |
| `CONFigure:DEBUg:ACTion` | `OFF` / `ON` | `OFF` / `ON` | 运行时开启/关闭动作耗时调试输出 |
| `READ:DEBUg:ACTion?`     |                  | `OFF` / `ON` | 查询动作耗时调试输出开关        |
| `CONFigure:DEBUg:EVENt`  | `OFF` / `ON` | `OFF` / `ON` | 运行时开启/关闭事件调试输出     |
| `READ:DEBUg:EVENt?`      |                  | `OFF` / `ON` | 查询事件调试输出开关            |
| `CONFigure:DEBUg:IO`     | `OFF` / `ON` | `OFF` / `ON` | 运行时开启/关闭 IO 调试输出     |
| `READ:DEBUg:IO?`         |                  | `OFF` / `ON` | 查询 IO 调试输出开关            |

> 调试开关为运行时状态，默认 `OFF`，不写入 Flash。

### 示例

```scpi
CONFigure:DEBUg:STATe ON
READ:DEBUg:STATe?
CONFigure:DEBUg:IO OFF
READ:DEBUg:IO?
```

## 设备日志指令

| 指令                    | 参数             | 响应                          | 说明                            |
| ----------------------- | ---------------- | ----------------------------- | ------------------------------- |
| `CONFigure:LOG:UART`  | `OFF` / `ON` | `OFF` / `ON`              | 开启/关闭设备日志实时 UART 输出 |
| `READ:LOG:UART?`      |                  | `OFF` / `ON`              | 查询实时 UART 输出开关          |
| `READ:LOG:NEXT?`      |                  | 一条日志或`EMPTY`           | 读取下一条设备日志记录          |
| `READ:LOG:ALL?`       |                  | 多行日志或`EMPTY`           | 读取当前全部待读设备日志记录    |
| `READ:LOG:STATus?`    |                  | `PEND:... CAP:... NEXT:...` | 查询设备日志缓冲区状态          |
| `CONFigure:LOG:CLEar` |                  | `OK`                        | 清空设备日志缓冲区              |

### 日志状态响应

```text
PEND:<pending> CAP:<capacity> NEXT:<next_seq> DROP:<drop_count> RDROP:<realtime_drop_count> UART:<0|1>
```

### 日志行格式

日志记录通常以时间戳和节点号开头，例如：

```text
[T+12.345s][N0][INFO][ACTION] CLOSE_DONE duration=300ms
```

通用格式如下：

```text
[T+<设备启动后时间>][N<节点号>][<级别>][<模块>] <事件及参数>
```

| 字段                                        | 说明                                   |
| ------------------------------------------- | -------------------------------------- |
| `T+`                                      | 本次启动后的相对时间，复位后重新计时   |
| `N`                                       | 节点号，单机默认通常为`N0`           |
| `INFO`                                    | 普通状态、动作、命令或恢复事件         |
| `WARN`                                    | 需要关注的异常或诊断事件               |
| `ERROR`                                   | 严重安全事件，如急停、激光防夹触发     |
| `STATE` / `ACTION` / `EVENT` / `IO` | 状态、动作、事件或 IO 快照             |

### 异常日志说明

上位机应以日志级别字段判断异常。当前固件中，急停和激光防夹为 `[ERROR][EVENT]`，其余运行诊断事件通常为 `[WARN][EVENT]`：

| 异常事件           | 日志示例                                                                  | 含义与操作说明                                      |
| ------------------ | ------------------------------------------------------------------------- | --------------------------------------------------- |
| 急停触发           | `[T+15.120s][N0][ERROR][EVENT] ESTOP close_elapsed=820ms`                | 系统开门并锁定；复位急停后重新按启动键恢复          |
| 异物阻挡           | `[T+16.020s][N0][ERROR][EVENT] LASER close_elapsed=1720ms`               | 系统开门并锁定；清除遮挡后重新按启动键恢复          |
| RS485 通信故障     | `[T+12.345s][N0][WARN][EVENT] RS485_FAULT arg0=0 arg1=0`                | IO 通信暂不可用；暂停操作并等待恢复日志             |
| Risk Mode 压力事件 | `[T+16.100s][N0][WARN][EVENT] RISK_PRESSURE close_elapsed=1800ms`       | 关门压力条件触发；检查工件和关门阻力                |
| 气压低/恢复        | `[T+20.000s][N0][WARN][EVENT] AIR_LOW low_after=500ms`                  | 气压低或恢复；按参数名区分 `low_after/clear_after` |
| IO 写入失败        | `[T+21.000s][N0][WARN][EVENT] IO_WRITE_FAIL target=2 cmd=0x0003`        | 输出写入失败；检查对应执行器与 IO 通信              |
| USB 插入失败       | `[T+22.000s][N0][WARN][EVENT] USB_INSERT_FAIL elapsed=2001ms reason=timeout` | USB 插入未到位；按 `reason` 检查传感或输出          |
| USB 回退失败       | `[T+23.000s][N0][WARN][EVENT] USB_RETRACT_FAIL elapsed=2001ms reason=sensor_conflict` | USB 回退未到位；按 `reason` 检查传感或输出          |

### USB 异常原因

| `reason` | 说明 |
| -------- | ---- |
| `timeout` | USB 行程超过 2000 ms 未到位 |
| `sensor_conflict` | USB 上位和下位传感同时有效 |
| `dual_output` | USB 插入输出和回退输出同时有效 |
| `unknown` | 未识别的原因码 |

RS485 通信恢复时会产生普通信息日志：

```text
[T+13.500s][N0][INFO][EVENT] RS485_RECOVERED arg0=0 arg1=0
```

该记录表示 IO 链路已经恢复，设备会退出 RS485 故障保护并请求关闭告警灯。`ESTOP` 和 `LASER` 会导致系统开门并锁定；清除急停或遮挡后，重新按下启动键即可解锁恢复。`RISK_PRESSURE` 当前没有单独的恢复事件，应结合后续系统状态、IO 状态和动作日志判断故障是否解除。

### 异常日志读取建议

1. 先发送 `READ:LOG:STATus?` 检查待处理数量和丢失计数。
2. 使用 `READ:LOG:NEXT?` 逐条读取，或使用 `READ:LOG:ALL?` 一次读取当前全部记录。
3. 筛选日志中的 `[WARN]` 和 `[ERROR]`；收到 `RS485_FAULT` 后继续等待 `RS485_RECOVERED`。
4. 若 `DROP` 或 `RDROP` 非零，说明日志不完整，应同时采集设备当前状态和 IO 状态辅助诊断。

> `READ:LOG:NEXT?` 和 `READ:LOG:ALL?` 为出队读取，已返回的记录不会再次返回。日志缓冲区容量当前为 `76` 条，位于 RAM 中，设备复位或掉电后会清空。
>
> `DROP` 表示待读缓冲区满后被覆盖的记录数；`RDROP` 表示开启实时 UART 输出时，实时发送队列来不及发送而丢失的记录数。`CONFigure:LOG:CLEar` 会清空日志及两个丢失计数。
>
> 开启 `CONFigure:LOG:UART ON` 后，日志会在 SCPI 串口上主动输出，属于非请求响应数据。上位机必须能够区分实时日志与 SCPI 指令响应；不能处理异步文本时应保持 `OFF`，改用查询指令轮询。

### 示例

```scpi
CONFigure:LOG:UART ON
READ:LOG:UART?
READ:LOG:STATus?
READ:LOG:NEXT?
READ:LOG:ALL?
CONFigure:LOG:CLEar
```

## 指令总表

| 类别   | 指令                                                                                                                                                                                               |
| ------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 基础   | `*CLS`, `*IDN?`, `*RST`, `*STB?`, `*WAI`, `*OPC?`                                                                                                                                      |
| 系统   | `SYSTem:ERRor[:NEXT]?`, `SYSTem:ERRor:COUNt?`, `SYSTem:VERSion?`, `SYSTem:UPTime?`, `SYSTem:REBoot`, `SYSTem:FLASH:ID?`                                                                |
| IDN    | `SYSTem:IDN1`, `SYSTem:IDN1?`, `SYSTem:IDN2`, `SYSTem:IDN2?`, `SYSTem:IDN3`, `SYSTem:IDN3?`, `SYSTem:IDN4`, `SYSTem:IDN4?`                                                         |
| 执行器 | `CONFigure:CYLInder#`, `READ:CYLInder#:STATe?`, `CONFigure:USB:AUTO`, `READ:USB:AUTO?`, `CONFigure:LOCK`, `READ:LOCK:STATe?`, `CONFigure:LED`, `READ:LED:STATe?`, `CONFigure:LED:MAP`, `READ:LED:MAP?` |
| 状态   | `READ:SYSTem:STATe?`, `READ:IO:ALL?`                                                                                                                                                           |
| 配置   | `CONFigure:BAUDrate`, `CONFigure:ESTOP:TYPE`, `READ:ESTOP:TYPE?`, `CONFigure:RISK:MODE`, `READ:RISK:MODE?`, `CONFigure:BOOT:DIAG`, `READ:BOOT:DIAG?`                  |
| OTA    | `SYSTem:OTA:STATus?`, `SYSTem:OTA:BOOT?`, `SYSTem:OTA:BEGIN`, `SYSTem:OTA:DATA`, `SYSTem:OTA:END`, `SYSTem:OTA:VERify?`, `SYSTem:OTA:COMMit`, `SYSTem:OTA:ABORt`                   |
| 调试   | `CONFigure:DEBUg:STATe`, `READ:DEBUg:STATe?`, `CONFigure:DEBUg:ACTion`, `READ:DEBUg:ACTion?`, `CONFigure:DEBUg:EVENt`, `READ:DEBUg:EVENt?`, `CONFigure:DEBUg:IO`, `READ:DEBUg:IO?` |
| 日志   | `CONFigure:LOG:UART`, `READ:LOG:UART?`, `READ:LOG:NEXT?`, `READ:LOG:ALL?`, `READ:LOG:STATus?`, `CONFigure:LOG:CLEar`                                                                   |

## 修订记录

| 日期 | 新增指令 | 修订内容 |
| ---- | -------- | -------- |
| `2026-07-28` | `CONFigure:CYLInder2`、`READ:CYLInder2:STATe?` | 修订 USB 手动控制和状态返回语义：`CLOSE/CLOSED` 表示 USB 插入/连接，`OPEN/OPENED` 表示 USB 拔出/回退；硬件输出动作不变。 |
| `2026-07-27` | `CONFigure:USB:AUTO`、`READ:USB:AUTO?` | 增加 USB 自动拔插流程相关指令；出厂时已按设备硬件配置完成，禁止修改；带 USB 气缸设备为 `ON`，不带 USB 气缸设备为 `OFF`；客户侧使用 `READ:USB:AUTO?` 确认是否开启。 |
