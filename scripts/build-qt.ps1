param(
    [string]$QtRoot = $env:QT_ROOT,
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [switch]$Test,
    [switch]$Package
)
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path $PSScriptRoot -Parent
$BuildPath = Join-Path $ProjectRoot 'build/qt'
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw 'CMake 3.24+, Visual Studio C++ tools and Qt 6.8+ (MSVC, Multimedia) are required.'
}
$ConfigureArgs = @('-S', $ProjectRoot, '-B', $BuildPath, '-DBUILD_TESTING=ON')
if ($QtRoot) { $ConfigureArgs += "-DCMAKE_PREFIX_PATH=$QtRoot" }
& cmake @ConfigureArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& cmake --build $BuildPath --config $Configuration --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if ($Test) {
    & ctest --test-dir $BuildPath -C $Configuration --output-on-failure
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
if ($Package) {
    & cmake --install $BuildPath --config $Configuration --prefix (Join-Path $ProjectRoot 'dist/qt')
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
