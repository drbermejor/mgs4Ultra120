param(
    [Parameter(Mandatory)]
    [ValidatePattern('^v\d+\.\d+\.\d+-alpha\.\d+$')]
    [string]$Version,

    [Parameter(Mandatory)]
    [string]$WindowsZip,

    [Parameter(Mandatory)]
    [ValidatePattern('^[a-fA-F0-9]{64}$')]
    [string]$ExpectedZipSha256,

    [string]$OutputDir
)

$ErrorActionPreference = "Stop"
$OutputDir = if ($OutputDir) {
    $OutputDir
} else {
    Join-Path $PSScriptRoot "..\..\dist"
}
$WindowsZip = (Resolve-Path -LiteralPath $WindowsZip).Path
$OutputDir = [IO.Path]::GetFullPath($OutputDir)
$ActualZipSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $WindowsZip).Hash
if ($ActualZipSha256 -ne $ExpectedZipSha256) {
    throw "Windows ZIP SHA-256 mismatch. Expected $ExpectedZipSha256; got $ActualZipSha256."
}

$VersionMatch = [regex]::Match($Version, '^v(\d+)\.(\d+)\.(\d+)-alpha\.(\d+)$')
$NumericVersion = '{0}.{1}.{2}.{3}' -f $VersionMatch.Groups[1].Value,
    $VersionMatch.Groups[2].Value, $VersionMatch.Groups[3].Value,
    $VersionMatch.Groups[4].Value
$ExpectedRootName = "MGS4Ultra120-$Version-windows"
$StageDir = Join-Path ([IO.Path]::GetTempPath()) (
    "mgs4ultra120-installer-" + [Guid]::NewGuid().ToString("N"))

$IsccCandidates = @(
    (Join-Path $env:LOCALAPPDATA "Programs\Inno Setup 6\ISCC.exe"),
    (Join-Path ${env:ProgramFiles(x86)} "Inno Setup 6\ISCC.exe"),
    (Join-Path $env:ProgramFiles "Inno Setup 6\ISCC.exe")
)
$Iscc = $IsccCandidates | Where-Object { $_ -and (Test-Path -LiteralPath $_) } |
    Select-Object -First 1
if (-not $Iscc) {
    throw "Inno Setup 6 was not found. Install JRSoftware.InnoSetup with winget."
}

try {
    New-Item -ItemType Directory -Path $StageDir -Force | Out-Null
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
    Expand-Archive -LiteralPath $WindowsZip -DestinationPath $StageDir
    $PayloadDir = Join-Path $StageDir $ExpectedRootName
    if (-not (Test-Path -LiteralPath $PayloadDir -PathType Container)) {
        throw "ZIP does not contain the expected package root: $ExpectedRootName"
    }

    & (Join-Path $PSScriptRoot "Test-WindowsPackage.ps1") -PackageDir $PayloadDir
    & $Iscc "/DAppVersion=$Version" "/DNumericVersion=$NumericVersion" `
        "/DPayloadDir=$PayloadDir" "/DBuildOutputDir=$OutputDir" `
        (Join-Path $PSScriptRoot "MGS4Ultra120.iss")
    if ($LASTEXITCODE -ne 0) { throw "Inno Setup compiler failed with exit code $LASTEXITCODE." }

    $Installer = Join-Path $OutputDir "MGS4Ultra120-$Version-windows-setup.exe"
    if (-not (Test-Path -LiteralPath $Installer -PathType Leaf)) {
        throw "The installer was not created: $Installer"
    }
    $InstallerHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Installer).Hash.ToLowerInvariant()
    Write-Host "Created: $Installer"
    Write-Host "Build-only SHA-256 (no separate release asset): $InstallerHash"
} finally {
    $ResolvedTemp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    $ResolvedStage = [IO.Path]::GetFullPath($StageDir)
    if ($ResolvedStage.StartsWith($ResolvedTemp,
        [StringComparison]::OrdinalIgnoreCase) -and
        (Split-Path -Leaf $ResolvedStage).StartsWith("mgs4ultra120-installer-")) {
        Remove-Item -LiteralPath $ResolvedStage -Recurse -Force -ErrorAction SilentlyContinue
    }
}
