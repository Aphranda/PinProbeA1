<#
.SYNOPSIS
    一次性文档目录迁移脚本：Doc/ 平铺目录 -> docs/ 域目录化管理。

.DESCRIPTION
    按 docs/docs/DOCS_DOMAIN_STRUCTURE_PLAN.md 的映射表执行物理迁移。
    - 已被 git 跟踪的文件使用 git mv，保留历史；
    - 未被跟踪的文件（.gitignore 命中的 zip/pdf/exe 或新增文件）使用 Move-Item。

    迁移完成后应运行：
        python Tools\docs_check.py
        python Tools\docs_check.py --strict-names

.NOTES
    该脚本只执行一次；重复执行时缺失的源文件会被跳过并打印 MISS。
#>

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$srcRoot = Join-Path $repoRoot 'Doc'

$map = @(
    # ---- 架构域 arch/ ----
    @{ From = 'HAOFV_ARCHITECTURE.md'; To = 'docs/arch/HAOFV_ARCHITECTURE.md' }

    # ---- 控制域 control/ ----
    @{ From = '时间分析.md'; To = 'docs/control/CONTROL_TIMING_ANALYSIS.md' }

    # ---- 接口域 interface/ ----
    @{ From = 'PinProbe A1 SCPI 指令表（内部）.md'; To = 'docs/interface/SCPI_COMMANDS.md' }

    # ---- 多机分布式域 multinode/ ----
    @{ From = 'PinProbeA1_基于HAOFV的CAN多机分布式架构.md'; To = 'docs/multinode/MULTINODE_CAN_HAOFV_ARCHITECTURE.md' }
    @{ From = 'PinProbeA1_多机架构设计方案_RAM反射内存.md'; To = 'docs/multinode/MULTINODE_REFLECTIVE_MEMORY_ARCHITECTURE.md' }

    # ---- OTA 域 ota/ ----
    @{ From = 'PinProbeA1_OTA固件升级与CAN多机分发方案.md'; To = 'docs/ota/OTA_CAN_DISTRIBUTION_ARCHITECTURE.md' }

    # ---- 硬件域 hardware/ ----
    @{ From = 'PinProbeA1_TPS7A2033失效初步分析报告_20261010.md'; To = 'docs/hardware/HARDWARE_TPS7A2033_FAILURE_ANALYSIS.md' }
    @{ From = 'PinProbeA1_USB5V输入防护优化报告_20261010.md'; To = 'docs/hardware/HARDWARE_USB5V_PROTECTION_DESIGN.md' }
    @{ From = 'PinProbeA1_USB自动拔插异常报告_A0.md'; To = 'docs/hardware/HARDWARE_USB_REPLUG_A0_ANALYSIS.md' }
    @{ From = 'PinProbeA1_USB自动拔插异常报告_B0.md'; To = 'docs/hardware/HARDWARE_USB_REPLUG_B0_ANALYSIS.md' }
    @{ From = 'BSM系列产品说明书.pdf'; To = 'docs/hardware/datasheets/BSM系列产品说明书.pdf' }
    @{ From = '参考资料/TI_TPS7A20_TPS7A2033_SBVS338H_datasheet.pdf'; To = 'docs/hardware/datasheets/TI_TPS7A20_TPS7A2033_SBVS338H_datasheet.pdf' }
    @{ From = '图/5V供电路径.svg'; To = 'docs/hardware/diagrams/5V供电路径.svg' }
    @{ From = '图/主机供电回路.svg'; To = 'docs/hardware/diagrams/主机供电回路.svg' }
    @{ From = '图/从机供电回路.svg'; To = 'docs/hardware/diagrams/从机供电回路.svg' }
    @{ From = '失效照片/image.png'; To = 'docs/hardware/evidence/TPS7A2033_failure_vin.png' }
    @{ From = '失效照片/PCB.png'; To = 'docs/hardware/evidence/PCB.png' }
    @{ From = '失效照片/Netlist_IO-PPA-CH318TUSBEXT-4F4-HASL-V1.0_2026-10-10.txt'; To = 'docs/hardware/netlist/Netlist_IO-PPA-CH318TUSBEXT-4F4-HASL-V1.0_2026-10-10.txt' }

    # ---- 验证域 validation/ ----
    @{ From = '代码审查报告_20260613.md'; To = 'docs/validation/VALIDATION_CODE_REVIEW_20260613.md' }
    @{ From = '代码审查报告_风险分析.md'; To = 'docs/validation/VALIDATION_RISK_REVIEW_20260612.md' }

    # ---- 发布域 release/ ----
    @{ From = 'Git分支与发布计划.md'; To = 'docs/release/RELEASE_BRANCH_PLAN.md' }
    @{ From = 'PinProbeA1客户外发包_20260728.zip'; To = 'docs/release/deliverables/PinProbeA1客户外发包_20260728.zip' }
    @{ From = 'PinProbeA1客户外发包_20260728/PinProbe A1 箱体控制 SCPI 指令说明20260728.pdf'; To = 'docs/release/deliverables/PinProbeA1客户外发包_20260728/PinProbe A1 箱体控制 SCPI 指令说明20260728.pdf' }
    @{ From = 'PinProbeA1客户外发包_20260728/PinProbeA1箱体控制程序.exe'; To = 'docs/release/deliverables/PinProbeA1客户外发包_20260728/PinProbeA1箱体控制程序.exe' }
    @{ From = 'PinProbeA1客户外发包_20260728/PinProbeA1箱体控制程序操作说明_20260728.pdf'; To = 'docs/release/deliverables/PinProbeA1客户外发包_20260728/PinProbeA1箱体控制程序操作说明_20260728.pdf' }

    # ---- 报告输出域 reports/（当前交付件）----
    @{ From = 'PinProbe A1 箱体控制 SCPI 指令说明20260924.html'; To = 'docs/reports/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260924.html' }
    @{ From = 'PinProbe A1 箱体控制 SCPI 指令说明20260924.pdf'; To = 'docs/reports/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260924.pdf' }
    @{ From = 'PinProbe A1 Box Control SCPI Command Reference 20260924.html'; To = 'docs/reports/scpi/PinProbe A1 Box Control SCPI Command Reference 20260924.html' }
    @{ From = 'PinProbe A1 Box Control SCPI Command Reference 20260924.pdf'; To = 'docs/reports/scpi/PinProbe A1 Box Control SCPI Command Reference 20260924.pdf' }
    @{ From = 'PinProbe A1 状态机与动作控制流程图20260915.html'; To = 'docs/reports/control/PinProbe A1 状态机与动作控制流程图20260915.html' }
    @{ From = 'PinProbe A1 状态机与动作控制流程图20260915.pdf'; To = 'docs/reports/control/PinProbe A1 状态机与动作控制流程图20260915.pdf' }
    @{ From = 'flowcharts/01-state-machine.svg'; To = 'docs/reports/control/flowcharts/01-state-machine.svg' }
    @{ From = 'flowcharts/02-door-usb-sequence.svg'; To = 'docs/reports/control/flowcharts/02-door-usb-sequence.svg' }
    @{ From = 'flowcharts/03-action-update.svg'; To = 'docs/reports/control/flowcharts/03-action-update.svg' }
    @{ From = 'flowcharts/a4-01-state-machine.svg'; To = 'docs/reports/control/flowcharts/a4-01-state-machine.svg' }
    @{ From = 'flowcharts/a4-02-door-usb.svg'; To = 'docs/reports/control/flowcharts/a4-02-door-usb.svg' }
    @{ From = 'flowcharts/a4-03-action-update.svg'; To = 'docs/reports/control/flowcharts/a4-03-action-update.svg' }
    @{ From = 'flowcharts/generate.mjs'; To = 'docs/reports/control/flowcharts/generate.mjs' }
    @{ From = 'flowcharts/generate-a4.mjs'; To = 'docs/reports/control/flowcharts/generate-a4.mjs' }
    @{ From = 'flowcharts/README.md'; To = 'docs/reports/control/flowcharts/README.md' }
    @{ From = 'PinProbe A1控制箱 嵌入式整体方案与架构报告.html'; To = 'docs/reports/arch/PinProbe A1控制箱 嵌入式整体方案与架构报告.html' }
    @{ From = 'PinProbe A1控制箱 嵌入式整体方案与架构报告.pdf'; To = 'docs/reports/arch/PinProbe A1控制箱 嵌入式整体方案与架构报告.pdf' }
    @{ From = 'PinProbeA1_TPS7A2033失效初步分析报告_20261010.html'; To = 'docs/reports/hardware/PinProbeA1_TPS7A2033失效初步分析报告_20261010.html' }
    @{ From = 'PinProbeA1_TPS7A2033失效初步分析报告_20261012.pdf'; To = 'docs/reports/hardware/PinProbeA1_TPS7A2033失效初步分析报告_20261012.pdf' }
    @{ From = 'PinProbeA1_USB5V输入防护优化报告_20261010.html'; To = 'docs/reports/hardware/PinProbeA1_USB5V输入防护优化报告_20261010.html' }
    @{ From = 'PinProbeA1_USB5V输入防护优化报告_20261010.pdf'; To = 'docs/reports/hardware/PinProbeA1_USB5V输入防护优化报告_20261010.pdf' }
    @{ From = 'PinProbeA1_USB自动拔插异常报告_A0.html'; To = 'docs/reports/hardware/PinProbeA1_USB自动拔插异常报告_A0.html' }
    @{ From = 'PinProbeA1_USB自动拔插异常报告_A0.pdf'; To = 'docs/reports/hardware/PinProbeA1_USB自动拔插异常报告_A0.pdf' }
    @{ From = 'PinProbeA1_USB自动拔插异常报告_B0.html'; To = 'docs/reports/hardware/PinProbeA1_USB自动拔插异常报告_B0.html' }
    @{ From = 'PinProbeA1_USB自动拔插异常报告_B0.pdf'; To = 'docs/reports/hardware/PinProbeA1_USB自动拔插异常报告_B0.pdf' }
    @{ From = 'PinProbeA1 Box Control Program Operation Guide Customer 20260728.html'; To = 'docs/reports/guide/PinProbeA1 Box Control Program Operation Guide Customer 20260728.html' }
    @{ From = 'PinProbeA1 Box Control Program Operation Guide.pdf'; To = 'docs/reports/guide/PinProbeA1 Box Control Program Operation Guide.pdf' }
    @{ From = 'PinProbeA1箱体控制程序操作说明_客户版_20260727.md'; To = 'docs/reports/guide/PinProbeA1箱体控制程序操作说明_客户版_20260727.md' }
    @{ From = 'PinProbeA1箱体控制程序操作说明_客户版_20260728.md'; To = 'docs/reports/guide/PinProbeA1箱体控制程序操作说明_客户版_20260728.md' }
    @{ From = 'PinProbeA1箱体控制程序操作说明_客户版_20260728.html'; To = 'docs/reports/guide/PinProbeA1箱体控制程序操作说明_客户版_20260728.html' }
    @{ From = 'PinProbeA1箱体控制程序操作说明_客户版_20260728.pdf'; To = 'docs/reports/guide/PinProbeA1箱体控制程序操作说明_客户版_20260728.pdf' }
    @{ From = 'PinProbeA1箱体控制程序操作说明_20260728.pdf'; To = 'docs/reports/guide/PinProbeA1箱体控制程序操作说明_20260728.pdf' }
    @{ From = 'assets/pinprobe_gui_main.png'; To = 'docs/reports/guide/assets/pinprobe_gui_main.png' }
    @{ From = 'assets/pinprobe_gui_ota.png'; To = 'docs/reports/guide/assets/pinprobe_gui_ota.png' }
    @{ From = 'assets/烧录区域.png'; To = 'docs/reports/guide/assets/烧录区域.png' }
    @{ From = 'assets/软件主体.png'; To = 'docs/reports/guide/assets/软件主体.png' }

    # ---- 工具域 tools/ 与静态资源 assets/ ----
    @{ From = 'model/AI生成PPT操作手册.html'; To = 'docs/tools/AI生成PPT操作手册.html' }
    @{ From = 'model/template.html'; To = 'docs/assets/template/template.html' }
    @{ From = 'model/logo.png'; To = 'docs/assets/template/logo.png' }
    @{ From = 'model/logo.svg'; To = 'docs/assets/template/logo.svg' }

    # ---- 跨域项目板 ----
    @{ From = '待办事项.md'; To = 'docs/PROJECT_TODO.md' }

    # ---- 历史域 legacy/ ----
    @{ From = 'PinprobeA1.md'; To = 'docs/legacy/control/LEGACY_PINPROBEA1_COMMAND_LIST.md' }
    @{ From = 'PinProbeLink.png'; To = 'docs/legacy/control/PinProbeLink.png' }
    @{ From = 'PinProbe A1 箱体控制 SCPI 指令说明0630.pdf'; To = 'docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明0630.pdf' }
    @{ From = 'PinProbe A1 箱体控制 SCPI 指令说明0727.html'; To = 'docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明0727.html' }
    @{ From = 'PinProbe A1 箱体控制 SCPI 指令说明0727.md'; To = 'docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明0727.md' }
    @{ From = 'PinProbe A1 箱体控制 SCPI 指令说明0727.pdf'; To = 'docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明0727.pdf' }
    @{ From = 'PinProbe A1 箱体控制 SCPI 指令说明20260728.md'; To = 'docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260728.md' }
    @{ From = 'PinProbe A1 箱体控制 SCPI 指令说明20260728.pdf'; To = 'docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260728.pdf' }
    @{ From = 'PinProbe A1 箱体控制 SCPI 指令说明20260811.html'; To = 'docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260811.html' }
    @{ From = 'PinProbe A1 箱体控制 SCPI 指令说明20260811.pdf'; To = 'docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260811.pdf' }
    @{ From = 'PinProbe A1 箱体控制 SCPI 指令说明20260911.html'; To = 'docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260911.html' }
    @{ From = 'PinProbe A1 箱体控制 SCPI 指令说明20260911.pdf'; To = 'docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260911.pdf' }
    @{ From = 'PinProbe A1 箱体控制 SCPI 指令说明20260912.html'; To = 'docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260912.html' }
    @{ From = 'PinProbe A1 箱体控制 SCPI 指令说明20260912.pdf'; To = 'docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260912.pdf' }
    @{ From = 'PinProbe A1 箱体控制 SCPI 指令说明20260912_固件实现版.pdf'; To = 'docs/legacy/scpi/PinProbe A1 箱体控制 SCPI 指令说明20260912_固件实现版.pdf' }
    @{ From = 'PinProbe A1 Box Control SCPI Command Reference 20260728.html'; To = 'docs/legacy/scpi/PinProbe A1 Box Control SCPI Command Reference 20260728.html' }
    @{ From = 'PinProbe A1 Box Control SCPI Command Reference 20260912.html'; To = 'docs/legacy/scpi/PinProbe A1 Box Control SCPI Command Reference 20260912.html' }
    @{ From = 'PinProbe A1 Box Control SCPI Command Reference 20260912.md'; To = 'docs/legacy/scpi/PinProbe A1 Box Control SCPI Command Reference 20260912.md' }
    @{ From = 'PinProbe A1 Box Control SCPI Command Reference 20260912.pdf'; To = 'docs/legacy/scpi/PinProbe A1 Box Control SCPI Command Reference 20260912.pdf' }
    @{ From = 'PinProbe A1 Box Control SCPI Command Reference.md'; To = 'docs/legacy/scpi/PinProbe A1 Box Control SCPI Command Reference.md' }
    @{ From = 'PinProbe A1 Box Control SCPI Command Reference.pdf'; To = 'docs/legacy/scpi/PinProbe A1 Box Control SCPI Command Reference.pdf' }
    @{ From = 'PinProbeA1_SCPI_20260912_mode_error201.pdf'; To = 'docs/legacy/scpi/PinProbeA1_SCPI_20260912_mode_error201.pdf' }
    @{ From = 'PinProbeA1_SCPI_20260912_mode_impl.pdf'; To = 'docs/legacy/scpi/PinProbeA1_SCPI_20260912_mode_impl.pdf' }
    @{ From = 'SaveWindows2026_6_16_23-33-06.TXT'; To = 'docs/legacy/logs/SaveWindows2026_6_16_23-33-06.TXT' }
    @{ From = 'SaveWindows2026_6_16_23-38-58.TXT'; To = 'docs/legacy/logs/SaveWindows2026_6_16_23-38-58.TXT' }
    @{ From = '今日工作总结_20260611.md'; To = 'docs/legacy/LEGACY_DAILY_SUMMARY_20260611.md' }

    # ---- 归档域 archive/ ----
    @{ From = 'untitled.642.png'; To = 'docs/archive/untitled.642.png' }
)

$moved = 0
$skipped = 0

foreach ($item in $map) {
    $fromRel = 'Doc/' + $item.From
    $toRel = $item.To
    $fromAbs = Join-Path $repoRoot ($fromRel -replace '/', '\')
    $toAbs = Join-Path $repoRoot ($toRel -replace '/', '\')

    if (-not (Test-Path -LiteralPath $fromAbs)) {
        Write-Output "MISS $fromRel"
        $skipped++
        continue
    }

    $toDir = Split-Path -Parent $toAbs
    if (-not (Test-Path -LiteralPath $toDir)) {
        New-Item -ItemType Directory -Path $toDir -Force | Out-Null
    }

    $tracked = (& git -C $repoRoot ls-files -- $fromRel) 2>$null
    if ($tracked) {
        & git -C $repoRoot mv -- $fromRel $toRel
        if ($LASTEXITCODE -ne 0) { throw "git mv failed: $fromRel -> $toRel" }
    }
    else {
        Move-Item -LiteralPath $fromAbs -Destination $toAbs
    }
    $moved++
}

# 清理空的 Doc/ 目录树
Get-ChildItem -LiteralPath $srcRoot -Recurse -Directory -Force |
    Sort-Object { $_.FullName.Length } -Descending |
    ForEach-Object {
        if (-not (Get-ChildItem -LiteralPath $_.FullName -Force)) { Remove-Item -LiteralPath $_.FullName -Force }
    }
if ((Test-Path -LiteralPath $srcRoot) -and -not (Get-ChildItem -LiteralPath $srcRoot -Force)) {
    Remove-Item -LiteralPath $srcRoot -Force
    Write-Output 'REMOVED Doc/ (empty)'
}

Write-Output "SUMMARY moved=$moved skipped=$skipped"
