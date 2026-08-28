param(
    [Parameter(Mandatory)][string]$Wrapper,
    [Parameter(Mandatory)][string]$Probe
)
$ErrorActionPreference = "Stop"
$Root = Join-Path ([IO.Path]::GetTempPath()) `
    ("mgs4-launcher-smoke-" + [Guid]::NewGuid().ToString("N"))
$LauncherDir = Join-Path $Root "Launcher"
$GameDir = Join-Path $Root "MGS4"
try {
    New-Item -ItemType Directory -Force -Path $LauncherDir, $GameDir | Out-Null
    Copy-Item -LiteralPath $Wrapper -Destination (Join-Path $LauncherDir "launcher.exe")
    Copy-Item -LiteralPath $Probe -Destination (Join-Path $GameDir "mgs4.exe")
    foreach ($Case in @(
        [pscustomobject]@{ Mode = "Windowed"; Language = "sp"; ExpectedLanguage = "sp"; Expected = "0" },
        [pscustomobject]@{ Mode = "Fullscreen"; Language = "ge"; ExpectedLanguage = "gr"; Expected = "0" }
    )) {
        [IO.File]::WriteAllText((Join-Path $GameDir "mgs4_ultrawide.ini"),
            "[Launcher]`nDisplayMode=$($Case.Mode)`nLanguage=$($Case.Language)`nRegion=eu`nSelfRegion=EU`nControllerType=XBOX`n",
            [Text.UTF8Encoding]::new($false))
        $Output = Join-Path $Root "arguments-$($Case.Mode).txt"
        $env:MGS4ULTRA120_PROBE_OUTPUT = $Output
        $Process = Start-Process -FilePath (Join-Path $LauncherDir "launcher.exe") `
            -Wait -PassThru
        if ($Process.ExitCode -ne 0) {
            throw "Wrapper probe failed for $($Case.Mode): $($Process.ExitCode)"
        }
        $ChildArguments = @(Get-Content -LiteralPath $Output)
        if ($ChildArguments.Count -ne 0) {
            throw "Wrapper exposed custom arguments on the child command line."
        }
        $BootstrapPath = Join-Path ([IO.Path]::GetTempPath()) "mgs4_param"
        $Bootstrap = [IO.File]::ReadAllBytes($BootstrapPath)
        $Count = [BitConverter]::ToUInt32($Bootstrap, 0)
        $Payload = [Text.Encoding]::UTF8.GetString(
            $Bootstrap, 4, $Bootstrap.Length - 4).TrimEnd([char]0)
        $Arguments = @($Payload.Split([char]8))
        if ($Count -ne $Arguments.Count) {
            throw "mgs4_param argument count does not match its payload."
        }
        $ResolutionIndex = [Array]::IndexOf($Arguments, "-resolution")
        if ($ResolutionIndex -lt 0 -or
            $Arguments[$ResolutionIndex + 1] -ne $Case.Expected) {
            throw "DisplayMode=$($Case.Mode) changed the official resolution slot from $($Case.Expected)."
        }
        $LanguageIndex = [Array]::IndexOf($Arguments, "-lan")
        if ($LanguageIndex -lt 0 -or
            $Arguments[$LanguageIndex + 1] -ne $Case.ExpectedLanguage) {
            throw "Language=$($Case.Language) was not normalized to $($Case.ExpectedLanguage)."
        }
        $RootIndex = [Array]::IndexOf($Arguments, "-launcherroot")
        if ($RootIndex -lt 0 -or $Arguments[$RootIndex + 1] -ne $LauncherDir) {
            throw "Launcher path with spaces was not preserved as one argument."
        }
    }
    Write-Host "Direct-launch wrapper smoke test passed."
} finally {
    Remove-Item Env:\MGS4ULTRA120_PROBE_OUTPUT -ErrorAction SilentlyContinue
    $ResolvedRoot = [IO.Path]::GetFullPath($Root)
    $ResolvedTemp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    if ($ResolvedRoot.StartsWith($ResolvedTemp,
        [StringComparison]::OrdinalIgnoreCase) -and
        (Split-Path -Leaf $ResolvedRoot).StartsWith("mgs4-launcher-smoke-")) {
        Remove-Item -LiteralPath $ResolvedRoot -Recurse -Force `
            -ErrorAction SilentlyContinue
    }
}
