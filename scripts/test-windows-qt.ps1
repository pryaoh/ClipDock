$ErrorActionPreference = 'Stop'
$Helper = Join-Path $PSScriptRoot 'windows-qt.ps1'
if (-not (Test-Path -LiteralPath $Helper)) { throw 'Windows Qt SDK discovery is not implemented.' }
. $Helper

$Fixture = Join-Path ([IO.Path]::GetTempPath()) ('clipdock-sdk-' + [guid]::NewGuid())
function Assert-True($Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}
try {
    $Kit = Join-Path $Fixture '.tools/qt/6.8.3/msvc2022_64'
    foreach ($RelativePath in @('lib/cmake/Qt6/Qt6Config.cmake',
            'lib/cmake/Qt6Multimedia/Qt6MultimediaConfig.cmake',
            'plugins/multimedia/ffmpegmediaplugin.dll', 'bin/Qt6Core.dll')) {
        $Path = Join-Path $Kit $RelativePath
        New-Item -ItemType Directory -Path (Split-Path $Path -Parent) -Force | Out-Null
        New-Item -ItemType File -Path $Path -Force | Out-Null
    }
    $Metadata = Join-Path $Kit 'mkspecs/qconfig.pri'
    New-Item -ItemType Directory -Path (Split-Path $Metadata -Parent) -Force | Out-Null
    Set-Content -LiteralPath $Metadata -Value "QT_VERSION = 6.8.3`nQT_ARCH = x86_64`nQT_MSVC_MAJOR_VERSION = 19"
    Assert-True (Test-ClipDockQtSdk $Kit) 'A complete Windows SDK must be accepted.'
    Assert-True ((Find-ClipDockQtSdk -ProjectRoot $Fixture) -eq $Kit) 'The project SDK must be found without PATH changes.'
    Assert-True ((Find-ClipDockQtSdk -ProjectRoot $Fixture -QtRoot $Kit) -eq $Kit) 'An explicit SDK must be honored.'
    Remove-Item -LiteralPath (Join-Path $Kit 'plugins/multimedia/ffmpegmediaplugin.dll')
    Assert-True (-not (Test-ClipDockQtSdk $Kit)) 'A kit without FFmpeg cannot capture the screen.'
    $Rejected = $false
    try { Find-ClipDockQtSdk -ProjectRoot $Fixture -QtRoot $Kit | Out-Null }
    catch { $Rejected = $_.Exception.Message -match 'FFmpeg' }
    Assert-True $Rejected 'An incomplete explicit kit must produce an actionable error.'
    Assert-True (-not (Test-ClipDockQtSdk (Join-Path $Fixture 'missing'))) 'Missing SDKs must be rejected.'
    New-Item -ItemType File -Path (Join-Path $Kit 'plugins/multimedia/ffmpegmediaplugin.dll') -Force | Out-Null
    Remove-Item -LiteralPath $Metadata
    Assert-True (-not (Test-ClipDockQtSdk $Kit)) 'A kit with no compiler/version metadata must be rejected.'
    foreach ($InvalidMetadata in @(
        "QT_VERSION = 6.7.3`nQT_ARCH = x86_64`nQT_MSVC_MAJOR_VERSION = 19",
        "QT_VERSION = 6.8.3`nQT_ARCH = arm64`nQT_MSVC_MAJOR_VERSION = 19",
        "QT_VERSION = 6.8.3`nQT_ARCH = x86_64")) {
        Set-Content -LiteralPath $Metadata -Value $InvalidMetadata
        Assert-True (-not (Test-ClipDockQtSdk $Kit)) 'Old, ARM and non-MSVC kits must be rejected.'
    }
    Set-Content -LiteralPath $Metadata -Value "QT_VERSION = 6.8.3`nQT_ARCH = x86_64`nQT_MSVC_MAJOR_VERSION = 19"
    Assert-True (Test-ClipDockQtSdk $Kit) 'A matching MSVC x64 Qt 6.8 kit must be accepted.'
    Write-Host 'Windows Qt SDK tests passed.'
} finally {
    # Only remove the uniquely named temporary fixture created above.
    $ResolvedFixture = [IO.Path]::GetFullPath($Fixture)
    $TempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    if ($ResolvedFixture.StartsWith($TempRoot, [StringComparison]::OrdinalIgnoreCase) -and
        (Split-Path $ResolvedFixture -Leaf) -like 'clipdock-sdk-*' -and
        (Test-Path -LiteralPath $ResolvedFixture)) {
        Remove-Item -LiteralPath $ResolvedFixture -Recurse -Force
    }
}
