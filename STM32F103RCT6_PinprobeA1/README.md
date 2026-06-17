# STM32F103RCT6 PinProbeA1

## EIDE 编译配置

工程配置文件：`.eide/eide.yml`

- 工程名：`STM32F103RCT6_PinprobeA1`
- 工程类型：`ARM`
- 目标芯片：`STM32F103RC`
- Device Pack：`.pack/Keil/STM32F1xx_DFP.2.3.0`
- 当前目标：`STM32F103RCT6_PinprobeA1`
- 工具链：`AC5`
- ARMCC 路径：`D:\Keil\ARM\ARMCC`
- CPU：`Cortex-M3`
- 浮点：`none`
- 输出目录：`build`
- 默认下载器：`OpenOCD`

### 编译选项

- C 标准：C99
- 优化等级：`level-1`
- Warning：`all-warnings`
- Debug 信息：开启
- MicroLIB：开启
- 每函数独立段：开启
- 诊断抑制：
  - `--diag_suppress=1`
  - `--diag_suppress=1295`

### 预定义宏

- `USE_HAL_DRIVER`
- `STM32F103xE`

### 关键头文件路径

- `Core/Inc`
- `Drivers/STM32F1xx_HAL_Driver/Inc`
- `Drivers/CMSIS/Device/ST/STM32F1xx/Include`
- `Drivers/CMSIS/Include`
- `Middlewares/Third_Party/FreeRTOS/Source/include`
- `Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2`
- `Middlewares/Third_Party/FreeRTOS/Source/portable/RVDS/ARM_CM3`
- `Hardware/BsmRelay/Inc`
- `Hardware/ModBus/Inc`
- `Hardware/libscpi/inc`
- `Hardware/libscpi/port`
- `Hardware/Flash/Inc`
- `Hardware/W25Q128/Inc`
- `Hardware/RamVector/Inc`
- `Hardware/AppLog/Inc`

### 内存布局

工程启用自定义散列文件：

- `useCustomScatterFile: true`
- `scatterFilePath: MDK-ARM/STM32F103RCT6_PinprobeA1.sct`

| Region | 起始地址 | 大小 | 用途 |
|---|---:|---:|---|
| `LR_IROM1` / `ER_IROM1` | `0x08000000` | `0x0003F800` | 主程序 Flash |
| `RW_IRAM1` | `0x20000000` | `0x0000A000` | 常规 RW/ZI RAM |
| `RW_RAMVEC` | `0x2000A000` | `0x00000400` | `RamVector` 专用 1024B |
| `RW_APPLOG` | `0x2000A400` | `0x00000500` | `AppLog` 专用 1280B |
| `RW_IRAM2` | `0x2000A900` | `0x00001700` | 预留/剩余 RAM |

## 正确构建流程

推荐方式是在 VSCode 中使用 EIDE 任务：

- `build`
- `rebuild`
- `flash`
- `build and flash`

也可以在 PowerShell 中直接调用 EIDE 构建器。当前机器用 `.NET` roll-forward 运行 EIDE 的 `unify_builder.exe`：

```powershell
$env:DOTNET_ROLL_FORWARD = "Major"
& "$env:USERPROFILE\.vscode\extensions\cl.eide-3.27.2\res\tools\win32\unify_builder\unify_builder.exe" `
  -p "build\STM32F103RCT6_PinprobeA1\builder.params"
```

该命令已验证可用，会复用 EIDE 生成的 `builder.params` 和 AC5 工具链配置。

成功构建后输出：

- `build/STM32F103RCT6_PinprobeA1/STM32F103RCT6_PinprobeA1.hex`
- `build/STM32F103RCT6_PinprobeA1/STM32F103RCT6_PinprobeA1.s19`
- `build/STM32F103RCT6_PinprobeA1/STM32F103RCT6_PinprobeA1.bin`

最近一次验证构建：

- Code：`46996`
- RO-data：`3532`
- RW-data：`2936`
- ZI-data：`27768`
- Total ROM：`50760` bytes，约 `49.57kB`
- 结果：`build successfully`

## 正确烧录流程

使用 EIDE 自带 OpenOCD 烧录当前 hex：

```powershell
& "$env:USERPROFILE\.eide\tools\openocd_7a1adfbec_mingw32\bin\openocd.exe" `
  -f interface/cmsis-dap.cfg `
  -c "cmsis_dap_backend hid" `
  -f target/stm32f1x.cfg `
  -c "adapter speed 1000" `
  -c "program build/PinProbeA1_Factory/PinProbeA1_Factory.hex verify reset exit"
```

关键点：

- 使用 CMSIS-DAP HID 后端：`cmsis_dap_backend hid`
- 目标配置：`target/stm32f1x.cfg`
- 下载文件：`build/PinProbeA1_Factory/PinProbeA1_Factory.hex`
- 烧录流程：`program ... verify reset exit`

成功烧录输出应包含：

```text
Info : [stm32f1x.cpu] Cortex-M3 r1p1 processor detected
Info : flash size = 256 KiB
** Programming Finished **
** Verified OK **
** Resetting Target **
```

## OTA/Bootloader 出厂镜像

OTA 版固件拆成两个镜像：

- Bootloader: `0x08000000`，大小上限 `0x6000`
- APP: `0x08006000`，大小上限 `0x39800`

生成出厂合并镜像：

```powershell
.\Script\build_factory_images.ps1
```

该脚本会依次构建 Bootloader、APP，并调用：

```powershell
python Tools\merge_ota_images.py
```

默认输出：

- `build/PinProbeA1_Factory/PinProbeA1_Factory.hex`
- `build/PinProbeA1_Factory/PinProbeA1_Factory.bin`
- `build/PinProbeA1_Factory/PinProbeA1_Factory.json`

烧录建议：

- 出厂整机烧录优先使用 `PinProbeA1_Factory.hex`
- 当前 EIDE 主目标的 flash 配置已指向 `PinProbeA1_Factory.hex`
- 如果烧录 `.bin`，基地址必须为 `0x08000000`
- OTA 上位机上传的应用固件仍使用 APP bin：`build/STM32F103RCT6_PinprobeA1/STM32F103RCT6_PinprobeA1.bin`
