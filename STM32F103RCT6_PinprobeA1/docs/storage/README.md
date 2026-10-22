# 存储与日志域

Status: Active
Domain: STORAGE
Canonical: `docs/storage/README.md`
Related: `docs/README.md`, `docs/arch/ARCH_PRODUCT_ARCHITECTURE.md`, `docs/ota/OTA_CAN_DISTRIBUTION_ARCHITECTURE.md`
Last updated: 2026-10-22

本目录定义固件的持久化与诊断证据：内部 Flash 配置页、W25Q128 外部分区、AppLog 环形日志，
以及它们与 RAM 向量表、OTA 槽位、多机快照的归属关系。

## 当前主线

| 当前路径 | 定位 | 使用规则 |
|---|---|---|
| `STORAGE_FLASH_ARCHITECTURE.md` | 内部 Flash 配置页 + W25Q128 分区表 + 快照区归属 + 唯一写者规则。 | 存储布局 canonical；OTA 分区在本文档声明归属、在 `docs/ota/` 定义流程。 |
| `STORAGE_LOG_ARCHITECTURE.md` | AppLog 表结构、级别/模块/事件编码、实时泵出与丢弃策略、回读方式。 | 日志语义 canonical。 |

## 事实源

| 事实 | 代码 |
|---|---|
| 配置页地址、结构体与版本、CRC 口径 | `Hardware/Flash/Inc/flash.h`、`Hardware/Flash/Src/flash.c` |
| W25Q128 分区地址与 OTA 独占区 | `Hardware/OTA/Inc/ota_manifest.h` |
| W25Q128 驱动 | `Hardware/W25Q128/Src/w25q128.c` |
| AppLog 表结构、事件/动作编码 | `Hardware/AppLog/Inc/app_log.h`、`Src/app_log.c` |
| 日志串口后端 | `Hardware/AppLog/Src/app_log_uart.c` |
| RAM 分区（`RW_RAMVEC`/`RW_APPLOG`） | `MDK-ARM/STM32F103RCT6_PinprobeA1.sct` |

## 边界

- 存储域定义「数据放在哪、什么格式、谁能写」，不定义业务动作（归 `docs/control/`）。
- 同一块物理区域只能有一个 owner 写者；跨域写入必须走该 owner 的接口。
- 日志是证据不是契约：日志内容不得被业务逻辑反向解析用于状态判定。
