## PinProbe A1 箱体控制 SCPI 指令说明

---

## 设备信息

|项目|内容|
|--|--|
|制造商|GTS|
|型号|PINPROBEA1|
|序列号|20250626|
|固件版本|V0.0.1|

## 基础指令（IEEE 488.2 强制指令）

|指令|参数|说明|
|--|--|--|
|`*CLS`||清除状态|
|`*IDN?`||查询设备身份信息|
|`*RST`||复位设备|
|`*STB?`||查询状态字节|
|`*WAI`||等待指令完成|
|`*OPC?`||查询操作完成状态|

> `*IDN?` 返回格式：`GTS,PINPROBEA1,20250626,V0.0.1`

## 系统指令

|指令|参数|说明|
|--|--|--|
|`SYSTem:ERRor[:NEXT]?`||查询下一个错误信息|
|`SYSTem:ERRor:COUNt?`||查询错误数量|

## 开关门控制指令

|指令|参数|说明|
|--|--|--|
|`CONFigure:CYLInder1`|`OPEN` / `CLOSE` 或 [执行状态](#执行状态)|控制箱门打开或关闭|
|`READ:CYLInder1:STATe?`|[返回值](#执行状态)|查询箱门当前状态|

## USB 拔插控制指令

|指令|参数|说明|
|--|--|--|
|`CONFigure:CYLInder2`|`OPEN` / `CLOSE` 或 [执行状态](#执行状态)|控制 USB 拔出或插入|
|`READ:CYLInder2:STATe?`|[返回值](#执行状态)|查询 USB 当前状态|

### 执行状态

|参数|说明|
|--|--|
|`CLOSE`|关闭 / 缩回（门关闭 / USB插入）|
|`OPEN`|打开 / 伸出（门打开 / USB拔出）|
|`CLOSING`|正在关闭 / 缩回中|
|`OPENING`|正在打开 / 伸出中|
|`CLOSED`|已关闭 / 已缩回|
|`OPENED`|已打开 / 已伸出|
|`CYL ERR`|执行错误|

### 示例

```
CONFigure:CYLInder1 OPEN    # 开门
CONFigure:CYLInder1 CLOSE   # 关门
CONFigure:CYLInder2 OPEN    # 拔出 USB
CONFigure:CYLInder2 CLOSE   # 插入 USB
READ:CYLInder1:STATe?       # 查询箱门状态
READ:CYLInder2:STATe?       # 查询 USB 状态
```

## 锁定控制指令

|指令|参数|说明|
|--|--|--|
|`CONFigure:LOCK`|[锁定状态](#锁定状态)|配置设备锁定状态|
|`READ:LOCK:STATe?`|[返回值](#可选锁定状态)|查询锁定状态|

### 锁定状态

|参数|说明|
|--|--|
|`UNLOCK`|解锁|
|`LOCKED`|锁定|
|`LOCK ERR`|锁定错误|

### 示例

```
CONFigure:LOCK LOCKED    # 锁定设备
CONFigure:LOCK UNLOCK    # 解锁设备
READ:LOCK:STATe?         # 查询锁定状态
```

## LED 指示灯控制指令

|指令|参数|说明|
|--|--|--|
|`CONFigure:LED`|[LED状态](#可选-led-状态)|配置LED指示灯|
|`READ:LED:STATe?`|[返回值](#可选-led-状态)|查询LED状态|

### LED 状态

|参数|说明|
|--|--|
|`OFF`|LED 关闭|
|`GREEN`|LED 绿色指示灯|
|`RED`|LED 红色指示灯|
|`YELLOW`|LED 黄色指示灯|
|`LED ERR`|LED 错误|

### 示例

```
CONFigure:LED GREEN    # 设置LED为绿色
CONFigure:LED RED      # 设置LED为红色
CONFigure:LED OFF      # 关闭LED
READ:LED:STATe?        # 查询LED状态
```

## 系统状态查询指令

|指令|参数|说明|
|--|--|--|
|`READ:SYSTem:STATe?`||查询系统状态|

### 系统状态返回值

|返回值|说明|
|--|--|
|`LOCK`|系统处于锁定状态|
|`IDLE`|系统处于空闲状态|
|`READY`|系统处于就绪状态|
|`RUNNING`|系统运行中|
|`EMERGENCY`|系统紧急停止（光栅触发或急停按下）|
|`COMPLETE`|系统运行完成|
|`SYS ERR`|系统错误|

### 示例

```
READ:SYSTem:STATe?    # 查询系统状态
```
