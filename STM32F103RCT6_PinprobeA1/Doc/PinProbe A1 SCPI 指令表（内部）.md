# PinProbe A1 SCPI 指令表（内部开发用）

> **来源**: [`Hardware/libscpi/port/scpi-def.c`](../Hardware/libscpi/port/scpi-def.c:567) — `scpi_commands[]` 命令表  
> **参考文档**: [`Doc/PinProbe A1 箱体控制 SCPI 指令说明.md`](PinProbe%20A1%20箱体控制%20SCPI%20指令说明.md)  
> **状态**: 此文档基于源代码自动整理，与实际固件行为保持一致

---

## 目录

- [1. IEEE 488.2 强制指令](#1-ieee-4882-强制指令)
- [2. SCPI 标准必需指令](#2-scpi-标准必需指令)
- [3. 系统身份信息配置指令](#3-系统身份信息配置指令)
- ~~[4. 射频开关控制指令（已废弃）](#4-射频开关控制指令已废弃)~~
- [5. 波特率配置指令](#5-波特率配置指令)
- [6. 气缸（门/USB）控制指令](#6-气缸门usb控制指令)
- [7. 锁定控制指令](#7-锁定控制指令)
- [8. LED 指示灯控制指令](#8-led-指示灯控制指令)
- [9. 系统状态查询指令](#9-系统状态查询指令)
- ~~[10. 链路切换指令（已废弃）](#10-链路切换指令已废弃)~~

---

## 1. IEEE 488.2 强制指令

6 条基础指令，定义于 [`scpi-def.c:567-592`](../Hardware/libscpi/port/scpi-def.c:567)。

| # | 指令 | 类型 | 回调函数 | 说明 |
|---|------|------|----------|------|
| 1 | `*CLS` | 写 | `SCPI_CoreCls` | 清除状态寄存器和错误队列 |
| 2 | `*IDN?` | 查询 | `SCPI_CoreIdnQ` | 查询设备身份信息，返回格式: `GTS,PINPROBEA1,20250626,V0.0.1` |
| 3 | `*RST` | 写 | `SCPI_CoreRst` | 复位设备到默认状态 |
| 4 | `*STB?` | 查询 | `SCPI_CoreStbQ` | 查询状态字节 (Status Byte) |
| 5 | `*WAI` | 写 | `SCPI_CoreWai` | 等待所有待处理操作完成 |
| 6 | `*OPC?` | 查询 | `SCPI_CoreOpcQ` | 查询操作完成状态，返回 `1` 表示完成 |

### 示例

```
*IDN?                  → GTS,PINPROBEA1,20250626,V0.0.1
*STB?                  → <status_byte_value>
*OPC?                  → 1
*CLS                   (清除状态)
*RST                   (复位设备)
*WAI                   (等待完成)
```

---

## 2. SCPI 标准必需指令

2 条标准指令，定义于 [`scpi-def.c:593-601`](../Hardware/libscpi/port/scpi-def.c:593)。

| # | 指令 | 类型 | 回调函数 | 说明 |
|---|------|------|----------|------|
| 7 | `SYSTem:ERRor[:NEXT]?` | 查询 | `SCPI_SystemErrorNextQ` | 查询并弹出错误队列中的下一个错误信息 |
| 8 | `SYSTem:ERRor:COUNt?` | 查询 | `SCPI_SystemErrorCountQ` | 查询当前错误队列中的错误数量 |

### 示例

```
SYSTem:ERRor:NEXT?     → -113,"Undefined header"
SYSTem:ERRor:COUNt?    → 0
```

---

## 3. 系统身份信息配置指令

8 条指令：`SYSTem:IDN1`~`IDN4` 读写各 4 条。这四个字段对应 `*IDN?` 返回的四个部分，可在运行时通过 SCPI 命令修改并持久化到 Flash。

定义于 [`scpi-def.c:602-634`](../Hardware/libscpi/port/scpi-def.c:602)。

| # | 指令 | 类型 | 回调函数 | 参数 | 说明 |
|---|------|------|----------|------|------|
| 9 | `SYSTem:IDN1` | 写 | `SCPI_SetIdn1` | `<string>` | 设置厂商名（制造商） |
| 10 | `SYSTem:IDN1?` | 查询 | `SCPI_ReadIdn1Q` | — | 查询厂商名 |
| 11 | `SYSTem:IDN2` | 写 | `SCPI_SetIdn2` | `<string>` | 设置产品型号 |
| 12 | `SYSTem:IDN2?` | 查询 | `SCPI_ReadIdn2Q` | — | 查询产品型号 |
| 13 | `SYSTem:IDN3` | 写 | `SCPI_SetIdn3` | `<string>` | 设置序列号/日期 |
| 14 | `SYSTem:IDN3?` | 查询 | `SCPI_ReadIdn3Q` | — | 查询序列号/日期 |
| 15 | `SYSTem:IDN4` | 写 | `SCPI_SetIdn4` | `<string>` | 设置固件版本号 |
| 16 | `SYSTem:IDN4?` | 查询 | `SCPI_ReadIdn4Q` | — | 查询固件版本号 |

### 示例

```
SYSTem:IDN1 "MyCompany"
SYSTem:IDN1?           → MyCompany
SYSTem:IDN2 "PINPROBEA1"
SYSTem:IDN2?           → PINPROBEA1
SYSTem:IDN3 "20250626"
SYSTem:IDN3?           → 20250626
SYSTem:IDN4 "V1.0.0"
SYSTem:IDN4?           → V1.0.0
*IDN?                  → MyCompany,PINPROBEA1,20250626,V1.0.0
```

---

## ~~4. 射频开关控制指令（已废弃）~~

> ⚠️ **以下指令已废弃，不应在新开发中使用**

| # | 指令 | 类型 | 回调函数 | 说明 |
|---|------|------|----------|------|
| ~~17~~ | ~~`CONFigure:SWITch#`~~ | ~~写~~ | ~~`SCPI_ConfigureSwitch`~~ | ~~配置射频开关通道~~ |
| ~~18~~ | ~~`READ:SWITch#:STATe?`~~ | ~~查询~~ | ~~`SCPI_ReadSwitchState`~~ | ~~查询指定射频开关通道的当前状态~~ |

### 废弃原因

射频开关控制目前直接通过底层 GPIO 控制，上层 SCPI 指令已不再使用。如有需要应通过 `CONFigure:LINK` 方式（也已废弃）控制链路切换，或直接操作硬件寄存器。

### 遗留代码位置

- 命令表条目: [`scpi-def.c:635-646`](../Hardware/libscpi/port/scpi-def.c:635)
- 回调实现 `SCPI_ConfigureSwitch`: [`scpi-def.c:255-274`](../Hardware/libscpi/port/scpi-def.c:255)
- 回调实现 `SCPI_ReadSwitchState`: [`scpi-def.c:344-358`](../Hardware/libscpi/port/scpi-def.c:344)
- 底层 `Switch_Write` / `Switch_Read`: [`scpi_switch.c:93-170`](../Hardware/RFLink/src/scpi_switch.c:93)

---

## 5. 波特率配置指令

1 条指令，定义于 [`scpi-def.c:639-642`](../Hardware/libscpi/port/scpi-def.c:639)。

| # | 指令 | 类型 | 回调函数 | 参数 | 说明 |
|---|------|------|----------|------|------|
| 19 | `CONFigure:BAUDrate` | 写 | `SCPI_Configurebaudrate` | `<115200>` | 配置 BSM 模块串口波特率为 115200 |

> 实现代码: [`scpi-def.c:276-341`](../Hardware/libscpi/port/scpi-def.c:276)

### 示例

```
CONFigure:BAUDrate 115200    → 115200 baudrate is enable
CONFigure:BAUDrate 9600      → ERROR: Only 115200 baudrate is supported
```

---

## 6. 气缸（门/USB）控制指令

2 条指令，定义于 [`scpi-def.c:655-662`](../Hardware/libscpi/port/scpi-def.c:655)。

| # | 指令 | 类型 | 回调函数 | 参数 | 说明 |
|---|------|------|----------|------|------|
| 20 | `CONFigure:CYLInder#` | 写 | `SCPI_ConfigureCylinder` | `OPEN` / `CLOSE` | 控制气缸动作（#=气缸编号） |
| 21 | `READ:CYLInder#:STATe?` | 查询 | `SCPI_ReadCylinderState` | — | 查询指定气缸的当前状态 |

### 气缸编号

| 编号 | 物理设备 | 说明 |
|------|----------|------|
| `CYLInder1` | 箱门气缸 | 控制测试箱箱门的打开/关闭 |
| `CYLInder2` | USB 插拔气缸 | 控制 USB 连接器的插入/拔出 |

### 参数 / 返回值

| 值 | 说明 |
|----|------|
| `CLOSE` | 关闭 / 缩回（门关闭 / USB 插入） |
| `OPEN` | 打开 / 伸出（门打开 / USB 拔出） |
| `CLOSING` | 正在关闭 / 缩回中（仅查询） |
| `OPENING` | 正在打开 / 伸出中（仅查询） |
| `CLOSED` | 已关闭 / 已缩回（仅查询） |
| `OPENED` | 已打开 / 已伸出（仅查询） |
| `CYL ERR` | 执行错误 |

> 实现代码: [`scpi-def.c:433-469`](../Hardware/libscpi/port/scpi-def.c:433)

### 示例

```
CONFigure:CYLInder1 OPEN     → OPENED
CONFigure:CYLInder1 CLOSE    → CLOSED
CONFigure:CYLInder2 OPEN     → OPENED
CONFigure:CYLInder2 CLOSE    → CLOSED
READ:CYLInder1:STATe?        → CLOSED
READ:CYLInder2:STATe?        → CLOSED
```

---

## 7. 锁定控制指令

2 条指令，定义于 [`scpi-def.c:663-670`](../Hardware/libscpi/port/scpi-def.c:663)。

| # | 指令 | 类型 | 回调函数 | 参数 | 说明 |
|---|------|------|----------|------|------|
| 22 | `CONFigure:LOCK` | 写 | `SCPI_ConfigureLOCK` | `LOCKED` / `UNLOCK` | 配置设备锁定/解锁 |
| 23 | `READ:LOCK:STATe?` | 查询 | `SCPI_ReadLOCKState` | — | 查询当前锁定状态 |

### 参数 / 返回值

| 值 | 说明 |
|----|------|
| `UNLOCK` | 解锁 |
| `LOCKED` | 锁定 |
| `LOCK ERR` | 锁定错误（仅查询） |

> 实现代码: [`scpi-def.c:478-506`](../Hardware/libscpi/port/scpi-def.c:478)

### 示例

```
CONFigure:LOCK LOCKED    → LOCKED
CONFigure:LOCK UNLOCK    → UNLOCK
READ:LOCK:STATe?         → UNLOCK
```

---

## 8. LED 指示灯控制指令

2 条指令，定义于 [`scpi-def.c:671-678`](../Hardware/libscpi/port/scpi-def.c:671)。

| # | 指令 | 类型 | 回调函数 | 参数 | 说明 |
|---|------|------|----------|------|------|
| 24 | `CONFigure:LED` | 写 | `SCPI_ConfigureLED` | 见下方颜色值 | 设置 LED 指示灯颜色 |
| 25 | `READ:LED:STATe?` | 查询 | `SCPI_ReadLEDState` | — | 查询当前 LED 状态 |

### 参数 / 返回值

| 值 | 说明 |
|----|------|
| `OFF` | LED 关闭 |
| `GREEN` | LED 绿色 |
| `RED` | LED 红色 |
| `YELLOW` | LED 黄色 |
| `LED ERR` | LED 错误（仅查询） |

> 实现代码: [`scpi-def.c:517-546`](../Hardware/libscpi/port/scpi-def.c:517)

### 示例

```
CONFigure:LED GREEN     → GREEN
CONFigure:LED RED       → RED
CONFigure:LED YELLOW    → YELLOW
CONFigure:LED OFF       → OFF
READ:LED:STATe?         → GREEN
```

---

## 9. 系统状态查询指令

1 条指令，定义于 [`scpi-def.c:679-682`](../Hardware/libscpi/port/scpi-def.c:679)。

| # | 指令 | 类型 | 回调函数 | 参数 | 说明 |
|---|------|------|----------|------|------|
| 26 | `READ:SYSTem:STATe?` | 查询 | `SCPI_ReadSystemState` | — | 查询系统整体运行状态 |

### 返回值

| 返回值 | 说明 |
|--------|------|
| `LOCK` | 系统处于锁定状态 |
| `IDLE` | 系统处于空闲状态 |
| `READY` | 系统处于就绪状态 |
| `RUNNING` | 系统运行中（测试执行中） |
| `EMERGENCY` | 紧急停止（光栅触发或急停按下） |
| `COMPLETE` | 系统运行完成 |
| `SYS ERR` | 系统错误 |

> 实现代码: [`scpi-def.c:558-565`](../Hardware/libscpi/port/scpi-def.c:558)

### 示例

```
READ:SYSTem:STATe?    → IDLE
READ:SYSTem:STATe?    → RUNNING
READ:SYSTem:STATe?    → COMPLETE
READ:SYSTem:STATe?    → EMERGENCY
```

---

## ~~10. 链路切换指令（已废弃）~~

> ⚠️ **以下指令已废弃，不应在新开发中使用**

| # | 指令 | 类型 | 回调函数 | 说明 |
|---|------|------|----------|------|
| ~~27~~ | ~~`CONFigure:LINK`~~ | ~~写~~ | ~~`SCPI_ConfigureLink`~~ | ~~配置射频链路切换到指定端口~~ |
| ~~28~~ | ~~`READ:LINK:STATe?`~~ | ~~查询~~ | ~~`SCPI_ReadLinkState`~~ | ~~查询当前链路连接的目标端口~~ |

### 废弃原因

链路切换功能已被废弃，如有需要应直接通过硬件 GPIO 控制射频开关矩阵。

### 遗留代码位置

- 命令表条目: [`scpi-def.c:647-654`](../Hardware/libscpi/port/scpi-def.c:647)
- 回调实现 `SCPI_ConfigureLink`: [`scpi-def.c:381-408`](../Hardware/libscpi/port/scpi-def.c:381)
- 回调实现 `SCPI_ReadLinkState`: [`scpi-def.c:410-420`](../Hardware/libscpi/port/scpi-def.c:410)
- 底层 `Link_Write` / `Link_Read`: [`scpi_switch.c:4-91`](../Hardware/RFLink/src/scpi_switch.c:4)

---

## 附录 A：命令总览表

| # | 完整指令 | 类型 | 分类 | 回调函数 | 源码行号 |
|---|----------|------|------|----------|----------|
| 1 | `*CLS` | 写 | IEEE 488.2 | `SCPI_CoreCls` | [`569`](../Hardware/libscpi/port/scpi-def.c:569) |
| 2 | `*IDN?` | 查询 | IEEE 488.2 | `SCPI_CoreIdnQ` | [`573`](../Hardware/libscpi/port/scpi-def.c:573) |
| 3 | `*RST` | 写 | IEEE 488.2 | `SCPI_CoreRst` | [`577`](../Hardware/libscpi/port/scpi-def.c:577) |
| 4 | `*STB?` | 查询 | IEEE 488.2 | `SCPI_CoreStbQ` | [`581`](../Hardware/libscpi/port/scpi-def.c:581) |
| 5 | `*WAI` | 写 | IEEE 488.2 | `SCPI_CoreWai` | [`585`](../Hardware/libscpi/port/scpi-def.c:585) |
| 6 | `*OPC?` | 查询 | IEEE 488.2 | `SCPI_CoreOpcQ` | [`589`](../Hardware/libscpi/port/scpi-def.c:589) |
| 7 | `SYSTem:ERRor[:NEXT]?` | 查询 | SCPI 标准 | `SCPI_SystemErrorNextQ` | [`595`](../Hardware/libscpi/port/scpi-def.c:595) |
| 8 | `SYSTem:ERRor:COUNt?` | 查询 | SCPI 标准 | `SCPI_SystemErrorCountQ` | [`599`](../Hardware/libscpi/port/scpi-def.c:599) |
| 9 | `SYSTem:IDN1` | 写 | 系统配置 | `SCPI_SetIdn1` | [`603`](../Hardware/libscpi/port/scpi-def.c:603) |
| 10 | `SYSTem:IDN1?` | 查询 | 系统配置 | `SCPI_ReadIdn1Q` | [`607`](../Hardware/libscpi/port/scpi-def.c:607) |
| 11 | `SYSTem:IDN2` | 写 | 系统配置 | `SCPI_SetIdn2` | [`611`](../Hardware/libscpi/port/scpi-def.c:611) |
| 12 | `SYSTem:IDN2?` | 查询 | 系统配置 | `SCPI_ReadIdn2Q` | [`615`](../Hardware/libscpi/port/scpi-def.c:615) |
| 13 | `SYSTem:IDN3` | 写 | 系统配置 | `SCPI_SetIdn3` | [`619`](../Hardware/libscpi/port/scpi-def.c:619) |
| 14 | `SYSTem:IDN3?` | 查询 | 系统配置 | `SCPI_ReadIdn3Q` | [`623`](../Hardware/libscpi/port/scpi-def.c:623) |
| 15 | `SYSTem:IDN4` | 写 | 系统配置 | `SCPI_SetIdn4` | [`627`](../Hardware/libscpi/port/scpi-def.c:627) |
| 16 | `SYSTem:IDN4?` | 查询 | 系统配置 | `SCPI_ReadIdn4Q` | [`631`](../Hardware/libscpi/port/scpi-def.c:631) |
| 19 | `CONFigure:BAUDrate` | 写 | 系统配置 | `SCPI_Configurebaudrate` | [`639`](../Hardware/libscpi/port/scpi-def.c:639) |
| 20 | `CONFigure:CYLInder#` | 写 | 气缸控制 | `SCPI_ConfigureCylinder` | [`655`](../Hardware/libscpi/port/scpi-def.c:655) |
| 21 | `READ:CYLInder#:STATe?` | 查询 | 气缸控制 | `SCPI_ReadCylinderState` | [`659`](../Hardware/libscpi/port/scpi-def.c:659) |
| 22 | `CONFigure:LOCK` | 写 | 锁定控制 | `SCPI_ConfigureLOCK` | [`663`](../Hardware/libscpi/port/scpi-def.c:663) |
| 23 | `READ:LOCK:STATe?` | 查询 | 锁定控制 | `SCPI_ReadLOCKState` | [`667`](../Hardware/libscpi/port/scpi-def.c:667) |
| 24 | `CONFigure:LED` | 写 | LED 控制 | `SCPI_ConfigureLED` | [`671`](../Hardware/libscpi/port/scpi-def.c:671) |
| 25 | `READ:LED:STATe?` | 查询 | LED 控制 | `SCPI_ReadLEDState` | [`675`](../Hardware/libscpi/port/scpi-def.c:675) |
| 26 | `READ:SYSTem:STATe?` | 查询 | 系统状态 | `SCPI_ReadSystemState` | [`679`](../Hardware/libscpi/port/scpi-def.c:679) |
| ~~17~~ | ~~`CONFigure:SWITch#`~~ | ~~写~~ | ~~射频开关（废弃）~~ | ~~`SCPI_ConfigureSwitch`~~ | [`635`](../Hardware/libscpi/port/scpi-def.c:635) |
| ~~18~~ | ~~`READ:SWITch#:STATe?`~~ | ~~查询~~ | ~~射频开关（废弃）~~ | ~~`SCPI_ReadSwitchState`~~ | [`643`](../Hardware/libscpi/port/scpi-def.c:643) |
| ~~27~~ | ~~`CONFigure:LINK`~~ | ~~写~~ | ~~链路切换（废弃）~~ | ~~`SCPI_ConfigureLink`~~ | [`647`](../Hardware/libscpi/port/scpi-def.c:647) |
| ~~28~~ | ~~`READ:LINK:STATe?`~~ | ~~查询~~ | ~~链路切换（废弃）~~ | ~~`SCPI_ReadLinkState`~~ | [`651`](../Hardware/libscpi/port/scpi-def.c:651) |

## 附录 B：代码结构索引

| 符号 | 文件 | 行号 | 说明 |
|------|------|------|------|
| `scpi_commands[]` | [`scpi-def.c`](../Hardware/libscpi/port/scpi-def.c) | 567 | SCPI 命令表（解析器入口） |
| `scpi_interface` | [`scpi-def.c`](../Hardware/libscpi/port/scpi-def.c) | 685 | SCPI 接口回调（write/error/control/flush/reset） |
| `scpi_context` | [`scpi-def.c`](../Hardware/libscpi/port/scpi-def.c) | 696 | SCPI 全局上下文实例 |
| `SCPI_SyncIdnFromFlash()` | [`scpi-def.c`](../Hardware/libscpi/port/scpi-def.c) | 66 | 从 Flash 同步 IDN 字符串 |
| `cylinder_source[]` | [`scpi-def.c`](../Hardware/libscpi/port/scpi-def.c) | 422 | 气缸状态选择表 |
| `lock_source[]` | [`scpi-def.c`](../Hardware/libscpi/port/scpi-def.c) | 471 | 锁定状态选择表 |
| `led_source[]` | [`scpi-def.c`](../Hardware/libscpi/port/scpi-def.c) | 508 | LED 状态选择表 |
| `sys_source[]` | [`scpi-def.c`](../Hardware/libscpi/port/scpi-def.c) | 548 | 系统状态选择表 |
| `link_source[]` | [`scpi-def.c`](../Hardware/libscpi/port/scpi-def.c) | 360 | 链路端口选择表（废弃） |
| `Switch_Write()` | [`scpi_switch.c`](../Hardware/RFLink/src/scpi_switch.c) | 93 | 射频开关写入（废弃） |
| `Switch_Read()` | [`scpi_switch.c`](../Hardware/RFLink/src/scpi_switch.c) | 124 | 射频开关读取（废弃） |
| `Link_Write()` | [`scpi_switch.c`](../Hardware/RFLink/src/scpi_switch.c) | 4 | 链路切换写入（废弃） |
| `Link_Read()` | [`scpi_switch.c`](../Hardware/RFLink/src/scpi_switch.c) | 64 | 链路切换读取（废弃） |
| `Cylinder_Write()` / `Cylinder_Status()` | [`BsmRelay.c`](../Hardware/BsmRelay/Src/BsmRelay.c) | — | 气缸控制/状态 |
| `Lock_Write()` / `Lock_Status()` | [`BsmRelay.c`](../Hardware/BsmRelay/Src/BsmRelay.c) | — | 锁定控制/状态 |
| `LED_Write()` / `LED_Status()` | [`BsmRelay.c`](../Hardware/BsmRelay/Src/BsmRelay.c) | — | LED 控制/状态 |
| `SYS_Status()` | [`BsmRelay.c`](../Hardware/BsmRelay/Src/BsmRelay.c) | — | 系统状态读取 |
| `huart3` | [`usart.c`](../Core/Src/usart.c) | — | UART3 句柄（BSM 通信） |

---

*文档生成时间: 2026-05-08 | 基于 [`scpi-def.c`](../Hardware/libscpi/port/scpi-def.c) `scpi_commands[]` 自动整理*
