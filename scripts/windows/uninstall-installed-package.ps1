$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "common.ps1")

$GameDir = Get-Mgs4Ultra120GameDir
if (-not $GameDir) { exit 0 }
if (-not [IO.File]::Exists([IO.Path]::Combine($GameDir, "mgs4.exe"))) {
    Clear-Mgs4Ultra120GameDir $GameDir
    exit 0
}

$PatchFilesExist = (Test-Path -LiteralPath (Join-Path $GameDir "winmm.dll")) -or
    (Test-Path -LiteralPath (Join-Path (Join-Path $GameDir "scripts") `
        "MGS4Ultra120.asi")) -or
    (Test-Path -LiteralPath (Join-Path $GameDir "mgs4_ultrawide.ini")) -or
    (Test-Path -LiteralPath (Join-Path $GameDir ".mgs4ultra120-backup"))
if ($PatchFilesExist) {
    & (Join-Path $PSScriptRoot "uninstall.ps1") -GameDir $GameDir
}
Clear-Mgs4Ultra120GameDir $GameDir
