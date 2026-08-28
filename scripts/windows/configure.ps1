param(
    [ValidateSet("stable", "ui-safe", "120", "120-ui", "fps-only-120", "ultrawide-only", "controller-fix-only")]
    [string]$Profile,
    [string]$GameDir = "${env:ProgramFiles(x86)}\Steam\steamapps\common\METAL GEAR SOLID 4\MGS4"
)
$ErrorActionPreference = "Stop"
$KnownExeSha256 = "9e8df67ea7f41e7f8306ce1a77584707209069b3c75389b3f00445efe459fe41"
$Ini = Join-Path $GameDir "mgs4_ultrawide.ini"
$Exe = Join-Path $GameDir "mgs4.exe"
$PackageDir = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$InstallDir = Split-Path -Parent $GameDir
$LauncherDir = Join-Path $InstallDir "Launcher"
$LauncherTarget = Join-Path $LauncherDir "launcher.exe"
$BackupDir = Join-Path $GameDir ".mgs4ultra120-backup"
$LauncherBackup = Join-Path $BackupDir "launcher.exe.preinstall"
$WrapperSource = Join-Path $PackageDir "bin\launcher.exe"

if (-not (Test-Path -LiteralPath $Ini)) { throw "mgs4_ultrawide.ini not found in: $GameDir" }
if (-not (Test-Path -LiteralPath $Exe)) { throw "mgs4.exe not found in: $GameDir" }
if (Get-Process mgs4 -ErrorAction SilentlyContinue) { throw "Exit the game before changing settings." }

function Get-IniValue([string]$Key) {
    $Match = [regex]::Match((Get-Content -Raw -LiteralPath $Ini), "(?m)^$([regex]::Escape($Key))=(.*)$")
    if (-not $Match.Success) { throw "Missing $Key in $Ini; reinstall the current patch." }
    return $Match.Groups[1].Value.Trim()
}

function Set-PatchSettings([int]$Width, [int]$Height, [decimal]$Fov,
                           [int]$Fps, [int]$Ui, [int]$UltrawideEnabled,
                           [int]$FpsOverrideEnabled, [int]$ControllerFixEnabled,
                           [int]$SkipUnityLauncher, [string]$Language,
                           [string]$ToggleHotkey, [int]$AllowUnsupported) {
    if ($Width -lt 640 -or $Width -gt 16384 -or $Height -lt 480 -or $Height -gt 16384) {
        throw "Width/height are outside the allowed range."
    }
    if ($Fov -lt 0.5 -or $Fov -gt 2.0) { throw "FOV multiplier must be between 0.5 and 2.0." }
    if ($Fps -notin @(30, 60, 120)) { throw "FPS must be 30, 60, or 120." }
    if ($Language -notin @("en", "sp", "fr", "it", "ge", "jp")) { throw "Unsupported direct-launch language." }
    if ($ToggleHotkey -notin @("Off", "F6", "F7", "F8", "F9", "F10", "F11", "F12")) { throw "Unsupported FPS toggle hotkey." }
    $Values = [ordered]@{
        UltrawideEnabled = $UltrawideEnabled
        FPSOverrideEnabled = $FpsOverrideEnabled
        Width = $Width; Height = $Height
        FOVMultiplier = $Fov.ToString("0.000", [Globalization.CultureInfo]::InvariantCulture)
        Limit = $Fps; ConstrainUITo16x9 = $Ui
        ControllerProfileFixEnabled = $ControllerFixEnabled
        SkipUnityLauncher = $SkipUnityLauncher
        Language = $Language
        ToggleHotkey = $ToggleHotkey
        AllowUnsupportedExecutable = $AllowUnsupported
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
        if ($SourceHash -eq $TargetHash) { return }
        if (-not (Test-Path -LiteralPath $LauncherBackup)) {
            Copy-Item -LiteralPath $LauncherTarget -Destination $LauncherBackup
        } else {
            $Timestamp = [DateTime]::UtcNow.ToString("yyyyMMddTHHmmssZ")
            Copy-Item -LiteralPath $LauncherBackup -Destination "$LauncherBackup.$Timestamp"
            Copy-Item -Force -LiteralPath $LauncherTarget -Destination $LauncherBackup
        }
        Copy-Item -Force -LiteralPath $WrapperSource -Destination $LauncherTarget
    } elseif (Test-Path -LiteralPath $LauncherBackup) {
        if (-not (Test-Path -LiteralPath $WrapperSource)) {
            throw "Direct-launch wrapper is missing; refusing an ambiguous launcher restore."
        }
        $SourceHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $WrapperSource).Hash
        $TargetHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $LauncherTarget).Hash
        if ($SourceHash -eq $TargetHash) {
            Move-Item -Force -LiteralPath $LauncherBackup -Destination $LauncherTarget
        } else {
            [Windows.Forms.MessageBox]::Show(
                "The launcher changed outside MGS4 Ultra120. It and the backup were left untouched.",
                "Launcher update detected", "OK", "Warning") | Out-Null
        }
    }
}

if ($PSBoundParameters.ContainsKey("Profile")) {
    $CurrentSkip = [int](Get-IniValue "SkipUnityLauncher")
    $CurrentLanguage = Get-IniValue "Language"
    $CurrentHotkey = Get-IniValue "ToggleHotkey"
    $CurrentAllowUnsupported = [int](Get-IniValue "AllowUnsupportedExecutable")
    switch ($Profile) {
        "stable" { Set-PatchSettings 3440 1440 1.000 60 0 1 1 1 $CurrentSkip $CurrentLanguage $CurrentHotkey $CurrentAllowUnsupported }
        "ui-safe" { Set-PatchSettings 3440 1440 1.000 60 1 1 1 1 $CurrentSkip $CurrentLanguage $CurrentHotkey $CurrentAllowUnsupported }
        "120" { Set-PatchSettings 3440 1440 1.000 120 0 1 1 1 $CurrentSkip $CurrentLanguage $CurrentHotkey $CurrentAllowUnsupported }
        "120-ui" { Set-PatchSettings 3440 1440 1.000 120 1 1 1 1 $CurrentSkip $CurrentLanguage $CurrentHotkey $CurrentAllowUnsupported }
        "fps-only-120" { Set-PatchSettings 3440 1440 1.000 120 0 0 1 0 $CurrentSkip $CurrentLanguage $CurrentHotkey $CurrentAllowUnsupported }
        "ultrawide-only" { Set-PatchSettings 3440 1440 1.000 60 0 1 0 0 $CurrentSkip $CurrentLanguage $CurrentHotkey $CurrentAllowUnsupported }
        "controller-fix-only" { Set-PatchSettings 3440 1440 1.000 60 0 0 0 1 $CurrentSkip $CurrentLanguage $CurrentHotkey $CurrentAllowUnsupported }
    }
    Write-Host "Applied profile: $Profile"
    exit 0
}

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[Windows.Forms.Application]::EnableVisualStyles()

$Form = [Windows.Forms.Form]@{
    Text = "MGS4 Ultra120 Configurator"
    StartPosition = "CenterScreen"
    ClientSize = [Drawing.Size]::new(620, 765)
    FormBorderStyle = "FixedDialog"
    MaximizeBox = $false
}
$Title = [Windows.Forms.Label]@{
    Text = "Ultrawide rendering and launch profile"
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
    $Control.Value = $Value
    $Form.Controls.Add($Control); return $Control
}

$UltrawideBox = [Windows.Forms.CheckBox]@{
    Text = "Enable ultrawide rendering and FOV module"
    Location = [Drawing.Point]::new(30, 62); Size = [Drawing.Size]::new(530, 25)
    Checked = (Get-IniValue "UltrawideEnabled") -eq "1"
}
$FpsOverrideBox = [Windows.Forms.CheckBox]@{
    Text = "Enable independent FPS override"
    Location = [Drawing.Point]::new(30, 90); Size = [Drawing.Size]::new(530, 25)
    Checked = (Get-IniValue "FPSOverrideEnabled") -eq "1"
}
$Form.Controls.AddRange(@($UltrawideBox, $FpsOverrideBox))

Add-Label "Render width" 130; $WidthBox = Add-Numeric 130 640 16384 ([decimal](Get-IniValue "Width"))
Add-Label "Render height" 167; $HeightBox = Add-Numeric 167 480 16384 ([decimal](Get-IniValue "Height"))
Add-Label "FOV multiplier (1.00 = original)" 204; $FovBox = Add-Numeric 204 0.50 2.00 ([decimal]::Parse((Get-IniValue "FOVMultiplier"), [Globalization.CultureInfo]::InvariantCulture)) 2

Add-Label "Frame-rate limit" 241
$FpsBox = [Windows.Forms.ComboBox]@{ Location = [Drawing.Point]::new(270, 238); Size = [Drawing.Size]::new(150, 28); DropDownStyle = "DropDownList" }
[void]$FpsBox.Items.AddRange(@("60 - stable", "120 - experimental", "30"))
$CurrentFps = Get-IniValue "Limit"; $FpsBox.SelectedIndex = if ($CurrentFps -eq "120") { 1 } elseif ($CurrentFps -eq "30") { 2 } else { 0 }
$Form.Controls.Add($FpsBox)

$UiBox = [Windows.Forms.CheckBox]@{
    Text = "Center identified UI draws in 16:9 (experimental)"
    Location = [Drawing.Point]::new(30, 285); Size = [Drawing.Size]::new(530, 28)
    Checked = (Get-IniValue "ConstrainUITo16x9") -eq "1"
}
$Form.Controls.Add($UiBox)

Add-Label "60/120 FPS toggle hotkey" 327
$HotkeyBox = [Windows.Forms.ComboBox]@{ Location = [Drawing.Point]::new(270, 324); Size = [Drawing.Size]::new(150, 28); DropDownStyle = "DropDownList" }
[void]$HotkeyBox.Items.AddRange(@("F10", "Off", "F6", "F7", "F8", "F9", "F11", "F12"))
$HotkeyBox.SelectedItem = Get-IniValue "ToggleHotkey"
if ($HotkeyBox.SelectedIndex -lt 0) { $HotkeyBox.SelectedItem = "F10" }
$Form.Controls.Add($HotkeyBox)

$ControllerFixBox = [Windows.Forms.CheckBox]@{
    Text = "Fix controller profile switching (recommended)"
    Location = [Drawing.Point]::new(30, 365); Size = [Drawing.Size]::new(550, 27)
    Checked = (Get-IniValue "ControllerProfileFixEnabled") -eq "1"
}
$SkipLauncherBox = [Windows.Forms.CheckBox]@{
    Text = "Skip the Unity launcher while keeping the normal Steam launch path"
    Location = [Drawing.Point]::new(30, 397); Size = [Drawing.Size]::new(560, 27)
    Checked = (Get-IniValue "SkipUnityLauncher") -eq "1"
}
$Form.Controls.AddRange(@($ControllerFixBox, $SkipLauncherBox))

Add-Label "Direct-launch language" 438
$LanguageBox = [Windows.Forms.ComboBox]@{ Location = [Drawing.Point]::new(270, 435); Size = [Drawing.Size]::new(150, 28); DropDownStyle = "DropDownList" }
[void]$LanguageBox.Items.AddRange(@("en", "sp", "fr", "it", "ge", "jp"))
$LanguageBox.SelectedItem = Get-IniValue "Language"
if ($LanguageBox.SelectedIndex -lt 0) { $LanguageBox.SelectedItem = "en" }
$Form.Controls.Add($LanguageBox)

$Hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Exe).Hash.ToLowerInvariant()
$Supported = $Hash -eq $KnownExeSha256
$Status = [Windows.Forms.Label]@{
    Text = if ($Supported) { "Executable: supported" } else { "Executable: UNVERIFIED - blocked unless unsafe override is enabled" }
    Location = [Drawing.Point]::new(30, 485); Size = [Drawing.Size]::new(550, 28)
    ForeColor = if ($Supported) { [Drawing.Color]::DarkGreen } else { [Drawing.Color]::DarkRed }
    Font = [Drawing.Font]::new("Segoe UI", 9, [Drawing.FontStyle]::Bold)
}
$Warning = [Windows.Forms.Label]@{
    Text = "All checkboxes are independent. 120 FPS can stall scripted scenes. The controller fix preserves the active native pad profile; it does not emulate a controller. UI safe area remains experimental."
    Location = [Drawing.Point]::new(30, 519); Size = [Drawing.Size]::new(550, 82)
}
$Form.Controls.AddRange(@($Status, $Warning))

$UnsupportedBox = [Windows.Forms.CheckBox]@{
    Text = "Attempt unsupported executable (unsafe; signatures still checked)"
    Location = [Drawing.Point]::new(30, 605); Size = [Drawing.Size]::new(560, 27)
    Checked = (Get-IniValue "AllowUnsupportedExecutable") -eq "1"
}
$Form.Controls.Add($UnsupportedBox)

$StableButton = [Windows.Forms.Button]@{ Text = "Restore stable defaults"; Location = [Drawing.Point]::new(30, 695); Size = [Drawing.Size]::new(170, 38) }
$SaveButton = [Windows.Forms.Button]@{ Text = "Save settings"; Location = [Drawing.Point]::new(325, 695); Size = [Drawing.Size]::new(125, 38) }
$CloseButton = [Windows.Forms.Button]@{ Text = "Close"; Location = [Drawing.Point]::new(465, 695); Size = [Drawing.Size]::new(105, 38) }
$StableButton.Add_Click({
    $WidthBox.Value = 3440; $HeightBox.Value = 1440; $FovBox.Value = 1.00
    $FpsBox.SelectedIndex = 0; $UiBox.Checked = $false
    $UltrawideBox.Checked = $true; $FpsOverrideBox.Checked = $true
    $ControllerFixBox.Checked = $true; $SkipLauncherBox.Checked = $false
    $HotkeyBox.SelectedItem = "F10"; $LanguageBox.SelectedItem = "en"
    $UnsupportedBox.Checked = $false
})
$SaveButton.Add_Click({
    if ($UnsupportedBox.Checked) {
        $Answer = [Windows.Forms.MessageBox]::Show(
            "Known code and data offsets will be attempted on an unverified executable. This may crash the game or corrupt its process state. Continue under your responsibility?",
            "Unsupported executable override", "YesNo", "Warning")
        if ($Answer -ne "Yes") { return }
    }
    if ($FpsOverrideBox.Checked -and $FpsBox.SelectedIndex -eq 1) {
        $Answer = [Windows.Forms.MessageBox]::Show(
            "120 FPS has reproduced a scripted-scene stall. Enable it anyway?",
            "Experimental 120 FPS", "YesNo", "Warning")
        if ($Answer -ne "Yes") { return }
    }
    $Fps = if ($FpsBox.SelectedIndex -eq 1) { 120 } elseif ($FpsBox.SelectedIndex -eq 2) { 30 } else { 60 }
    Set-LauncherWrapper $SkipLauncherBox.Checked
    Set-PatchSettings ([int]$WidthBox.Value) ([int]$HeightBox.Value) $FovBox.Value $Fps ([int]$UiBox.Checked) ([int]$UltrawideBox.Checked) ([int]$FpsOverrideBox.Checked) ([int]$ControllerFixBox.Checked) ([int]$SkipLauncherBox.Checked) ([string]$LanguageBox.SelectedItem) ([string]$HotkeyBox.SelectedItem) ([int]$UnsupportedBox.Checked)
    [Windows.Forms.MessageBox]::Show("Settings saved. Restart the game to apply them.", "MGS4 Ultra120", "OK", "Information") | Out-Null
})
$CloseButton.Add_Click({ $Form.Close() })
$Form.Controls.AddRange(@($StableButton, $SaveButton, $CloseButton))
[void]$Form.ShowDialog()
