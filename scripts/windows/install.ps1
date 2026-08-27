param(
    [string]$GameDir = "${env:ProgramFiles(x86)}\Steam\steamapps\common\METAL GEAR SOLID 4\MGS4"
)
$ErrorActionPreference = "Stop"
$PackageDir = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$BackupDir = Join-Path $GameDir ".mgs4ultra120-backup"
if (-not (Test-Path (Join-Path $GameDir "mgs4.exe"))) { throw "mgs4.exe not found in: $GameDir" }
if (Get-Process mgs4 -ErrorAction SilentlyContinue) { throw "Exit the game before installing." }
New-Item -ItemType Directory -Force -Path $BackupDir | Out-Null
foreach ($Name in @("winmm.dll", "mgs4_ultrawide.ini")) {
    $Target = Join-Path $GameDir $Name
    $Backup = Join-Path $BackupDir "$Name.preinstall"
    if ((Test-Path $Target) -and -not (Test-Path $Backup)) { Copy-Item -LiteralPath $Target -Destination $Backup }
}
Copy-Item -Force (Join-Path $PackageDir "bin\winmm.dll") (Join-Path $GameDir "winmm.dll")
Copy-Item -Force (Join-Path $PackageDir "config\mgs4_ultrawide.ini") (Join-Path $GameDir "mgs4_ultrawide.ini")
Write-Host "Installed in: $GameDir"
