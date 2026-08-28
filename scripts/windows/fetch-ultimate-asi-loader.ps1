param(
    [string]$OutputDir
)

$ErrorActionPreference = "Stop"
$Version = "v9.7.4"
$ArchiveName = "Ultimate-ASI-Loader-NoPDB_x64.zip"
$ArchiveSha256 = "E5860E7D9A1805267535B65749575B5E406CC6EA3325C7392189C578815045D1"
$DllSha256 = "031A3E5576D91DCE1E438D36B9A3D462C7334AB4791990A8FF1E3DDC0E132DAF"
$Uri = "https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/download/$Version/$ArchiveName"
$RepoDir = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$OutputDir = if ($OutputDir) {
    [IO.Path]::GetFullPath($OutputDir)
} else {
    Join-Path $RepoDir "build-third-party\ultimate-asi-loader"
}
$StageDir = Join-Path ([IO.Path]::GetTempPath()) (
    "mgs4ultra120-ual-" + [Guid]::NewGuid().ToString("N"))

try {
    New-Item -ItemType Directory -Path $StageDir -Force | Out-Null
    $Archive = Join-Path $StageDir $ArchiveName
    Invoke-WebRequest -UseBasicParsing -Uri $Uri -OutFile $Archive
    $ActualArchiveHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Archive).Hash
    if ($ActualArchiveHash -ne $ArchiveSha256) {
        throw "Ultimate ASI Loader archive hash mismatch. Expected $ArchiveSha256; got $ActualArchiveHash."
    }
    $Expanded = Join-Path $StageDir "expanded"
    Expand-Archive -LiteralPath $Archive -DestinationPath $Expanded
    $UpstreamDll = Join-Path $Expanded "dinput8.dll"
    if (-not (Test-Path -LiteralPath $UpstreamDll -PathType Leaf)) {
        throw "The pinned Ultimate ASI Loader archive does not contain dinput8.dll."
    }
    $ActualDllHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $UpstreamDll).Hash
    if ($ActualDllHash -ne $DllSha256) {
        throw "Ultimate ASI Loader DLL hash mismatch. Expected $DllSha256; got $ActualDllHash."
    }
    $VersionInfo = (Get-Item -LiteralPath $UpstreamDll).VersionInfo
    if ($VersionInfo.FileDescription -ne "Ultimate ASI Loader" -or
        $VersionInfo.ProductVersion -notlike "9.7.4*") {
        throw "The pinned DLL version metadata is unexpected."
    }
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
    Copy-Item -Force -LiteralPath $UpstreamDll `
        -Destination (Join-Path $OutputDir "winmm.dll")
    [IO.File]::WriteAllText((Join-Path $OutputDir "UPSTREAM.txt"),
        "Ultimate ASI Loader $Version`nSource: $Uri`nArchive SHA-256: $ArchiveSha256`nDLL SHA-256: $DllSha256`n",
        [Text.UTF8Encoding]::new($false))
    Write-Host "Fetched pinned Ultimate ASI Loader ${Version}: $OutputDir"
    Write-Host "winmm.dll SHA-256: $DllSha256"
} finally {
    $ResolvedTemp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    $ResolvedStage = [IO.Path]::GetFullPath($StageDir)
    if ($ResolvedStage.StartsWith($ResolvedTemp,
        [StringComparison]::OrdinalIgnoreCase) -and
        (Split-Path -Leaf $ResolvedStage).StartsWith("mgs4ultra120-ual-")) {
        Remove-Item -LiteralPath $ResolvedStage -Recurse -Force `
            -ErrorAction SilentlyContinue
    }
}
