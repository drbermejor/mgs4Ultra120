param(
    [string]$GameDir = "${env:ProgramFiles(x86)}\Steam\steamapps\common\METAL GEAR SOLID 4\MGS4"
)
$ErrorActionPreference = "Stop"
$PackageDir = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$BackupDir = Join-Path $GameDir ".mgs4ultra120-backup"
$InstallDir = Split-Path -Parent $GameDir
$LauncherTarget = Join-Path (Join-Path $InstallDir "Launcher") "launcher.exe"
$LauncherBackup = Join-Path $BackupDir "launcher.exe.preinstall"
$WrapperSource = Join-Path $PackageDir "bin\launcher.exe"
if (Get-Process mgs4 -ErrorAction SilentlyContinue) { throw "Exit the game before uninstalling." }
$LauncherConflict = $false
if (Test-Path -LiteralPath $LauncherBackup) {
    if ((Test-Path -LiteralPath $WrapperSource) -and (Test-Path -LiteralPath $LauncherTarget) -and
        ((Get-FileHash -Algorithm SHA256 -LiteralPath $WrapperSource).Hash -eq
         (Get-FileHash -Algorithm SHA256 -LiteralPath $LauncherTarget).Hash)) {
        Move-Item -Force -LiteralPath $LauncherBackup -Destination $LauncherTarget
    } else {
        $LauncherConflict = $true
        Write-Warning "Launcher changed outside MGS4 Ultra120; preserving it and the launcher backup."
    }
}
foreach ($Name in @("winmm.dll", "mgs4_ultrawide.ini")) {
    $Target = Join-Path $GameDir $Name
    $Backup = Join-Path $BackupDir "$Name.preinstall"
    if (Test-Path $Backup) { Move-Item -Force -LiteralPath $Backup -Destination $Target }
    else { Remove-Item -Force -ErrorAction SilentlyContinue -LiteralPath $Target }
}
if (-not $LauncherConflict) {
    Remove-Item -Force -Recurse -ErrorAction SilentlyContinue -LiteralPath $BackupDir
}
Write-Host "Uninstalled from: $GameDir"
