param(
    [ValidateSet("stable", "ui-safe", "120", "120-ui", "fps-only-120", "ultrawide-only")]
    [string]$Profile,
    [string]$GameDir = "${env:ProgramFiles(x86)}\Steam\steamapps\common\METAL GEAR SOLID 4\MGS4"
)
$ErrorActionPreference = "Stop"
$KnownExeSha256 = "9e8df67ea7f41e7f8306ce1a77584707209069b3c75389b3f00445efe459fe41"
$Ini = Join-Path $GameDir "mgs4_ultrawide.ini"
$Exe = Join-Path $GameDir "mgs4.exe"

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
                           [int]$FpsOverrideEnabled) {
    if ($Width -lt 640 -or $Width -gt 16384 -or $Height -lt 480 -or $Height -gt 16384) {
        throw "Width/height are outside the allowed range."
    }
    if ($Fov -lt 0.5 -or $Fov -gt 2.0) { throw "FOV multiplier must be between 0.5 and 2.0." }
    if ($Fps -notin @(30, 60, 120)) { throw "FPS must be 30, 60, or 120." }
    $Values = [ordered]@{
        UltrawideEnabled = $UltrawideEnabled
        FPSOverrideEnabled = $FpsOverrideEnabled
        Width = $Width; Height = $Height
        FOVMultiplier = $Fov.ToString("0.000", [Globalization.CultureInfo]::InvariantCulture)
        Limit = $Fps; ConstrainUITo16x9 = $Ui
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

if ($PSBoundParameters.ContainsKey("Profile")) {
    switch ($Profile) {
        "stable" { Set-PatchSettings 3440 1440 1.000 60 0 1 1 }
        "ui-safe" { Set-PatchSettings 3440 1440 1.000 60 1 1 1 }
        "120" { Set-PatchSettings 3440 1440 1.000 120 0 1 1 }
        "120-ui" { Set-PatchSettings 3440 1440 1.000 120 1 1 1 }
        "fps-only-120" { Set-PatchSettings 3440 1440 1.000 120 0 0 1 }
        "ultrawide-only" { Set-PatchSettings 3440 1440 1.000 60 0 1 0 }
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
    ClientSize = [Drawing.Size]::new(620, 570)
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
        Location = [Drawing.Point]::new(270, $Y - 3); Size = [Drawing.Size]::new(150, 26)
        Minimum = $Minimum; Maximum = $Maximum; Value = $Value; DecimalPlaces = $Decimals
    }
    if ($Decimals -gt 0) { $Control.Increment = 0.05 }
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

$Hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Exe).Hash.ToLowerInvariant()
$Supported = $Hash -eq $KnownExeSha256
$Status = [Windows.Forms.Label]@{
    Text = if ($Supported) { "Executable: supported" } else { "Executable: UNSUPPORTED - the DLL will fail safely" }
    Location = [Drawing.Point]::new(30, 350); Size = [Drawing.Size]::new(550, 28)
    ForeColor = if ($Supported) { [Drawing.Color]::DarkGreen } else { [Drawing.Color]::DarkRed }
    Font = [Drawing.Font]::new("Segoe UI", 9, [Drawing.FontStyle]::Bold)
}
$Warning = [Windows.Forms.Label]@{
    Text = "Ultrawide/FOV and FPS are independent modules. Disabled modules leave the corresponding game behavior untouched. 120 FPS can stall scripted scenes."
    Location = [Drawing.Point]::new(30, 384); Size = [Drawing.Size]::new(550, 58)
}
$Form.Controls.AddRange(@($Status, $Warning))

$StableButton = [Windows.Forms.Button]@{ Text = "Restore stable defaults"; Location = [Drawing.Point]::new(30, 500); Size = [Drawing.Size]::new(170, 38) }
$SaveButton = [Windows.Forms.Button]@{ Text = "Save settings"; Location = [Drawing.Point]::new(325, 500); Size = [Drawing.Size]::new(125, 38) }
$CloseButton = [Windows.Forms.Button]@{ Text = "Close"; Location = [Drawing.Point]::new(465, 500); Size = [Drawing.Size]::new(105, 38) }
$StableButton.Add_Click({
    $WidthBox.Value = 3440; $HeightBox.Value = 1440; $FovBox.Value = 1.00
    $FpsBox.SelectedIndex = 0; $UiBox.Checked = $false
    $UltrawideBox.Checked = $true; $FpsOverrideBox.Checked = $true
})
$SaveButton.Add_Click({
    if ($FpsOverrideBox.Checked -and $FpsBox.SelectedIndex -eq 1) {
        $Answer = [Windows.Forms.MessageBox]::Show(
            "120 FPS has reproduced a scripted-scene stall. Enable it anyway?",
            "Experimental 120 FPS", "YesNo", "Warning")
        if ($Answer -ne "Yes") { return }
    }
    $Fps = if ($FpsBox.SelectedIndex -eq 1) { 120 } elseif ($FpsBox.SelectedIndex -eq 2) { 30 } else { 60 }
    Set-PatchSettings ([int]$WidthBox.Value) ([int]$HeightBox.Value) $FovBox.Value $Fps ([int]$UiBox.Checked) ([int]$UltrawideBox.Checked) ([int]$FpsOverrideBox.Checked)
    [Windows.Forms.MessageBox]::Show("Settings saved. Restart the game to apply them.", "MGS4 Ultra120", "OK", "Information") | Out-Null
})
$CloseButton.Add_Click({ $Form.Close() })
$Form.Controls.AddRange(@($StableButton, $SaveButton, $CloseButton))
[void]$Form.ShowDialog()
