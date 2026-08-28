$script:MgsFpsUnlockVersion = "0.1.0"
$script:MgsFpsUnlockReleaseUrl =
    "https://github.com/cipherxof/MGSFPSUnlock/releases/tag/0.1.0"
$script:MgsFpsUnlockArchiveUrl =
    "https://github.com/cipherxof/MGSFPSUnlock/releases/download/0.1.0/MGSFPSUnlock.zip"
$script:MgsFpsUnlockArchiveSha256 =
    "F5DCA70B095DD7EA9A6F181677BC37F35A97FFA068EE6FC6B9B269407CDE4D8A"
$script:MgsFpsUnlockAsiSha256 =
    "9DA6F4BF1478E78DD94627EF0B1BD8255E0D3CB1CF343464D9951775B0674679"

function Get-MgsFpsUnlockPaths([string]$GameDir) {
    $ScriptsDir = Join-Path $GameDir "scripts"
    return [pscustomobject]@{
        ScriptsDir = $ScriptsDir
        Asi = Join-Path $ScriptsDir "MGSFPSUnlock.asi"
        Ini = Join-Path $ScriptsDir "MGSFPSUnlock.ini"
        BackupDir = Join-Path $GameDir ".mgs4ultra120-backup"
    }
}

function Test-MgsFpsUnlockInstalled([string]$GameDir) {
    $Paths = Get-MgsFpsUnlockPaths $GameDir
    return [IO.File]::Exists($Paths.Asi) -and [IO.File]::Exists($Paths.Ini)
}

function Get-MgsFpsUnlockTarget([string]$GameDir) {
    $Paths = Get-MgsFpsUnlockPaths $GameDir
    if (-not [IO.File]::Exists($Paths.Ini)) { return $null }
    $Match = [regex]::Match((Get-Content -Raw -LiteralPath $Paths.Ini),
        '(?m)^\s*TargetFrameRate\s*=\s*(\d+)\s*$')
    if (-not $Match.Success) { return $null }
    return [int]$Match.Groups[1].Value
}

function Set-MgsFpsUnlockTarget([string]$GameDir, [int]$TargetFrameRate) {
    if ($TargetFrameRate -notin @(30, 60, 120)) {
        throw "MGSFPSUnlock target frame rate must be 30, 60 or 120."
    }
    $Paths = Get-MgsFpsUnlockPaths $GameDir
    if (-not [IO.File]::Exists($Paths.Ini)) {
        throw "MGSFPSUnlock.ini is missing. Install the improved FPS component first."
    }
    $Text = Get-Content -Raw -LiteralPath $Paths.Ini
    if ($Text -notmatch '(?m)^\s*TargetFrameRate\s*=') {
        throw "MGSFPSUnlock.ini has an unexpected format."
    }
    $Text = [regex]::Replace($Text,
        '(?m)^\s*TargetFrameRate\s*=.*$',
        "TargetFrameRate = $TargetFrameRate", 1)
    $Temporary = "$($Paths.Ini).mgs4ultra120.tmp"
    [IO.File]::WriteAllText($Temporary, $Text,
        [Text.UTF8Encoding]::new($false))
    Move-Item -Force -LiteralPath $Temporary -Destination $Paths.Ini
}

function Install-MgsFpsUnlock(
    [string]$GameDir,
    [string]$ArchivePath = ""
) {
    if (Get-Process mgs4 -ErrorAction SilentlyContinue) {
        throw "Exit the game before installing or updating MGSFPSUnlock."
    }
    if (-not [IO.File]::Exists([IO.Path]::Combine($GameDir, "mgs4.exe"))) {
        throw "mgs4.exe was not found in the selected MGS4 folder."
    }

    $WorkDir = Join-Path ([IO.Path]::GetTempPath()) (
        "mgsfpsunlock-" + [Guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Path $WorkDir | Out-Null
    try {
        $Archive = Join-Path $WorkDir "MGSFPSUnlock.zip"
        if ($ArchivePath) {
            Copy-Item -LiteralPath $ArchivePath -Destination $Archive
        } else {
            Write-Host "Downloading MGSFPSUnlock $script:MgsFpsUnlockVersion from its official GitHub release..."
            Invoke-WebRequest -UseBasicParsing -Uri $script:MgsFpsUnlockArchiveUrl `
                -OutFile $Archive
        }
        $ArchiveHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Archive).Hash
        if ($ArchiveHash -ne $script:MgsFpsUnlockArchiveSha256) {
            throw "MGSFPSUnlock download failed SHA-256 verification. Expected $script:MgsFpsUnlockArchiveSha256 but received $ArchiveHash. Do not use this file."
        }

        $Expanded = Join-Path $WorkDir "expanded"
        Expand-Archive -LiteralPath $Archive -DestinationPath $Expanded
        $PayloadRoot = Join-Path $Expanded "MGSFPSUnlock\scripts"
        $AsiSource = Join-Path $PayloadRoot "MGSFPSUnlock.asi"
        $IniSource = Join-Path $PayloadRoot "MGSFPSUnlock.ini"
        if (-not [IO.File]::Exists($AsiSource) -or
            -not [IO.File]::Exists($IniSource)) {
            throw "The verified MGSFPSUnlock archive does not contain its expected ASI and INI files."
        }
        $AsiHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $AsiSource).Hash
        if ($AsiHash -ne $script:MgsFpsUnlockAsiSha256) {
            throw "The MGSFPSUnlock ASI inside the verified archive has an unexpected hash."
        }

        $Paths = Get-MgsFpsUnlockPaths $GameDir
        New-Item -ItemType Directory -Force -Path $Paths.ScriptsDir | Out-Null
        New-Item -ItemType Directory -Force -Path $Paths.BackupDir | Out-Null
        $AsiBackup = Join-Path $Paths.BackupDir "MGSFPSUnlock.asi.preinstall"
        $AsiMarker = Join-Path $Paths.BackupDir "mgsfpsunlock-asi-installed.sha256"
        $AsiReuseMarker = Join-Path $Paths.BackupDir "mgsfpsunlock-asi-reused.sha256"

        if ([IO.File]::Exists($Paths.Asi)) {
            $TargetHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Paths.Asi).Hash
            $RecordedHash = ""
            if ([IO.File]::Exists($AsiMarker)) {
                $RecordedHash = (Get-Content -Raw -LiteralPath $AsiMarker).Trim()
            }
            $Managed = $RecordedHash -match '^[0-9a-fA-F]{64}$' -and
                $TargetHash -eq $RecordedHash
            if (-not $Managed -and $TargetHash -eq $AsiHash) {
                [IO.File]::WriteAllText($AsiReuseMarker, $TargetHash,
                    [Text.Encoding]::ASCII)
                Remove-Item -Force -LiteralPath $AsiMarker `
                    -ErrorAction SilentlyContinue
            } else {
                if (-not $Managed -and -not [IO.File]::Exists($AsiBackup)) {
                    Copy-Item -LiteralPath $Paths.Asi -Destination $AsiBackup
                } elseif (-not $Managed) {
                    $Timestamp = [DateTime]::UtcNow.ToString("yyyyMMddTHHmmssZ")
                    $Displaced = Join-Path $Paths.BackupDir `
                        "MGSFPSUnlock.asi.displaced.$Timestamp"
                    Copy-Item -LiteralPath $Paths.Asi -Destination $Displaced
                }
                Copy-Item -Force -LiteralPath $AsiSource -Destination $Paths.Asi
                [IO.File]::WriteAllText($AsiMarker, $AsiHash,
                    [Text.Encoding]::ASCII)
                Remove-Item -Force -LiteralPath $AsiReuseMarker `
                    -ErrorAction SilentlyContinue
            }
        } else {
            Copy-Item -LiteralPath $AsiSource -Destination $Paths.Asi
            [IO.File]::WriteAllText($AsiMarker, $AsiHash,
                [Text.Encoding]::ASCII)
            Remove-Item -Force -LiteralPath $AsiReuseMarker `
                -ErrorAction SilentlyContinue
        }

        $IniBackup = Join-Path $Paths.BackupDir "MGSFPSUnlock.ini.preinstall"
        $IniCreatedMarker = Join-Path $Paths.BackupDir "mgsfpsunlock-ini-created"
        if ([IO.File]::Exists($Paths.Ini)) {
            if (-not [IO.File]::Exists($IniBackup) -and
                -not [IO.File]::Exists($IniCreatedMarker)) {
                Copy-Item -LiteralPath $Paths.Ini -Destination $IniBackup
            }
        } else {
            Copy-Item -LiteralPath $IniSource -Destination $Paths.Ini
            [IO.File]::WriteAllText($IniCreatedMarker,
                $script:MgsFpsUnlockVersion, [Text.Encoding]::ASCII)
        }
        Set-MgsFpsUnlockTarget $GameDir 120
        Write-Host "Installed MGSFPSUnlock $script:MgsFpsUnlockVersion at 120 FPS."
    } catch {
        throw "Improved 120 FPS installation failed. $($_.Exception.Message) Official release: $script:MgsFpsUnlockReleaseUrl"
    } finally {
        $ResolvedTemp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
        $ResolvedWork = [IO.Path]::GetFullPath($WorkDir)
        if ($ResolvedWork.StartsWith($ResolvedTemp,
                [StringComparison]::OrdinalIgnoreCase) -and
            (Split-Path -Leaf $ResolvedWork).StartsWith("mgsfpsunlock-")) {
            Remove-Item -LiteralPath $ResolvedWork -Recurse -Force `
                -ErrorAction SilentlyContinue
        }
    }
}

function Remove-MgsFpsUnlock([string]$GameDir) {
    $Paths = Get-MgsFpsUnlockPaths $GameDir
    $AsiBackup = Join-Path $Paths.BackupDir "MGSFPSUnlock.asi.preinstall"
    $AsiMarker = Join-Path $Paths.BackupDir "mgsfpsunlock-asi-installed.sha256"
    $AsiReuseMarker = Join-Path $Paths.BackupDir "mgsfpsunlock-asi-reused.sha256"
    $Conflict = $false

    if ([IO.File]::Exists($AsiReuseMarker)) {
        Remove-Item -Force -LiteralPath $AsiReuseMarker
    } elseif ([IO.File]::Exists($Paths.Asi)) {
        $RecordedHash = ""
        if ([IO.File]::Exists($AsiMarker)) {
            $RecordedHash = (Get-Content -Raw -LiteralPath $AsiMarker).Trim()
        }
        $TargetHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Paths.Asi).Hash
        if ($RecordedHash -match '^[0-9a-fA-F]{64}$' -and
            $TargetHash -eq $RecordedHash) {
            if ([IO.File]::Exists($AsiBackup)) {
                Move-Item -Force -LiteralPath $AsiBackup -Destination $Paths.Asi
            } else {
                Remove-Item -Force -LiteralPath $Paths.Asi
            }
            Remove-Item -Force -LiteralPath $AsiMarker `
                -ErrorAction SilentlyContinue
        } else {
            $Conflict = $true
            Write-Warning "MGSFPSUnlock.asi changed outside MGS4 Ultra120; preserving it and its backup."
        }
    } elseif ([IO.File]::Exists($AsiBackup)) {
        New-Item -ItemType Directory -Force -Path $Paths.ScriptsDir | Out-Null
        Move-Item -Force -LiteralPath $AsiBackup -Destination $Paths.Asi
        Remove-Item -Force -LiteralPath $AsiMarker -ErrorAction SilentlyContinue
    } else {
        Remove-Item -Force -LiteralPath $AsiMarker -ErrorAction SilentlyContinue
    }

    $IniBackup = Join-Path $Paths.BackupDir "MGSFPSUnlock.ini.preinstall"
    $IniCreatedMarker = Join-Path $Paths.BackupDir "mgsfpsunlock-ini-created"
    if ([IO.File]::Exists($IniBackup)) {
        Move-Item -Force -LiteralPath $IniBackup -Destination $Paths.Ini
    } elseif ([IO.File]::Exists($IniCreatedMarker)) {
        Remove-Item -Force -LiteralPath $Paths.Ini -ErrorAction SilentlyContinue
    }
    Remove-Item -Force -LiteralPath $IniCreatedMarker -ErrorAction SilentlyContinue
    return $Conflict
}
