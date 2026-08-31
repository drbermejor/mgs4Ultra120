param([string]$GameDir)

$ErrorActionPreference = "Stop"
$Mgs4Ultra120Version = "v0.3.4-alpha.6"
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[Windows.Forms.Application]::EnableVisualStyles()
$Ui = @{
        Form = "MGS4 Ultra120 $Mgs4Ultra120Version - Easy setup"
        Title = "Install MGS4 Ultra120 $Mgs4Ultra120Version"
        Help = "Verify the game folder and click the blue button. The game must be closed. Core setup works offline."
        Path = "Game folder (must contain mgs4.exe):"
        Found = "Game detected correctly"
        Missing = "mgs4.exe was not found: correct the folder before continuing"
        NotFound = "The MGS4 folder was not found automatically"
        Browse = "Browse..."
        BrowseHelp = "Select the METAL GEAR SOLID 4\MGS4 folder containing mgs4.exe"
        Install = "1. Install / update and configure"
        Configure = "Open configurator only"
        Uninstall = "Uninstall and restore originals"
        Instructions = "Instructions"
        Close = "Close"
        ImprovedFps = "Install / update improved 120 FPS support (cipherxof/MGSFPSUnlock 0.1.0; internet required)"
        Done = "Setup is complete. You can now start the game normally from Steam."
        NotSaved = "The patch was installed, but the configurator closed without saving settings. Reopen the configurator to finish setup."
        InstallFailed = "Installation could not be completed"
        ConfigureFailed = "The configurator could not be opened"
        RemoveQuestion = "Remove MGS4 Ultra120 and restore the backed-up original files?"
        RemoveTitle = "Uninstall MGS4 Ultra120"
        Removed = "Uninstall complete; original files were restored."
        RemoveFailed = "Uninstall could not be completed"
}

$PackageDir = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$GameRelative = "steamapps\common\METAL GEAR SOLID 4\MGS4"
. (Join-Path $PSScriptRoot "common.ps1")
. (Join-Path $PSScriptRoot "mgsfpsunlock.ps1")

function Find-Mgs4Directories {
    $Roots = [Collections.Generic.List[string]]::new()
    foreach ($RegistryPath in @(
        "HKCU:\Software\Valve\Steam",
        "HKLM:\Software\WOW6432Node\Valve\Steam",
        "HKLM:\Software\Valve\Steam"
    )) {
        try {
            $Steam = Get-ItemProperty -LiteralPath $RegistryPath -ErrorAction Stop
            foreach ($Name in @("SteamPath", "InstallPath")) {
                if ($Steam.$Name) { $Roots.Add([string]$Steam.$Name) }
            }
        } catch {}
    }
    if (${env:ProgramFiles(x86)}) {
        $Roots.Add((Join-Path ${env:ProgramFiles(x86)} "Steam"))
    }
    if ($env:ProgramFiles) { $Roots.Add((Join-Path $env:ProgramFiles "Steam")) }

    $Libraries = [Collections.Generic.List[string]]::new()
    foreach ($Root in ($Roots | Select-Object -Unique)) {
        if (-not $Root) { continue }
        $Libraries.Add($Root)
        # Join-Path asks the PowerShell drive provider to resolve drive letters.
        # A stale Steam library on a disconnected E:/H: drive therefore aborted
        # discovery before the valid libraries were checked. IO.Path.Combine is
        # purely syntactic and File.Exists safely returns false for offline roots.
        $Vdf = [IO.Path]::Combine($Root, "steamapps\libraryfolders.vdf")
        if ([IO.File]::Exists($Vdf)) {
            $Text = Get-Content -Raw -LiteralPath $Vdf
            foreach ($Match in [regex]::Matches($Text, '"path"\s+"([^"]+)"')) {
                $Libraries.Add($Match.Groups[1].Value.Replace('\\', '\'))
            }
        }
    }

    $Found = foreach ($Library in ($Libraries | Select-Object -Unique)) {
        if (-not $Library) { continue }
        $Candidate = [IO.Path]::Combine($Library, $GameRelative)
        $Executable = [IO.Path]::Combine($Candidate, "mgs4.exe")
        if ([IO.File]::Exists($Executable)) {
            (Resolve-Path -LiteralPath $Candidate).Path
        }
    }
    return @($Found | Select-Object -Unique)
}

function Install-Patch([string]$Target, [bool]$IncludeImprovedFps) {
    Install-Mgs4Ultra120Patch $Target $PackageDir
    if (-not $IncludeImprovedFps) { return "" }
    try {
        Install-MgsFpsUnlock $Target
        return ""
    } catch {
        return "Core installation succeeded, but improved 120 FPS was not installed.`n`n$($_.Exception.Message)`n`nYou can use the game at its normal FPS and retry Easy Setup later."
    }
}

$Detected = @(Find-Mgs4Directories)
if (-not $GameDir) {
    $SavedGameDir = Get-Mgs4Ultra120GameDir
    if ($SavedGameDir -and
        [IO.File]::Exists([IO.Path]::Combine($SavedGameDir, "mgs4.exe"))) {
        $GameDir = $SavedGameDir
    } elseif ($Detected.Count -gt 0) {
        $GameDir = $Detected[0]
    }
}
if (-not $GameDir) { $GameDir = $Ui.NotFound }

$Form = [Windows.Forms.Form]@{
    Text = $Ui.Form
    StartPosition = "CenterScreen"
    ClientSize = [Drawing.Size]::new(720, 395)
    FormBorderStyle = "FixedDialog"
    MaximizeBox = $false
}
$Title = [Windows.Forms.Label]@{
    Text = $Ui.Title
    Location = [Drawing.Point]::new(24, 18)
    Size = [Drawing.Size]::new(660, 34)
    Font = [Drawing.Font]::new("Segoe UI", 15, [Drawing.FontStyle]::Bold)
}
$Help = [Windows.Forms.Label]@{
    Text = $Ui.Help
    Location = [Drawing.Point]::new(26, 58)
    Size = [Drawing.Size]::new(660, 54)
}
$PathLabel = [Windows.Forms.Label]@{
    Text = $Ui.Path
    Location = [Drawing.Point]::new(26, 116)
    Size = [Drawing.Size]::new(660, 22)
}
$PathBox = [Windows.Forms.TextBox]@{
    Text = $GameDir
    Location = [Drawing.Point]::new(26, 140)
    Size = [Drawing.Size]::new(555, 27)
}
$Browse = [Windows.Forms.Button]@{
    Text = $Ui.Browse
    Location = [Drawing.Point]::new(594, 137)
    Size = [Drawing.Size]::new(98, 32)
}
$PathStatus = [Windows.Forms.Label]@{
    Location = [Drawing.Point]::new(26, 174)
    Size = [Drawing.Size]::new(660, 24)
    Font = [Drawing.Font]::new("Segoe UI", 9, [Drawing.FontStyle]::Bold)
}
$ImprovedFpsBox = [Windows.Forms.CheckBox]@{
    Text = $Ui.ImprovedFps
    Location = [Drawing.Point]::new(26, 202)
    Size = [Drawing.Size]::new(660, 28)
    Checked = $true
}
$Install = [Windows.Forms.Button]@{
    Text = $Ui.Install
    Location = [Drawing.Point]::new(26, 240)
    Size = [Drawing.Size]::new(320, 48)
    BackColor = [Drawing.Color]::RoyalBlue
    ForeColor = [Drawing.Color]::White
    Font = [Drawing.Font]::new("Segoe UI", 9, [Drawing.FontStyle]::Bold)
}
$Configure = [Windows.Forms.Button]@{
    Text = $Ui.Configure
    Location = [Drawing.Point]::new(360, 240)
    Size = [Drawing.Size]::new(200, 48)
}
$Uninstall = [Windows.Forms.Button]@{
    Text = $Ui.Uninstall
    Location = [Drawing.Point]::new(26, 322)
    Size = [Drawing.Size]::new(250, 40)
}
$Instructions = [Windows.Forms.Button]@{
    Text = $Ui.Instructions
    Location = [Drawing.Point]::new(290, 322)
    Size = [Drawing.Size]::new(130, 40)
}
$Close = [Windows.Forms.Button]@{
    Text = $Ui.Close
    Location = [Drawing.Point]::new(566, 322)
    Size = [Drawing.Size]::new(126, 40)
}
$Form.Controls.AddRange(@($Title, $Help, $PathLabel, $PathBox, $Browse,
                           $PathStatus, $ImprovedFpsBox, $Install, $Configure, $Uninstall,
                          $Instructions, $Close))
$Form.AcceptButton = $Install

function Update-PathStatus {
    $Valid = [IO.File]::Exists([IO.Path]::Combine($PathBox.Text, "mgs4.exe"))
    $PathStatus.Text = if ($Valid) { "OK - " + $Ui.Found } else { $Ui.Missing }
    $PathStatus.ForeColor = if ($Valid) { [Drawing.Color]::DarkGreen } else { [Drawing.Color]::DarkRed }
    $Install.Enabled = $Valid
    $Configure.Enabled = $Valid
    $Uninstall.Enabled = $Valid
}
Update-PathStatus

$Browse.Add_Click({
    $Picker = [Windows.Forms.FolderBrowserDialog]::new()
    $Picker.Description = $Ui.BrowseHelp
    if (Test-Path -LiteralPath $PathBox.Text) { $Picker.SelectedPath = $PathBox.Text }
    if ($Picker.ShowDialog() -eq "OK") {
        $PathBox.Text = $Picker.SelectedPath
        Update-PathStatus
    }
})
$PathBox.Add_TextChanged({ Update-PathStatus })
$Install.Add_Click({
    try {
        $FpsWarning = Install-Patch $PathBox.Text $ImprovedFpsBox.Checked
        $ConfigureResult = @(& (Join-Path $PSScriptRoot "configure.ps1") `
            -GameDir $PathBox.Text)
        if ($ConfigureResult -contains $true) {
            [Windows.Forms.MessageBox]::Show($Ui.Done,
                "MGS4 Ultra120", "OK", "Information") | Out-Null
            if ($FpsWarning) {
                [Windows.Forms.MessageBox]::Show($FpsWarning,
                    "Optional 120 FPS component", "OK", "Warning") | Out-Null
            }
        } else {
            [Windows.Forms.MessageBox]::Show($Ui.NotSaved,
                "MGS4 Ultra120", "OK", "Warning") | Out-Null
        }
    } catch {
        [Windows.Forms.MessageBox]::Show($_.Exception.Message,
            $Ui.InstallFailed, "OK", "Error") | Out-Null
    }
})
$Configure.Add_Click({
    try {
        & (Join-Path $PSScriptRoot "configure.ps1") -GameDir $PathBox.Text
    } catch {
        [Windows.Forms.MessageBox]::Show($_.Exception.Message,
            $Ui.ConfigureFailed, "OK", "Error") | Out-Null
    }
})
$Uninstall.Add_Click({
    $Answer = [Windows.Forms.MessageBox]::Show(
        $Ui.RemoveQuestion, $Ui.RemoveTitle, "YesNo", "Question")
    if ($Answer -ne "Yes") { return }
    try {
        & (Join-Path $PSScriptRoot "uninstall.ps1") -GameDir $PathBox.Text
        [Windows.Forms.MessageBox]::Show($Ui.Removed,
            "MGS4 Ultra120", "OK", "Information") | Out-Null
    } catch {
        [Windows.Forms.MessageBox]::Show($_.Exception.Message,
            $Ui.RemoveFailed, "OK", "Error") | Out-Null
    }
})
$Instructions.Add_Click({
    Start-Process "https://github.com/drbermejor/mgs4Ultra120/blob/main/docs/INSTALL_WINDOWS.md"
})
$Close.Add_Click({ $Form.Close() })
[void]$Form.ShowDialog()
