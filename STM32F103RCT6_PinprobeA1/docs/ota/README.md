# OTA 与启动域

Status: Active
Domain: OTA
Canonical: `docs/ota/README.md`
Related: `docs/README.md`, `docs/storage/STORAGE_FLASH_ARCHITECTURE.md`, `docs/multinode/MULTINODE_CAN_HAOFV_ARCHITECTURE.md`, `docs/release/RELEASE_CHECKLIST.md`
Last updated: 2026-10-22

本目录是固件升级与启动流程的唯一入口：内部 Flash 分区与 Bootloader 交接、W25Q128 A/B 槽位、
Manifest 与 boot flag、单机 OTA 流程、CAN 多机分发，以及出厂合并镜像。

## 当前主线

| 当前路径 | 定位 | 使用规则 |
|---|---|---|
| `OTA_CAN_DISTRIBUTION_ARCHITECTURE.md` | OTA 总体架构：Flash/W25Q128 布局、Manifest、单机 SCPI 传输、Bootloader 规则、CAN 分发协议、多机状态机、错误码与实施阶段 A–F。 | OTA 域 canonical（含多机分发）。 |
| `OTA_TODO.md` | OTA 待办与阶段门禁。 | 只维护未完成事项。 |

## 相关文档

| 当前路径 | 定位 |
|---|---|
| `../storage/STORAGE_FLASH_ARCHITECTURE.md` | 内部 Flash 配置页与 W25Q128 分区的 canonical 定义方（OTA 分区数据必须在两边同源）。 |
| `../multinode/MULTINODE_CAN_HAOFV_ARCHITECTURE.md` | CAN 多机命令通道，OTA 分发复用其功能码空间。 |
| `../release/RELEASE_CHECKLIST.md` | 出厂镜像与升级前检查项。 |
| `../arch/ARCH_PRODUCT_ARCHITECTURE.md` | Bootloader/APP 地址与尺寸上限的来源。 |

## 事实源

| 事实 | 代码 |
|---|---|
| 分区地址与尺寸、Manifest 结构、错误码、状态机 | `Hardware/OTA/Inc/ota_manifest.h`、`ota_manager.h` |
| Bootloader 选择与回滚 | `Bootloader/Src/boot_main.c` |
| 出厂合并镜像生成 | `Script/build_factory_images.ps1`、`Tools/merge_ota_images.py` |

## 边界

- OTA 域不定义 W25Q128 的通用分区归属（归 `docs/storage/`），只在其中声明 OTA 独占区域。
- Bootloader 不依赖 RTOS，不得引入 FreeRTOS API。
- 任何改变 A/B 槽位地址或 Manifest 版本的改动都属于破坏性变更，必须先过 `docs/release/RELEASE_CHECKLIST.md`。
