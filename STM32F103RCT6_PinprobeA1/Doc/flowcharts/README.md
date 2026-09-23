# PinProbe A1 当前控制流程图

依据 2026-09-15 工作区实际源码绘制，UTF-8 编码。图中描述当前实现，不代表固件已修改或已经过实机验证。

主要交付：[三页 A4 竖版 HTML](../PinProbe%20A1%20状态机与动作控制流程图20260915.html)。一份文件包含三张 A4 页面，流程图内嵌，无外部资源依赖；可直接打开或打印。版式复用 `Doc/PinProbe A1 箱体控制 SCPI 指令说明20260912.html` 的完整样式表与 GTS 标志，沿用字体、页边距、页眉页脚与表格样式。

三页依次为主状态机、门与 USB 动作时序、动作执行与 IO 更新闭环。对应打印版 SVG 为 `a4-01-state-machine.svg`、`a4-02-door-usb.svg`、`a4-03-action-update.svg`。

以下为包含更多条件说明的大尺寸 SVG：

1. [主状态机与异常分支](01-state-machine.svg)：7 个主状态、状态纠偏、安全事件、气缸状态和控制模式。
2. [门与 USB 自动动作时序](02-door-usb-sequence.svg)：从关门准备到 USB 插入、关门、开门及 USB 拔出，标明 2 秒超时起点。
3. [动作执行与 IO 更新链路](03-action-update.svg)：线程周期、命令槽仲裁、执行顺序、IO 回读和通信恢复。

SVG 可直接用浏览器打开、缩放，也可导入支持 SVG 的文档或绘图软件。每张图包含标题、图例或时序说明及源码定位。

## 阅读边界

- 主状态 `COMPLETE` 表示关门完成阶段；开门过程中仍可保持此状态。机械动作应同时查看门 / USB 气缸状态。
- 正常自动拔出是“开门到位 → 满足自动拔出条件 → 投递 USB 拔出 → 写输出 → 回读到拔出输出 → 开始 2 秒行程超时”。当前没有开门后固定等 2 秒再拔出的延时。
- 自动拔出还要求已解锁、无 USB 故障、无急停、无关门准备意图、气缸槽空闲、USB 尚未拔到位且尚未在拔出。如果插入输出仍有效，还要求 USB 已插到位，才允许这一自动纠偏分支回退。
- 开门完成判定在自动 USB 纠偏段之后，因此正常 `COMPLETE → IDLE` 后通常在后续状态轮次投递 USB 拔出。实际等待受按钮、灯色、待执行命令等条件影响。
- SCPI 直接操作 USB 气缸不经过自动开门到位编排。`USB:AUTO OFF` 禁用此处 2 秒超时判断，但传感器 / 输出冲突仍可使气缸镜像为 `ERR`。
- `READY` 的 500 ms 实际从任意门按钮按下开始计时，到期时检查双按钮同时有效；代码没有要求双按钮连续同时按住 500 ms。图按实际条件绘制。
- 插入超时只豁免 `RUNNING`，不豁免 `COMPLETE`。完成分支虽有 USB 未到位黄闪诊断，后续底层超时仍可能触发退回 `IDLE`；不能将黄闪注释理解为整个状态机永久屏蔽 USB 故障。
- `IS_ANY_LASER` 包含 `IN_LASER1` 气压位。风险完成条件把该位为 1 作为压力正常，激光判断也把它纳入非零判断；这部分按源码呈现，物理含义需结合接线与配置核对。
- 通信不可信时状态逻辑提前返回，执行器会消费并丢弃命令，不会自动重放。图中的“停止本轮逻辑”不是硬件输出已关闭的确认。
- 任务使用线程标志唤醒。25 ms / 50 ms 是名义节拍，不是每个命令响应或通信完成的保证时间。
- 图覆盖当前本机控制链路。`StateVectorTask` 使用 `RamVector_Init(0)`；未将预留 CAN 多机同步画成已经接入的命令来源。

## 源码索引

| 内容 | 位置 |
| --- | --- |
| 状态枚举、命令、优先级、气缸状态 | `Hardware/RamVector/Inc/ram_vector.h` |
| 时序常量 | `Hardware/RamVector/Src/state_vector.c:128` |
| 模式切换、SCPI 关门意图 | `Hardware/RamVector/Src/state_vector.c:159` |
| IO 快照、链路保护、消抖 | `Hardware/RamVector/Src/state_vector.c:250` |
| 主状态纠偏、门输出沿计时 | `Hardware/RamVector/Src/state_vector.c:491` |
| USB 计时、到位与故障 | `Hardware/RamVector/Src/state_vector.c:627` |
| 自动 USB 拔出条件 | `Hardware/RamVector/Src/state_vector.c:775` |
| 关门完成、开门完成、紧急恢复 | `Hardware/RamVector/Src/state_vector.c:796` |
| 急停与激光触发 | `Hardware/RamVector/Src/state_vector.c:910` |
| 电源按钮、SCPI 与物理门流程 | `Hardware/RamVector/Src/state_vector.c:969` |
| 灯效、异常退回、状态发布 | `Hardware/RamVector/Src/state_vector.c:1205` |
| ModBus 与状态任务、定时唤醒 | `App/Src/app_tasks.c:59` |
| 命令槽投递、取走与清空、镜像更新 | `Hardware/RamVector/Src/ram_vector.c:41` |
| 命令执行顺序及链路门控 | `Hardware/RamVector/Src/cmd_exec.c:117` |
| 气缸方向对应实际输出 | `Hardware/BsmRelay/Src/BsmRelay.c:88` |
| SCPI 气缸动作入口 | `Hardware/libscpi/port/scpi-def.c:320` |

## 更新图稿

图稿由同目录生成器生成，不依赖第三方包。在项目根目录运行：

```powershell
node Doc/flowcharts/generate.mjs
node Doc/flowcharts/generate-a4.mjs
```

`generate.mjs` 生成大尺寸 SVG；`generate-a4.mjs` 生成打印版 SVG 和三页 HTML，运行时读取指定参考 HTML 的样式及标志。源码流程改变后，需要先同步生成器中的节点与条件，再重新生成；生成器不会自动解析 C 代码。
