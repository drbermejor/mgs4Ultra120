param(
    [Parameter(Mandatory)]
    [string]$Loader,

    [Parameter(Mandatory)]
    [string]$Plugin,

    [Parameter(Mandatory)]
    [string]$Probe
)

$ErrorActionPreference = "Stop"
$Loader = (Resolve-Path -LiteralPath $Loader).Path
$Plugin = (Resolve-Path -LiteralPath $Plugin).Path
$Probe = (Resolve-Path -LiteralPath $Probe).Path
$SmokeRoot = Join-Path ([IO.Path]::GetTempPath()) (
    "mgs4ultra120-asi-loader-smoke-" + [Guid]::NewGuid().ToString("N"))

try {
    $ScriptsDir = Join-Path $SmokeRoot "scripts"
    New-Item -ItemType Directory -Path $ScriptsDir -Force | Out-Null
    Copy-Item -LiteralPath $Loader -Destination (Join-Path $SmokeRoot "winmm.dll")
    Copy-Item -LiteralPath $Plugin `
        -Destination (Join-Path $ScriptsDir "MGS4Ultra120.asi")
    Copy-Item -LiteralPath $Probe `
        -Destination (Join-Path $SmokeRoot "asi_loader_probe.exe")
    [IO.File]::WriteAllText(
        (Join-Path $SmokeRoot "mgs4_ultrawide.ini"),
        "[Patch]`r`nUltrawideEnabled=1`r`n[Ultrawide]`r`nWidth=3440`r`nHeight=1440`r`nFOVMultiplier=1.250`r`nNativeCameraFOV=1`r`n[Supersampling]`r`nSupersamplingEnabled=0`r`nRenderScale=1.000`r`n",
        [Text.UTF8Encoding]::new($false))
    $Process = Start-Process -FilePath (Join-Path $SmokeRoot "asi_loader_probe.exe") `
        -WorkingDirectory $SmokeRoot -Wait -PassThru -WindowStyle Hidden
    if ($Process.ExitCode -ne 0) {
        throw "Ultimate ASI Loader probe failed with exit code $($Process.ExitCode)."
    }
    $Log = Join-Path $SmokeRoot "mgs4_ultrawide.log"
    if (-not (Test-Path -LiteralPath $Log -PathType Leaf)) {
        throw "Ultimate ASI Loader did not load MGS4Ultra120.asi."
    }
    $LogText = Get-Content -Raw -LiteralPath $Log
    if ($LogText -notmatch "unrecognized mgs4.exe version") {
        throw "The ASI plugin loaded but did not complete its safe unsupported-executable path."
    }
    if ($LogText -match "invalid display configuration" -or
        $LogText -notmatch "FOVMultiplier exceeds the tested 1.200 recommendation") {
        throw "The ASI rejected or failed to warn about an above-recommendation FOV value."
    }
    Write-Host "Ultimate ASI Loader integration smoke test passed."
} finally {
    $ResolvedTemp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    $ResolvedSmoke = [IO.Path]::GetFullPath($SmokeRoot)
    if ($ResolvedSmoke.StartsWith($ResolvedTemp,
        [StringComparison]::OrdinalIgnoreCase) -and
        (Split-Path -Leaf $ResolvedSmoke).StartsWith(
            "mgs4ultra120-asi-loader-smoke-")) {
        Remove-Item -LiteralPath $ResolvedSmoke -Recurse -Force `
            -ErrorAction SilentlyContinue
    }
}
