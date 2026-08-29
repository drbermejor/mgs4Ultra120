param(
    [Parameter(Mandatory)]
    [string]$PackageDir,

    [switch]$Portable
)

$ErrorActionPreference = "Stop"
$PackageDir = (Resolve-Path -LiteralPath $PackageDir).Path
$RequiredFiles = @(
    "VERSION",
    "MGS4Ultra120-Setup.cmd",
    "bin\winmm.dll",
    "bin\MGS4Ultra120.asi",
    "bin\MGS4CenteredHUD16x9.asi",
    "bin\launcher.exe",
    "config\mgs4_ultrawide.ini",
    "config\mgs4_centered_hud_16x9.ini",
    "scripts\windows\setup.ps1",
    "scripts\windows\common.ps1",
    "scripts\windows\install.ps1",
    "scripts\windows\configure.ps1",
    "scripts\windows\uninstall.ps1",
    "scripts\windows\uninstall-installed-package.ps1",
    "scripts\windows\mgsfpsunlock.ps1",
    "third_party\ultimate_asi_loader\LICENSE.txt",
    "third_party\ultimate_asi_loader\README.md"
)
if (-not $Portable) {
    $RequiredFiles += @(
        "Manual-Install\winmm.dll",
        "Manual-Install\scripts\MGS4Ultra120.asi",
        "Manual-Install\scripts\MGS4CenteredHUD16x9.asi",
        "Manual-Install\mgs4_ultrawide.ini",
        "Manual-Install\mgs4_centered_hud_16x9.ini",
        "Manual-Install\README.txt"
    )
}

foreach ($RelativePath in $RequiredFiles) {
    $Path = Join-Path $PackageDir $RelativePath
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Required package file is missing: $RelativePath"
    }
}

$VersionPath = Join-Path $PackageDir "VERSION"
$PackageVersion = (Get-Content -Raw -LiteralPath $VersionPath).Trim()
if ($PackageVersion -ne "v0.3.4-alpha.4") {
    throw "Package VERSION is incorrect: $PackageVersion"
}

$ExpectedLoaderHash =
    "031A3E5576D91DCE1E438D36B9A3D462C7334AB4791990A8FF1E3DDC0E132DAF"
$LoaderPath = Join-Path $PackageDir "bin\winmm.dll"
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $LoaderPath).Hash -ne
    $ExpectedLoaderHash) {
    throw "The package does not contain the pinned Ultimate ASI Loader v9.7.4."
}
$LoaderInfo = (Get-Item -LiteralPath $LoaderPath).VersionInfo
if ($LoaderInfo.FileDescription -ne "Ultimate ASI Loader" -or
    $LoaderInfo.ProductVersion -notlike "9.7.4*") {
    throw "Ultimate ASI Loader version metadata is incorrect."
}
foreach ($RelativePath in @(
    "bin\winmm.dll", "bin\MGS4Ultra120.asi",
    "bin\MGS4CenteredHUD16x9.asi"
)) {
    $Bytes = [IO.File]::ReadAllBytes((Join-Path $PackageDir $RelativePath))
    if ($Bytes.Length -lt 256 -or $Bytes[0] -ne 0x4d -or $Bytes[1] -ne 0x5a) {
        throw "$RelativePath is not a valid PE image."
    }
    $PeOffset = [BitConverter]::ToInt32($Bytes, 0x3c)
    if ($PeOffset -lt 0 -or $PeOffset + 6 -gt $Bytes.Length -or
        [BitConverter]::ToUInt32($Bytes, $PeOffset) -ne 0x00004550 -or
        [BitConverter]::ToUInt16($Bytes, $PeOffset + 4) -ne 0x8664) {
        throw "$RelativePath is not an x86-64 PE image."
    }
}
$AsiVersionPath = Join-Path $PackageDir "bin\MGS4Ultra120.asi"
$HudAsiVersionPath = Join-Path $PackageDir "bin\MGS4CenteredHUD16x9.asi"
$WrapperVersionPath = Join-Path $PackageDir "bin\launcher.exe"
$AsiVersionInfo = (Get-Item -LiteralPath $AsiVersionPath).VersionInfo
$HudAsiVersionInfo = (Get-Item -LiteralPath $HudAsiVersionPath).VersionInfo
$WrapperVersionInfo = (Get-Item -LiteralPath $WrapperVersionPath).VersionInfo
foreach ($VersionInfo in @($AsiVersionInfo, $HudAsiVersionInfo,
        $WrapperVersionInfo)) {
    if ($VersionInfo.FileVersion -ne $PackageVersion -or
        $VersionInfo.ProductVersion -ne $PackageVersion) {
        throw "Embedded binary version does not match VERSION."
    }
}
if ($HudAsiVersionInfo.OriginalFilename -ne "MGS4CenteredHUD16x9.asi" -or
    $HudAsiVersionInfo.FileDescription -ne
        "MGS4 Ultra120 experimental centered HUD") {
    throw "Centered-HUD binary identity metadata is incorrect."
}
if (-not $Portable) {
    foreach ($Pair in @(
        @("bin\winmm.dll", "Manual-Install\winmm.dll"),
        @("bin\MGS4Ultra120.asi", "Manual-Install\scripts\MGS4Ultra120.asi"),
        @("bin\MGS4CenteredHUD16x9.asi", "Manual-Install\scripts\MGS4CenteredHUD16x9.asi"),
        @("config\mgs4_ultrawide.ini", "Manual-Install\mgs4_ultrawide.ini"),
        @("config\mgs4_centered_hud_16x9.ini", "Manual-Install\mgs4_centered_hud_16x9.ini")
    )) {
        $SourceHash = (Get-FileHash -Algorithm SHA256 `
            -LiteralPath (Join-Path $PackageDir $Pair[0])).Hash
        $ManualHash = (Get-FileHash -Algorithm SHA256 `
            -LiteralPath (Join-Path $PackageDir $Pair[1])).Hash
        if ($SourceHash -ne $ManualHash) {
            throw "Manual-install payload does not match $($Pair[0])."
        }
    }
}

foreach ($RelativePath in @(
    "scripts\windows\setup.ps1",
    "scripts\windows\common.ps1",
    "scripts\windows\install.ps1",
    "scripts\windows\configure.ps1",
    "scripts\windows\uninstall.ps1",
    "scripts\windows\uninstall-installed-package.ps1",
    "scripts\windows\mgsfpsunlock.ps1"
)) {
    $Tokens = $null
    $Errors = $null
    [void][Management.Automation.Language.Parser]::ParseFile(
        (Join-Path $PackageDir $RelativePath), [ref]$Tokens, [ref]$Errors)
    if ($Errors.Count -ne 0) {
        throw "PowerShell syntax error in ${RelativePath}: $($Errors[0].Message)"
    }
}

$SmokeRoot = Join-Path ([IO.Path]::GetTempPath()) (
    "mgs4ultra120-smoke-" + [Guid]::NewGuid().ToString("N"))
$GameDir = Join-Path $SmokeRoot "steamapps\common\METAL GEAR SOLID 4\MGS4"
$LauncherDir = Join-Path $SmokeRoot "steamapps\common\METAL GEAR SOLID 4\Launcher"
$LauncherSettingsDir = Join-Path $SmokeRoot `
    "steamapps\common\METAL GEAR SOLID 4\mgs4_savedata_win\123\launcher"
$PreexistingDll = [Text.Encoding]::UTF8.GetBytes("pre-existing winmm test file")
$PreexistingAsi = [Text.Encoding]::UTF8.GetBytes("pre-existing ASI test file")
$PreexistingIni = [Text.Encoding]::UTF8.GetBytes("pre-existing ini test file")
$PreexistingLauncher = [Text.Encoding]::UTF8.GetBytes("pre-existing launcher test file")

try {
    $PreviousPackageTestTarget = $env:MGS4ULTRA120_PACKAGE_TEST_GAME_DIR
    $env:MGS4ULTRA120_PACKAGE_TEST_GAME_DIR = $GameDir
    New-Item -ItemType Directory -Path $GameDir -Force | Out-Null
    New-Item -ItemType Directory -Path $LauncherDir -Force | Out-Null
    New-Item -ItemType Directory -Path $LauncherSettingsDir -Force | Out-Null
    [IO.File]::WriteAllBytes((Join-Path $GameDir "mgs4.exe"),
        [Text.Encoding]::UTF8.GetBytes("MGS4 Ultra120 smoke-test placeholder"))
    [IO.File]::WriteAllBytes((Join-Path $GameDir "winmm.dll"), $PreexistingDll)
    New-Item -ItemType Directory -Path (Join-Path $GameDir "scripts") `
        -Force | Out-Null
    [IO.File]::WriteAllBytes(
        (Join-Path (Join-Path $GameDir "scripts") "MGS4Ultra120.asi"),
        $PreexistingAsi)
    [IO.File]::WriteAllBytes((Join-Path $GameDir "mgs4_ultrawide.ini"), $PreexistingIni)
    [IO.File]::WriteAllBytes((Join-Path $LauncherDir "launcher.exe"),
        $PreexistingLauncher)
    $LauncherSettingsPath = Join-Path $LauncherSettingsDir "launcher_sv"
    $OriginalLauncherValues = [ordered]@{
        ResolutionFullW = "2560"; ResolutionFullH = "1440"
        ResolutionWindowW = "1280"; ResolutionWindowH = "720"
        WindowSizeW = "1280"; WindowSizeH = "720"; WindowMode = "0"
        prevPlayLanguage = "5"
    }
    $LauncherSettings = [pscustomobject]@{
        keyList = @($OriginalLauncherValues.Keys) + @("UnrelatedSetting")
        valueList = @($OriginalLauncherValues.Values) + @("keep-me")
    }
    [IO.File]::WriteAllText($LauncherSettingsPath,
        ($LauncherSettings | ConvertTo-Json -Compress),
        [Text.UTF8Encoding]::new($false))

    # A second loader would enumerate the same scripts and can initialize the
    # patch twice. Setup must stop before mutating the installation.
    $DuplicateLoader = Join-Path $GameDir "dinput8.dll"
    Copy-Item -LiteralPath $LoaderPath -Destination $DuplicateLoader
    $ConflictBlocked = $false
    try {
        & (Join-Path $PackageDir "scripts\windows\install.ps1") -GameDir $GameDir
    } catch {
        $ConflictBlocked = $_.Exception.Message -match
            'Conflicting old/duplicate loader files'
    }
    Remove-Item -Force -LiteralPath $DuplicateLoader
    if (-not $ConflictBlocked) {
        throw "Setup did not block a duplicate Ultimate ASI Loader proxy."
    }

    $OldRenamedAsi = Join-Path $GameDir "scripts\MGS4Ultra120-old.asi"
    Copy-Item -LiteralPath $AsiVersionPath -Destination $OldRenamedAsi
    $OldAsiBlocked = $false
    try {
        & (Join-Path $PackageDir "scripts\windows\install.ps1") -GameDir $GameDir
    } catch {
        $OldAsiBlocked = $_.Exception.Message -match
            'possible old MGS4 Ultra120 ASI'
    }
    Remove-Item -Force -LiteralPath $OldRenamedAsi
    if (-not $OldAsiBlocked) {
        throw "Setup did not block a renamed old MGS4 Ultra120 ASI."
    }

    & (Join-Path $PackageDir "scripts\windows\install.ps1") -GameDir $GameDir

    # Simulate a user selecting Spanish before applying the stable profile.
    # Both the direct wrapper INI and the original Unity launcher state must
    # retain the same language.
    $InstalledConfigPath = Join-Path $GameDir "mgs4_ultrawide.ini"

    # A model-1 default must migrate once to the visually equivalent
    # single-owner value. Once marked as model 2, a deliberate 1.050 choice
    # must survive future managed updates.
    $LegacyConfig = Get-Content -Raw -LiteralPath $InstalledConfigPath
    $LegacyConfig = [regex]::Replace(
        $LegacyConfig, '(?m)^FOVMultiplier=.*$', 'FOVMultiplier=1.050')
    $LegacyConfig = [regex]::Replace(
        $LegacyConfig, '(?m)^FOVModelVersion=.*\r?\n', '')
    [IO.File]::WriteAllText($InstalledConfigPath, $LegacyConfig,
        [Text.UTF8Encoding]::new($false))
    & (Join-Path $PackageDir "scripts\windows\install.ps1") -GameDir $GameDir
    $MigratedConfig = Get-Content -Raw -LiteralPath $InstalledConfigPath
    if ($MigratedConfig -notmatch '(?m)^FOVMultiplier=1\.200\r?$' -or
        $MigratedConfig -notmatch '(?m)^FOVModelVersion=2\r?$') {
        $MigratedFov = [regex]::Match(
            $MigratedConfig, '(?m)^FOVMultiplier=.*$').Value
        $MigratedModel = [regex]::Match(
            $MigratedConfig, '(?m)^FOVModelVersion=.*$').Value
        throw "The legacy repeated-route FOV default did not migrate to model 2 ($MigratedFov; $MigratedModel)."
    }
    $DeliberateConfig = [regex]::Replace(
        $MigratedConfig, '(?m)^FOVMultiplier=.*$', 'FOVMultiplier=1.050')
    [IO.File]::WriteAllText($InstalledConfigPath, $DeliberateConfig,
        [Text.UTF8Encoding]::new($false))
    & (Join-Path $PackageDir "scripts\windows\install.ps1") -GameDir $GameDir
    if ((Get-Content -Raw -LiteralPath $InstalledConfigPath) -notmatch
        '(?m)^FOVMultiplier=1\.050\r?$') {
        throw "A model-2 update overwrote a deliberate FOV 1.050 choice."
    }

    $AboveRecommendationConfig = [regex]::Replace(
        (Get-Content -Raw -LiteralPath $InstalledConfigPath),
        '(?m)^FOVMultiplier=.*$', 'FOVMultiplier=1.350')
    [IO.File]::WriteAllText($InstalledConfigPath,
        $AboveRecommendationConfig, [Text.UTF8Encoding]::new($false))
    & (Join-Path $PackageDir "scripts\windows\install.ps1") -GameDir $GameDir
    if ((Get-Content -Raw -LiteralPath $InstalledConfigPath) -notmatch
        '(?m)^FOVMultiplier=1\.350\r?$') {
        throw "A managed update overwrote an above-recommendation user FOV value."
    }

    $OptOutConfig = Get-Content -Raw -LiteralPath $InstalledConfigPath
    $OptOutConfig = [regex]::Replace(
        $OptOutConfig, '(?m)^NativeCameraFOV=.*$', 'NativeCameraFOV=0')
    [IO.File]::WriteAllText($InstalledConfigPath, $OptOutConfig,
        [Text.UTF8Encoding]::new($false))
    & (Join-Path $PackageDir "scripts\windows\install.ps1") -GameDir $GameDir
    if ((Get-Content -Raw -LiteralPath $InstalledConfigPath) -notmatch
        '(?m)^NativeCameraFOV=0\r?$') {
        throw "A managed update silently re-enabled experimental native FOV."
    }

    $InstalledConfig = Get-Content -Raw -LiteralPath $InstalledConfigPath
    $InstalledConfig = [regex]::Replace(
        $InstalledConfig, '(?m)^Language=.*$', 'Language=sp')
    [IO.File]::WriteAllText($InstalledConfigPath, $InstalledConfig,
        [Text.UTF8Encoding]::new($false))

    $InstalledDll = Join-Path $GameDir "winmm.dll"
    $PackageDll = Join-Path $PackageDir "bin\winmm.dll"
    $InstalledDllHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $InstalledDll).Hash
    $PackageDllHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $PackageDll).Hash
    if ($InstalledDllHash -ne $PackageDllHash) {
        throw "Installed winmm.dll does not match the bundled payload."
    }
    $DllMarker = Join-Path $GameDir `
        ".mgs4ultra120-backup\winmm-installed.sha256"
    if ((Get-Content -Raw -LiteralPath $DllMarker).Trim() -ne $PackageDllHash) {
        throw "The installed DLL hash marker is missing or incorrect."
    }
    $InstalledAsi = Join-Path (Join-Path $GameDir "scripts") `
        "MGS4Ultra120.asi"
    $PackageAsi = Join-Path $PackageDir "bin\MGS4Ultra120.asi"
    $InstalledAsiHash = (Get-FileHash -Algorithm SHA256 `
        -LiteralPath $InstalledAsi).Hash
    $PackageAsiHash = (Get-FileHash -Algorithm SHA256 `
        -LiteralPath $PackageAsi).Hash
    if ($InstalledAsiHash -ne $PackageAsiHash) {
        throw "Installed MGS4Ultra120.asi does not match the bundled plugin."
    }
    $AsiMarker = Join-Path $GameDir `
        ".mgs4ultra120-backup\asi-installed.sha256"
    if ((Get-Content -Raw -LiteralPath $AsiMarker).Trim() -ne $PackageAsiHash) {
        throw "The installed ASI hash marker is missing or incorrect."
    }
    $InstalledHudAsi = Join-Path (Join-Path $GameDir "scripts") `
        "MGS4CenteredHUD16x9.asi"
    $PackageHudAsi = Join-Path $PackageDir "bin\MGS4CenteredHUD16x9.asi"
    $PackageHudAsiHash = (Get-FileHash -Algorithm SHA256 `
        -LiteralPath $PackageHudAsi).Hash
    if ((Get-FileHash -Algorithm SHA256 -LiteralPath $InstalledHudAsi).Hash -ne
        $PackageHudAsiHash) {
        throw "Installed centered-HUD ASI does not match the bundled plugin."
    }
    $HudAsiMarker = Join-Path $GameDir `
        ".mgs4ultra120-backup\hud-asi-installed.sha256"
    if ((Get-Content -Raw -LiteralPath $HudAsiMarker).Trim() -ne
        $PackageHudAsiHash) {
        throw "The installed centered-HUD ASI hash marker is incorrect."
    }

    & (Join-Path $PackageDir "scripts\windows\configure.ps1") `
        -GameDir $GameDir -Profile stable
    $IniText = Get-Content -Raw -LiteralPath (Join-Path $GameDir "mgs4_ultrawide.ini")
    foreach ($ExpectedLine in @(
        "UltrawideEnabled=1",
        "FPSOverrideEnabled=0",
        "SupersamplingEnabled=0",
        "RenderScale=1.500",
        "FOVMultiplier=1.200",
        "NativeCameraFOV=1",
        "ExperimentalCinematicFOV=0",
        "CinematicFOVMultiplier=inherit",
        "FOVModelVersion=2",
        "Limit=60",
        "ControllerProfileFixEnabled=1",
        "Language=sp"
    )) {
        if ($IniText -notmatch "(?m)^$([regex]::Escape($ExpectedLine))\r?$") {
            throw "Stable profile did not write: $ExpectedLine"
        }
    }
    $HudIniText = Get-Content -Raw -LiteralPath `
        (Join-Path $GameDir "mgs4_centered_hud_16x9.ini")
    foreach ($ExpectedLine in @(
        "Enabled=0", "Width=3440", "Height=1440",
        "CenterHUDIn16x9=0", "FullCanvasTest=0",
        "EmitterTransformTest=1", "Preview3DUniformFitTest=1"
    )) {
        if ($HudIniText -notmatch "(?m)^$([regex]::Escape($ExpectedLine))\r?$") {
            throw "Stable profile did not write centered-HUD setting: $ExpectedLine"
        }
    }
    if ($HudIniText -match '(?m)^PreviewRTVGateTest=') {
        throw "Public package exposes the private preview RTV gate."
    }
    & (Join-Path $PackageDir "scripts\windows\configure.ps1") `
        -GameDir $GameDir -Profile 16x9-supersampling
    $IniText = Get-Content -Raw -LiteralPath `
        (Join-Path $GameDir "mgs4_ultrawide.ini")
    foreach ($ExpectedLine in @(
        "UltrawideEnabled=0", "Width=1920", "Height=1080",
        "FOVMultiplier=1.000", "NativeCameraFOV=0",
        "ExperimentalCinematicFOV=0", "SupersamplingEnabled=1",
        "RenderScale=2.000", "ControllerProfileFixEnabled=1",
        "DisplayMode=Windowed", "UsePrimaryPhysicalResolution=0"
    )) {
        if ($IniText -notmatch "(?m)^$([regex]::Escape($ExpectedLine))\r?$") {
            throw "16:9 supersampling profile did not write: $ExpectedLine"
        }
    }
    $HudIniText = Get-Content -Raw -LiteralPath `
        (Join-Path $GameDir "mgs4_centered_hud_16x9.ini")
    foreach ($ExpectedLine in @("Enabled=0", "Width=1920", "Height=1080")) {
        if ($HudIniText -notmatch "(?m)^$([regex]::Escape($ExpectedLine))\r?$") {
            throw "16:9 supersampling HUD profile did not write: $ExpectedLine"
        }
    }
    $ResetIni = Get-Content -Raw -LiteralPath `
        (Join-Path $GameDir "mgs4_ultrawide.ini")
    $ResetIni = [regex]::Replace($ResetIni, '(?m)^Width=.*$', 'Width=3440')
    $ResetIni = [regex]::Replace($ResetIni, '(?m)^Height=.*$', 'Height=1440')
    [IO.File]::WriteAllText((Join-Path $GameDir "mgs4_ultrawide.ini"),
        $ResetIni, [Text.UTF8Encoding]::new($false))
    & (Join-Path $PackageDir "scripts\windows\configure.ps1") `
        -GameDir $GameDir -Profile stable
    $InstalledLauncher = Join-Path $LauncherDir "launcher.exe"
    $PackageLauncher = Join-Path $PackageDir "bin\launcher.exe"
    $InstalledLauncherHash = (Get-FileHash -Algorithm SHA256 `
        -LiteralPath $InstalledLauncher).Hash
    $PackageLauncherHash = (Get-FileHash -Algorithm SHA256 `
        -LiteralPath $PackageLauncher).Hash
    if ($InstalledLauncherHash -ne $PackageLauncherHash) {
        throw "Default profile did not enable the direct-launch wrapper."
    }
    $WrapperMarker = Join-Path $GameDir `
        ".mgs4ultra120-backup\launcher-wrapper-installed.sha256"
    if ((Get-Content -Raw -LiteralPath $WrapperMarker).Trim() -ne
        $PackageLauncherHash) {
        throw "The installed wrapper hash marker is missing or incorrect."
    }
    $OriginalLauncherBackup = Join-Path $GameDir `
        ".mgs4ultra120-backup\launcher.exe.preinstall"
    [IO.File]::WriteAllBytes($InstalledLauncher,
        [Text.Encoding]::UTF8.GetBytes("simulated older project wrapper"))
    & (Join-Path $PackageDir "scripts\windows\configure.ps1") `
        -GameDir $GameDir -Profile stable
    if ([Convert]::ToBase64String([IO.File]::ReadAllBytes(
        $OriginalLauncherBackup)) -ne
        [Convert]::ToBase64String($PreexistingLauncher)) {
        throw "A wrapper update replaced the original Unity-launcher backup."
    }
    if ((Get-FileHash -Algorithm SHA256 -LiteralPath $InstalledLauncher).Hash -ne
        $PackageLauncherHash) {
        throw "A wrapper update did not install the current packaged wrapper."
    }
    if (-not (Get-ChildItem -LiteralPath (Split-Path -Parent $WrapperMarker) `
        -Filter "launcher.exe.displaced.*" -File)) {
        throw "A wrapper update did not preserve the displaced active file."
    }
    $OriginalDllBackup = Join-Path $GameDir `
        ".mgs4ultra120-backup\winmm.dll.preinstall"
    $SimulatedOldDll = [Text.Encoding]::UTF8.GetBytes(
        "simulated older managed proxy")
    [IO.File]::WriteAllBytes($InstalledDll, $SimulatedOldDll)
    $SimulatedOldDllHash = (Get-FileHash -Algorithm SHA256 `
        -LiteralPath $InstalledDll).Hash
    [IO.File]::WriteAllText($DllMarker, $SimulatedOldDllHash,
        [Text.Encoding]::ASCII)
    & (Join-Path $PackageDir "scripts\windows\install.ps1") -GameDir $GameDir
    if ([Convert]::ToBase64String([IO.File]::ReadAllBytes(
        $OriginalDllBackup)) -ne [Convert]::ToBase64String($PreexistingDll)) {
        throw "A DLL update replaced the original pre-install backup."
    }
    if ((Get-FileHash -Algorithm SHA256 -LiteralPath $InstalledDll).Hash -ne
        $PackageDllHash) {
        throw "A DLL update did not install the current packaged ASI loader."
    }
    $OriginalAsiBackup = Join-Path $GameDir `
        ".mgs4ultra120-backup\MGS4Ultra120.asi.preinstall"
    $SimulatedOldAsi = [Text.Encoding]::UTF8.GetBytes(
        "simulated older managed ASI plugin")
    [IO.File]::WriteAllBytes($InstalledAsi, $SimulatedOldAsi)
    $SimulatedOldAsiHash = (Get-FileHash -Algorithm SHA256 `
        -LiteralPath $InstalledAsi).Hash
    [IO.File]::WriteAllText($AsiMarker, $SimulatedOldAsiHash,
        [Text.Encoding]::ASCII)
    & (Join-Path $PackageDir "scripts\windows\install.ps1") -GameDir $GameDir
    if ([Convert]::ToBase64String([IO.File]::ReadAllBytes(
        $OriginalAsiBackup)) -ne [Convert]::ToBase64String($PreexistingAsi)) {
        throw "An ASI update replaced the original pre-install backup."
    }
    if ((Get-FileHash -Algorithm SHA256 -LiteralPath $InstalledAsi).Hash -ne
        $PackageAsiHash) {
        throw "An ASI update did not install the current packaged plugin."
    }
    $UpdatedLauncherSettings = Get-Content -Raw -LiteralPath `
        $LauncherSettingsPath | ConvertFrom-Json
    $LauncherMap = @{}
    for ($Index = 0; $Index -lt $UpdatedLauncherSettings.keyList.Count; $Index++) {
        $LauncherMap[[string]$UpdatedLauncherSettings.keyList[$Index]] =
            [string]$UpdatedLauncherSettings.valueList[$Index]
    }
    foreach ($Expected in @(
        "ResolutionFullW=3440", "ResolutionFullH=1440",
        "ResolutionWindowW=3440", "ResolutionWindowH=1440",
        "WindowSizeW=3440", "WindowSizeH=1440", "WindowMode=1",
        "prevPlayLanguage=6"
    )) {
        $Parts = $Expected.Split('=')
        if ($LauncherMap[$Parts[0]] -ne $Parts[1]) {
            throw "Windows launcher display synchronization failed: $Expected"
        }
    }

    # The official launcher uses Def.LANGUAGE (not the zero-based launcher-UI
    # enum) for prevPlayLanguage. Verify every public language code so a future
    # UI change cannot silently select a neighbouring language.
    $LanguageValues = [ordered]@{
        jp = "1"; en = "2"; fr = "3"; it = "4"
        gr = "5"; sp = "6"; pt = "7"
    }
    foreach ($LanguageEntry in $LanguageValues.GetEnumerator()) {
        $LanguageIni = Get-Content -Raw -LiteralPath $InstalledConfigPath
        $LanguageIni = [regex]::Replace($LanguageIni,
            '(?m)^Language=.*$', "Language=$($LanguageEntry.Key)")
        [IO.File]::WriteAllText($InstalledConfigPath, $LanguageIni,
            [Text.UTF8Encoding]::new($false))
        & (Join-Path $PackageDir "scripts\windows\configure.ps1") `
            -GameDir $GameDir -Profile stable
        $LanguageSettings = Get-Content -Raw -LiteralPath `
            $LauncherSettingsPath | ConvertFrom-Json
        $LanguageMap = @{}
        for ($Index = 0; $Index -lt $LanguageSettings.keyList.Count; $Index++) {
            $LanguageMap[[string]$LanguageSettings.keyList[$Index]] =
                [string]$LanguageSettings.valueList[$Index]
        }
        if ($LanguageMap.prevPlayLanguage -ne $LanguageEntry.Value) {
            throw "Language=$($LanguageEntry.Key) did not map to the official launcher value $($LanguageEntry.Value)."
        }
        if ((Get-Content -Raw -LiteralPath $InstalledConfigPath) -notmatch
            "(?m)^Language=$([regex]::Escape($LanguageEntry.Key))\r?$") {
            throw "Stable profile did not preserve Language=$($LanguageEntry.Key)."
        }
    }

    $IniPath = Join-Path $GameDir "mgs4_ultrawide.ini"
    $Customized = Get-Content -Raw -LiteralPath $IniPath
    $Customized = [regex]::Replace($Customized, '(?m)^Width=.*$', 'Width=5120')
    $Customized = [regex]::Replace($Customized, '(?m)^Language=.*$', 'Language=sp')
    $Customized = [regex]::Replace($Customized, '(?m)^Limit=.*$', 'Limit=120')
    $Customized = [regex]::Replace($Customized,
        '(?m)^SupersamplingEnabled=.*$', 'SupersamplingEnabled=1')
    $Customized = [regex]::Replace($Customized,
        '(?m)^RenderScale=.*$', 'RenderScale=2.000')
    $Customized = [regex]::Replace($Customized,
        '(?m)^ExperimentalCinematicFOV=.*$', 'ExperimentalCinematicFOV=1')
    $Customized = [regex]::Replace($Customized,
        '(?m)^CinematicFOVMultiplier=.*$', 'CinematicFOVMultiplier=1.300')
    [IO.File]::WriteAllText($IniPath, $Customized,
        [Text.UTF8Encoding]::new($false))
    $HudIniPath = Join-Path $GameDir "mgs4_centered_hud_16x9.ini"
    $CustomizedHud = [regex]::Replace(
        (Get-Content -Raw -LiteralPath $HudIniPath),
        '(?m)^Enabled=.*$', 'Enabled=1')
    [IO.File]::WriteAllText($HudIniPath, $CustomizedHud,
        [Text.UTF8Encoding]::new($false))
    & (Join-Path $PackageDir "scripts\windows\install.ps1") -GameDir $GameDir
    $UpdatedIni = Get-Content -Raw -LiteralPath $IniPath
    foreach ($PreservedLine in @(
        "Width=5120", "Language=sp", "SkipUnityLauncher=1",
        "SupersamplingEnabled=1", "RenderScale=2.000",
        "ExperimentalCinematicFOV=1", "CinematicFOVMultiplier=1.300"
    )) {
        if ($UpdatedIni -notmatch "(?m)^$([regex]::Escape($PreservedLine))\r?$") {
            throw "Update did not preserve setting: $PreservedLine"
        }
    }
    if ($UpdatedIni -notmatch '(?m)^Limit=60\r?$') {
        throw "Update did not migrate the previous experimental 120 FPS value to 60."
    }
    $UpdatedHud = Get-Content -Raw -LiteralPath $HudIniPath
    if ($UpdatedHud -notmatch '(?m)^Enabled=1\r?$' -or
        $UpdatedHud -notmatch '(?m)^Width=5120\r?$') {
        throw "Update did not preserve the HUD opt-in or synchronize its width."
    }

    & (Join-Path $PackageDir "scripts\windows\uninstall.ps1") -GameDir $GameDir

    if ([Convert]::ToBase64String([IO.File]::ReadAllBytes(
        (Join-Path $GameDir "winmm.dll"))) -ne
        [Convert]::ToBase64String($PreexistingDll)) {
        throw "Uninstall did not restore the pre-existing winmm.dll."
    }
    if ([Convert]::ToBase64String([IO.File]::ReadAllBytes(
        (Join-Path (Join-Path $GameDir "scripts") "MGS4Ultra120.asi"))) -ne
        [Convert]::ToBase64String($PreexistingAsi)) {
        throw "Uninstall did not restore the pre-existing ASI plugin."
    }
    if (Test-Path -LiteralPath (Join-Path (Join-Path $GameDir "scripts") `
            "MGS4CenteredHUD16x9.asi")) {
        throw "Uninstall left the managed centered-HUD ASI behind."
    }
    if (Test-Path -LiteralPath (Join-Path $GameDir `
            "mgs4_centered_hud_16x9.ini")) {
        throw "Uninstall left the managed centered-HUD INI behind."
    }
    if ([Convert]::ToBase64String([IO.File]::ReadAllBytes(
        (Join-Path $GameDir "mgs4_ultrawide.ini"))) -ne
        [Convert]::ToBase64String($PreexistingIni)) {
        throw "Uninstall did not restore the pre-existing INI file."
    }
    if ([Convert]::ToBase64String([IO.File]::ReadAllBytes(
        (Join-Path $LauncherDir "launcher.exe"))) -ne
        [Convert]::ToBase64String($PreexistingLauncher)) {
        throw "Uninstall did not restore the original Unity launcher."
    }
    $RestoredLauncherSettings = Get-Content -Raw -LiteralPath `
        $LauncherSettingsPath | ConvertFrom-Json
    $RestoredMap = @{}
    for ($Index = 0; $Index -lt $RestoredLauncherSettings.keyList.Count; $Index++) {
        $RestoredMap[[string]$RestoredLauncherSettings.keyList[$Index]] =
            [string]$RestoredLauncherSettings.valueList[$Index]
    }
    foreach ($Key in $OriginalLauncherValues.Keys) {
        if ($RestoredMap[$Key] -ne $OriginalLauncherValues[$Key]) {
            throw "Uninstall did not restore official launcher setting: $Key"
        }
    }
    if ($RestoredMap.UnrelatedSetting -ne "keep-me") {
        throw "Uninstall changed an unrelated official launcher setting."
    }
    if (Test-Path -LiteralPath (Join-Path $GameDir ".mgs4ultra120-backup")) {
        throw "Backup directory remained after a clean uninstall."
    }

    Remove-Item -Force -LiteralPath (Join-Path $GameDir "winmm.dll")
    Remove-Item -Force -LiteralPath `
        (Join-Path (Join-Path $GameDir "scripts") "MGS4Ultra120.asi")
    & (Join-Path $PackageDir "scripts\windows\install.ps1") -GameDir $GameDir
    & (Join-Path $PackageDir "scripts\windows\uninstall.ps1") -GameDir $GameDir
    if (Test-Path -LiteralPath (Join-Path $GameDir "winmm.dll")) {
        throw "Clean install/uninstall left the managed winmm.dll behind."
    }
    if (Test-Path -LiteralPath `
        (Join-Path (Join-Path $GameDir "scripts") "MGS4Ultra120.asi")) {
        throw "Clean install/uninstall left the managed ASI plugin behind."
    }
    if (Test-Path -LiteralPath `
        (Join-Path (Join-Path $GameDir "scripts") "MGS4CenteredHUD16x9.asi")) {
        throw "Clean install/uninstall left the centered-HUD ASI behind."
    }

    # A loader supplied by another mod must be reused and left in place. Add a
    # harmless overlay byte so its hash differs while PE version metadata still
    # identifies it as Ultimate ASI Loader.
    $ExternalLoader = [Collections.Generic.List[byte]]::new()
    $ExternalLoader.AddRange([byte[]][IO.File]::ReadAllBytes($PackageDll))
    $ExternalLoader.Add(0)
    [IO.File]::WriteAllBytes((Join-Path $GameDir "winmm.dll"),
        $ExternalLoader.ToArray())
    $ExternalLoaderHash = (Get-FileHash -Algorithm SHA256 `
        -LiteralPath (Join-Path $GameDir "winmm.dll")).Hash
    & (Join-Path $PackageDir "scripts\windows\install.ps1") -GameDir $GameDir
    if ((Get-FileHash -Algorithm SHA256 `
        -LiteralPath (Join-Path $GameDir "winmm.dll")).Hash -ne
        $ExternalLoaderHash) {
        throw "Setup replaced an existing compatible Ultimate ASI Loader."
    }
    & (Join-Path $PackageDir "scripts\windows\uninstall.ps1") -GameDir $GameDir
    if ((Get-FileHash -Algorithm SHA256 `
        -LiteralPath (Join-Path $GameDir "winmm.dll")).Hash -ne
        $ExternalLoaderHash) {
        throw "Uninstall removed or changed another mod's ASI loader."
    }
    Remove-Item -Force -LiteralPath (Join-Path $GameDir "winmm.dll")

    # Version metadata alone is insufficient: an x86 loader cannot be reused
    # by this x64 game. Corrupt only the PE machine field while retaining the
    # upstream version resource, then verify setup replaces it safely.
    $WrongArchitectureLoader = [IO.File]::ReadAllBytes($PackageDll)
    $WrongArchitecturePeOffset = [BitConverter]::ToInt32(
        $WrongArchitectureLoader, 0x3c)
    [BitConverter]::GetBytes([UInt16]0x014c).CopyTo(
        $WrongArchitectureLoader, $WrongArchitecturePeOffset + 4)
    [IO.File]::WriteAllBytes((Join-Path $GameDir "winmm.dll"),
        $WrongArchitectureLoader)
    $WrongArchitectureHash = (Get-FileHash -Algorithm SHA256 `
        -LiteralPath (Join-Path $GameDir "winmm.dll")).Hash
    & (Join-Path $PackageDir "scripts\windows\install.ps1") -GameDir $GameDir
    if ((Get-FileHash -Algorithm SHA256 `
        -LiteralPath (Join-Path $GameDir "winmm.dll")).Hash -ne
        $PackageDllHash) {
        throw "Setup reused an incompatible x86 Ultimate ASI Loader."
    }
    & (Join-Path $PackageDir "scripts\windows\uninstall.ps1") -GameDir $GameDir
    if ((Get-FileHash -Algorithm SHA256 `
        -LiteralPath (Join-Path $GameDir "winmm.dll")).Hash -ne
        $WrongArchitectureHash) {
        throw "Uninstall did not restore the incompatible pre-existing loader."
    }
    Remove-Item -Force -LiteralPath (Join-Path $GameDir "winmm.dll")

    # Some official launcher_sv variants omit WindowSizeW/WindowSizeH while
    # retaining the equivalent ResolutionWindowW/ResolutionWindowH fields.
    # These redundant optional fields must never block saving patch settings.
    $SettingsWithoutWindowSizeW = [pscustomobject]@{
        keyList = @(
            "ResolutionFullW", "ResolutionFullH",
            "ResolutionWindowW", "ResolutionWindowH",
            "WindowSizeH", "WindowMode", "UnrelatedSetting"
        )
        valueList = @("2560", "1440", "1280", "720", "720", "0", "keep-me")
    }
    [IO.File]::WriteAllText($LauncherSettingsPath,
        ($SettingsWithoutWindowSizeW | ConvertTo-Json -Compress),
        [Text.UTF8Encoding]::new($false))
    . (Join-Path $PackageDir "scripts\windows\common.ps1")
    Set-Mgs4Ultra120WindowsDisplaySettings $GameDir 3440 1440 "Windowed"
    $OptionalFieldResult = Get-Content -Raw -LiteralPath `
        $LauncherSettingsPath | ConvertFrom-Json
    $OptionalFieldMap = Get-Mgs4Ultra120LauncherSettingMap $OptionalFieldResult
    foreach ($Expected in @(
        "ResolutionWindowW=3440", "ResolutionWindowH=1440",
        "WindowSizeH=1440", "WindowMode=1", "UnrelatedSetting=keep-me"
    )) {
        $Parts = $Expected.Split('=')
        if ([string]$OptionalFieldResult.valueList[
            $OptionalFieldMap[$Parts[0]]] -ne $Parts[1]) {
            throw "Optional launcher field compatibility failed: $Expected"
        }
    }
    if ($OptionalFieldMap.ContainsKey("WindowSizeW")) {
        throw "Display synchronization invented a missing optional launcher field."
    }

    Write-Host "Windows package smoke test passed."
} finally {
    $env:MGS4ULTRA120_PACKAGE_TEST_GAME_DIR = $PreviousPackageTestTarget
    try {
        $Saved = (Get-ItemProperty -Path "HKCU:\Software\MGS4Ultra120" `
            -Name LastGameDir -ErrorAction Stop).LastGameDir
        if ($Saved -eq $GameDir) {
            Remove-ItemProperty -Path "HKCU:\Software\MGS4Ultra120" `
                -Name LastGameDir -ErrorAction SilentlyContinue
        }
    } catch {}
    $ResolvedTemp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    $ResolvedSmoke = [IO.Path]::GetFullPath($SmokeRoot)
    if ($ResolvedSmoke.StartsWith($ResolvedTemp,
        [StringComparison]::OrdinalIgnoreCase) -and
        (Split-Path -Leaf $ResolvedSmoke).StartsWith("mgs4ultra120-smoke-")) {
        Remove-Item -LiteralPath $ResolvedSmoke -Recurse -Force -ErrorAction SilentlyContinue
    }
}
