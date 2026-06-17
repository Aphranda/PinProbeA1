param(
    [string]$EideVersion = "cl.eide-3.27.2"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$builder = Join-Path $env:USERPROFILE ".vscode\extensions\$EideVersion\res\tools\win32\unify_builder\unify_builder.exe"

if (!(Test-Path $builder)) {
    throw "EIDE builder not found: $builder"
}

function Invoke-CheckedNative {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,
        [string[]]$Arguments
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $FilePath $($Arguments -join ' ')"
    }
}

Push-Location $root
try {
    $env:DOTNET_ROLL_FORWARD = "Major"
    Invoke-CheckedNative $builder @("-p", "build\PinProbeA1_Bootloader\builder.params")
    Invoke-CheckedNative $builder @("-p", "build\STM32F103RCT6_PinprobeA1\builder.params")
    Invoke-CheckedNative "python" @("Tools\merge_ota_images.py")
}
finally {
    Pop-Location
}
