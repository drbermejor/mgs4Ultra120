param(
    [string]$GameDir = "${env:ProgramFiles(x86)}\Steam\steamapps\common\METAL GEAR SOLID 4\MGS4"
)
$ErrorActionPreference = "Stop"
if (-not $GameDir -or -not [IO.Directory]::Exists($GameDir)) {
    throw "The selected MGS4 folder is unavailable. Reconnect its drive or choose it again from Easy setup."
}
$PackageDir = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
. (Join-Path $PSScriptRoot "common.ps1")
$BackupDir = Join-Path $GameDir ".mgs4ultra120-backup"
$InstallDir = Split-Path -Parent $GameDir
$LauncherTarget = Join-Path (Join-Path $InstallDir "Launcher") "launcher.exe"
$LauncherBackup = Join-Path $BackupDir "launcher.exe.preinstall"
$LauncherHashMarker = Join-Path $BackupDir "launcher-wrapper-installed.sha256"
$WrapperSource = Join-Path $PackageDir "bin\launcher.exe"
if (Get-Process mgs4 -ErrorAction SilentlyContinue) { throw "Exit the game before uninstalling." }
$DisplayMetadata = Join-Path $BackupDir "windows-launcher-settings.json"
if (Test-Path -LiteralPath $DisplayMetadata) {
    Restore-Mgs4Ultra120WindowsDisplaySettings $GameDir
}
$LauncherConflict = $false
if (Test-Path -LiteralPath $LauncherBackup) {
    $ManagedHashes = [Collections.Generic.HashSet[string]]::new(
        [StringComparer]::OrdinalIgnoreCase)
    if (Test-Path -LiteralPath $WrapperSource) {
        [void]$ManagedHashes.Add((Get-FileHash -Algorithm SHA256 `
            -LiteralPath $WrapperSource).Hash)
    }
    if (Test-Path -LiteralPath $LauncherHashMarker) {
        $RecordedHash = (Get-Content -Raw -LiteralPath $LauncherHashMarker).Trim()
        if ($RecordedHash -match '^[0-9a-fA-F]{64}$') {
            [void]$ManagedHashes.Add($RecordedHash)
        }
    }
    if ((Test-Path -LiteralPath $LauncherTarget) -and
        $ManagedHashes.Contains((Get-FileHash -Algorithm SHA256 `
            -LiteralPath $LauncherTarget).Hash)) {
        Move-Item -Force -LiteralPath $LauncherBackup -Destination $LauncherTarget
        Remove-Item -Force -LiteralPath $LauncherHashMarker `
            -ErrorAction SilentlyContinue
    } else {
        $LauncherConflict = $true
        Write-Warning "Launcher changed outside MGS4 Ultra120; preserving it and the launcher backup."
    }
}
$DllConflict = $false
$DllTarget = Join-Path $GameDir "winmm.dll"
$DllBackup = Join-Path $BackupDir "winmm.dll.preinstall"
$DllHashMarker = Join-Path $BackupDir "winmm-installed.sha256"
if (Test-Path -LiteralPath $DllTarget) {
    $BundledDll = Join-Path $PackageDir "bin\winmm.dll"
    $ManagedHashes = [Collections.Generic.HashSet[string]]::new(
        [StringComparer]::OrdinalIgnoreCase)
    if (Test-Path -LiteralPath $BundledDll) {
        [void]$ManagedHashes.Add((Get-FileHash -Algorithm SHA256 `
            -LiteralPath $BundledDll).Hash)
    }
    if (Test-Path -LiteralPath $DllHashMarker) {
        $RecordedHash = (Get-Content -Raw -LiteralPath $DllHashMarker).Trim()
        if ($RecordedHash -match '^[0-9a-fA-F]{64}$') {
            [void]$ManagedHashes.Add($RecordedHash)
        }
    }
    foreach ($LegacyHash in $Mgs4Ultra120LegacyDllHashes) {
        [void]$ManagedHashes.Add($LegacyHash)
    }
    if ($ManagedHashes.Contains((Get-FileHash -Algorithm SHA256 `
        -LiteralPath $DllTarget).Hash)) {
        if (Test-Path -LiteralPath $DllBackup) {
            Move-Item -Force -LiteralPath $DllBackup -Destination $DllTarget
        } else {
            Remove-Item -Force -LiteralPath $DllTarget
        }
        Remove-Item -Force -LiteralPath $DllHashMarker `
            -ErrorAction SilentlyContinue
    } else {
        $DllConflict = $true
        Write-Warning "winmm.dll changed outside MGS4 Ultra120; preserving it and its backup."
    }
} elseif (Test-Path -LiteralPath $DllBackup) {
    Move-Item -Force -LiteralPath $DllBackup -Destination $DllTarget
    Remove-Item -Force -LiteralPath $DllHashMarker -ErrorAction SilentlyContinue
} else {
    Remove-Item -Force -LiteralPath $DllHashMarker -ErrorAction SilentlyContinue
}
$IniTarget = Join-Path $GameDir "mgs4_ultrawide.ini"
$IniBackup = Join-Path $BackupDir "mgs4_ultrawide.ini.preinstall"
if (Test-Path -LiteralPath $IniBackup) {
    Move-Item -Force -LiteralPath $IniBackup -Destination $IniTarget
} else {
    Remove-Item -Force -ErrorAction SilentlyContinue -LiteralPath $IniTarget
}
if (-not $LauncherConflict -and -not $DllConflict) {
    Remove-Item -Force -Recurse -ErrorAction SilentlyContinue -LiteralPath $BackupDir
}
Clear-Mgs4Ultra120GameDir $GameDir
Write-Host "Uninstalled from: $GameDir"
if ($LauncherConflict) {
    throw "The launcher changed outside MGS4 Ultra120. The launcher and its backup were preserved; setup files must remain installed until this conflict is resolved."
}
if ($DllConflict) {
    throw "winmm.dll changed outside MGS4 Ultra120. It and its backup were preserved; setup files must remain installed until this conflict is resolved."
}
