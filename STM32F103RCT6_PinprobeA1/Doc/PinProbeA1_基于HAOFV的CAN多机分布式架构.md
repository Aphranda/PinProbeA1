# PinProbe A1 基于 HAOFV 的 CAN 多机分布式架构

## 文档信息

| 项目 | 内容 |
|---|---|
| 状态 | Draft |
| 创建日期 | 2026-08-14 |
| 适用项目 | PinProbe A1 控制箱 |
| 目标平台 | STM32F103RCT6 / FreeRTOS / RamVector / CAN |
| 参考架构 | HAOFV: Hybrid Active Object Function Block Vector Architecture |
| 关联文档 | `PinProbe A1控制箱 嵌入式整体方案与架构报告.html`、`PinProbeA1_多机架构设计方案_RAM反射内存.md`、`PinProbeA1_OTA固件升级与CAN多机分发方案.md` |

## 1. 目标与边界

本文档定义 PinProbe A1 从当前单机控制主线演进到 CAN 多机协同系统时的分布式软件架构。

核心目标：

- 保持当前单机架构稳定：SCPI、StateVector、RamVector、ModBusTask、AppLog 的边界不推翻。
- 通过 CAN 建立 1~8 节点的共同事实层，支持节点状态、IO 镜像、命令意图、ACK/NACK、stale 和诊断证据同步。
- 多机控制采用“本地 owner 执行”模型：主机只发布意图，目标节点本地状态机判断是否可执行。
- 为后续 PLC 接入、CAN OTA、节点部署门禁、故障复盘和版本一致性检查打基础。

明确边界：

- 不引入完整 IEC 61499 运行时，不支持运行时动态部署功能块。
- 不把 CAN 设计成远程函数调用总线。
- 不让 SCPI、PLC 或 CAN 直接越过 StateVector 操作本地 IO。
- 不改变 USB、DUT、门限位等已有 IO 语义；现场接线问题通过 IO 反馈闭环和诊断暴露。
- 不在第一阶段追求高精度共同时间；先实现 `seq/stale/ack/nack/timestamp_ms` 的可靠控制闭环。

## 2. 架构命名

PinProbe A1 多机阶段采用 HAOFV 的轻量落地形式：

```text
PinProbe Distributed Vector Architecture
```

中文简称：

```text
PinProbe 分布式向量黑板架构
```

一句话描述：

```text
本机 RamVector 管单机事实，
CAN Vector Sync 管多机共同事实，
StateVector 管本机安全决策，
Command Slot + ACK/NACK 管跨节点意图闭环。
```

## 3. 总体分层

```text
SCPI / PLC / Debug Tool
        |
        v
Command / Config Entry
        |
        v
Local RamVector  <-------------------------+
        |                                  |
        v                                  |
StateVector 本机状态机                     |
        |                                  |
        v                                  |
CmdExec / ModBusTask / BSM IO              |
        |                                  |
        v                                  |
Physical IO Feedback ----------------------+

CAN 多机侧向链路:

Local RamVector Snapshot
        |
        v
CAN Vector Sync
        |
        v
Remote Node Snapshot Table
        |
        v
Deployment Gate / Cluster Health / Diagnostics
```

设计原则：

- 控制入口只表达意图。
- 状态机只读 IO 镜像并发布命令槽。
- 执行层独占现场总线。
- CAN 只同步事实、命令、确认和证据。
- 查询读本地快照，不临时阻塞等待远端。

## 4. 从 HAOFV 吸收的关键规则

| HAOFV 规则 | PinProbe 多机落地 |
|---|---|
| Vector Blackboard 管事实 | 当前 `RamVector` 继续作为本机事实中心，多机扩展为节点快照表 |
| Distributed RefMem 管共同事实 | CAN Vector Sync 同步节点状态、IO 摘要、命令序号、ACK/NACK、stale |
| 每个字段唯一 writer | SCPI 写命令/配置，StateVector 写状态决策，ModBusTask 写本机 IO，CANCom 写远端快照 |
| 跨节点只传意图和事实 | 主机写 command slot，目标节点本地消费并回 ACK/NACK |
| 查询读 snapshot | `READ:*?` 读取本地缓存；远端 stale 时返回 stale，不现场等待 |
| Deployment Gate | RUN 前检查节点角色、版本、配置 CRC、IO profile、在线状态 |
| Diagnostics evidence | 故障记录 source node、target node、seq、nack reason、stale age |
| 表驱动 | CAN ID、命令码、节点角色、部署检查项、错误码采用静态表 |

## 5. 节点模型

### 5.1 节点编号

| 节点 | 含义 |
|---|---|
| `N0` | 单机/本机默认日志编号，兼容当前 AppLog |
| `N1~N8` | CAN 多机物理节点编号 |

建议规则：

- 单机模式下 `node_id=0` 或未启用 CAN，多机逻辑全部旁路。
- 多机模式下每个物理节点必须配置唯一 `node_id=1~8`。
- CAN 网络内不允许两个节点 claim 同一个 `node_id`。
- 日志输出仍使用 `[N<id>]`，便于单机和多机复盘统一。

### 5.2 节点角色

| 角色 | 职责 |
|---|---|
| `HOST` | 接收 PLC/SCPI 多机指令，编排目标节点命令，维护部署门禁 |
| `WORKER` | 执行本机箱体动作，发布本机状态与 ACK/NACK |
| `MAINTENANCE` | 维护/调试节点，可参与查询和 OTA，不参与生产 RUN |
| `SIMULATOR` | 调试模拟节点，仅用于实验室验证 |

第一阶段可以采用“任意节点可作为 HOST”的弱主机模型；进入生产模式后建议通过 Flash 配置固定 host node。

### 5.3 节点事实

每个节点至少发布以下事实：

```c
typedef struct {
    uint8_t  online;
    uint8_t  node_id;
    uint8_t  role;
    uint8_t  state;
    uint8_t  error_code;
    uint8_t  io_profile_id;
    uint8_t  fw_ver_major;
    uint8_t  fw_ver_minor;
    uint16_t heartbeat_seq;
    uint16_t command_seq_seen;
    uint16_t ack_seq;
    uint16_t nack_seq;
    uint8_t  nack_code;
    uint8_t  stale;
    uint16_t stale_age_ms;
} PinProbe_NodeFact_t;
```

当前 `Vector_NodeStatus_t` 已经提供 `online/state/hw_ver/fw_ver/error_code/heartbeat`，第一阶段不强制重排 1024B RamVector，可通过：

- `node_status[8]` 放最小在线事实。
- `io_state[8]` 放每个节点 IO 快照。
- `ext_params` 或独立运行时表放 `seq/ack/nack/stale/io_profile/config_crc` 等多机扩展字段。

## 6. Vector 字段 owner

| 字段/区域 | 唯一 writer | Reader | 生命周期 |
|---|---|---|---|
| 本机 IO 镜像 `io_state[local]` | ModBusTask | StateVector / SCPI / CANCom | 每 25ms 更新 |
| 远端 IO 镜像 `io_state[remote]` | CANCom | SCPI / DeploymentGate / Diagnostics | 收到 CAN 快照后更新，超时 stale |
| 本机状态 `global_state` | StateVector | SCPI / CANCom / AppLog | 状态变化时更新 |
| 远端状态 `node_status[remote]` | CANCom | SCPI / DeploymentGate | 心跳/状态帧更新 |
| 本机命令槽 | SCPI / StateVector / CANCom 通过 RamVector API | CmdExec | Take 后清除 |
| 远端命令跟踪 | HOST Command Manager | SCPI / Diagnostics | 命令完成或超时后归档 |
| ACK/NACK | 目标节点 CAN owner | HOST / Diagnostics | 按 command_seq 匹配 |
| Flash 配置 | SCPI 配置命令 / Config Manager | StateVector / CANCom | 写入后持久化 |
| 日志 AppLog | AppLog | SCPI / Debug Tool | 环形缓冲 |

硬约束：

- 禁止业务代码直接写裸 RamVector 字段，必须走 API 或 owner 模块。
- 远端节点状态只能由 CANCom 更新，本机 StateVector 不得直接伪造远端状态。
- SCPI 查询不改变实时状态，不触发 CAN 现场读。

## 7. 多机命令模型

### 7.1 命令语义

跨节点命令不是远程 IO 写操作，而是命令意图：

```text
HOST
  -> target_mask + command_seq + command + params
  -> CAN command frame
  -> target node CANCom
  -> local RamVector command slot
  -> local StateVector / CmdExec
  -> ACK/NACK + result state
```

目标节点收到命令后必须本地检查：

- RS485 链路是否 OK。
- 当前系统状态是否允许动作。
- 急停、激光、门限位、DUT、USB 等安全条件是否满足。
- USB 自动流程开启时，DUT/USB 是否满足本地 IO 反馈闭环。
- 命令序号是否重复或过期。

### 7.2 ACK/NACK 时机

| 回应 | 含义 | 触发时机 |
|---|---|---|
| `ACK_ACCEPTED` | 命令已被目标 owner 接收 | 写入本地命令槽成功 |
| `ACK_DONE` | 动作完成或目标状态已达成 | 本机状态机确认完成 |
| `NACK_BUSY` | 本机忙或命令槽被高优先级占用 | 接收阶段 |
| `NACK_NOT_READY` | 当前状态不允许执行 | 接收阶段 |
| `NACK_SAFETY` | 急停/激光/门/USB/DUT 安全条件不满足 | 接收或执行阶段 |
| `NACK_STALE` | 依赖的远端事实已过期 | HOST 编排阶段 |
| `NACK_VERSION` | 部署门禁不通过 | RUN 前或命令接收阶段 |
| `NACK_TIMEOUT` | HOST 等待目标节点超时 | HOST 跟踪阶段 |

建议生产流程以 `ACK_DONE` 作为动作闭环完成条件，而不是只看 `ACK_ACCEPTED`。

## 8. CAN Vector Sync 协议

### 8.1 CAN ID 编码

沿用现有 OTA 方案中的 11-bit 编码方向：

```text
CAN_ID = (func << 6) | (src_node << 3) | dst_node
```

字段：

- `func`: 4 bit 功能码。
- `src_node`: 3 bit 源节点，`1~8` 映射为编码 `0~7` 或保留当前工程约定。
- `dst_node`: 3 bit 目标节点；广播帧目标可使用 `0` 或通过 payload `target_mask` 表达。

### 8.2 功能码建议

| func | 名称 | 方向 | 用途 |
|---:|---|---|---|
| `0x0` | `HEARTBEAT` | 广播 | 在线、状态、heartbeat_seq |
| `0x1` | `IO_SUMMARY` | 广播 | IO 摘要、raw in/out、rs485_ok |
| `0x2` | `COMMAND` | HOST -> target | 下发命令意图 |
| `0x3` | `ACK_NACK` | target -> HOST | 命令确认、拒绝、完成 |
| `0x4` | `EMERGENCY` | 任意 -> 广播 | 急停、激光、安全故障广播 |
| `0x5` | `DEPLOY_CLAIM` | 广播 | 节点身份、角色、版本、配置 CRC |
| `0x6` | `SNAPSHOT_REQ` | 单播 | 请求节点快照 |
| `0x7` | `SNAPSHOT_RESP` | 单播/分片 | 回传节点快照 |
| `0x8~0xF` | `OTA_*` | 复用 OTA 文档 | OTA 分发帧 |

### 8.3 核心帧

#### HEARTBEAT

```text
byte0: protocol_version
byte1: node_id
byte2: role
byte3: state
byte4: error_code
byte5: heartbeat_seq_l
byte6: heartbeat_seq_h
byte7: flags
```

#### IO_SUMMARY

```text
byte0: node_id
byte1: raw_in_lo
byte2: raw_in_hi
byte3: raw_out_lo
byte4: raw_out_hi
byte5: cylinder_state_packed
byte6: rs485_ok
byte7: io_seq
```

#### COMMAND

```text
byte0: command_seq_l
byte1: command_seq_h
byte2: target_mask
byte3: command_class
byte4: command_code
byte5: priority
byte6: param0
byte7: param1
```

复杂参数不得强行塞进一帧；后续用 `COMMAND_EXT` 或 snapshot 分片扩展。

#### ACK_NACK

```text
byte0: command_seq_l
byte1: command_seq_h
byte2: responder_node
byte3: ack_state
byte4: nack_code
byte5: result_state
byte6: evidence_l
byte7: evidence_h
```

#### EMERGENCY

```text
byte0: source_node
byte1: emergency_code
byte2: state
byte3: raw_in_lo
byte4: raw_in_hi
byte5: raw_out_lo
byte6: raw_out_hi
byte7: evidence_seq
```

## 9. stale 与质量门禁

每个远端节点维护：

```text
last_seen_ms
heartbeat_seq
io_seq
stale
stale_age_ms
drop_count
crc_error_count
last_error
```

建议默认门限：

| 项目 | 建议值 | 处理 |
|---|---:|---|
| Heartbeat 周期 | 100ms | 每节点广播 |
| IO Summary 周期 | 50ms 或状态变化立即发 | 用于调试和联动观测 |
| stale 超时 | 500ms | 标记节点 stale |
| required 节点 stale | 500ms | Deployment Gate 不允许 RUN |
| 命令 ACK_ACCEPTED 超时 | 100ms | HOST 记录 `CAN_TIMEOUT` |
| 命令 ACK_DONE 超时 | 按动作类型配置 | 超时后 NACK_TIMEOUT / 故障归因 |

查询规则：

- 本机查询永远返回本机最新快照。
- 远端查询返回快照 + stale 状态。
- 不允许为了查询临时发 CAN 请求并阻塞等待结果。

## 10. Deployment Gate

进入多机 RUN 前必须通过部署门禁。

### 10.1 检查项

| 检查项 | 规则 | 失败处理 |
|---|---|---|
| 节点唯一性 | 不允许重复 node_id | `CLAIM_CONFLICT` |
| 必需节点在线 | required mask 内节点必须在线且非 stale | `NODE_STALE` |
| 固件版本 | 主版本一致，次版本满足兼容矩阵 | `FW_MISMATCH` |
| IO profile | IO 语义表版本一致 | `IO_PROFILE_MISMATCH` |
| 配置 CRC | 参与 RUN 的关键配置一致 | `CONFIG_CRC_MISMATCH` |
| 安全状态 | 急停/激光/门状态允许进入 | `SAFETY_NOT_READY` |
| 本机链路 | 每节点 RS485 OK | `LOCAL_IO_FAULT` |
| OTA 状态 | 没有节点处于 OTA 写入/待提交 | `OTA_BUSY` |

### 10.2 Gate 状态

```text
UNKNOWN
COLLECTING
PASS
FAIL
STALE
MAINTENANCE_ONLY
```

RUN 命令只在 Gate 为 `PASS` 时允许下发到 required 节点。

## 11. 多机状态机

### 11.1 集群状态

```text
CLUSTER_INIT
  -> DISCOVERY
  -> DEPLOY_CHECK
  -> CLUSTER_IDLE
  -> CLUSTER_READY
  -> CLUSTER_RUNNING
  -> CLUSTER_COMPLETE
  -> CLUSTER_FAULT
  -> CLUSTER_MAINTENANCE
```

集群状态不替代单机状态。单机仍使用：

```text
LOCK / IDLE / READY / RUNNING / EMERGENCY / COMPLETE / INIT
```

关系：

- 集群状态由 HOST 的 Cluster Manager 推导。
- 单机状态由各节点 StateVector 推导。
- 集群状态只能基于节点事实和 ACK/NACK，不直接修改远端状态。

### 11.2 典型关门流程

```text
1. HOST 收到多机关门意图
2. Deployment Gate 检查 required 节点
3. HOST 生成 command_seq，向 target_mask 下发 COMMAND:CLOSE_PREPARE
4. 各节点本地进入 READY 或返回 NACK
5. HOST 收齐 ACK_ACCEPTED
6. HOST 下发 COMMAND:CLOSE_CONFIRM
7. 各节点本地 StateVector 执行 DUT/USB/门安全闭环
8. 各节点完成后返回 ACK_DONE 或 NACK_SAFETY/NACK_TIMEOUT
9. HOST 根据目标节点结果进入 CLUSTER_COMPLETE 或 CLUSTER_FAULT
```

注意：

- DUT 检测和 USB 自动插入仍是各节点本地 StateVector 的职责。
- 如果某节点 `USB:AUTO:OFF`，该节点本地 DUT 自动检测旁路，但查询仍有效。
- 如果某节点 USB 输入输出不一致，目标节点返回 `NACK_SAFETY`，HOST 不应替它强制关门。

## 12. 安全与故障广播

安全事件分两层：

| 层 | 职责 |
|---|---|
| 本机安全 | 本地 StateVector 立即进入安全动作，如急停开门、红灯、锁定 |
| 集群安全 | CAN EMERGENCY 广播，让其他节点进入预定义安全策略 |

建议策略：

- 急停：本机立即处理，同时广播 `EMERGENCY_ESTOP`。
- 激光防夹：本机立即处理，可按配置广播。
- RS485 本机故障：本机禁止自动动作，广播 `LOCAL_IO_FAULT`。
- USB/DUT 安全未满足：不刷屏广播；在动作触发时通过 NACK/AppLog 归因。
- CAN stale：HOST 集群进入 `CLUSTER_FAULT`，目标节点本机可保持安全状态。

## 13. 配置与持久化

### 13.1 Flash 配置项

多机阶段建议增加：

```text
node_id
node_role
required_node_mask
can_bitrate
cluster_profile_id
io_profile_id
config_crc
deployment_epoch
host_node_id
```

原则：

- 配置命令写 Flash 固化。
- 配置变更后更新 `config_crc`。
- 影响部署门禁的配置变更应要求重启或重新 Discovery。

### 13.2 W25Q128 使用边界

W25Q128 适合存放：

- RamVector/节点快照备份。
- OTA 镜像和元数据。
- 多机部署 profile。
- 故障日志和运行审计。

W25Q128 不适合承载：

- 实时命令槽。
- 高频 IO 原始波形。
- 状态机每周期临时变量。

## 14. CAN OTA 分发融合方案

CAN OTA 是多机分布式架构中的维护域工作流，复用节点发现、Deployment Gate、ACK/NACK、stale 和诊断证据，但固件数据本身不进入 RamVector。

### 14.1 总体链路

```text
PC / 上位机
  -> SCPI binary block
  -> HOST 写入本机 W25Q128 OTA Slot
  -> HOST 本地完整 CRC32 / Manifest 校验
  -> CAN OTA ANNOUNCE
  -> target nodes ACCEPT / REJECT
  -> CAN 逻辑 block 分片分发
  -> target nodes 写入本机 W25Q128
  -> target nodes 本地完整校验
  -> target nodes READY / VERIFY_FAIL
  -> HOST 收齐 READY
  -> HOST 广播 OTA_COMMIT
  -> 各节点复位进入 Bootloader
  -> Bootloader 从本机 W25Q128 刷写内部 App
  -> 新 App 自检并 CONFIRM
```

关键原则：

- HOST 分发的是上位机传入并已在本机 W25Q128 校验通过的镜像。
- HOST 不读取自己正在运行的内部 Flash 来分发。
- 每个目标节点只信任自己本地 W25Q128 中完整校验通过的镜像。
- 默认不允许部分提交；只要 target mask 中有一个节点未 READY，HOST 不广播 COMMIT。
- OTA_BUSY / OTA_PENDING / BOOT_UPDATING 节点不允许进入生产 RUN。

### 14.2 OTA 与分布式黑板的关系

| 数据 | 位置 | 说明 |
|---|---|---|
| 固件 payload | CAN OTA 分片 + W25Q128 OTA Slot | 不进入 RamVector |
| OTA 状态摘要 | RamVector `ext_params` 或独立运行表 | 用于 SCPI、Tools、Cluster Health 查询 |
| 节点在线/stale | `node_status[8]` + CAN heartbeat | OTA 分发前必须检查 |
| ACK/NACK | CAN `OTA_*` 响应 + HOST 跟踪表 | 按 image_id/block_index/node_id 聚合 |
| Manifest / Boot flags | W25Q128 OTA metadata 区 | Bootloader 和 App 共同使用 |
| 诊断证据 | AppLog + OTA status summary | 记录失败节点、block、CRC、错误码 |

建议 OTA 状态摘要：

```c
typedef struct {
    uint8_t  ota_state;
    uint8_t  ota_role;          /* 0=none, 1=host, 2=target */
    uint8_t  target_mask;
    uint8_t  accept_mask;
    uint8_t  ready_mask;
    uint8_t  error_node;
    uint8_t  error_code;
    uint16_t progress_permille;
    uint32_t image_version;
    uint32_t image_size;
    uint32_t image_crc32;
    uint16_t active_block;
    uint16_t retry_count;
} Vector_OTASummary_t;
```

### 14.3 CAN OTA 功能码

OTA 复用统一 CAN ID 编码：

```text
CAN_ID = (func << 6) | (src_node << 3) | dst_node
```

其中 `0x00~0x07` 保留给多机控制和节点事实同步，`0x08~0x0F` 给 OTA：

| func | 方向 | 消息 |
|---:|---|---|
| `0x08` | HOST -> target/broadcast | `OTA_ANNOUNCE` / `OTA_ANNOUNCE_CRC` |
| `0x09` | target -> HOST | `OTA_ACCEPT` / `OTA_REJECT` |
| `0x0A` | HOST -> target/broadcast | `OTA_BLOCK_BEGIN` |
| `0x0B` | HOST -> target/broadcast | `OTA_BLOCK_DATA` |
| `0x0C` | target -> HOST | `OTA_BLOCK_ACK` / `OTA_BLOCK_NACK` |
| `0x0D` | HOST -> target/broadcast | `OTA_VERIFY` / `OTA_COMMIT` / `OTA_ABORT` |
| `0x0E` | target -> HOST | `OTA_READY` / `OTA_VERIFY_FAIL` |
| `0x0F` | 双向 | `OTA_QUERY` / `OTA_STATUS` |

### 14.4 分片策略

经典 CAN payload 只有 8 字节，OTA 使用“两层分片”：

- 逻辑 block 默认 256B。
- `OTA_BLOCK_BEGIN` 声明 `block_index/block_len/block_crc16`。
- `OTA_BLOCK_DATA` 携带 `image_id/seq_in_block/data[6]`。
- 目标节点按 block 缓存或直接写入 W25Q128 staging 区。
- 每个 block 完成后目标节点回 `OTA_BLOCK_ACK` 或 `OTA_BLOCK_NACK`。
- NACK 必须携带 `block_index/error_code/received_crc16`，HOST 只重发失败 block。
- 单 block 最大重试建议 3 次；超过后本次 OTA 进入 ERROR，不进入 COMMIT。

### 14.5 OTA 状态机

HOST OTA 状态：

```text
IDLE
  -> RECEIVE_FROM_PC
  -> LOCAL_VERIFY
  -> CAN_ANNOUNCE
  -> WAIT_ACCEPT
  -> CAN_DISTRIBUTE
  -> CAN_VERIFY
  -> WAIT_ALL_READY
  -> COMMIT_ALL
  -> DONE
  -> ERROR / ABORTED
```

TARGET OTA 状态：

```text
IDLE
  -> WAIT_ANNOUNCE
  -> RECEIVING
  -> VERIFYING
  -> READY
  -> COMMIT_PENDING
  -> BOOT_UPDATING
  -> CONFIRMED
  -> ERROR / ROLLBACK
```

状态机约束：

- `OTA_ANNOUNCE` 前 HOST 必须检查 target 节点非 stale。
- `OTA_ACCEPT` 前 target 必须检查本机处于安全静止状态。
- `OTA_VERIFY` 后 target 必须完成本机 W25Q128 镜像 CRC32 校验。
- `OTA_COMMIT` 前 HOST 必须再次检查全部 target 仍 READY 且非 stale。
- Bootloader 不依赖 FreeRTOS、SCPI、ModBus、RamVector 或 AppLog。

### 14.6 Deployment Gate 与 OTA Gate

OTA 使用独立 Gate，但与生产 RUN Gate 共享节点事实。

| Gate 项 | OTA BEGIN / ANNOUNCE | OTA COMMIT |
|---|---|---|
| required target online | 必须通过 | 必须通过 |
| target stale | 禁止 | 禁止 |
| node_id conflict | 禁止 | 禁止 |
| firmware compatibility | 检查最低 Bootloader / hw target | 检查 Manifest |
| system safe idle | 必须静止 | 必须静止 |
| RS485 fault | 可拒绝生产 RUN；OTA 可按策略允许维护 | COMMIT 前默认禁止 |
| emergency / laser | 禁止 | 禁止 |
| door moving | 禁止 | 禁止 |
| OTA partial ready | 不适用 | 禁止 COMMIT |

### 14.7 OTA 错误码

| 错误码 | 名称 | 说明 |
|---:|---|---|
| `0` | `OK` | 无错误 |
| `1` | `BUSY` | OTA 会话已存在或节点忙 |
| `2` | `BAD_STATE` | 当前系统状态不允许 OTA |
| `3` | `BAD_PARAM` | 参数错误 |
| `4` | `FLASH_FAIL` | W25Q128 写/擦/读回失败 |
| `5` | `SIZE_OVERFLOW` | 镜像超过允许范围 |
| `6` | `CRC_FAIL` | block 或镜像 CRC 校验失败 |
| `7` | `CAN_TIMEOUT` | CAN ACK/READY 超时 |
| `8` | `CAN_NACK` | 子节点拒绝或请求失败 |
| `9` | `NODE_NOT_READY` | 目标节点未全部 READY |
| `10` | `BOOT_FAIL` | Bootloader 更新失败 |
| `11` | `APP_NOT_CONFIRMED` | 新 App 未确认 |

### 14.8 与现有 OTA 文档的关系

`PinProbeA1_OTA固件升级与CAN多机分发方案.md` 继续作为 OTA 细节设计来源，包含 W25Q128 分区、Manifest、Boot flags、SCPI binary block、Bootloader 刷写和回滚策略。

本文档只规定 OTA 如何纳入多机 HAOFV 风格架构：

- OTA 使用同一套 CAN 编码空间。
- OTA 状态摘要进入节点事实。
- OTA ACK/NACK 纳入 Cluster Diagnostics。
- OTA_BUSY 会影响生产 RUN Gate。
- OTA COMMIT 必须遵守“全部 READY 才提交”的集群一致性规则。

## 15. SCPI / PLC 接口边界

SCPI 在多机阶段仍是维护与调试入口。

建议新增查询类：

```text
READ:NODE:LIST?
READ:NODE:STATe? <node>
READ:NODE:IO? <node>
READ:NODE:STALE? <node>
READ:CLUSTer:STATe?
READ:CLUSTer:GATE?
READ:CLUSTer:ERRor?
```

建议新增配置类：

```text
CONFigure:NODE:ID <1-8>
CONFigure:NODE:ROLE HOST|WORKER|MAINTENANCE
CONFigure:CLUSTer:MASK <0-255>
CONFigure:CAN:BITRate <rate>
```

建议新增动作类：

```text
ROUTe:CLUSTer:CYLinder OPEN|CLOSE,<mask>
ROUTe:CLUSTer:LOCK LOCK|UNLOCK,<mask>
ROUTe:CLUSTer:LED GREEN|RED|YELLOW|OFF,<mask>
```

建议新增 OTA 维护类：

```text
SYSTem:OTA:TARGET <mask>
SYSTem:OTA:DISTRibute
SYSTem:OTA:NODEs?
SYSTem:OTA:COMMit:ALL
SYSTem:OTA:ABORt:ALL
```

接口原则：

- 单机原有 SCPI 默认只作用于本节点。
- 多机动作必须显式使用 cluster/node 命令，不隐式广播。
- 查询远端状态时返回 snapshot 与 stale 信息。
- PLC 接入时建议通过 Modbus 寄存器映射同一套 cluster command slot。
- OTA 上传期间应暂停普通轮询和压力测试，避免 SCPI 响应与 binary block 交叉。

## 16. 诊断与日志

AppLog 需要继续保持“解释发生了什么”的职责。

多机日志建议格式：

```text
[T+123.456s][N1][INFO][CAN] HEARTBEAT node=2 seq=18 state=IDLE
[T+124.000s][N1][WARN][CLUSTER] NODE_STALE node=3 age=525ms
[T+125.200s][N2][WARN][EVENT] USB_INSERT_FAIL elapsed=0ms reason=io_mismatch
[T+125.220s][N1][ERROR][CLUSTER] COMMAND_NACK seq=42 node=2 reason=SAFETY
[T+130.500s][N1][ERROR][OTA] BLOCK_NACK image=3 node=4 block=128 err=CRC_FAIL
```

需要记录的证据：

- command_seq
- source_node
- target_node / target_mask
- ack_state
- nack_code
- stale_age_ms
- local_state
- raw_in/raw_out
- config_crc / io_profile_id
- ota image_id / block_index / image_crc32

## 17. 第一阶段实施路线

### M1: CAN 基础数据面

- 初始化 CAN1、过滤器和收发队列。
- 实现 `HEARTBEAT`、`IO_SUMMARY`。
- 更新 `node_status[8]` 和远端 `io_state[8]`。
- 实现 stale 检测和 AppLog 事件。

验收：

- 2 个节点互相看到在线状态。
- 拔掉一个节点后 500ms 内标记 stale。
- `READ:NODE:LIST?` 可看到 online/stale。

### M2: 多机命令槽

- 实现 `COMMAND`、`ACK_NACK`。
- HOST 维护 command_seq 和目标 ACK 位图。
- 目标节点只把合法命令写入本机 RamVector 命令槽。

验收：

- HOST 下发开门/关门到指定 mask。
- 目标节点本地执行，返回 ACK_DONE。
- 忙、急停、USB/DUT 条件失败时返回 NACK。

### M3: Deployment Gate

- 增加节点 claim 帧。
- 增加版本、配置 CRC、IO profile 检查。
- RUN 前必须 Gate PASS。

验收：

- 重复 node_id 被拒绝。
- 必需节点 stale 时拒绝 RUN。
- IO profile 不一致时拒绝 RUN。

### M4: PLC / Modbus 多机入口

- 将 cluster command slot 映射到 Modbus 寄存器。
- PLC 写入 target_mask、cmd、seq。
- HOST 返回 ACK/NACK 汇总。

验收：

- PLC 接入任意 HOST 节点可控制目标节点。
- 查询不阻塞 CAN。

### M5: CAN OTA 接入

- 复用 `PinProbeA1_OTA固件升级与CAN多机分发方案.md` 的 Manifest、W25Q128 分区、Boot flags 和 Bootloader 策略。
- 实现 `OTA_ANNOUNCE / ACCEPT / BLOCK_BEGIN / BLOCK_DATA / BLOCK_ACK / VERIFY / READY / COMMIT / ABORT`。
- OTA 状态摘要进入节点事实。
- Gate 拒绝 OTA_BUSY 节点进入 RUN。
- HOST 只在所有目标节点 READY 后广播 COMMIT。

验收：

- 目标节点全部 READY 才 COMMIT。
- 任一节点 NACK 默认整体不提交。
- block NACK 后只重发失败 block，超过最大重试进入 ERROR。
- 新 App 未 CONFIRM 时 Bootloader 能进入回滚或维护状态。

## 18. 代码落点建议

| 模块 | 建议职责 |
|---|---|
| `Hardware/CANCom` | CAN frame 编解码、收发、远端 snapshot 更新 |
| `Hardware/RamVector` | 本机命令/状态/IO，远端最小节点事实 |
| `Hardware/Cluster` | Deployment Gate、command_seq、ACK/NACK 聚合 |
| `Hardware/OTA` | OTA Manifest、状态机、CAN 分发、Boot flags 摘要 |
| `Hardware/AppLog` | 多机事件、stale、NACK、claim conflict 诊断 |
| `Hardware/Flash` | node_id、role、mask、CAN bitrate、profile CRC 持久化 |
| `Hardware/libscpi/port/scpi-def.c` | 多机查询和维护命令入口 |
| `Tools` | 节点列表、远端 IO、Gate 状态、NACK 证据显示 |

第一阶段可以不新增完整 Active Object 框架；用现有 FreeRTOS 任务模型承载：

```text
CAN RX IRQ -> CANCom ring buffer
CANCom service/task -> update remote snapshot / post local command
defaultTask or Cluster service -> stale scan / deployment gate
StateVectorTask -> local safety decision
ModBusTask -> local IO execution
SCPITask -> query/config/command entry
```

## 19. 风险与约束

| 风险 | 约束 |
|---|---|
| RamVector 1024B 已接近满 | 第一阶段不重排结构体，优先使用现有 node_status/io_state，扩展字段用版本化 TLV 或独立运行表 |
| CAN 变成远程 IO 写 | 所有远端动作必须进入目标节点本地命令槽和 StateVector |
| 查询阻塞实时链路 | 查询只读 snapshot，stale 明确返回 |
| 多节点配置漂移 | Deployment Gate 使用 config_crc/io_profile_id/fw version |
| 重复节点 ID | Claim conflict 直接阻止 required 节点 RUN |
| 安全事件刷屏 | 静态异常只锁存状态，动作触发和状态边沿才记录日志 |
| OTA 与生产 RUN 冲突 | OTA_BUSY 节点不允许进入 RUN |
| 多机 OTA 部分提交 | 默认全部 READY 才 COMMIT，任一目标节点失败则整体 ABORT/ERROR |
| OTA 数据污染 RamVector | RamVector 只放 OTA 摘要，payload 只进入 W25Q128 OTA Slot |
| USB/DUT 本地条件被 HOST 绕过 | HOST 只能下发意图，目标节点本地安全闭环拥有最终否决权 |

## 20. 最终结论

PinProbe A1 后续多机不需要推翻当前单机主线。推荐采用：

```text
现有单机 RamVector / StateVector
+ CAN Vector Sync
+ Node Snapshot
+ Command Seq
+ ACK/NACK
+ Stale
+ Deployment Gate
+ AppLog Evidence
+ CAN OTA Maintenance Flow
```

这条路线吸收了 HAOFV 中最有价值的分布式黑板思想，同时保持当前 STM32F103 资源约束下的实现可控。第一阶段优先实现节点事实同步和 stale 检测；第二阶段再实现多机命令槽；第三阶段通过部署门禁把多机 RUN 做成可验证、可诊断、可扩展的控制闭环。
