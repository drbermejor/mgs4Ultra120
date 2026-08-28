param(
    [string]$GameDir = "${env:ProgramFiles(x86)}\Steam\steamapps\common\METAL GEAR SOLID 4\MGS4",
    [switch]$IncludeImproved120
)
$ErrorActionPreference = "Stop"
$PackageDir = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
. (Join-Path $PSScriptRoot "common.ps1")
Install-Mgs4Ultra120Patch $GameDir $PackageDir
. (Join-Path $PSScriptRoot "mgsfpsunlock.ps1")
if ($IncludeImproved120) {
    Install-MgsFpsUnlock $GameDir
}
Write-Host "Installed in: $GameDir"
