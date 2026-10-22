# 今日工作总结 2026-06-11

## 一、架构重构

### 1.1 RS485 通讯层独立
- RS485 模块从 `Hardware/BsmRelay` 迁移到 `Hardware/ModBus`
- ModBusTask 成为唯一 RS485 操作者（30ms→25ms SysTimer 周期）
- StateVectorTask 纯逻辑，不碰 RS485（20ms→25ms SysTimer 周期）
- SCPI 全部改为读向量表 + PostCmd，不再直接写 RS485

### 1.2 向量表完善
- 补齐 node_status[8]（64B），1024B 完全对齐主线布局
- node=0 单机出厂模式，node=1-8 多机模式
- 散列文件 `RW_RAMVEC @ 0x2000A000`，编译时大小校验

### 1.3 任务调度优化
- SysTimer (25ms 硬件定时器) 替代 osDelay 做精确节拍
- ModBusTask: Normal1 优先级，RS485 IO + CmdExec
- StateVectorTask: Normal 优先级，纯逻辑
- defaultTask: 集群健康监控占位（1s 周期）
- COMMutex 删除——所有 RS485 操作已收敛到 ModBusTask

## 二、状态机完善

### 2.1 IO 观测驱动状态
- 所有状态流转改为 IO 观测驱动（先读世界，再纠偏）
- 事件只产生 PostCmd，状态由 IO 反馈确定
- power_out=0 全局回 Lock

### 2.2 防夹手检测方案
- 气压(0x20) + 激光2(0x40) + 激光3(0x80) + 激光4(0x0100)
- 关门后 1/3 时间检测（door_close_default_ms * 2/3）
- 前置条件：door_close_time_learned = 1
- 关门完成 3s 稳定期后，气压每 1.5s 周期告警

### 2.3 关门时间学习
- 仅从开门限位开始的关门才更新 door_close_default_ms
- door_close_time_learned 粘滞标志，防止中间位置误导

### 2.4 风险模式
- CONFigure:RISK:MODE ON/OFF (Flash 持久化)
- 气压传感器替代限位：气压正常 + 超时 > default_ms → Complete

### 2.5 上电位置确认
- 解锁后首次 Idle → 根据门限位输出压紧信号
- 中间位置（缓冲装置）→ 不自动操作，等人为介入

## 三、通讯优化

### 3.1 UART DMA 双缓冲审查
- 修复 USART3 TX DMA ISR 不重置 Usart3_TX_Flag
- U1_Printf/U3_Printf 改为非阻塞（TX 忙则跳过）
- 删除死代码 Usart1/3_Tx_Wait、DMA_Usart1/3_Tx_Data
- 统一命名：Uart1_BuffIsReady、Uart3_RxLength 等
- USART1 IDLE ISR 加 data_length 记录

### 3.2 RS485 健壮性
- TX 超时 100ms（原来无限）
- CRC16 查表（10 例手册验证）
- IO_Read 帧长校验 ≥6
- RS485_WaitRx 轮询双缓冲替代盲等 HAL_Delay(10)
- rs485_io_ok 由 SetRS485_Ok 唯一写入，不受 SCPI 污染

## 四、SCPI 完善

### 4.1 新增命令
- READ:IO:ALL? — 全 IO 状态查询
- CONFigure:ESTOP:TYPE NC/NO — 急停输入类型（Flash 持久化）
- CONFigure:RISK:MODE ON/OFF — 风险模式
- SYSTem:IDN1-4 读写（Flash 持久化）

### 4.2 全部 SCPI 读命令改为从向量表读取
- Cylinder/Lock/LED/SYS/IO 状态全部走 RamVector
- 写入全部走 RamVector_PostCmd
- 消除 SCPI ↔ ModBusTask 的 RS485 竞争

## 五、Python GUI

- IO 指示灯面板（16 输入 + 16 输出 LED）
- IO 原始值显示（IN:0xXX,0xXX OUT:0xXX,0xXX）
- 急停类型/风险模式/IDN 配置按钮
- 激光1→气压标签修正
- 自动轮询加入 READ:IO:ALL?

## 六、代码质量

- 门限位消抖（3 次 150ms）
- LOCK 防重复触发（lock_released 标志）
- E-STOP 常开/常闭可配
- 动作时间宏（VECTOR_DEBUG）
- 命名规范统一（UART 相关）
- 散列文件 useCustomScatterFile

## 七、待办规划

- 已完成 30+ 项
- 按 P0-P4 优先级排列待办
- Flash 相关统一归入"多机阶段"
- OTA 方案列入 P3

---

**核心成果**：架构从"多线程竞争 RS485"升级为"向量表隔离 + 单 ModBusTask 独占 IO"，可扩展至多机 CAN 组网。
