<#
.SYNOPSIS
  Builds the Release configuration, installs every program into dist\ and zips each folder.

.DESCRIPTION
  Result:
    dist\Soldier 2.0\          dist\Soldier 2.0.zip
    dist\Piton\                dist\Piton.zip
    dist\Keyboard Simulator\   dist\Keyboard Simulator.zip
    dist\Painter\              dist\Painter.zip
  Each folder is self-contained (exe, SDL3.dll, the Visual C++ runtime DLLs, data\, README.txt).

.PARAMETER SkipBuild
  Do not configure and build; install what build\vs2022 already contains.

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File tools\package.ps1
#>
param([switch]$SkipBuild)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root 'build\vs2022'
$dist = Join-Path $root 'dist'

function Invoke-Step([string]$exe, [string[]]$arguments) {
    & $exe @arguments
    if ($LASTEXITCODE -ne 0) { throw "$exe $($arguments -join ' ') failed with exit code $LASTEXITCODE" }
}

Push-Location $root
try {
    if (-not $SkipBuild) {
        Invoke-Step cmake @('--preset', 'vs2022')
        Invoke-Step cmake @('--build', '--preset', 'release')
    }
    if (Test-Path $dist) { Remove-Item -Recurse -Force $dist }
    Invoke-Step cmake @('--install', $build, '--config', 'Release', '--prefix', $dist)

    $folders = Get-ChildItem -Directory $dist
    foreach ($folder in $folders) {
        $zip = Join-Path $dist ($folder.Name + '.zip')
        Compress-Archive -Path $folder.FullName -DestinationPath $zip -CompressionLevel Optimal
        Write-Host ("{0,-28} {1,8:N0} KB" -f ($folder.Name + '.zip'), ((Get-Item $zip).Length / 1KB))
    }
    if ($folders.Count -eq 0) { throw 'nothing was installed' }
}
finally {
    Pop-Location
}
