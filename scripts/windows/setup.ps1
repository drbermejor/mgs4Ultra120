param([string]$GameDir)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[Windows.Forms.Application]::EnableVisualStyles()

$PackageDir = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$GameRelative = "steamapps\common\METAL GEAR SOLID 4\MGS4"

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
        $Vdf = Join-Path $Root "steamapps\libraryfolders.vdf"
        if (Test-Path -LiteralPath $Vdf) {
            $Text = Get-Content -Raw -LiteralPath $Vdf
            foreach ($Match in [regex]::Matches($Text, '"path"\s+"([^"]+)"')) {
                $Libraries.Add($Match.Groups[1].Value.Replace('\\', '\'))
            }
        }
    }

    $Found = foreach ($Library in ($Libraries | Select-Object -Unique)) {
        $Candidate = Join-Path $Library $GameRelative
        if (Test-Path -LiteralPath (Join-Path $Candidate "mgs4.exe")) {
            (Resolve-Path -LiteralPath $Candidate).Path
        }
    }
    return @($Found | Select-Object -Unique)
}

function Install-Patch([string]$Target) {
    if (Get-Process mgs4 -ErrorAction SilentlyContinue) {
        throw "Exit the game before installing or updating the patch."
    }
    if (-not (Test-Path -LiteralPath (Join-Path $Target "mgs4.exe"))) {
        throw "mgs4.exe was not found in the selected MGS4 folder."
    }
    $BackupDir = Join-Path $Target ".mgs4ultra120-backup"
    New-Item -ItemType Directory -Force -Path $BackupDir | Out-Null
    foreach ($Name in @("winmm.dll", "mgs4_ultrawide.ini")) {
        $Destination = Join-Path $Target $Name
        $Backup = Join-Path $BackupDir "$Name.preinstall"
        if ((Test-Path -LiteralPath $Destination) -and
            -not (Test-Path -LiteralPath $Backup)) {
            Copy-Item -LiteralPath $Destination -Destination $Backup
        }
    }
    Copy-Item -Force -LiteralPath (Join-Path $PackageDir "bin\winmm.dll") `
        -Destination (Join-Path $Target "winmm.dll")
    Copy-Item -Force -LiteralPath (Join-Path $PackageDir "config\mgs4_ultrawide.ini") `
        -Destination (Join-Path $Target "mgs4_ultrawide.ini")
}

$Detected = @(Find-Mgs4Directories)
if (-not $GameDir -and $Detected.Count -gt 0) { $GameDir = $Detected[0] }
if (-not $GameDir) { $GameDir = "MGS4 folder was not found automatically" }

$Form = [Windows.Forms.Form]@{
    Text = "MGS4 Ultra120 - Easy setup"
    StartPosition = "CenterScreen"
    ClientSize = [Drawing.Size]::new(720, 265)
    FormBorderStyle = "FixedDialog"
    MaximizeBox = $false
}
$Title = [Windows.Forms.Label]@{
    Text = "Install and configure MGS4 Ultra120"
    Location = [Drawing.Point]::new(24, 18)
    Size = [Drawing.Size]::new(660, 34)
    Font = [Drawing.Font]::new("Segoe UI", 15, [Drawing.FontStyle]::Bold)
}
$Help = [Windows.Forms.Label]@{
    Text = "The Steam installation is detected automatically. Select the MGS4 folder manually only if the path below is wrong. Close the game before continuing."
    Location = [Drawing.Point]::new(26, 58)
    Size = [Drawing.Size]::new(660, 45)
}
$PathBox = [Windows.Forms.TextBox]@{
    Text = $GameDir
    Location = [Drawing.Point]::new(26, 112)
    Size = [Drawing.Size]::new(555, 27)
}
$Browse = [Windows.Forms.Button]@{
    Text = "Browse..."
    Location = [Drawing.Point]::new(594, 109)
    Size = [Drawing.Size]::new(98, 32)
}
$Install = [Windows.Forms.Button]@{
    Text = "Install / update"
    Location = [Drawing.Point]::new(26, 174)
    Size = [Drawing.Size]::new(145, 42)
}
$Configure = [Windows.Forms.Button]@{
    Text = "Open configurator"
    Location = [Drawing.Point]::new(184, 174)
    Size = [Drawing.Size]::new(155, 42)
}
$Uninstall = [Windows.Forms.Button]@{
    Text = "Uninstall"
    Location = [Drawing.Point]::new(438, 174)
    Size = [Drawing.Size]::new(112, 42)
}
$Close = [Windows.Forms.Button]@{
    Text = "Close"
    Location = [Drawing.Point]::new(566, 174)
    Size = [Drawing.Size]::new(126, 42)
}
$Form.Controls.AddRange(@($Title, $Help, $PathBox, $Browse, $Install,
                          $Configure, $Uninstall, $Close))

$Browse.Add_Click({
    $Picker = [Windows.Forms.FolderBrowserDialog]::new()
    $Picker.Description = "Select the METAL GEAR SOLID 4\MGS4 folder containing mgs4.exe"
    if (Test-Path -LiteralPath $PathBox.Text) { $Picker.SelectedPath = $PathBox.Text }
    if ($Picker.ShowDialog() -eq "OK") { $PathBox.Text = $Picker.SelectedPath }
})
$Install.Add_Click({
    try {
        Install-Patch $PathBox.Text
        [Windows.Forms.MessageBox]::Show(
            "Installation complete. The configurator will now open. Choose any combination of fixes and click Save settings.",
            "MGS4 Ultra120", "OK", "Information") | Out-Null
        & (Join-Path $PSScriptRoot "configure.ps1") -GameDir $PathBox.Text
    } catch {
        [Windows.Forms.MessageBox]::Show($_.Exception.Message,
            "Installation failed", "OK", "Error") | Out-Null
    }
})
$Configure.Add_Click({
    try {
        & (Join-Path $PSScriptRoot "configure.ps1") -GameDir $PathBox.Text
    } catch {
        [Windows.Forms.MessageBox]::Show($_.Exception.Message,
            "Could not open configurator", "OK", "Error") | Out-Null
    }
})
$Uninstall.Add_Click({
    $Answer = [Windows.Forms.MessageBox]::Show(
        "Remove MGS4 Ultra120 and restore files backed up before installation?",
        "Uninstall MGS4 Ultra120", "YesNo", "Question")
    if ($Answer -ne "Yes") { return }
    try {
        & (Join-Path $PSScriptRoot "uninstall.ps1") -GameDir $PathBox.Text
        [Windows.Forms.MessageBox]::Show("Uninstall complete.",
            "MGS4 Ultra120", "OK", "Information") | Out-Null
    } catch {
        [Windows.Forms.MessageBox]::Show($_.Exception.Message,
            "Uninstall failed", "OK", "Error") | Out-Null
    }
})
$Close.Add_Click({ $Form.Close() })
[void]$Form.ShowDialog()
