param(
    [string]$EideVersion = "cl.eide-3.27.2"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$builder = Join-Path $env:USERPROFILE ".vscode\extensions\$EideVersion\res\tools\win32\unify_builder\unify_builder.exe"

if (!(Test-Path $builder)) {
    throw "EIDE builder not found: $builder"
}

Push-Location $root
try {
    $env:DOTNET_ROLL_FORWARD = "Major"
    & $builder -p "build\PinProbeA1_Bootloader\builder.params"
    & $builder -p "build\STM32F103RCT6_PinprobeA1\builder.params"
    python "Tools\merge_ota_images.py"
}
finally {
    Pop-Location
}
