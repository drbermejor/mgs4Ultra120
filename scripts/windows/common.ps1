$Mgs4Ultra120Version = "v0.3.3-alpha.1"
$Mgs4Ultra120RegistryPath = "HKCU:\Software\MGS4Ultra120"
$Mgs4Ultra120LegacyDllHashes = @(
    # v0.3.1-alpha.1/alpha.2 two-export proxy. Keeping this hash lets an
    # in-place alpha.3 update adopt the old project DLL instead of backing it
    # up as if it belonged to another mod.
    "C3B28D0307EF4DF2E029930168FF691224923A7F84F8CC6DD5522CDF002C7D9B",
    # Final native alpha.3 proxy. Manual alpha.3 installations did not always
    # have an ownership marker, so alpha.4 must still migrate them safely.
    "9BB4022045C63EA94868C3AA4D9DA52D3E37CA56CFC0E859AFF122F6EB818BEB"
)

function Test-UltimateAsiLoader([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $false }
    try {
        $Info = (Get-Item -LiteralPath $Path).VersionInfo
        if ($Info.FileDescription -ne "Ultimate ASI Loader" -or
            $Info.ProductName -notlike "Ultimate-ASI-Loader-x64*") {
            return $false
        }
        $Bytes = [IO.File]::ReadAllBytes($Path)
        if ($Bytes.Length -lt 256 -or $Bytes[0] -ne 0x4d -or
            $Bytes[1] -ne 0x5a) {
            return $false
        }
        $PeOffset = [BitConverter]::ToInt32($Bytes, 0x3c)
        return $PeOffset -ge 0 -and $PeOffset + 6 -le $Bytes.Length -and
            [BitConverter]::ToUInt32($Bytes, $PeOffset) -eq 0x00004550 -and
            [BitConverter]::ToUInt16($Bytes, $PeOffset + 4) -eq 0x8664
    } catch {
        return $false
    }
}

function Get-Mgs4Ultra120ConflictingLoaders([string]$GameDir) {
    $Conflicts = [Collections.Generic.List[string]]::new()
    # A second Ultimate ASI Loader proxy can load every ASI twice. Do not guess
    # from a DLL name alone (ReShade and other tools may legitimately use one
    # of these names); require Ultimate ASI Loader's PE version metadata.
    foreach ($Name in @(
        "d3d8.dll", "d3d9.dll", "d3d11.dll", "dinput8.dll", "dsound.dll",
        "version.dll", "wininet.dll", "winhttp.dll", "xinput1_3.dll",
        "xinput9_1_0.dll"
    )) {
        $Candidate = Join-Path $GameDir $Name
        if (Test-UltimateAsiLoader $Candidate) {
            $Conflicts.Add("$Name (second Ultimate ASI Loader proxy)")
        }
    }

    # Old or unpacked builds can leave a second copy of this patch under a
    # different name/location. Ultimate ASI Loader scans ASIs, so both copies
    # would install the same hooks.
    foreach ($Directory in @($GameDir, (Join-Path $GameDir "scripts"))) {
        if (-not (Test-Path -LiteralPath $Directory -PathType Container)) {
            continue
        }
        foreach ($Candidate in Get-ChildItem -LiteralPath $Directory -File `
                -Filter "*.asi" -ErrorAction SilentlyContinue) {
            $Canonical = [IO.Path]::Combine($GameDir, "scripts",
                "MGS4Ultra120.asi")
            if ($Candidate.FullName.Equals($Canonical,
                    [StringComparison]::OrdinalIgnoreCase)) { continue }
            if ($Candidate.BaseName -match
                    '(?i)(mgs4.*ultra|ultrawide.*mgs4|mgs4ultra120)') {
                $Relative = if ($Directory.Equals($GameDir,
                        [StringComparison]::OrdinalIgnoreCase)) {
                    $Candidate.Name
                } else {
                    "scripts\$($Candidate.Name)"
                }
                $Conflicts.Add("$Relative (possible old MGS4 Ultra120 ASI)")
            }
        }
    }
    return @($Conflicts | Select-Object -Unique)
}

function Test-Mgs4Ultra120Config([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $false }
    $Text = Get-Content -Raw -LiteralPath $Path
    return $Text -match '(?m)^\[Patch\]\s*$' -and
        $Text -match '(?m)^UltrawideEnabled=' -and
        $Text -match '(?m)^Width=' -and
        $Text -match '(?m)^Limit=' -and
        $Text -match '(?m)^SkipUnityLauncher='
}

function Merge-Mgs4Ultra120Config([string]$Template, [string]$Existing,
                                  [string]$Destination) {
    $TemplateText = Get-Content -Raw -LiteralPath $Template
    $ExistingText = Get-Content -Raw -LiteralPath $Existing
    $ExistingFovModel = [regex]::Match($ExistingText,
        '(?m)^FOVModelVersion=(\d+)\s*$')
    $NeedsSingleOwnerFovMigration = -not $ExistingFovModel.Success -or
        [int]$ExistingFovModel.Groups[1].Value -lt 2
    foreach ($Key in @(
        "UltrawideEnabled", "FPSOverrideEnabled", "AllowUnsupportedExecutable",
        "Width", "Height", "FOVMultiplier", "NativeCameraFOV",
        "SupersamplingEnabled",
        "RenderScale", "Limit", "ControllerProfileFixEnabled",
        "SkipUnityLauncher", "Region", "SelfRegion", "Language", "ControllerType",
        "DisplayMode", "UsePrimaryPhysicalResolution"
    )) {
        $ExistingMatch = [regex]::Match($ExistingText,
            "(?m)^$([regex]::Escape($Key))=(.*)$")
        if (-not $ExistingMatch.Success) { continue }
        $Pattern = "(?m)^$([regex]::Escape($Key))=.*$"
        if ([regex]::IsMatch($TemplateText, $Pattern)) {
            $Value = $ExistingMatch.Groups[1].Value.Trim()
            $TemplateText = [regex]::Replace($TemplateText, $Pattern,
                "$Key=$Value", 1)
        }
    }
    # The project ASI must never compete with the complete timing hooks in
    # MGSFPSUnlock. Preserve the legacy keys for compatible INI editing, but
    # force the old single-field override off during every Windows update.
    $TemplateText = [regex]::Replace($TemplateText,
        '(?m)^FPSOverrideEnabled=.*$', 'FPSOverrideEnabled=0', 1)
    $TemplateText = [regex]::Replace($TemplateText,
        '(?m)^Limit=120$', 'Limit=60', 1)
    # NativeCameraFOV is user-selectable in v0.3.3. Preserve an explicit opt-out
    # so a managed update never silently re-enables the experimental route.
    # FOV model 1 applied the multiplier repeatedly in the camera rebuild
    # chain. Model 2 owns it once at route 03. Migrate the old 1.050 default to
    # the visually validated single-owner 1.200 exactly once; after the marker
    # exists, a user may deliberately select 1.050 without it being rewritten.
    $FovMatch = [regex]::Match($TemplateText,
        '(?m)^FOVMultiplier=(.*)$')
    if ($FovMatch.Success) {
        $ParsedFov = 0.0
        $FovText = $FovMatch.Groups[1].Value.Trim().Replace(',', '.')
        if (-not [double]::TryParse($FovText,
                [Globalization.NumberStyles]::Float,
                [Globalization.CultureInfo]::InvariantCulture,
                [ref]$ParsedFov) -or [double]::IsNaN($ParsedFov) -or
                [double]::IsInfinity($ParsedFov) -or $ParsedFov -lt 0.5) {
            $TemplateText = [regex]::Replace($TemplateText,
                '(?m)^FOVMultiplier=.*$', 'FOVMultiplier=1.200', 1)
        } elseif ($NeedsSingleOwnerFovMigration -and
                  [Math]::Abs($ParsedFov - 1.05) -lt 0.0005) {
            $TemplateText = [regex]::Replace($TemplateText,
                '(?m)^FOVMultiplier=.*$', 'FOVMultiplier=1.200', 1)
        }
    }
    $Temporary = "$Destination.tmp"
    [IO.File]::WriteAllText($Temporary, $TemplateText,
        [Text.UTF8Encoding]::new($false))
    Move-Item -Force -LiteralPath $Temporary -Destination $Destination
}

function Save-Mgs4Ultra120GameDir([string]$GameDir) {
    New-Item -Path $Mgs4Ultra120RegistryPath -Force | Out-Null
    Set-ItemProperty -Path $Mgs4Ultra120RegistryPath -Name LastGameDir `
        -Value $GameDir -Type String
}

function Get-Mgs4Ultra120GameDir {
    try {
        return [string](Get-ItemProperty -Path $Mgs4Ultra120RegistryPath `
            -Name LastGameDir -ErrorAction Stop).LastGameDir
    } catch {
        return $null
    }
}

function Clear-Mgs4Ultra120GameDir([string]$GameDir) {
    $Saved = Get-Mgs4Ultra120GameDir
    if ($Saved -and $Saved -eq $GameDir) {
        Remove-ItemProperty -Path $Mgs4Ultra120RegistryPath -Name LastGameDir `
            -ErrorAction SilentlyContinue
    }
}

function Get-Mgs4Ultra120LauncherSettingsPath([string]$GameDir) {
    $InstallDir = Split-Path -Parent $GameDir
    $SaveRoot = Join-Path $InstallDir "mgs4_savedata_win"
    if (-not [IO.Directory]::Exists($SaveRoot)) { return $null }

    try {
        $AccountId = [uint64](Get-ItemProperty `
            -LiteralPath "HKCU:\Software\Valve\Steam\ActiveProcess" `
            -Name ActiveUser -ErrorAction Stop).ActiveUser
        if ($AccountId -ne 0) {
            $SteamId64 = $AccountId + [uint64]76561197960265728
            $ActivePath = [IO.Path]::Combine($SaveRoot, [string]$SteamId64,
                "launcher", "launcher_sv")
            if ([IO.File]::Exists($ActivePath)) { return $ActivePath }
        }
    } catch {}

    $Candidates = @(Get-ChildItem -LiteralPath $SaveRoot -Directory `
        -ErrorAction SilentlyContinue | ForEach-Object {
            $Candidate = [IO.Path]::Combine($_.FullName, "launcher", "launcher_sv")
            if ([IO.File]::Exists($Candidate)) { $Candidate }
        })
    if ($Candidates.Count -eq 1) { return $Candidates[0] }
    return $null
}

function Get-Mgs4Ultra120LauncherSettingsObject([string]$Path) {
    $Data = Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json
    if (-not $Data.keyList -or -not $Data.valueList -or
        $Data.keyList.Count -ne $Data.valueList.Count) {
        throw "The official launcher settings file has an unexpected format: $Path"
    }
    return $Data
}

function Get-Mgs4Ultra120LauncherSettingMap($Data) {
    $Map = @{}
    for ($Index = 0; $Index -lt $Data.keyList.Count; $Index++) {
        $Map[[string]$Data.keyList[$Index]] = $Index
    }
    return $Map
}

function Write-Mgs4Ultra120LauncherSettings([string]$Path, $Data) {
    $Json = $Data | ConvertTo-Json -Compress -Depth 4
    $Temporary = "$Path.mgs4ultra120.tmp"
    [IO.File]::WriteAllText($Temporary, $Json,
        [Text.UTF8Encoding]::new($false))
    Move-Item -Force -LiteralPath $Temporary -Destination $Path
}

function Set-Mgs4Ultra120WindowsDisplaySettings(
    [string]$GameDir, [int]$Width, [int]$Height, [string]$DisplayMode,
    [string]$Language = "en") {
    if ($DisplayMode -notin @("Windowed", "Fullscreen")) {
        throw "Windows display mode must be Windowed or Fullscreen."
    }
    if ($Language -eq "ge") { $Language = "gr" }
    $LauncherLanguage = @{
        jp = "1"; en = "2"; fr = "3"; it = "4"
        gr = "5"; sp = "6"; pt = "7"
    }
    if (-not $LauncherLanguage.ContainsKey($Language)) {
        throw "Unsupported game language: $Language"
    }
    $SettingsPath = Get-Mgs4Ultra120LauncherSettingsPath $GameDir
    if (-not $SettingsPath) {
        if ($DisplayMode -eq "Fullscreen") {
            throw "The active Steam user's launcher settings were not found. Open the official Unity launcher once before enabling exclusive fullscreen."
        }
        Write-Warning "Official launcher settings were not found; the wrapper will still request windowed mode."
        return
    }

    $Data = Get-Mgs4Ultra120LauncherSettingsObject $SettingsPath
    $Map = Get-Mgs4Ultra120LauncherSettingMap $Data
    $Desired = [ordered]@{
        ResolutionFullW = [string]$Width
        ResolutionFullH = [string]$Height
        ResolutionWindowW = [string]$Width
        ResolutionWindowH = [string]$Height
        WindowSizeW = [string]$Width
        WindowSizeH = [string]$Height
        WindowMode = if ($DisplayMode -eq "Fullscreen") { "0" } else { "1" }
        # Def.LANGUAGE in the official IL2CPP launcher: JP=1, EN=2, FR=3,
        # IT=4, German=5, Spanish=6, Portuguese=7.
        prevPlayLanguage = $LauncherLanguage[$Language]
    }
    $HasWindowPair =
        ($Map.ContainsKey("ResolutionWindowW") -and
         $Map.ContainsKey("ResolutionWindowH")) -or
        ($Map.ContainsKey("WindowSizeW") -and
         $Map.ContainsKey("WindowSizeH"))
    $HasFullscreenPair = $Map.ContainsKey("ResolutionFullW") -and
        $Map.ContainsKey("ResolutionFullH")
    $CanSynchronize = $Map.ContainsKey("WindowMode") -and
        $(if ($DisplayMode -eq "Fullscreen") {
            $HasFullscreenPair
        } else {
            $HasWindowPair
        })
    if (-not $CanSynchronize) {
        $Message = "The official launcher settings do not contain the fields required for $DisplayMode mode; no official display values were changed."
        if ($DisplayMode -eq "Fullscreen") { throw $Message }
        Write-Warning "$Message The patch and direct launcher can still request windowed mode; language will be synchronized independently when possible."
    }

    $Applied = [ordered]@{}
    $MissingOptional = [Collections.Generic.List[string]]::new()
    $DisplayKeys = @(
        "ResolutionFullW", "ResolutionFullH", "ResolutionWindowW",
        "ResolutionWindowH", "WindowSizeW", "WindowSizeH", "WindowMode")
    foreach ($Key in $Desired.Keys) {
        if (-not $CanSynchronize -and $Key -in $DisplayKeys) { continue }
        if ($Map.ContainsKey($Key)) {
            $Applied[$Key] = $Desired[$Key]
        } else {
            $MissingOptional.Add($Key)
        }
    }
    if ($MissingOptional.Count -gt 0) {
        Write-Warning ("Official launcher settings omit optional fields: " +
            ($MissingOptional -join ", ") +
            ". All available display values will still be synchronized.")
    }
    if ($Applied.Count -eq 0) { return }

    $BackupDir = Join-Path $GameDir ".mgs4ultra120-backup"
    New-Item -ItemType Directory -Force -Path $BackupDir | Out-Null
    $MetadataPath = Join-Path $BackupDir "windows-launcher-settings.json"
    if (Test-Path -LiteralPath $MetadataPath) {
        $Metadata = Get-Content -Raw -LiteralPath $MetadataPath | ConvertFrom-Json
        foreach ($Key in $Applied.Keys) {
            if (-not $Metadata.Original.PSObject.Properties[$Key]) {
                $Metadata.Original | Add-Member -NotePropertyName $Key `
                    -NotePropertyValue ([string]$Data.valueList[$Map[$Key]])
            }
        }
    } else {
        $Original = [ordered]@{}
        foreach ($Key in $Applied.Keys) {
            $Original[$Key] = [string]$Data.valueList[$Map[$Key]]
        }
        $Metadata = [pscustomobject]@{
            SettingsPath = $SettingsPath
            Original = [pscustomobject]$Original
            Applied = [pscustomobject]@{}
        }
    }

    foreach ($Key in $Applied.Keys) {
        $Data.valueList[$Map[$Key]] = $Applied[$Key]
    }
    Write-Mgs4Ultra120LauncherSettings $SettingsPath $Data
    $Metadata.SettingsPath = $SettingsPath
    $Metadata.Applied = [pscustomobject]$Applied
    [IO.File]::WriteAllText($MetadataPath,
        ($Metadata | ConvertTo-Json -Depth 5),
        [Text.UTF8Encoding]::new($false))
}

function Restore-Mgs4Ultra120WindowsDisplaySettings([string]$GameDir) {
    $MetadataPath = Join-Path (Join-Path $GameDir ".mgs4ultra120-backup") `
        "windows-launcher-settings.json"
    if (-not (Test-Path -LiteralPath $MetadataPath)) { return }
    $Metadata = Get-Content -Raw -LiteralPath $MetadataPath | ConvertFrom-Json
    $SettingsPath = [string]$Metadata.SettingsPath
    if (-not [IO.File]::Exists($SettingsPath)) {
        Write-Warning "Official launcher settings moved or disappeared; display-setting metadata was preserved."
        return
    }
    $Data = Get-Mgs4Ultra120LauncherSettingsObject $SettingsPath
    $Map = Get-Mgs4Ultra120LauncherSettingMap $Data
    foreach ($Property in $Metadata.Original.PSObject.Properties) {
        $Key = $Property.Name
        if (-not $Map.ContainsKey($Key)) { continue }
        $AppliedValue = [string]$Metadata.Applied.$Key
        if ([string]$Data.valueList[$Map[$Key]] -eq $AppliedValue) {
            $Data.valueList[$Map[$Key]] = [string]$Property.Value
        } else {
            Write-Warning "Preserving a launcher setting changed after MGS4 Ultra120: $Key"
        }
    }
    Write-Mgs4Ultra120LauncherSettings $SettingsPath $Data
    Remove-Item -Force -LiteralPath $MetadataPath
}

function Install-Mgs4Ultra120Patch([string]$GameDir, [string]$PackageDir) {
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
        throw "Exit the game before installing or updating the patch."
    }
    if (-not $GameDir -or -not [IO.Directory]::Exists($GameDir)) {
        throw "The selected MGS4 folder is unavailable. Reconnect its drive or choose a different folder."
    }
    if (-not [IO.File]::Exists([IO.Path]::Combine($GameDir, "mgs4.exe"))) {
        throw "mgs4.exe was not found in the selected MGS4 folder."
    }

    $LoaderConflicts = @(Get-Mgs4Ultra120ConflictingLoaders $GameDir)
    if ($LoaderConflicts.Count -gt 0) {
        throw ("Conflicting old/duplicate loader files were found:`n - " +
            ($LoaderConflicts -join "`n - ") +
            "`n`nRemove old MGS4 Ultra120 builds and alternative ASI-loader copies, then run setup again. Do not delete unrelated DLLs unless you know what installed them.")
    }

    $BackupDir = Join-Path $GameDir ".mgs4ultra120-backup"
    New-Item -ItemType Directory -Force -Path $BackupDir | Out-Null

    $DllDestination = Join-Path $GameDir "winmm.dll"
    $DllBackup = Join-Path $BackupDir "winmm.dll.preinstall"
    $DllSource = Join-Path $PackageDir "bin\winmm.dll"
    $DllHashMarker = Join-Path $BackupDir "winmm-installed.sha256"
    $DllReuseMarker = Join-Path $BackupDir "winmm-reused.sha256"
    $SourceHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $DllSource).Hash
    $InstallBundledLoader = $true
    if (Test-Path -LiteralPath $DllDestination) {
        $TargetHash = (Get-FileHash -Algorithm SHA256 `
            -LiteralPath $DllDestination).Hash
        $RecordedHash = $null
        if (Test-Path -LiteralPath $DllHashMarker) {
            $Candidate = (Get-Content -Raw -LiteralPath $DllHashMarker).Trim()
            if ($Candidate -match '^[0-9a-fA-F]{64}$') { $RecordedHash = $Candidate }
        }
        $Managed = $TargetHash -eq $SourceHash -or
            ($RecordedHash -and $TargetHash -eq $RecordedHash) -or
            $TargetHash -in $Mgs4Ultra120LegacyDllHashes
        if (-not $Managed -and (Test-UltimateAsiLoader $DllDestination)) {
            # Cooperate with a loader installed by another mod. Record the
            # exact reused file so uninstall can leave it untouched.
            $InstallBundledLoader = $false
            [IO.File]::WriteAllText($DllReuseMarker, $TargetHash,
                [Text.Encoding]::ASCII)
            Remove-Item -Force -LiteralPath $DllHashMarker `
                -ErrorAction SilentlyContinue
        } elseif (-not $Managed -and -not (Test-Path -LiteralPath $DllBackup)) {
            Copy-Item -LiteralPath $DllDestination -Destination $DllBackup
        } elseif (-not $Managed) {
            $Timestamp = [DateTime]::UtcNow.ToString("yyyyMMddTHHmmssZ")
            Copy-Item -LiteralPath $DllDestination -Destination `
                (Join-Path $BackupDir "winmm.dll.displaced.$Timestamp")
        }
    }
    if ($InstallBundledLoader) {
        Copy-Item -Force -LiteralPath $DllSource -Destination $DllDestination
        [IO.File]::WriteAllText($DllHashMarker, $SourceHash,
            [Text.Encoding]::ASCII)
        Remove-Item -Force -LiteralPath $DllReuseMarker `
            -ErrorAction SilentlyContinue
    }

    $ScriptsDir = Join-Path $GameDir "scripts"
    New-Item -ItemType Directory -Force -Path $ScriptsDir | Out-Null
    $AsiDestination = Join-Path $ScriptsDir "MGS4Ultra120.asi"
    $AsiSource = Join-Path $PackageDir "bin\MGS4Ultra120.asi"
    $AsiBackup = Join-Path $BackupDir "MGS4Ultra120.asi.preinstall"
    $AsiHashMarker = Join-Path $BackupDir "asi-installed.sha256"
    $AsiSourceHash = (Get-FileHash -Algorithm SHA256 `
        -LiteralPath $AsiSource).Hash
    if (Test-Path -LiteralPath $AsiDestination) {
        $AsiTargetHash = (Get-FileHash -Algorithm SHA256 `
            -LiteralPath $AsiDestination).Hash
        $AsiRecordedHash = $null
        if (Test-Path -LiteralPath $AsiHashMarker) {
            $Candidate = (Get-Content -Raw -LiteralPath $AsiHashMarker).Trim()
            if ($Candidate -match '^[0-9a-fA-F]{64}$') {
                $AsiRecordedHash = $Candidate
            }
        }
        $AsiManaged = $AsiTargetHash -eq $AsiSourceHash -or
            ($AsiRecordedHash -and $AsiTargetHash -eq $AsiRecordedHash)
        if (-not $AsiManaged -and -not (Test-Path -LiteralPath $AsiBackup)) {
            Copy-Item -LiteralPath $AsiDestination -Destination $AsiBackup
        } elseif (-not $AsiManaged) {
            $Timestamp = [DateTime]::UtcNow.ToString("yyyyMMddTHHmmssZ")
            Copy-Item -LiteralPath $AsiDestination -Destination `
                (Join-Path $BackupDir "MGS4Ultra120.asi.displaced.$Timestamp")
        }
    }
    Copy-Item -Force -LiteralPath $AsiSource -Destination $AsiDestination
    [IO.File]::WriteAllText($AsiHashMarker, $AsiSourceHash,
        [Text.Encoding]::ASCII)

    $IniDestination = Join-Path $GameDir "mgs4_ultrawide.ini"
    $IniTemplate = Join-Path $PackageDir "config\mgs4_ultrawide.ini"
    $IniBackup = Join-Path $BackupDir "mgs4_ultrawide.ini.preinstall"
    if (Test-Mgs4Ultra120Config $IniDestination) {
        Merge-Mgs4Ultra120Config $IniTemplate $IniDestination $IniDestination
    } else {
        if ((Test-Path -LiteralPath $IniDestination) -and
            -not (Test-Path -LiteralPath $IniBackup)) {
            Copy-Item -LiteralPath $IniDestination -Destination $IniBackup
        }
        Copy-Item -Force -LiteralPath $IniTemplate -Destination $IniDestination
    }

    Save-Mgs4Ultra120GameDir $GameDir
}
