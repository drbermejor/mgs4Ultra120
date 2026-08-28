param(
    [Parameter(Mandatory)]
    [string]$ProxyDll,

    [string]$SmokeTestExe
)

$ErrorActionPreference = "Stop"
$ProxyDll = (Resolve-Path -LiteralPath $ProxyDll).Path
$SystemWinmm = Join-Path $env:SystemRoot "System32\winmm.dll"

$VsWhere = Join-Path ${env:ProgramFiles(x86)} `
    "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path -LiteralPath $VsWhere)) {
    throw "vswhere.exe was not found. Install Visual Studio Build Tools."
}
$VisualStudio = & $VsWhere -latest -products * -property installationPath
$Dumpbin = Get-ChildItem -LiteralPath (Join-Path $VisualStudio "VC\Tools\MSVC") `
    -Filter dumpbin.exe -Recurse | Select-Object -First 1 -ExpandProperty FullName
if (-not $Dumpbin) { throw "dumpbin.exe was not found." }

function Get-PeExports([string]$Path) {
    $Exports = [Collections.Generic.List[object]]::new()
    $FunctionCount = 0
    foreach ($Line in (& $Dumpbin /nologo /exports $Path)) {
        if ($Line -match '^\s+(\d+) number of functions\s*$') {
            $FunctionCount = [int]$Matches[1]
        }
        if ($Line -match '^\s+(\d+)\s+[0-9A-F]+\s+[0-9A-F]+(?:\s+(\S+))?\s*$') {
            $Exports.Add([pscustomobject]@{
                Ordinal = [int]$Matches[1]
                Name = if ($Matches[2]) { $Matches[2] } else { "" }
            })
        }
    }
    return [pscustomobject]@{
        FunctionCount = $FunctionCount
        NamedExports = @($Exports)
    }
}

$ExpectedInfo = Get-PeExports $SystemWinmm
$ActualInfo = Get-PeExports $ProxyDll
$Expected = @($ExpectedInfo.NamedExports)
$Actual = @($ActualInfo.NamedExports)
if ($ExpectedInfo.FunctionCount -ne 181 -or $ActualInfo.FunctionCount -ne 181 -or
    $Expected.Count -ne 180 -or $Actual.Count -ne 180) {
    throw "Expected 181 functions and 180 named WinMM exports; " +
        "system=$($ExpectedInfo.FunctionCount)/$($Expected.Count), " +
        "proxy=$($ActualInfo.FunctionCount)/$($Actual.Count)."
}

$ExpectedMap = $Expected | ForEach-Object { "$($_.Ordinal):$($_.Name)" }
$ActualMap = $Actual | ForEach-Object { "$($_.Ordinal):$($_.Name)" }
$Difference = @(Compare-Object $ExpectedMap $ActualMap)
if ($Difference.Count -ne 0) {
    $Preview = ($Difference | Select-Object -First 10 | Out-String).Trim()
    throw "The proxy export table differs from the system WinMM table:`n$Preview"
}

Write-Host "WinMM export verification passed: 181 ordinals, 180 named exports."
if ($SmokeTestExe) {
    $SmokeTestExe = (Resolve-Path -LiteralPath $SmokeTestExe).Path
    & $SmokeTestExe
    if ($LASTEXITCODE -ne 0) {
        throw "WinMM smoke test failed with exit code $LASTEXITCODE."
    }
}
