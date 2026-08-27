param(
    [string]$GameDir = "${env:ProgramFiles(x86)}\Steam\steamapps\common\METAL GEAR SOLID 4\MGS4"
)
$ErrorActionPreference = "Stop"
$BackupDir = Join-Path $GameDir ".mgs4ultra120-backup"
if (Get-Process mgs4 -ErrorAction SilentlyContinue) { throw "Exit the game before uninstalling." }
foreach ($Name in @("winmm.dll", "mgs4_ultrawide.ini")) {
    $Target = Join-Path $GameDir $Name
    $Backup = Join-Path $BackupDir "$Name.preinstall"
    if (Test-Path $Backup) { Move-Item -Force -LiteralPath $Backup -Destination $Target }
    else { Remove-Item -Force -ErrorAction SilentlyContinue -LiteralPath $Target }
}
Remove-Item -Force -ErrorAction SilentlyContinue -LiteralPath $BackupDir
Write-Host "Uninstalled from: $GameDir"
