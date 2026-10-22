# 存储与 Flash 架构

Status: Active
Domain: STORAGE
Canonical: `docs/storage/STORAGE_FLASH_ARCHITECTURE.md`
Related: `docs/storage/STORAGE_LOG_ARCHITECTURE.md`, `docs/ota/OTA_CAN_DISTRIBUTION_ARCHITECTURE.md`, `docs/arch/ARCH_PRODUCT_ARCHITECTURE.md`, `docs/multinode/MULTINODE_REFLECTIVE_MEMORY_ARCHITECTURE.md`
Last updated: 2026-10-22

本文档定义 PinProbeA1 的两级存储：STM32F103RCT6 内部 Flash 中的配置页，以及 W25Q128（16 MB）
外部 Flash 的分区与写者。事实源：`Hardware/Flash/Inc/flash.h`、`Hardware/OTA/Inc/ota_manifest.h`、
`MDK-ARM/STM32F103RCT6_PinprobeA1.sct`。

## 一、内部 Flash 配置页

| 项 | 值 | 说明 |
|---|---:|---|
| 页号 | 127 | STM32F103 高密度：2 KB/页 |
| 地址 | `0x0803F800` | `FLASH_BASE + 127 × 0x800` |
| 大小 | `0x800`（2 KB） | 链接脚本把 `ER_IROM1` 收缩到 `0x3F800` 以免覆盖 |
| 魔数 | `0xA5A5A5A5` | 判断配置是否有效 |
| 结构版本 | `0x00020007` | 字段增删必须递增 |
| 结构大小 | 124 字节 | 编译期断言：`sizeof(Flash_Config_t) == 124` |
| CRC | offset 120，覆盖 magic..脉冲配置末尾 | Bootloader 与本工程共用覆盖率，不得改动 |

### 1.1 字段分组

| 分组 | 字段 | 说明 |
|---|---|---|
| 元数据 | `magic`、`version` | 有效性判定 |
| 设备标识 | `device_id`、`hw_version`、`fw_version_major/minor`、`serial_number`、`device_name[24]` | RS485 地址（1–247）、硬件/固件版本、序列号、名称 |
| 运行配置 | `default_channel`、`default_mode`、`rs485_baudrate` | 上电默认通道、默认 IO 模式（DIRECT/COMMAND）、链路波特率 |
| SCPI 标识 | `scpi_idn1..4` | `*IDN?` 四段，长度 12/20/12/12 |
| 外设与策略 | `estop_type`、`risk_mode`、`boot_diag_uart`、`led_green_io`、`led_red_io`、`led_yellow_io`、`usb_auto_enable`、`dut_auto_enable`、`mode_enable` | 急停常开/常闭、风险模式（气压传感器替代限位）、Boot 串口诊断、LED 输出 IO 编号、USB 自动流程、DUT 参与自动流程、控制模式切换使能 |
| 触发类型 | `input_pulse_mask`、`output_pulse_mask`、`output_pulse_width_ms` | bit=0 电平、bit=1 脉冲；脉冲宽度 1–60000 ms，默认 100 ms |
| 校验 | `crc` | CRC32 |

### 1.2 写入约束

- 只有 `Hardware/Flash` 模块可擦写该页；其他模块通过 `Flash_Set*` 接口修改后调用 `Flash_Save()`。
- 擦写会短暂阻塞，禁止在状态机同周期内连续调用；同一配置项多次修改应合并为一次保存。
- `Flash_LoadDefaults()` 只改内存缓存，不落盘。
- 配置结构是 Bootloader 与 App 的共同契约：改动必须同时验证两端的 CRC 覆盖率。

## 二、外部 Flash（W25Q128，16 MB）

分区定义在 `Hardware/OTA/Inc/ota_manifest.h`：

| 区域 | 偏移 | 大小 | owner | 用途 |
|---|---:|---:|---|---|
| Boot Info | `0x000000` | `0x001000`（4 KB） | Bootloader | 启动信息与诊断 |
| Snapshot Active | `0x001000` | `0x002000`（8 KB） | 快照 owner | 当前生效的 RAM 向量快照 |
| Snapshot Backup | `0x003000` | `0x002000`（8 KB） | 快照 owner | 上一版快照（双区冗余） |
| Event Log | `0x005000` | `0x010000`（64 KB） | 日志持久化 | 事件历史落盘区（当前未启用落盘） |
| OTA Area | `0x015000` | `0x400000`（4 MB） | OTA 域 | OTA 全部分区，见下 |

OTA 子分区：

| 区域 | 偏移（相对 OTA Area） | 大小 | 用途 |
|---|---:|---:|---|
| Metadata A | `+0x0000` | `0x1000` | Manifest A |
| Metadata B | `+0x1000` | `0x1000` | Manifest B（A/B 冗余） |
| Boot Flags | `+0x2000` | `0x1000` | 启动选择与确认状态（magic `POTF`） |
| Control Log | `+0x3000` | `0x1000` | OTA 控制事件日志 |
| Slot A | `+0x4000` | `0x80000`（512 KB） | 应用镜像槽 A |
| Slot B | `+0xA4000` | `0x80000`（512 KB） | 应用镜像槽 B |

Manifest 关键常量：magic `POTA`、版本 `0x00010000`、产品 ID `PAP1`、镜像类型 `APP=1`、
SCPI 分片 `128 B`、最大节点数 8。

## 三、RAM 中的存储相关区域

| 区域 | 地址 | 大小 | 内容 | 掉电行为 |
|---|---:|---:|---|---|
| `RW_RAMVEC` | `0x2000A000` | 1 KB | `RAM_Vector_t`（magic/version/timestamp/crc32 + 命令槽 + 节点 + IO 镜像） | 丢失；需要快照则写 W25Q128 |
| `RW_APPLOG` | `0x2000A400` | 1.25 KB | `AppLog_Table_t`（76 条 × 16 B 环形表） | 丢失；Event Log 分区仅预留 |

## 四、唯一写者规则

| 区域 | 唯一写者 | 允许的间接访问 |
|---|---|---|
| 内部 Flash 配置页 | `Hardware/Flash` | SCPI 命令经 `Flash_Set*` + `Flash_Save` |
| Boot Info / Boot Flags | Bootloader | App 只读 |
| Snapshot Active/Backup | 快照 owner（App 侧单任务） | 多机同步只读 `RAM_Vector_t` |
| OTA 分区 | OTA 域（`ota_manager`） | SCPI 传输与 CAN 分发都经 OTA 会话 |
| AppLog RAM 表 | `AppLog` 模块 | 各业务模块用 `AppLog_*` 写入，不直接改表 |

违反唯一写者规则会导致 CRC/一致性问题，属于缺陷而非「临时可行」。

## 五、未决问题

| 编号 | 问题 | 去向 |
|---|---|---|
| `STORAGE-Q1` | Event Log 分区（64 KB）已预留但未落盘，掉电后事件仅存 RAM。 | `docs/PROJECT_TODO.md` |
| `STORAGE-Q2` | 快照双区切换的写入与回放流程未实测。 | `docs/multinode/MULTINODE_TODO.md` `MULTINODE-06` |
| `STORAGE-Q3` | `Flash_Config_t` 中 `default_channel`/`default_mode` 与当前产品语义（门/USB 动作）不一致，字段来源是更早的开关量产品。 | `docs/PROJECT_TODO.md` |
