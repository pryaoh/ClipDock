param(
    [string]$QtRoot = $env:QT_ROOT,
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [switch]$Test,
    [switch]$Package,
    [switch]$Run
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'windows-qt.ps1')
$ProjectRoot = Split-Path $PSScriptRoot -Parent
$BuildPath = Join-Path $ProjectRoot 'build/qt'
Initialize-ClipDockBuildTools
$QtRoot = Find-ClipDockQtSdk -ProjectRoot $ProjectRoot -QtRoot $QtRoot
if (-not $QtRoot) {
    & (Join-Path $PSScriptRoot 'setup-windows-qt.ps1')
    $QtRoot = Find-ClipDockQtSdk -ProjectRoot $ProjectRoot
}
# The executables in the build tree need Qt and FFmpeg DLLs for both tests and -Run.
$env:PATH = "$(Join-Path $QtRoot 'bin');$env:PATH"
$Compiler = (Get-Command cl).Source
$ConfigureArgs = @('-S', $ProjectRoot, '-B', $BuildPath, '-G', 'Ninja Multi-Config',
    '-DBUILD_TESTING=ON', "-DCMAKE_CXX_COMPILER=$Compiler", "-DCMAKE_PREFIX_PATH=$QtRoot", "-DQt6_DIR=$QtRoot/lib/cmake/Qt6")
$SdkMarker = Join-Path $BuildPath '.clipdock-qt-root'
$CompilerMarker = Join-Path $BuildPath '.clipdock-msvc-compiler'
$PreviousQt = if (Test-Path -LiteralPath $SdkMarker) { (Get-Content -LiteralPath $SdkMarker -Raw).Trim() } else { '' }
$PreviousCompiler = if (Test-Path -LiteralPath $CompilerMarker) { (Get-Content -LiteralPath $CompilerMarker -Raw).Trim() } else { '' }
if ((Test-Path -LiteralPath (Join-Path $BuildPath 'CMakeCache.txt')) -and
    ($PreviousQt -ne $QtRoot -or $PreviousCompiler -ne $Compiler)) {
    $ConfigureArgs += '--fresh'
}
& cmake @ConfigureArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Set-Content -LiteralPath $SdkMarker -Value $QtRoot
Set-Content -LiteralPath $CompilerMarker -Value $Compiler
& cmake --build $BuildPath --config $Configuration --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if ($Test) {
    & ctest --test-dir $BuildPath -C $Configuration --output-on-failure
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
if ($Package) {
    & cmake --install $BuildPath --config $Configuration --prefix (Join-Path $ProjectRoot 'dist/qt')
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    $ApplicationPath = Join-Path $ProjectRoot 'dist/qt/bin/ClipDock.exe'
    $PluginName = if ($Configuration -eq 'Debug') { 'ffmpegmediaplugind.dll' } else { 'ffmpegmediaplugin.dll' }
    if (-not (Test-Path -LiteralPath (Join-Path $ProjectRoot "dist/qt/plugins/multimedia/$PluginName"))) {
        throw 'The packaged app is missing the FFmpeg multimedia plugin required for screen capture.'
    }
    Write-Host "Windows package ready: $ApplicationPath"
} else {
    $ApplicationPath = Join-Path $BuildPath "$Configuration/ClipDock.exe"
}
if ($Run) {
    Start-Process -FilePath $ApplicationPath -WorkingDirectory (Split-Path $ApplicationPath -Parent)
}
