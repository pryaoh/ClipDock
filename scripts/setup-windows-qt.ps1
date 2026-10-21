param([string]$Python)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'windows-qt.ps1')
if ($env:OS -ne 'Windows_NT') { throw 'Run this script on Windows.' }
$ProjectRoot = Split-Path $PSScriptRoot -Parent
$Kit = Join-Path $ProjectRoot ".tools/qt/$script:ClipDockQtVersion/msvc2022_64"
if (Test-ClipDockQtSdk $Kit) {
    Write-Host "Qt SDK ready: $Kit"
    return
}
$Installer = Join-Path $ProjectRoot '.tools/qt-installer-windows'
$InstallerPython = Join-Path $Installer 'Scripts/python.exe'
if (-not (Test-Path -LiteralPath $InstallerPython)) {
    if (-not $Python -and (Get-Command py -ErrorAction SilentlyContinue)) { $Python = & py -3 -c 'import sys; print(sys.executable)' }
    if (-not $Python) {
        $Command = Get-Command python -ErrorAction SilentlyContinue
        if ($Command -and $Command.Source -notlike '*WindowsApps*') { $Python = $Command.Source }
    }
    if (-not $Python -and (Get-Command uv -ErrorAction SilentlyContinue)) {
        $Python = & uv python find --no-python-downloads --cache-dir (Join-Path $ProjectRoot '.tools/uv-cache')
    }
    if (-not $Python) { throw 'Install Python 3.9+ from python.org, or pass -Python C:/path/to/python.exe.' }
    & $Python -m venv $Installer
    if ($LASTEXITCODE -ne 0) { throw 'Failed to create the project-local Qt installer environment.' }
}
& $InstallerPython -c "import importlib.util, sys; sys.exit(0 if importlib.util.find_spec('aqt') else 1)"
if ($LASTEXITCODE -ne 0) {
    & $InstallerPython -m pip --disable-pip-version-check install --no-cache-dir 'aqtinstall>=3.3,<4'
    if ($LASTEXITCODE -ne 0) { throw 'Qt installer download failed. Check your internet connection and retry.' }
}
Write-Host "Installing Qt $script:ClipDockQtVersion (MSVC x64, Multimedia, Shader Tools) into .tools/qt."
& $InstallerPython -m aqt install-qt windows desktop $script:ClipDockQtVersion win64_msvc2022_64 --outputdir (Join-Path $ProjectRoot '.tools/qt') --modules qtmultimedia qtshadertools
if ($LASTEXITCODE -ne 0) { throw 'Qt SDK download failed. Check your internet connection and retry.' }
if (-not (Test-ClipDockQtSdk $Kit)) { throw "Qt installation is missing Multimedia or the FFmpeg plugin: $Kit" }
Write-Host "Qt SDK ready: $Kit"
