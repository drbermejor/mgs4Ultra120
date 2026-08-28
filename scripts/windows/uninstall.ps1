param(
    [string]$GameDir = "${env:ProgramFiles(x86)}\Steam\steamapps\common\METAL GEAR SOLID 4\MGS4"
)
$ErrorActionPreference = "Stop"
if (-not $GameDir -or -not [IO.Directory]::Exists($GameDir)) {
    throw "The selected MGS4 folder is unavailable. Reconnect its drive or choose it again from Easy setup."
}
$PackageDir = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
. (Join-Path $PSScriptRoot "common.ps1")
. (Join-Path $PSScriptRoot "mgsfpsunlock.ps1")
$BackupDir = Join-Path $GameDir ".mgs4ultra120-backup"
$InstallDir = Split-Path -Parent $GameDir
$LauncherTarget = Join-Path (Join-Path $InstallDir "Launcher") "launcher.exe"
$LauncherBackup = Join-Path $BackupDir "launcher.exe.preinstall"
$LauncherHashMarker = Join-Path $BackupDir "launcher-wrapper-installed.sha256"
$WrapperSource = Join-Path $PackageDir "bin\launcher.exe"
$PackageTestTarget = [string]$env:MGS4ULTRA120_PACKAGE_TEST_GAME_DIR
$IsIsolatedPackageTest = $PackageTestTarget -and
    [IO.Path]::GetFullPath($PackageTestTarget).Equals(
        [IO.Path]::GetFullPath($GameDir),
        [StringComparison]::OrdinalIgnoreCase) -and
    [IO.Path]::GetFullPath($GameDir).StartsWith(
        [IO.Path]::GetFullPath([IO.Path]::GetTempPath()),
        [StringComparison]::OrdinalIgnoreCase)
if (-not $IsIsolatedPackageTest -and
    (Get-Process mgs4 -ErrorAction SilentlyContinue)) {
    throw "Exit the game before uninstalling."
}
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
$AsiConflict = $false
$AsiTarget = Join-Path (Join-Path $GameDir "scripts") "MGS4Ultra120.asi"
$AsiBackup = Join-Path $BackupDir "MGS4Ultra120.asi.preinstall"
$AsiHashMarker = Join-Path $BackupDir "asi-installed.sha256"
if (Test-Path -LiteralPath $AsiTarget) {
    $BundledAsi = Join-Path $PackageDir "bin\MGS4Ultra120.asi"
    $ManagedHashes = [Collections.Generic.HashSet[string]]::new(
        [StringComparer]::OrdinalIgnoreCase)
    if (Test-Path -LiteralPath $BundledAsi) {
        [void]$ManagedHashes.Add((Get-FileHash -Algorithm SHA256 `
            -LiteralPath $BundledAsi).Hash)
    }
    if (Test-Path -LiteralPath $AsiHashMarker) {
        $RecordedHash = (Get-Content -Raw -LiteralPath $AsiHashMarker).Trim()
        if ($RecordedHash -match '^[0-9a-fA-F]{64}$') {
            [void]$ManagedHashes.Add($RecordedHash)
        }
    }
    if ($ManagedHashes.Contains((Get-FileHash -Algorithm SHA256 `
        -LiteralPath $AsiTarget).Hash)) {
        if (Test-Path -LiteralPath $AsiBackup) {
            Move-Item -Force -LiteralPath $AsiBackup -Destination $AsiTarget
        } else {
            Remove-Item -Force -LiteralPath $AsiTarget
        }
        Remove-Item -Force -LiteralPath $AsiHashMarker `
            -ErrorAction SilentlyContinue
    } else {
        $AsiConflict = $true
        Write-Warning "MGS4Ultra120.asi changed outside setup; preserving it and its backup."
    }
} elseif (Test-Path -LiteralPath $AsiBackup) {
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $AsiTarget) |
        Out-Null
    Move-Item -Force -LiteralPath $AsiBackup -Destination $AsiTarget
    Remove-Item -Force -LiteralPath $AsiHashMarker -ErrorAction SilentlyContinue
} else {
    Remove-Item -Force -LiteralPath $AsiHashMarker -ErrorAction SilentlyContinue
}
$MgsFpsUnlockConflict = Remove-MgsFpsUnlock $GameDir
$ScriptsDir = Join-Path $GameDir "scripts"
if ((Test-Path -LiteralPath $ScriptsDir -PathType Container) -and
    -not (Get-ChildItem -Force -LiteralPath $ScriptsDir | Select-Object -First 1)) {
    Remove-Item -Force -LiteralPath $ScriptsDir
}

$DllConflict = $false
$DllTarget = Join-Path $GameDir "winmm.dll"
$DllBackup = Join-Path $BackupDir "winmm.dll.preinstall"
$DllHashMarker = Join-Path $BackupDir "winmm-installed.sha256"
$DllReuseMarker = Join-Path $BackupDir "winmm-reused.sha256"
$ReusedLoader = Test-Path -LiteralPath $DllReuseMarker
if ($ReusedLoader) {
    # The loader belonged to another mod before this installation. Never
    # remove or restore over it, even if that other mod updated it meanwhile.
    if (Test-Path -LiteralPath $DllBackup) {
        $DllConflict = $true
        Write-Warning "A reused ASI loader is active while an older winmm.dll backup still exists; preserving both and the setup files."
    }
    Remove-Item -Force -LiteralPath $DllReuseMarker -ErrorAction SilentlyContinue
    Remove-Item -Force -LiteralPath $DllHashMarker -ErrorAction SilentlyContinue
} elseif (Test-Path -LiteralPath $DllTarget) {
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
if (-not $LauncherConflict -and -not $DllConflict -and -not $AsiConflict -and
    -not $MgsFpsUnlockConflict) {
    Remove-Item -Force -Recurse -ErrorAction SilentlyContinue -LiteralPath $BackupDir
}
Clear-Mgs4Ultra120GameDir $GameDir
Write-Host "Uninstalled from: $GameDir"
if ($LauncherConflict) {
    throw "The launcher changed outside MGS4 Ultra120. The launcher and its backup were preserved; setup files must remain installed until this conflict is resolved."
}
if ($DllConflict) {
    throw "winmm.dll ownership changed outside MGS4 Ultra120. Active and backed-up files were preserved; setup files must remain installed until this conflict is resolved."
}
if ($AsiConflict) {
    throw "MGS4Ultra120.asi changed outside MGS4 Ultra120. It and its backup were preserved; setup files must remain installed until this conflict is resolved."
}
if ($MgsFpsUnlockConflict) {
    throw "MGSFPSUnlock.asi changed outside MGS4 Ultra120. It and its backup were preserved; setup files must remain installed until this conflict is resolved."
}
