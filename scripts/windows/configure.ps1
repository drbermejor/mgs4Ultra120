param(
    [ValidateSet("stable", "fps-only-120", "ultrawide-only", "controller-fix-only")]
    [string]$Profile,
    [ValidateSet("Windowed", "Fullscreen")]
    [string]$WindowsDisplayMode,
    [string]$GameDir = "${env:ProgramFiles(x86)}\Steam\steamapps\common\METAL GEAR SOLID 4\MGS4"
)
$Mgs4Ultra120Version = "v0.3.4-alpha.1"
$ErrorActionPreference = "Stop"
$KnownExeSha256 = "9e8df67ea7f41e7f8306ce1a77584707209069b3c75389b3f00445efe459fe41"
if (-not $GameDir -or -not [IO.Directory]::Exists($GameDir)) {
    throw "The selected MGS4 folder is unavailable. Reconnect its drive or choose it again from Easy setup."
}
$Ini = Join-Path $GameDir "mgs4_ultrawide.ini"
$HudIni = Join-Path $GameDir "mgs4_centered_hud_16x9.ini"
$Exe = Join-Path $GameDir "mgs4.exe"
$PackageDir = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
. (Join-Path $PSScriptRoot "common.ps1")
. (Join-Path $PSScriptRoot "mgsfpsunlock.ps1")
$InstallDir = Split-Path -Parent $GameDir
$LauncherDir = Join-Path $InstallDir "Launcher"
$LauncherTarget = Join-Path $LauncherDir "launcher.exe"
$BackupDir = Join-Path $GameDir ".mgs4ultra120-backup"
$LauncherBackup = Join-Path $BackupDir "launcher.exe.preinstall"
$LauncherHashMarker = Join-Path $BackupDir "launcher-wrapper-installed.sha256"
$WrapperSource = Join-Path $PackageDir "bin\launcher.exe"

function Test-Mgs4Ultra120AutoHdrEnabled {
    try {
        $GlobalSettings = [string](Get-ItemProperty -LiteralPath `
            "HKCU:\Software\Microsoft\DirectX\UserGpuPreferences" `
            -Name DirectXUserGlobalSettings -ErrorAction Stop).DirectXUserGlobalSettings
        return $GlobalSettings -match '(?:^|;)AutoHDREnable=1(?:;|$)'
    } catch {
        return $false
    }
}

if (-not (Test-Path -LiteralPath $Ini)) { throw "mgs4_ultrawide.ini not found in: $GameDir" }
if (-not (Test-Path -LiteralPath $Exe)) { throw "mgs4.exe not found in: $GameDir" }
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
    throw "Exit the game before changing settings."
}

function Get-IniValue([string]$Key) {
    $Match = [regex]::Match((Get-Content -Raw -LiteralPath $Ini), "(?m)^$([regex]::Escape($Key))=(.*)$")
    if (-not $Match.Success) { throw "Missing $Key in $Ini; reinstall the current patch." }
    return $Match.Groups[1].Value.Trim()
}

function Get-HudIniValue([string]$Key) {
    if (-not (Test-Path -LiteralPath $HudIni -PathType Leaf)) {
        throw "mgs4_centered_hud_16x9.ini is missing; reinstall the current patch."
    }
    $Match = [regex]::Match((Get-Content -Raw -LiteralPath $HudIni),
        "(?m)^$([regex]::Escape($Key))=(.*)$")
    if (-not $Match.Success) {
        throw "Missing $Key in $HudIni; reinstall the current patch."
    }
    return $Match.Groups[1].Value.Trim()
}

function Set-HudSettings([int]$Enabled, [int]$Width, [int]$Height) {
    if ($Enabled -notin @(0, 1)) { throw "Centered HUD state must be 0 or 1." }
    $Text = Get-Content -Raw -LiteralPath $HudIni
    foreach ($Entry in ([ordered]@{
        Enabled = $Enabled; Width = $Width; Height = $Height
    }).GetEnumerator()) {
        $Pattern = "(?m)^$([regex]::Escape($Entry.Key))=.*$"
        if (-not [regex]::IsMatch($Text, $Pattern)) {
            throw "Missing $($Entry.Key) in $HudIni"
        }
        $Text = [regex]::Replace($Text, $Pattern,
            "$($Entry.Key)=$($Entry.Value)", 1)
    }
    $Temporary = "$HudIni.tmp"
    [IO.File]::WriteAllText($Temporary, $Text,
        [Text.UTF8Encoding]::new($false))
    Move-Item -Force -LiteralPath $Temporary -Destination $HudIni
}

function Set-PatchSettings([int]$Width, [int]$Height, [decimal]$Fov,
                           [int]$NativeCameraFov,
                           [int]$CinematicFovEnabled,
                           [string]$CinematicFovMultiplier,
                           [int]$SupersamplingEnabled, [decimal]$RenderScale,
                           [int]$UltrawideEnabled,
                           [int]$ControllerFixEnabled,
                           [int]$SkipUnityLauncher, [string]$Language,
                           [int]$AllowUnsupported, [string]$DisplayMode,
                           [int]$UsePrimaryPhysicalResolution) {
    if ($Width -lt 640 -or $Width -gt 16384 -or $Height -lt 480 -or $Height -gt 16384) {
        throw "Width/height are outside the allowed range."
    }
    if ($Fov -lt 0.5) { throw "FOV multiplier must be at least 0.5." }
    if ($NativeCameraFov -notin @(0, 1)) { throw "NativeCameraFOV must be 0 or 1." }
    if ($CinematicFovEnabled -notin @(0, 1)) {
        throw "ExperimentalCinematicFOV must be 0 or 1."
    }
    $CinematicFovMultiplier = $CinematicFovMultiplier.Trim().Replace(',', '.')
    if ($CinematicFovMultiplier -ne "inherit") {
        $ParsedCinematicFov = 0.0
        if (-not [double]::TryParse($CinematicFovMultiplier,
                [Globalization.NumberStyles]::Float,
                [Globalization.CultureInfo]::InvariantCulture,
                [ref]$ParsedCinematicFov) -or
            [double]::IsNaN($ParsedCinematicFov) -or
            [double]::IsInfinity($ParsedCinematicFov) -or
            $ParsedCinematicFov -lt 0.5) {
            throw "Cinematic FOV must be 'inherit' or a finite value of at least 0.5."
        }
        $CinematicFovMultiplier = $ParsedCinematicFov.ToString(
            "0.000", [Globalization.CultureInfo]::InvariantCulture)
    }
    if ($RenderScale -lt 1.0) { throw "Supersampling render scale must be at least 1.0." }
    if ($SupersamplingEnabled -notin @(0, 1)) { throw "SupersamplingEnabled must be 0 or 1." }
    if ($SupersamplingEnabled -eq 1 -and
        ([decimal]$Width * $RenderScale -gt [uint32]::MaxValue -or
         [decimal]$Height * $RenderScale -gt [uint32]::MaxValue)) {
        throw "The requested internal render size does not fit the game's 32-bit resolution fields."
    }
    if ($Language -eq "ge") { $Language = "gr" }
    if ($Language -notin @("en", "sp", "fr", "it", "gr", "jp", "pt")) { throw "Unsupported game language." }
    if ($DisplayMode -notin @("Windowed", "Fullscreen")) { throw "Unsupported Windows display mode." }
    $Values = [ordered]@{
        UltrawideEnabled = $UltrawideEnabled
        FPSOverrideEnabled = 0
        Width = $Width; Height = $Height
        FOVMultiplier = $Fov.ToString("0.000", [Globalization.CultureInfo]::InvariantCulture)
        NativeCameraFOV = $NativeCameraFov
        ExperimentalCinematicFOV = $CinematicFovEnabled
        CinematicFOVMultiplier = $CinematicFovMultiplier
        FOVModelVersion = 2
        SupersamplingEnabled = $SupersamplingEnabled
        RenderScale = $RenderScale.ToString("0.000", [Globalization.CultureInfo]::InvariantCulture)
        Limit = 60
        ControllerProfileFixEnabled = $ControllerFixEnabled
        SkipUnityLauncher = $SkipUnityLauncher
        Language = $Language
        AllowUnsupportedExecutable = $AllowUnsupported
        DisplayMode = $DisplayMode
        UsePrimaryPhysicalResolution = $UsePrimaryPhysicalResolution
    }
    $Text = Get-Content -Raw -LiteralPath $Ini
    foreach ($Entry in $Values.GetEnumerator()) {
        $Pattern = "(?m)^$([regex]::Escape($Entry.Key))=.*$"
        if (-not [regex]::IsMatch($Text, $Pattern)) { throw "Missing $($Entry.Key) in $Ini" }
        $Text = [regex]::Replace($Text, $Pattern, "$($Entry.Key)=$($Entry.Value)", 1)
    }
    $Temporary = "$Ini.tmp"
    [IO.File]::WriteAllText($Temporary, $Text, [Text.UTF8Encoding]::new($false))
    Move-Item -Force -LiteralPath $Temporary -Destination $Ini
}

function Set-LauncherWrapper([bool]$Enabled) {
    if (-not (Test-Path -LiteralPath $LauncherTarget)) { throw "Unity launcher not found in: $LauncherDir" }
    New-Item -ItemType Directory -Force -Path $BackupDir | Out-Null
    if ($Enabled) {
        if (-not (Test-Path -LiteralPath $WrapperSource)) { throw "Direct-launch wrapper not found in: $PackageDir\bin" }
        $SourceHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $WrapperSource).Hash
        $TargetHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $LauncherTarget).Hash
        if ($SourceHash -eq $TargetHash) {
            [IO.File]::WriteAllText($LauncherHashMarker, $SourceHash,
                [Text.Encoding]::ASCII)
            return
        }
        if (-not (Test-Path -LiteralPath $LauncherBackup)) {
            Copy-Item -LiteralPath $LauncherTarget -Destination $LauncherBackup
        } else {
            # A managed backup is the original Unity launcher and must never be
            # replaced by an older MGS4 Ultra120 wrapper during package update.
            # Keep the displaced active file as an audit copy instead.
            $Timestamp = [DateTime]::UtcNow.ToString("yyyyMMddTHHmmssZ")
            Copy-Item -LiteralPath $LauncherTarget -Destination `
                (Join-Path $BackupDir "launcher.exe.displaced.$Timestamp")
        }
        Copy-Item -Force -LiteralPath $WrapperSource -Destination $LauncherTarget
        [IO.File]::WriteAllText($LauncherHashMarker, $SourceHash,
            [Text.Encoding]::ASCII)
    } elseif (Test-Path -LiteralPath $LauncherBackup) {
        $ManagedHashes = [Collections.Generic.HashSet[string]]::new(
            [StringComparer]::OrdinalIgnoreCase)
        if (Test-Path -LiteralPath $WrapperSource) {
            [void]$ManagedHashes.Add((Get-FileHash -Algorithm SHA256 `
                -LiteralPath $WrapperSource).Hash)
        }
        if (Test-Path -LiteralPath $LauncherHashMarker) {
            $RecordedHash = (Get-Content -Raw -LiteralPath `
                $LauncherHashMarker).Trim()
            if ($RecordedHash -match '^[0-9a-fA-F]{64}$') {
                [void]$ManagedHashes.Add($RecordedHash)
            }
        }
        $TargetHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $LauncherTarget).Hash
        if ($ManagedHashes.Contains($TargetHash)) {
            Move-Item -Force -LiteralPath $LauncherBackup -Destination $LauncherTarget
            Remove-Item -Force -LiteralPath $LauncherHashMarker `
                -ErrorAction SilentlyContinue
        } else {
            [Windows.Forms.MessageBox]::Show(
                "The launcher changed outside MGS4 Ultra120. It and the backup were left untouched.",
                "Launcher update detected", "OK", "Warning") | Out-Null
        }
    }
}

if ($PSBoundParameters.ContainsKey("Profile")) {
    if (Test-Mgs4Ultra120AutoHdrEnabled) {
        Write-Warning "Windows Auto HDR is enabled. Native multi-monitor testing reproduced a red sweep/flicker during focus changes; disable Auto HDR before launching MGS4."
    }
    $CurrentSkip = [int](Get-IniValue "SkipUnityLauncher")
    $CurrentLanguage = Get-IniValue "Language"
    $CurrentAllowUnsupported = [int](Get-IniValue "AllowUnsupportedExecutable")
    $CurrentDisplayMode = Get-IniValue "DisplayMode"
    $CurrentAutoResolution = [int](Get-IniValue "UsePrimaryPhysicalResolution")
    $CurrentWidth = [int](Get-IniValue "Width")
    $CurrentHeight = [int](Get-IniValue "Height")
    $RequestedDisplayMode = if ($PSBoundParameters.ContainsKey("WindowsDisplayMode")) {
        $WindowsDisplayMode
    } elseif ($Profile -eq "stable") {
        "Windowed"
    } else {
        $CurrentDisplayMode
    }
    switch ($Profile) {
        "stable" { Set-PatchSettings $CurrentWidth $CurrentHeight 1.200 1 0 inherit 0 1.50 1 1 $CurrentSkip $CurrentLanguage $CurrentAllowUnsupported $RequestedDisplayMode 1 }
        "fps-only-120" { Set-PatchSettings $CurrentWidth $CurrentHeight 1.000 0 0 inherit 0 1.50 0 0 $CurrentSkip $CurrentLanguage $CurrentAllowUnsupported $RequestedDisplayMode $CurrentAutoResolution }
        "ultrawide-only" { Set-PatchSettings $CurrentWidth $CurrentHeight 1.200 1 0 inherit 0 1.50 1 0 $CurrentSkip $CurrentLanguage $CurrentAllowUnsupported $RequestedDisplayMode $CurrentAutoResolution }
        "controller-fix-only" { Set-PatchSettings $CurrentWidth $CurrentHeight 1.000 0 0 inherit 0 1.50 0 1 $CurrentSkip $CurrentLanguage $CurrentAllowUnsupported $RequestedDisplayMode $CurrentAutoResolution }
    }
    Set-HudSettings 0 $CurrentWidth $CurrentHeight
    if ($Profile -eq "fps-only-120" -and
        -not (Test-MgsFpsUnlockInstalled $GameDir)) {
        Install-MgsFpsUnlock $GameDir
    }
    if (Test-MgsFpsUnlockInstalled $GameDir) {
        Set-MgsFpsUnlockTarget $GameDir 120
    }
    Set-Mgs4Ultra120WindowsDisplaySettings $GameDir `
        ([int](Get-IniValue "Width")) ([int](Get-IniValue "Height")) `
        (Get-IniValue "DisplayMode") (Get-IniValue "Language")
    Set-LauncherWrapper ((Get-IniValue "SkipUnityLauncher") -eq "1")
    Write-Host "Applied profile: $Profile"
    exit 0
}

Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class Mgs4DpiAwareness {
    [DllImport("user32.dll")]
    public static extern bool SetProcessDpiAwarenessContext(IntPtr value);

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct DEVMODE {
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string dmDeviceName;
        public ushort dmSpecVersion, dmDriverVersion, dmSize, dmDriverExtra;
        public uint dmFields;
        public int dmPositionX, dmPositionY;
        public uint dmDisplayOrientation, dmDisplayFixedOutput;
        public short dmColor, dmDuplex, dmYResolution, dmTTOption, dmCollate;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string dmFormName;
        public ushort dmLogPixels;
        public uint dmBitsPerPel, dmPelsWidth, dmPelsHeight;
        public uint dmDisplayFlags, dmDisplayFrequency;
        public uint dmICMMethod, dmICMIntent, dmMediaType, dmDitherType;
        public uint dmReserved1, dmReserved2, dmPanningWidth, dmPanningHeight;
    }

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    private static extern bool EnumDisplaySettings(
        string deviceName, int modeNumber, ref DEVMODE mode);

    public static int[] GetCurrentPhysicalMode(string deviceName) {
        var mode = new DEVMODE();
        mode.dmDeviceName = new string('\0', 32);
        mode.dmFormName = new string('\0', 32);
        mode.dmSize = (ushort)Marshal.SizeOf(typeof(DEVMODE));
        if (!EnumDisplaySettings(deviceName, -1, ref mode)) {
            throw new InvalidOperationException(
                "Cannot read the physical display mode for " + deviceName);
        }
        return new int[] { (int)mode.dmPelsWidth, (int)mode.dmPelsHeight };
    }
}
'@
[void][Mgs4DpiAwareness]::SetProcessDpiAwarenessContext([IntPtr](-4))
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[Windows.Forms.Application]::EnableVisualStyles()
$Ui = @{
        Form = "MGS4 Ultra120 $Mgs4Ultra120Version Configurator"
        Title = "Recommended configuration: save without changing anything"
        Ultrawide = "Enable ultrawide rendering and FOV correction"
        ImprovedFpsOn = "Improved FPS unlock installed (cipherxof/MGSFPSUnlock)"
        ImprovedFpsOff = "Core mode: optional MGSFPSUnlock is not installed"
        Width = "Physical output width"; Height = "Physical output height"
        AutoSize = "Primary monitor physical size"
        Fov = "FOV (1.20 tested 21:9 recommendation)"
        NativeFov = "Experimental native FOV (disable if unstable)"
        CinematicFov = "Experimental cinematic FOV (requires native FOV)"
        CinematicInherit = "Use gameplay FOV for cinematics"
        CinematicValue = "Separate cinematic FOV"
        CenteredHud = "Experimental centered 16:9 HUD (DX12 only)"
        Supersampling = "Experimental supersampling (off by default)"
        RenderScale = "Internal render scale"
        Presentation = "Windows presentation"
        DisplayItems = @("Windowed at native size (recommended)", "Exclusive fullscreen (advanced)")
        FrameRate = "Frame-rate limit"
        FpsItems = @("120 - corrected timing (recommended)", "60", "30")
        Controller = "Fix controller profile switching (recommended)"
        Skip = "Skip Unity launcher while keeping the Steam launch path"
        Language = "Game language"
        Supported = "Executable: supported and verified"
        UnsupportedStatus = "Executable: UNVERIFIED - unsafe override required"
        Unsupported = "Attempt unsupported executable (unsafe)"
        Defaults = "Use recommended settings"
        Save = "Save and close"; Close = "Cancel"
        Saved = "Settings saved. Start the game normally from Steam."
        SaveFailed = "Settings were not saved"
        AutoHdrTitle = "Auto HDR warning"
        NvidiaTitle = "NVIDIA multi-monitor warning"
        FullscreenTitle = "Exclusive fullscreen"
        SupersamplingTitle = "Experimental supersampling"
        ExperimentalTitle = "Experimental rendering options"
        UnsupportedTitle = "Unsupported executable"
        AutoHdrMessage = "Auto HDR is enabled. The multi-monitor test reproduced a red sweep during focus changes. It was not the final cause, but disabling it is recommended while testing. Save anyway?"
        NvidiaMessage = "On the tested NVIDIA system with 240/144 Hz monitors, G-SYNC/VRR caused display WATCHDOG events and a red sweep. Ten focus transitions were clean with G-SYNC disabled, including the final 3440x1440 test. This tool will not change the driver setting. Save anyway?"
        UnsupportedMessage = "Known offsets will be attempted on an unverified executable. This can crash the game. Continue under your responsibility?"
        FullscreenMessage = "Exclusive fullscreen can interact badly with HDR, VRR/G-SYNC or multiple monitors. The physical resolution will be synchronized first. Continue?"
        ExperimentalMessage = "The cinematic-FOV and centered-HUD options are experimental. Expanded cinematics can reveal actors, geometry or animation transitions earlier than intended, and some menus or text may still be misplaced by the HUD transform. If you see a problem, close the game and disable the affected option to return to the reference behavior. Continue?"
}

$Form = [Windows.Forms.Form]@{
    Text = $Ui.Form
    StartPosition = "CenterScreen"
    ClientSize = [Drawing.Size]::new(620, 930)
    FormBorderStyle = "FixedDialog"
    MaximizeBox = $false
    AutoScroll = $true
}
$Title = [Windows.Forms.Label]@{
    Text = $Ui.Title
    Location = [Drawing.Point]::new(24, 18); Size = [Drawing.Size]::new(560, 32)
    Font = [Drawing.Font]::new("Segoe UI", 15, [Drawing.FontStyle]::Bold)
}
$Form.Controls.Add($Title)

function Add-Label([string]$Text, [int]$Y) {
    $Label = [Windows.Forms.Label]@{ Text = $Text; Location = [Drawing.Point]::new(30, $Y); Size = [Drawing.Size]::new(225, 24) }
    $Form.Controls.Add($Label)
}
function Add-Numeric([int]$Y, [decimal]$Minimum, [decimal]$Maximum, [decimal]$Value, [int]$Decimals = 0) {
    $Control = [Windows.Forms.NumericUpDown]@{
        Location = [Drawing.Point]::new(270, $Y - 3)
        Size = [Drawing.Size]::new(150, 26)
    }
    # PowerShell hashtable property assignment order is not guaranteed. Setting
    # Value in the initializer can therefore happen before Maximum, while the
    # WinForms control still has its default maximum of 100. That made valid
    # widths such as 3440 or 5120 abort the configurator at startup.
    $Control.DecimalPlaces = $Decimals
    $Control.Minimum = $Minimum
    $Control.Maximum = $Maximum
    if ($Decimals -gt 0) { $Control.Increment = 0.05 }
    if ($Value -lt $Minimum) { $Value = $Minimum }
    if ($Value -gt $Maximum) { $Value = $Maximum }
    $Control.Value = $Value
    $Form.Controls.Add($Control); return $Control
}

$UltrawideBox = [Windows.Forms.CheckBox]@{
    Text = $Ui.Ultrawide
    Location = [Drawing.Point]::new(30, 62); Size = [Drawing.Size]::new(530, 25)
    Checked = (Get-IniValue "UltrawideEnabled") -eq "1"
}
$MgsFpsInstalled = Test-MgsFpsUnlockInstalled $GameDir
$ImprovedFpsLabel = [Windows.Forms.Label]@{
    Text = if ($MgsFpsInstalled) { $Ui.ImprovedFpsOn } else { $Ui.ImprovedFpsOff }
    Location = [Drawing.Point]::new(30, 90); Size = [Drawing.Size]::new(530, 25)
    ForeColor = if ($MgsFpsInstalled) { [Drawing.Color]::DarkGreen } else { [Drawing.Color]::DarkGoldenrod }
}
$Form.Controls.AddRange(@($UltrawideBox, $ImprovedFpsLabel))

Add-Label $Ui.Width 130; $WidthBox = Add-Numeric 130 640 16384 ([decimal](Get-IniValue "Width"))
Add-Label $Ui.Height 167; $HeightBox = Add-Numeric 167 480 16384 ([decimal](Get-IniValue "Height"))
$PrimaryScreen = [Windows.Forms.Screen]::PrimaryScreen
$PrimaryPhysicalMode = [Mgs4DpiAwareness]::GetCurrentPhysicalMode(
    $PrimaryScreen.DeviceName)
$PrimaryWidth = [decimal]$PrimaryPhysicalMode[0]
$PrimaryHeight = [decimal]$PrimaryPhysicalMode[1]
$NvidiaMultiDisplay = $false
try {
    $NvidiaMultiDisplay = [Windows.Forms.Screen]::AllScreens.Count -gt 1 -and
        [bool](Get-CimInstance Win32_VideoController -ErrorAction Stop |
            Where-Object Name -Match 'NVIDIA')
} catch {}
$AutoResolutionBox = [Windows.Forms.CheckBox]@{
    Text = "$($Ui.AutoSize)`n($PrimaryWidth x $PrimaryHeight)"
    Location = [Drawing.Point]::new(435, 126); Size = [Drawing.Size]::new(165, 58)
    Checked = (Get-IniValue "UsePrimaryPhysicalResolution") -eq "1"
}
$AutoResolutionBox.Add_CheckedChanged({
    $WidthBox.Enabled = -not $AutoResolutionBox.Checked
    $HeightBox.Enabled = -not $AutoResolutionBox.Checked
    if ($AutoResolutionBox.Checked) {
        $WidthBox.Value = $PrimaryWidth
        $HeightBox.Value = $PrimaryHeight
    }
})
$Form.Controls.Add($AutoResolutionBox)
if ($AutoResolutionBox.Checked) {
    $WidthBox.Value = $PrimaryWidth; $HeightBox.Value = $PrimaryHeight
    $WidthBox.Enabled = $false; $HeightBox.Enabled = $false
}
Add-Label $Ui.Fov 204; $FovBox = Add-Numeric 204 0.500 ([decimal]::MaxValue) ([decimal]::Parse((Get-IniValue "FOVMultiplier"), [Globalization.CultureInfo]::InvariantCulture)) 3
$NativeFovBox = [Windows.Forms.CheckBox]@{
    Text = $Ui.NativeFov
    Location = [Drawing.Point]::new(435, 199); Size = [Drawing.Size]::new(175, 42)
    Checked = (Get-IniValue "NativeCameraFOV") -eq "1"
}
$Form.Controls.Add($NativeFovBox)

$CinematicFovBox = [Windows.Forms.CheckBox]@{
    Text = $Ui.CinematicFov
    Location = [Drawing.Point]::new(30, 241); Size = [Drawing.Size]::new(330, 27)
    Checked = (Get-IniValue "ExperimentalCinematicFOV") -eq "1"
}
$CinematicInheritBox = [Windows.Forms.CheckBox]@{
    Text = $Ui.CinematicInherit
    Location = [Drawing.Point]::new(365, 241); Size = [Drawing.Size]::new(225, 27)
    Checked = (Get-IniValue "CinematicFOVMultiplier") -eq "inherit"
}
Add-Label $Ui.CinematicValue 278
$CurrentCinematicText = Get-IniValue "CinematicFOVMultiplier"
$CurrentCinematicValue = if ($CurrentCinematicText -eq "inherit") {
    [decimal]::Parse((Get-IniValue "FOVMultiplier"),
        [Globalization.CultureInfo]::InvariantCulture)
} else {
    [decimal]::Parse($CurrentCinematicText,
        [Globalization.CultureInfo]::InvariantCulture)
}
$CinematicFovValueBox = Add-Numeric 278 0.500 ([decimal]::MaxValue) `
    $CurrentCinematicValue 3
$CenteredHudBox = [Windows.Forms.CheckBox]@{
    Text = $Ui.CenteredHud
    Location = [Drawing.Point]::new(30, 315); Size = [Drawing.Size]::new(550, 27)
    Checked = (Get-HudIniValue "Enabled") -eq "1"
}
function Update-CinematicControls {
    $CinematicFovBox.Enabled = $NativeFovBox.Checked
    $CinematicInheritBox.Enabled = $NativeFovBox.Checked -and
        $CinematicFovBox.Checked
    $CinematicFovValueBox.Enabled = $NativeFovBox.Checked -and
        $CinematicFovBox.Checked -and
        -not $CinematicInheritBox.Checked
}
$NativeFovBox.Add_CheckedChanged({
    if (-not $NativeFovBox.Checked) { $CinematicFovBox.Checked = $false }
    Update-CinematicControls
})
$CinematicFovBox.Add_CheckedChanged({ Update-CinematicControls })
$CinematicInheritBox.Add_CheckedChanged({ Update-CinematicControls })
$Form.Controls.AddRange(@($CinematicFovBox, $CinematicInheritBox,
    $CenteredHudBox))
Update-CinematicControls

$SupersamplingBox = [Windows.Forms.CheckBox]@{
    Text = $Ui.Supersampling
    Location = [Drawing.Point]::new(30, 352); Size = [Drawing.Size]::new(550, 27)
    Checked = (Get-IniValue "SupersamplingEnabled") -eq "1"
}
$Form.Controls.Add($SupersamplingBox)
Add-Label $Ui.RenderScale 389
$RenderScaleBox = Add-Numeric 389 1.00 1000000.00 ([decimal]::Parse(
    (Get-IniValue "RenderScale"), [Globalization.CultureInfo]::InvariantCulture)) 2
$RenderPreview = [Windows.Forms.Label]@{
    Location = [Drawing.Point]::new(435, 385); Size = [Drawing.Size]::new(165, 44)
    ForeColor = [Drawing.Color]::DarkGoldenrod
}
$Form.Controls.Add($RenderPreview)
function Update-SupersamplingPreview {
    $RenderScaleBox.Enabled = $SupersamplingBox.Checked
    $Scale = if ($SupersamplingBox.Checked) {
        [decimal]$RenderScaleBox.Value
    } else {
        [decimal]1.0
    }
    $RenderWidth = [Math]::Round([decimal]$WidthBox.Value * $Scale,
        0, [MidpointRounding]::AwayFromZero)
    $RenderHeight = [Math]::Round([decimal]$HeightBox.Value * $Scale,
        0, [MidpointRounding]::AwayFromZero)
    if ($SupersamplingBox.Checked -and $RenderWidth -ge 4096) {
        $RenderPreview.Text = "Internal: $RenderWidth x $RenderHeight`nCrosshair risk: keep width below 4096"
        $RenderPreview.ForeColor = [Drawing.Color]::DarkRed
    } else {
        $RenderPreview.Text = "Internal: $RenderWidth x $RenderHeight"
        $RenderPreview.ForeColor = [Drawing.Color]::DarkGoldenrod
    }
}
$SupersamplingBox.Add_CheckedChanged({ Update-SupersamplingPreview })
$RenderScaleBox.Add_ValueChanged({ Update-SupersamplingPreview })
$WidthBox.Add_ValueChanged({ Update-SupersamplingPreview })
$HeightBox.Add_ValueChanged({ Update-SupersamplingPreview })
Update-SupersamplingPreview

Add-Label $Ui.Presentation 426
$DisplayModeBox = [Windows.Forms.ComboBox]@{ Location = [Drawing.Point]::new(270, 423); Size = [Drawing.Size]::new(300, 28); DropDownStyle = "DropDownList" }
[void]$DisplayModeBox.Items.AddRange($Ui.DisplayItems)
$DisplayModeBox.SelectedIndex = if ((Get-IniValue "DisplayMode") -eq "Fullscreen") { 1 } else { 0 }
$Form.Controls.Add($DisplayModeBox)

Add-Label $Ui.FrameRate 463
$FpsBox = [Windows.Forms.ComboBox]@{ Location = [Drawing.Point]::new(270, 460); Size = [Drawing.Size]::new(150, 28); DropDownStyle = "DropDownList" }
[void]$FpsBox.Items.AddRange($Ui.FpsItems)
$CurrentFps = Get-MgsFpsUnlockTarget $GameDir
$FpsBox.SelectedIndex = if ($CurrentFps -eq 60) { 1 } elseif ($CurrentFps -eq 30) { 2 } else { 0 }
$FpsBox.Enabled = $MgsFpsInstalled
$Form.Controls.Add($FpsBox)

$ControllerFixBox = [Windows.Forms.CheckBox]@{
    Text = $Ui.Controller
    Location = [Drawing.Point]::new(30, 507); Size = [Drawing.Size]::new(550, 27)
    Checked = (Get-IniValue "ControllerProfileFixEnabled") -eq "1"
}
$SkipLauncherBox = [Windows.Forms.CheckBox]@{
    Text = $Ui.Skip
    Location = [Drawing.Point]::new(30, 539); Size = [Drawing.Size]::new(560, 27)
    Checked = (Get-IniValue "SkipUnityLauncher") -eq "1"
}
$Form.Controls.AddRange(@($ControllerFixBox, $SkipLauncherBox))

Add-Label $Ui.Language 580
$LanguageBox = [Windows.Forms.ComboBox]@{ Location = [Drawing.Point]::new(270, 577); Size = [Drawing.Size]::new(150, 28); DropDownStyle = "DropDownList" }
$LanguageCodes = [ordered]@{
    "English (en)" = "en"
    "Spanish (sp)" = "sp"
    "French (fr)" = "fr"
    "Italian (it)" = "it"
    "German (gr)" = "gr"
    "Japanese (jp)" = "jp"
    "Portuguese (pt)" = "pt"
}
[void]$LanguageBox.Items.AddRange([string[]]$LanguageCodes.Keys)
$CurrentLanguageCode = Get-IniValue "Language"
if ($CurrentLanguageCode -eq "ge") { $CurrentLanguageCode = "gr" }
$LanguageBox.SelectedItem = [string]($LanguageCodes.GetEnumerator() |
    Where-Object Value -eq $CurrentLanguageCode | Select-Object -First 1 -ExpandProperty Key)
if ($LanguageBox.SelectedIndex -lt 0) { $LanguageBox.SelectedItem = "English (en)" }
$Form.Controls.Add($LanguageBox)

$Hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Exe).Hash.ToLowerInvariant()
$Supported = $Hash -eq $KnownExeSha256
$Status = [Windows.Forms.Label]@{
    Text = if ($Supported) { $Ui.Supported } else { $Ui.UnsupportedStatus }
    Location = [Drawing.Point]::new(30, 627); Size = [Drawing.Size]::new(550, 28)
    ForeColor = if ($Supported) { [Drawing.Color]::DarkGreen } else { [Drawing.Color]::DarkRed }
    Font = [Drawing.Font]::new("Segoe UI", 9, [Drawing.FontStyle]::Bold)
}
$Warning = [Windows.Forms.Label]@{
    Text = if (Test-Mgs4Ultra120AutoHdrEnabled) {
        "WARNING: Auto HDR is enabled. On NVIDIA multi-monitor systems, also disable G-SYNC/VRR for MGS4. Native-size windowed is recommended."
    } elseif ($NvidiaMultiDisplay) {
        "NVIDIA MULTI-MONITOR WARNING: the 240/144 Hz test required G-SYNC/VRR off to prevent red sweeps and WATCHDOG events. The configurator never changes the driver."
    } else {
        $(if ($MgsFpsInstalled) {
            "Native-size windowed and corrected 120 FPS are recommended. Cinematic FOV and centered HUD are experimental and disabled by recommended settings."
        } else {
            "Core mode uses the game's normal FPS settings. Reopen Easy Setup with the optional 120 FPS box checked to add MGSFPSUnlock."
        })
    }
    Location = [Drawing.Point]::new(30, 661); Size = [Drawing.Size]::new(550, 92)
    ForeColor = if ((Test-Mgs4Ultra120AutoHdrEnabled) -or $NvidiaMultiDisplay) { [Drawing.Color]::DarkRed } else { [Drawing.Color]::Black }
}
$Form.Controls.AddRange(@($Status, $Warning))

$UnsupportedBox = [Windows.Forms.CheckBox]@{
    Text = $Ui.Unsupported
    Location = [Drawing.Point]::new(30, 760); Size = [Drawing.Size]::new(560, 27)
    Checked = (Get-IniValue "AllowUnsupportedExecutable") -eq "1"
}
$Form.Controls.Add($UnsupportedBox)

$StableButton = [Windows.Forms.Button]@{ Text = $Ui.Defaults; Location = [Drawing.Point]::new(30, 858); Size = [Drawing.Size]::new(200, 38) }
$SaveButton = [Windows.Forms.Button]@{ Text = $Ui.Save; Location = [Drawing.Point]::new(300, 858); Size = [Drawing.Size]::new(150, 38); BackColor = [Drawing.Color]::RoyalBlue; ForeColor = [Drawing.Color]::White; Font = [Drawing.Font]::new("Segoe UI", 9, [Drawing.FontStyle]::Bold) }
$CloseButton = [Windows.Forms.Button]@{ Text = $Ui.Close; Location = [Drawing.Point]::new(465, 858); Size = [Drawing.Size]::new(105, 38) }
$StableButton.Add_Click({
    $AutoResolutionBox.Checked = $true
    $WidthBox.Value = $PrimaryWidth; $HeightBox.Value = $PrimaryHeight; $FovBox.Value = 1.20
    $NativeFovBox.Checked = $true
    $CinematicFovBox.Checked = $false
    $CinematicInheritBox.Checked = $true
    $CinematicFovValueBox.Value = 1.20
    $CenteredHudBox.Checked = $false
    $DisplayModeBox.SelectedIndex = 0
    $SupersamplingBox.Checked = $false; $RenderScaleBox.Value = 1.50
    $FpsBox.SelectedIndex = 0
    $UltrawideBox.Checked = $true
    $ControllerFixBox.Checked = $true; $SkipLauncherBox.Checked = $true
    # Language is a user preference, not a rendering default. Keep the current
    # selection when recommended settings are restored.
    $UnsupportedBox.Checked = $false
})
$script:Mgs4Ultra120SettingsSaved = $false
$SaveButton.Add_Click({
  try {
    if (Test-Mgs4Ultra120AutoHdrEnabled) {
        $Answer = [Windows.Forms.MessageBox]::Show(
            $Ui.AutoHdrMessage, $Ui.AutoHdrTitle, "YesNo", "Warning")
        if ($Answer -ne "Yes") { return }
    }
    if ($NvidiaMultiDisplay -and -not (Test-Mgs4Ultra120AutoHdrEnabled)) {
        $Answer = [Windows.Forms.MessageBox]::Show(
            $Ui.NvidiaMessage, $Ui.NvidiaTitle, "YesNo", "Warning")
        if ($Answer -ne "Yes") { return }
    }
    if ($UnsupportedBox.Checked) {
        $Answer = [Windows.Forms.MessageBox]::Show(
            $Ui.UnsupportedMessage, $Ui.UnsupportedTitle, "YesNo", "Warning")
        if ($Answer -ne "Yes") { return }
    }
    if ($SupersamplingBox.Checked) {
        $RenderWidth = [Math]::Round([decimal]$WidthBox.Value *
            [decimal]$RenderScaleBox.Value, 0,
            [MidpointRounding]::AwayFromZero)
        $RenderHeight = [Math]::Round([decimal]$HeightBox.Value *
            [decimal]$RenderScaleBox.Value, 0,
            [MidpointRounding]::AwayFromZero)
        $Answer = [Windows.Forms.MessageBox]::Show(
            "Experimental supersampling will render internally at $RenderWidth x $RenderHeight and present at $([int]$WidthBox.Value) x $([int]$HeightBox.Value). Known crosshair limitation: an internal width of exactly 4096 can flicker, and higher widths can make the reticle disappear depending on aiming depth. Keep the internal width below 4096; 3956 x 1656 is the validated stable setting for 3440 x 1440 output. It does not require AMD VSR, NVIDIA DSR or a desktop-resolution change. It can sharply reduce performance, exhaust VRAM, crash the game or graphics driver, and make the Steam overlay/HUD smaller because the complete frame is downsampled. No automatic limit is applied. Windowed presentation is recommended. Continue under your responsibility?",
            $Ui.SupersamplingTitle, "YesNo", "Warning")
        if ($Answer -ne "Yes") { return }
    }
    if ($CinematicFovBox.Checked -or $CenteredHudBox.Checked) {
        $Answer = [Windows.Forms.MessageBox]::Show(
            $Ui.ExperimentalMessage, $Ui.ExperimentalTitle, "YesNo", "Warning")
        if ($Answer -ne "Yes") { return }
    }
    if ($DisplayModeBox.SelectedIndex -eq 1) {
        $Answer = [Windows.Forms.MessageBox]::Show(
            $Ui.FullscreenMessage, $Ui.FullscreenTitle, "YesNo", "Warning")
        if ($Answer -ne "Yes") { return }
    }
    $Fps = if ($FpsBox.SelectedIndex -eq 1) { 60 } elseif ($FpsBox.SelectedIndex -eq 2) { 30 } else { 120 }
    if ($AutoResolutionBox.Checked) {
        $WidthBox.Value = $PrimaryWidth; $HeightBox.Value = $PrimaryHeight
    }
    $DisplayMode = if ($DisplayModeBox.SelectedIndex -eq 1) { "Fullscreen" } else { "Windowed" }
    $LanguageCode = [string]$LanguageCodes[[string]$LanguageBox.SelectedItem]
    $CinematicValue = if ($CinematicInheritBox.Checked) {
        "inherit"
    } else {
        $CinematicFovValueBox.Value.ToString(
            "0.000", [Globalization.CultureInfo]::InvariantCulture)
    }
    Set-PatchSettings ([int]$WidthBox.Value) ([int]$HeightBox.Value) `
        $FovBox.Value ([int]$NativeFovBox.Checked) `
        ([int]$CinematicFovBox.Checked) $CinematicValue `
        ([int]$SupersamplingBox.Checked) $RenderScaleBox.Value `
        ([int]$UltrawideBox.Checked) ([int]$ControllerFixBox.Checked) `
        ([int]$SkipLauncherBox.Checked) $LanguageCode `
        ([int]$UnsupportedBox.Checked) $DisplayMode `
        ([int]$AutoResolutionBox.Checked)
    Set-HudSettings ([int]$CenteredHudBox.Checked) `
        ([int]$WidthBox.Value) ([int]$HeightBox.Value)
    Set-Mgs4Ultra120WindowsDisplaySettings $GameDir `
        ([int]$WidthBox.Value) ([int]$HeightBox.Value) $DisplayMode $LanguageCode
    Set-LauncherWrapper $SkipLauncherBox.Checked
    if ($MgsFpsInstalled) {
        Set-MgsFpsUnlockTarget $GameDir $Fps
    }
    $script:Mgs4Ultra120SettingsSaved = $true
    [Windows.Forms.MessageBox]::Show($Ui.Saved, "MGS4 Ultra120 $Mgs4Ultra120Version", "OK", "Information") | Out-Null
    $Form.Close()
  } catch {
    [Windows.Forms.MessageBox]::Show($_.Exception.Message, $Ui.SaveFailed,
        "OK", "Error") | Out-Null
  }
})
$CloseButton.Add_Click({ $Form.Close() })
$Form.Controls.AddRange(@($StableButton, $SaveButton, $CloseButton))
$Form.AcceptButton = $SaveButton
$Form.CancelButton = $CloseButton
[void]$Form.ShowDialog()
Write-Output $script:Mgs4Ultra120SettingsSaved
