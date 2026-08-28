param(
    [Parameter(Mandatory)]
    [string]$PackageDir
)

$ErrorActionPreference = "Stop"
$PackageDir = (Resolve-Path -LiteralPath $PackageDir).Path
$RequiredFiles = @(
    "MGS4Ultra120-Setup.cmd",
    "bin\winmm.dll",
    "bin\launcher.exe",
    "config\mgs4_ultrawide.ini",
    "scripts\windows\setup.ps1",
    "scripts\windows\common.ps1",
    "scripts\windows\install.ps1",
    "scripts\windows\configure.ps1",
    "scripts\windows\uninstall.ps1",
    "scripts\windows\uninstall-installed-package.ps1"
)

foreach ($RelativePath in $RequiredFiles) {
    $Path = Join-Path $PackageDir $RelativePath
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Required package file is missing: $RelativePath"
    }
}

foreach ($RelativePath in @(
    "scripts\windows\setup.ps1",
    "scripts\windows\common.ps1",
    "scripts\windows\install.ps1",
    "scripts\windows\configure.ps1",
    "scripts\windows\uninstall.ps1",
    "scripts\windows\uninstall-installed-package.ps1"
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
$PreexistingIni = [Text.Encoding]::UTF8.GetBytes("pre-existing ini test file")
$PreexistingLauncher = [Text.Encoding]::UTF8.GetBytes("pre-existing launcher test file")

try {
    New-Item -ItemType Directory -Path $GameDir -Force | Out-Null
    New-Item -ItemType Directory -Path $LauncherDir -Force | Out-Null
    New-Item -ItemType Directory -Path $LauncherSettingsDir -Force | Out-Null
    [IO.File]::WriteAllBytes((Join-Path $GameDir "mgs4.exe"),
        [Text.Encoding]::UTF8.GetBytes("MGS4 Ultra120 smoke-test placeholder"))
    [IO.File]::WriteAllBytes((Join-Path $GameDir "winmm.dll"), $PreexistingDll)
    [IO.File]::WriteAllBytes((Join-Path $GameDir "mgs4_ultrawide.ini"), $PreexistingIni)
    [IO.File]::WriteAllBytes((Join-Path $LauncherDir "launcher.exe"),
        $PreexistingLauncher)
    $LauncherSettingsPath = Join-Path $LauncherSettingsDir "launcher_sv"
    $OriginalLauncherValues = [ordered]@{
        ResolutionFullW = "2560"; ResolutionFullH = "1440"
        ResolutionWindowW = "1280"; ResolutionWindowH = "720"
        WindowSizeW = "1280"; WindowSizeH = "720"; WindowMode = "0"
    }
    $LauncherSettings = [pscustomobject]@{
        keyList = @($OriginalLauncherValues.Keys) + @("UnrelatedSetting")
        valueList = @($OriginalLauncherValues.Values) + @("keep-me")
    }
    [IO.File]::WriteAllText($LauncherSettingsPath,
        ($LauncherSettings | ConvertTo-Json -Compress),
        [Text.UTF8Encoding]::new($false))

    & (Join-Path $PackageDir "scripts\windows\install.ps1") -GameDir $GameDir

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

    & (Join-Path $PackageDir "scripts\windows\configure.ps1") `
        -GameDir $GameDir -Profile stable
    $IniText = Get-Content -Raw -LiteralPath (Join-Path $GameDir "mgs4_ultrawide.ini")
    foreach ($ExpectedLine in @(
        "UltrawideEnabled=1",
        "FPSOverrideEnabled=1",
        "Limit=60",
        "ConstrainUITo16x9=0",
        "ControllerProfileFixEnabled=1"
    )) {
        if ($IniText -notmatch "(?m)^$([regex]::Escape($ExpectedLine))$") {
            throw "Stable profile did not write: $ExpectedLine"
        }
    }
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
        throw "A DLL update did not install the current packaged proxy."
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
        "WindowSizeW=3440", "WindowSizeH=1440", "WindowMode=1"
    )) {
        $Parts = $Expected.Split('=')
        if ($LauncherMap[$Parts[0]] -ne $Parts[1]) {
            throw "Windows launcher display synchronization failed: $Expected"
        }
    }

    $IniPath = Join-Path $GameDir "mgs4_ultrawide.ini"
    $Customized = Get-Content -Raw -LiteralPath $IniPath
    $Customized = [regex]::Replace($Customized, '(?m)^Width=.*$', 'Width=5120')
    $Customized = [regex]::Replace($Customized, '(?m)^Language=.*$', 'Language=sp')
    $Customized = [regex]::Replace($Customized, '(?m)^ToggleHotkey=.*$', 'ToggleHotkey=F9')
    [IO.File]::WriteAllText($IniPath, $Customized,
        [Text.UTF8Encoding]::new($false))
    & (Join-Path $PackageDir "scripts\windows\install.ps1") -GameDir $GameDir
    $UpdatedIni = Get-Content -Raw -LiteralPath $IniPath
    foreach ($PreservedLine in @(
        "Width=5120", "Language=sp", "ToggleHotkey=F9", "SkipUnityLauncher=1"
    )) {
        if ($UpdatedIni -notmatch "(?m)^$([regex]::Escape($PreservedLine))$") {
            throw "Update did not preserve setting: $PreservedLine"
        }
    }

    & (Join-Path $PackageDir "scripts\windows\uninstall.ps1") -GameDir $GameDir

    if ([Convert]::ToBase64String([IO.File]::ReadAllBytes(
        (Join-Path $GameDir "winmm.dll"))) -ne
        [Convert]::ToBase64String($PreexistingDll)) {
        throw "Uninstall did not restore the pre-existing winmm.dll."
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
    & (Join-Path $PackageDir "scripts\windows\install.ps1") -GameDir $GameDir
    & (Join-Path $PackageDir "scripts\windows\uninstall.ps1") -GameDir $GameDir
    if (Test-Path -LiteralPath (Join-Path $GameDir "winmm.dll")) {
        throw "Clean install/uninstall left the managed winmm.dll behind."
    }

    Write-Host "Windows package smoke test passed."
} finally {
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
