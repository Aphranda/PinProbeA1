# STM32F103RCT6 PinProbeA1

## EIDE 编译配置

工程配置文件：`.eide/eide.yml`

- 工程名：`STM32F103RCT6_PinprobeA1`
- 工程类型：`ARM`
- 目标芯片：`STM32F103RC`
- Device Pack：`.pack/Keil/STM32F1xx_DFP.2.3.0`
- 输出目录：`build`
- 当前目标：`STM32F103RCT6_PinprobeA1`
- 工具链：`AC5`
- CPU：`Cortex-M3`
- 浮点：`none`
- 调试器配置：`cortex-debug`
- 默认下载器：`OpenOCD`

本机工具路径：

- EIDE GCC ARM：`${userHome}/.eide/tools/gcc_arm`
- EIDE OpenOCD：`${userHome}/.eide/tools/openocd_7a1adfbec_mingw32/bin/openocd.exe`
- Keil ARMCC：`D:\Keil\ARM\ARMCC`
- Keil uVision：`D:\Keil\UV4\UV4.exe`

当前 `.eide/eide.yml` 中目标工具链为 `AC5`，因此编译主要依赖 Keil ARMCC 路径；GCC ARM 路径仅作为本机可用工具记录。

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
- 输出格式：`elf`

### 预定义宏

- `USE_HAL_DRIVER`
- `STM32F103xE`

### 源码目录

- `Core`
- `Hardware`

EIDE 虚拟工程中显式加入的主要源码：

- `Core/Src/main.c`
- `Core/Src/freertos.c`
- `Core/Src/gpio.c`
- `Core/Src/can.c`
- `Core/Src/dma.c`
- `Core/Src/iwdg.c`
- `Core/Src/spi.c`
- `Core/Src/usart.c`
- `Core/Src/stm32f1xx_it.c`
- `Core/Src/stm32f1xx_hal_msp.c`
- `Core/Src/stm32f1xx_hal_timebase_tim.c`
- `Hardware/RamVector/Src/ram_vector.c`
- `Hardware/RamVector/Src/cmd_exec.c`
- `Hardware/RamVector/Src/state_vector.c`
- `Hardware/AppLog/Src/app_log.c`
- `Hardware/AppLog/Src/app_log_uart.c`
- STM32F1 HAL 驱动源码
- FreeRTOS CMSIS-RTOS V2 及内核源码

### 头文件路径

- `.`
- `Core/Inc`
- `Drivers/STM32F1xx_HAL_Driver/Inc`
- `Drivers/STM32F1xx_HAL_Driver/Inc/Legacy`
- `Drivers/CMSIS/Device/ST/STM32F1xx/Include`
- `Drivers/CMSIS/Include`
- `Middlewares/Third_Party/FreeRTOS/Source/include`
- `Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2`
- `Middlewares/Third_Party/FreeRTOS/Source/portable/RVDS/ARM_CM3`
- `MDK-ARM/RTE/_STM32F103RCT6_PinprobeA1`
- `Hardware/BsmRelay/Inc`
- `Hardware/ModBus/Inc`
- `Hardware/libscpi/inc`
- `Hardware/libscpi/port`
- `Hardware/RFLink/inc`
- `Hardware/CheckMode/inc`
- `Hardware/Flash/Inc`
- `Hardware/RamVector/Inc`
- `Hardware/AppLog/Inc`

### 链接与内存布局

工程启用自定义散列文件：

- `useCustomScatterFile: true`
- `scatterFilePath: MDK-ARM/STM32F103RCT6_PinprobeA1.sct`

散列文件当前布局：

| Region | 起始地址 | 大小 | 用途 |
|---|---:|---:|---|
| `LR_IROM1` / `ER_IROM1` | `0x08000000` | `0x0003F800` | 主程序 Flash |
| `RW_IRAM1` | `0x20000000` | `0x0000A000` | 常规 RW/ZI RAM |
| `RW_RAMVEC` | `0x2000A000` | `0x00000400` | `RamVector` 专用 1024B |
| `RW_APPLOG` | `0x2000A400` | `0x00000500` | `AppLog` 专用 1280B |
| `RW_IRAM2` | `0x2000A900` | `0x00001700` | 预留/剩余 RAM |

`RAM_Vector_t` 固定放入 `RamVector` section，`AppLog_Table_t` 固定放入 `AppLog` section。

### 下载器配置

默认下载器：

- `OpenOCD`
- base address：`0x08000000`
- interface：`cmsis-dap-v1`
- target：`stm32f1x`

其他已配置下载器：

- `STLink`：SWD，地址 `0x8000000`，速度 `4000`，烧录后运行
- `JLink`：速度 `8000`
- `pyOCD`：base address `0x08000000`，速度 `4M`

### 构建产物

成功构建后输出：

- `build/STM32F103RCT6_PinprobeA1/STM32F103RCT6_PinprobeA1.hex`
- `build/STM32F103RCT6_PinprobeA1/STM32F103RCT6_PinprobeA1.s19`
- `build/STM32F103RCT6_PinprobeA1/STM32F103RCT6_PinprobeA1.bin`

最近一次构建记录：

- Code：`46688`
- RO-data：`3528`
- RW-data：`2936`
- ZI-data：`27768`
- Total ROM：`50456` bytes，约 `49.27kB`
- Total RW：`30704` bytes，约 `29.98kB`

## 命令行构建与烧录记录

### 构建方式

推荐在 VSCode 中使用 EIDE 任务：

- `build`
- `rebuild`
- `flash`
- `build and flash`

`.vscode/tasks.json` 中这些任务调用 EIDE 扩展命令：

- `${command:eide.project.build}`
- `${command:eide.project.rebuild}`
- `${command:eide.project.uploadToDevice}`
- `${command:eide.project.buildAndFlash}`

命令行直接调用 EIDE 的 `unify_builder.exe` 时，本机当前缺少 x64 `.NET 6.0` 运行时：

```text
Framework: 'Microsoft.NETCore.App', version '6.0.0' (x64)
```

因此普通 PowerShell 中不建议直接调用 `unify_builder.exe`；使用 VSCode/EIDE 内置任务更稳。

### 已验证烧录命令

使用 EIDE 自带 OpenOCD 烧录当前 hex：

```powershell
& "$env:USERPROFILE\.eide\tools\openocd_7a1adfbec_mingw32\bin\openocd.exe" `
  -f interface/cmsis-dap.cfg `
  -c "cmsis_dap_backend hid" `
  -f target/stm32f1x.cfg `
  -c "adapter speed 1000" `
  -c "program build/STM32F103RCT6_PinprobeA1/STM32F103RCT6_PinprobeA1.hex verify reset exit"
```

关键点：

- 必须显式指定 `cmsis_dap_backend hid`
- 目标配置：`target/stm32f1x.cfg`
- 下载文件：`build/STM32F103RCT6_PinprobeA1/STM32F103RCT6_PinprobeA1.hex`
- 烧录流程：`program ... verify reset exit`

未指定 HID 后端时，OpenOCD 会尝试 CMSIS-DAPv2 bulk 接口，本机出现过如下失败：

```text
Warn : could not claim interface: Operation not supported or unimplemented on this platform
Error: CMSIS-DAP command CMD_INFO failed.
** OpenOCD init failed **
```

改用 HID 后端后已验证成功：

```text
Info : [stm32f1x.cpu] Cortex-M3 r1p1 processor detected
Info : flash size = 256 KiB
** Programming Finished **
** Verified OK **
** Resetting Target **
```
