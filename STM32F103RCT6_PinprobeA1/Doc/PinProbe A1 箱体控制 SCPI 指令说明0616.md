## PinProbe A1 箱体控制 SCPI 指令说明

---

## 设备信息

|项目|说明|
|--|--|
|制造商|GTS|
|型号|PINPROBEA1|
|序列号|20250626|
|固件版本|0.0.9|

## 基础指令（IEEE 488.2 强制指令）

|指令|参数|说明|
|--|--|--|
|`*CLS`||清除状态和错误队列|
|`*IDN?`||查询设备身份信息|
|`*RST`||复位 SCPI 解析器状态|
|`*STB?`||查询状态字节|
|`*WAI`||等待指令完成|
|`*OPC?`||查询操作完成状态|

> `*IDN?` 返回当前 SCPI IDN 配置：`制造商,型号,序列号,固件版本`

## 系统指令

|指令|参数|说明|
|--|--|--|
|`SYSTem:ERRor[:NEXT]?`||查询下一条错误信息|
|`SYSTem:ERRor:COUNt?`||查询错误数量|
|`SYSTem:VERSion?`||查询固件版本和 Git Hash|
|`SYSTem:UPTime?`||查询系统运行时间，单位为秒|

### 示例

```scpi
SYSTem:ERRor[:NEXT]?    # 查询下一条错误
SYSTem:ERRor:COUNt?     # 查询错误数量
SYSTem:VERSion?         # 查询固件版本
SYSTem:UPTime?          # 查询运行时间
```

## 设备身份配置指令

|指令|参数|说明|
|--|--|--|
|`SYSTem:IDN1`|`<string>`|配置制造商字段，并保存到 Flash|
|`SYSTem:IDN1?`||查询制造商字段|
|`SYSTem:IDN2`|`<string>`|配置型号字段，并保存到 Flash|
|`SYSTem:IDN2?`||查询型号字段|
|`SYSTem:IDN3`|`<string>`|配置序列号/日期字段，并保存到 Flash|
|`SYSTem:IDN3?`||查询序列号/日期字段|
|`SYSTem:IDN4`|`<string>`|配置固件版本字段，并保存到 Flash|
|`SYSTem:IDN4?`||查询固件版本字段|

### 示例

```scpi
SYSTem:IDN3?    # 查询当前序列号/日期字段
SYSTem:IDN4?    # 查询当前固件版本字段
*IDN?           # 查询当前全部 IDN 字段
```

## 通信配置指令

|指令|参数|说明|
|--|--|--|
|`CONFigure:BAUDrate`|`115200`|配置 BSM 波特率。当前仅支持 `115200`|

### 示例

```scpi
CONFigure:BAUDrate 115200
```

## 箱门开关控制指令

|指令|参数|说明|
|--|--|--|
|`CONFigure:CYLInder1`|`OPEN` / `CLOSE`|打开或关闭箱门|
|`READ:CYLInder1:STATe?`|[返回值](#执行器状态)|查询箱门当前状态|

## USB 拔插控制指令

|指令|参数|说明|
|--|--|--|
|`CONFigure:CYLInder2`|`OPEN` / `CLOSE`|拔出或插入 USB 接头|
|`READ:CYLInder2:STATe?`|[返回值](#执行器状态)|查询 USB 当前状态|

### 执行器状态

|返回值|说明|
|--|--|
|`CLOSE`|缩回/关闭指令已接收|
|`OPEN`|伸出/打开指令已接收|
|`CLOSING`|正在缩回/关闭|
|`OPENING`|正在伸出/打开|
|`CLOSED`|已缩回/已关闭|
|`OPENED`|已伸出/已打开|
|`CYL ERR`|执行器错误|

### 示例

```scpi
CONFigure:CYLInder1 OPEN     # 打开箱门
CONFigure:CYLInder1 CLOSE    # 关闭箱门
CONFigure:CYLInder2 OPEN     # 拔出 USB
CONFigure:CYLInder2 CLOSE    # 插入 USB
READ:CYLInder1:STATe?        # 查询箱门状态
READ:CYLInder2:STATe?        # 查询 USB 状态
```

## 锁控制指令

|指令|参数|说明|
|--|--|--|
|`CONFigure:LOCK`|[锁状态](#锁状态)|配置设备锁状态|
|`READ:LOCK:STATe?`|[返回值](#锁状态)|查询锁状态|

### 锁状态

|参数/返回值|说明|
|--|--|
|`UNLOCK`|解锁|
|`LOCKED`|锁定|
|`LOCK ERR`|锁错误|

### 示例

```scpi
CONFigure:LOCK LOCKED    # 锁定设备
CONFigure:LOCK UNLOCK    # 解锁设备
READ:LOCK:STATe?         # 查询锁状态
```

## LED 指示灯控制指令

|指令|参数|说明|
|--|--|--|
|`CONFigure:LED`|[LED 状态](#led-状态)|配置 LED 指示灯|
|`READ:LED:STATe?`|[返回值](#led-状态)|查询 LED 状态|

### LED 状态

|参数/返回值|说明|
|--|--|
|`OFF`|LED 关闭|
|`GREEN`|绿色 LED|
|`RED`|红色 LED|
|`YELLOW`|黄色 LED|
|`LED ERR`|LED 错误|

### 示例

```scpi
CONFigure:LED GREEN    # 设置 LED 为绿色
CONFigure:LED RED      # 设置 LED 为红色
CONFigure:LED OFF      # 关闭 LED
READ:LED:STATe?        # 查询 LED 状态
```

## 系统状态查询指令

|指令|参数|说明|
|--|--|--|
|`READ:SYSTem:STATe?`||查询系统状态|

### 系统状态返回值

|返回值|说明|
|--|--|
|`LOCK`|系统处于锁定状态|
|`IDLE`|系统空闲|
|`READY`|系统就绪|
|`RUNNING`|系统运行中|
|`EMERGENCY`|急停触发|
|`COMPLETE`|系统操作完成|
|`SYS ERR`|系统错误|

### 示例

```scpi
READ:SYSTem:STATe?    # 查询系统状态
```

## IO 状态查询指令

|指令|参数|说明|
|--|--|--|
|`READ:IO:ALL?`||查询全部原始输入和输出状态|

### 返回格式

```text
IN:0xHH,0xHH OUT:0xHH,0xHH
```

### 示例

```scpi
READ:IO:ALL?
```

## 急停输入类型指令

|指令|参数|说明|
|--|--|--|
|`CONFigure:ESTOP:TYPE`|`NC` / `NO`|配置急停输入类型，并保存到 Flash|
|`CONFigure:ESTOP:TYPE?`||查询急停输入类型|

### 急停类型

|参数/返回值|说明|
|--|--|
|`NC`|常闭|
|`NO`|常开|

### 示例

```scpi
CONFigure:ESTOP:TYPE NC
CONFigure:ESTOP:TYPE?
```

## Risk Mode 指令

|指令|参数|说明|
|--|--|--|
|`CONFigure:RISK:MODE`|`OFF` / `ON`|配置 Risk Mode，并保存到 Flash|
|`CONFigure:RISK:MODE?`||查询 Risk Mode|

### Risk Mode

|参数/返回值|说明|
|--|--|
|`OFF`|关闭 Risk Mode|
|`ON`|开启 Risk Mode|

### 示例

```scpi
CONFigure:RISK:MODE ON
CONFigure:RISK:MODE?
```
