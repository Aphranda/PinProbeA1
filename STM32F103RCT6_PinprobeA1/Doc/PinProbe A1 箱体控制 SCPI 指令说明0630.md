## PinProbe A1 箱体控制 SCPI 指令说明

---

## 设备信息

| 项目         | 默认值                                              | 说明                                          |
| ------------ | --------------------------------------------------- | --------------------------------------------- |
| 制造商       | `GTS`                                             | `*IDN?` 字段 1，可通过 `SYSTem:IDN1` 修改 |
| 型号         | `PINPROBEA1`                                      | `*IDN?` 字段 2，可通过 `SYSTem:IDN2` 修改 |
| 序列号       | `20250626`                                        | `*IDN?` 字段 3，可通过 `SYSTem:IDN3` 修改 |
| IDN 固件版本 | `V0.0.9`                                          | `*IDN?` 字段 4，可通过 `SYSTem:IDN4` 修改 |
| 编译固件版本 | `v1.0.0+6d11fb1d196d26abd9ee9afceb67ac799df0fbb9` | `SYSTem:VERSion?` 返回                      |

> `*IDN?` 返回当前 SCPI IDN 配置：`制造商,型号,序列号,IDN固件版本`。IDN 字段保存在 Flash 配置中，可能与编译固件版本不同。

## 通信约定

| 项目         | 说明                                                                      |
| ------------ | ------------------------------------------------------------------------- |
| 串口波特率   | `115200`                                                                |
| 指令结束符   | `\r\n`                                                                  |
| 普通响应     | 单行文本                                                                  |
| 日志批量响应 | `READ:LOG:ALL?` 可能返回多行文本                                        |
| OTA 数据块   | 使用 SCPI definite-length binary block，当前单块 payload 最大`128` 字节 |

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
| `SYSTem:VERSion?`      |      | `v1.0.0+<git-hash>`                              | 查询编译固件版本和 Git Hash        |
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
SYSTem:IDN3 "20250630"
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

## 箱门与 USB 执行器指令

| 指令                      | 参数                 | 响应                     | 说明               |
| ------------------------- | -------------------- | ------------------------ | ------------------ |
| `CONFigure:CYLInder1`   | `OPEN` / `CLOSE` | `OPEN` / `CLOSE`     | 打开或关闭箱门     |
| `READ:CYLInder1:STATe?` |                      | [执行器状态](#执行器状态) | 查询箱门当前状态   |
| `CONFigure:CYLInder2`   | `OPEN` / `CLOSE` | `OPEN` / `CLOSE`     | USB 接头伸出或缩回 |
| `READ:CYLInder2:STATe?` |                      | [执行器状态](#执行器状态) | 查询 USB 当前状态  |

### 执行器状态

| 返回值      | 说明                |
| ----------- | ------------------- |
| `CLOSE`   | 缩回/关闭指令已接收 |
| `OPEN`    | 伸出/打开指令已接收 |
| `CLOSING` | 正在缩回/关闭       |
| `OPENING` | 正在伸出/打开       |
| `CLOSED`  | 已缩回/已关闭       |
| `OPENED`  | 已伸出/已打开       |
| `CYL ERR` | 执行器错误          |

### 示例

```scpi
CONFigure:CYLInder1 OPEN
CONFigure:CYLInder1 CLOSE
READ:CYLInder1:STATe?
CONFigure:CYLInder2 OPEN
CONFigure:CYLInder2 CLOSE
READ:CYLInder2:STATe?
```

## 锁控制指令

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
| `CONFigure:LED:MAP?` |                                            | `G,R,Y` 格式 | 查询 IO5/IO6/IO7 对应的 LED 颜色                 |

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
CONFigure:LED:MAP?
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
| `CONFigure:ESTOP:TYPE?` |                  | `NC` / `NO`  | 查询急停输入类型                             |
| `CONFigure:RISK:MODE`   | `OFF` / `ON` | `OFF` / `ON` | 配置 Risk Mode，并保存到 Flash               |
| `CONFigure:RISK:MODE?`  |                  | `OFF` / `ON` | 查询 Risk Mode                               |
| `CONFigure:BOOT:DIAG`   | `OFF` / `ON` | `OFF` / `ON` | 配置 Bootloader 诊断串口输出，并保存到 Flash |
| `CONFigure:BOOT:DIAG?`  |                  | `OFF` / `ON` | 查询 Bootloader 诊断串口输出开关             |

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
CONFigure:ESTOP:TYPE?
CONFigure:RISK:MODE ON
CONFigure:RISK:MODE?
CONFigure:BOOT:DIAG ON
CONFigure:BOOT:DIAG?
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

## AppLog 日志指令

| 指令                    | 参数             | 响应                          | 说明                            |
| ----------------------- | ---------------- | ----------------------------- | ------------------------------- |
| `CONFigure:LOG:UART`  | `OFF` / `ON` | `OFF` / `ON`              | 开启/关闭 AppLog 实时 UART 输出 |
| `READ:LOG:UART?`      |                  | `OFF` / `ON`              | 查询实时 UART 输出开关          |
| `READ:LOG:NEXT?`      |                  | 一条日志或`EMPTY`           | 读取下一条 AppLog 记录          |
| `READ:LOG:ALL?`       |                  | 多行日志或`EMPTY`           | 读取当前全部待读 AppLog 记录    |
| `READ:LOG:STATus?`    |                  | `PEND:... CAP:... NEXT:...` | 查询 AppLog 缓冲区状态          |
| `CONFigure:LOG:CLEar` |                  | `OK`                        | 清空 AppLog 缓冲区              |

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

| 字段                                        | 说明                                                       |
| ------------------------------------------- | ---------------------------------------------------------- |
| `T+`                                      | 设备本次启动后的相对时间，不是日期时间；设备复位后重新计时 |
| `N`                                       | 节点号，单机默认通常为`N0`                               |
| `INFO`                                    | 普通状态、动作、命令或恢复事件                             |
| `WARN`                                    | 已检测到需要关注的异常或安全事件                           |
| `ERROR`                                   | 严重错误级别；当前版本预留，现有异常事件均输出为`WARN`   |
| `STATE` / `ACTION` / `EVENT` / `IO` | 状态迁移、执行动作、离散事件或 IO 快照                     |

### 异常日志说明

上位机应以日志级别字段判断异常，当前版本将以下事件记录为 `[WARN][EVENT]`：

| 异常事件           | 日志示例                                                            | 含义及参数说明                                                                                                                    |
| ------------------ | ------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------- |
| RS485 通信故障     | `[T+12.345s][N0][WARN][EVENT] RS485_FAULT arg0=0 arg1=0`          | 连续通信失败达到固件阈值，IO 链路不可用；状态机暂停本轮自动操作并请求黄灯告警                                                     |
| 急停触发           | `[T+15.120s][N0][WARN][EVENT] ESTOP close_elapsed=820ms`          | 急停在关门流程中触发；`close_elapsed` 为从关门开始到触发的时间                                                                  |
| 异物阻挡           | `[T+16.020s][N0][WARN][EVENT] LASER close_elapsed=1720ms`         | 关门流程中检测到异物阻挡；`close_elapsed` 为从关门开始到触发的时间                                                              |
| Risk Mode 压力事件 | `[T+16.100s][N0][WARN][EVENT] RISK_PRESSURE close_elapsed=1800ms` | Risk Mode 下关门压力条件触发；`close_elapsed` 为从关门开始到触发的时间                                                          |
| 气压低/恢复        | `[T+20.000s][N0][WARN][EVENT] AIR_LOW low_after=500ms`            | `low_after` 表示气压低持续时间；`clear_after` 表示气压恢复前的持续时间。两种记录当前均为 `WARN`，须通过参数名区分触发与恢复 |
| IO 写入失败        | `[T+21.000s][N0][WARN][EVENT] IO_WRITE_FAIL target=2 cmd=0x0003`  | 执行器写 IO 失败；`target=1/2/3` 分别表示气缸 1、气缸 2、LED，`cmd` 为内部命令值                                              |

RS485 通信恢复时会产生普通信息日志：

```text
[T+13.500s][N0][INFO][EVENT] RS485_RECOVERED arg0=0 arg1=0
```

该记录表示 IO 链路已经恢复，固件会退出 RS485 故障保护并请求关闭告警灯。`ESTOP`、`LASER` 和 `RISK_PRESSURE` 当前没有单独的恢复事件，应结合后续系统状态、IO 状态和动作日志判断故障是否解除。

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
| 执行器 | `CONFigure:CYLInder#`, `READ:CYLInder#:STATe?`, `CONFigure:LOCK`, `READ:LOCK:STATe?`, `CONFigure:LED`, `READ:LED:STATe?`, `CONFigure:LED:MAP`, `CONFigure:LED:MAP?`                |
| 状态   | `READ:SYSTem:STATe?`, `READ:IO:ALL?`                                                                                                                                                           |
| 配置   | `CONFigure:BAUDrate`, `CONFigure:ESTOP:TYPE`, `CONFigure:ESTOP:TYPE?`, `CONFigure:RISK:MODE`, `CONFigure:RISK:MODE?`, `CONFigure:BOOT:DIAG`, `CONFigure:BOOT:DIAG?`                  |
| OTA    | `SYSTem:OTA:STATus?`, `SYSTem:OTA:BOOT?`, `SYSTem:OTA:BEGIN`, `SYSTem:OTA:DATA`, `SYSTem:OTA:END`, `SYSTem:OTA:VERify?`, `SYSTem:OTA:COMMit`, `SYSTem:OTA:ABORt`                   |
| 调试   | `CONFigure:DEBUg:STATe`, `READ:DEBUg:STATe?`, `CONFigure:DEBUg:ACTion`, `READ:DEBUg:ACTion?`, `CONFigure:DEBUg:EVENt`, `READ:DEBUg:EVENt?`, `CONFigure:DEBUg:IO`, `READ:DEBUg:IO?` |
| 日志   | `CONFigure:LOG:UART`, `READ:LOG:UART?`, `READ:LOG:NEXT?`, `READ:LOG:ALL?`, `READ:LOG:STATus?`, `CONFigure:LOG:CLEar`                                                                   |
