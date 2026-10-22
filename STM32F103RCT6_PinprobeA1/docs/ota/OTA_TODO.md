# OTA 域待办

Status: Active
Domain: OTA
Canonical: `docs/ota/OTA_TODO.md`
Related: `docs/PROJECT_TODO.md`, `docs/ota/OTA_CAN_DISTRIBUTION_ARCHITECTURE.md`, `docs/storage/STORAGE_FLASH_ARCHITECTURE.md`, `docs/release/RELEASE_CHECKLIST.md`
Last updated: 2026-10-22

本清单只维护 OTA 域**未完成**事项。阶段编号沿用
`OTA_CAN_DISTRIBUTION_ARCHITECTURE.md` 第 15 节的 A–F 划分。

## P0 必须完成

- [ ] `OTA-01` 阶段 A 收口：`ota_manifest.h` 中 W25Q128 分区与内部 Flash 布局和
  `docs/storage/STORAGE_FLASH_ARCHITECTURE.md` 对齐，消除「文档写了但代码没有」的区域。
  验收：两侧地址/尺寸表逐行一致。
- [ ] `OTA-02` 阶段 D 验证：单机 OTA 全链路（上位机传输 → CRC 校验 → 写入 B 槽 → boot flag →
  Bootloader 切换 → 启动确认）跑通，含中断一次后重传。验收：串口日志 + 版本号变化证据。
- [ ] `OTA-03` 回滚验证：升级后不确认时自动回退到 A 槽，且配置页与快照不被破坏。
  验收：断电重启 3 次均回到旧版本。

## P1 应该完成

- [ ] `OTA-04` 阶段 C 梳理：Bootloader 代码与 `Bootloader/Src/boot_main.c` 的实际行为对齐文档，
  补「Boot 不依赖 RTOS、无动态内存」的静态检查说明。
- [ ] `OTA-05` 出厂镜像链固化：`Script/build_factory_images.ps1` 输出（`.hex`/`.bin`/`.json`）的字段与
  校验规则写入文档，并对 `Tools/merge_ota_images.py` 的参数做一次可复现验证。
- [ ] `OTA-06` 下载缓冲归属确认：OTA 数据缓冲应落在 `RW_IRAM2` 预留区还是外部 Flash，
  给出内存预算（对应 `ARCH-Q3`）。

## P2 可以延后

- [ ] `OTA-07` 阶段 E：CAN 多机分发实测（前置 `MULTINODE-01` 位率确认）。
- [ ] `OTA-08` 阶段 F：产线与维护工具（批量烧录、版本采集）。
- [ ] `OTA-09` 建立 `OTA_TASK_PROGRESS.md`，记录各阶段证据。
