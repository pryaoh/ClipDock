# Shared by the Windows setup, build and SDK regression checks (PowerShell 5.1+).
$script:ClipDockQtVersion = '6.8.3'

function Test-ClipDockQtSdk([string]$QtRoot) {
    if (-not $QtRoot) { return $false }
    foreach ($RelativePath in @('lib/cmake/Qt6/Qt6Config.cmake',
            'lib/cmake/Qt6Multimedia/Qt6MultimediaConfig.cmake',
            'plugins/multimedia/ffmpegmediaplugin.dll', 'bin/Qt6Core.dll', 'mkspecs/qconfig.pri')) {
        if (-not (Test-Path -LiteralPath (Join-Path $QtRoot $RelativePath) -PathType Leaf)) { return $false }
    }
    $Metadata = Get-Content -LiteralPath (Join-Path $QtRoot 'mkspecs/qconfig.pri') -Raw
    $VersionMatch = [regex]::Match($Metadata, '(?m)^QT_VERSION\s*=\s*(\d+\.\d+\.\d+)\s*$')
    return $VersionMatch.Success -and ([version]$VersionMatch.Groups[1].Value -ge [version]'6.8.0') -and
        ($Metadata -match '(?m)^QT_ARCH\s*=\s*x86_64\s*$') -and
        ($Metadata -match '(?m)^QT_MSVC_MAJOR_VERSION\s*=\s*19\s*$')
}

function Find-ClipDockQtSdk([string]$ProjectRoot, [string]$QtRoot) {
    if ($QtRoot) {
        if (-not (Test-ClipDockQtSdk $QtRoot)) {
            throw "QtRoot must point to a Windows MSVC x64 Qt 6.8+ kit with Multimedia and the FFmpeg plugin: $QtRoot"
        }
        return (Resolve-Path -LiteralPath $QtRoot).Path
    }
    $LocalKit = Join-Path $ProjectRoot ".tools/qt/$script:ClipDockQtVersion/msvc2022_64"
    if (Test-ClipDockQtSdk $LocalKit) { return $LocalKit }
    if ($env:QTDIR -and (Test-ClipDockQtSdk $env:QTDIR)) { return $env:QTDIR }
    if (Test-Path -LiteralPath 'C:/Qt') {
        foreach ($Version in (Get-ChildItem -LiteralPath 'C:/Qt' -Directory | Sort-Object Name -Descending)) {
            $Kit = Join-Path $Version.FullName 'msvc2022_64'
            if (Test-ClipDockQtSdk $Kit) { return $Kit }
        }
    }
    return $null
}

function Initialize-ClipDockBuildTools {
    if ($env:OS -ne 'Windows_NT') { throw 'Run this script on Windows.' }
    $VsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (Test-Path -LiteralPath $VsWhere) {
        $VsRoot = & $VsWhere -latest -products '*' -version '[17.0,18.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if (-not $VsRoot) {
            $VsRoot = & $VsWhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        }
        if ($VsRoot) {
            $CMakeBin = Join-Path $VsRoot 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin'
            $NinjaBin = Join-Path $VsRoot 'Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja'
            if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { $env:PATH = "$CMakeBin;$env:PATH" }
            if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) { $env:PATH = "$NinjaBin;$env:PATH" }
            # Match the Qt MSVC 2022 kit. Newer VS installations can also host v143.
            # MSVC 19.51 currently crashes while optimizing the settings test (/O2).
            $Toolset = Get-ChildItem -LiteralPath (Join-Path $VsRoot 'VC/Tools/MSVC') -Directory |
                Where-Object { $_.Name -match '^14\.(3\d|4\d)\.' } |
                Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
            if (-not $Toolset) {
                throw 'Install MSVC v143 - VS 2022 C++ x64/x86 build tools in Visual Studio Installer (Individual components).'
            }
            Import-Module (Join-Path $VsRoot 'Common7/Tools/Microsoft.VisualStudio.DevShell.dll')
            Enter-VsDevShell -VsInstallPath $VsRoot -SkipAutomaticLocation -DevCmdArguments "-arch=x64 -host_arch=x64 -vcvars_ver=$($Toolset.Name) -no_logo" | Out-Null
        }
    }
    foreach ($Tool in @('cmake', 'ctest', 'ninja', 'cl')) {
        if (-not (Get-Command $Tool -ErrorAction SilentlyContinue)) {
            throw "Missing $Tool. Install Visual Studio 2022 or later with Desktop development with C++ and C++ CMake tools for Windows."
        }
    }
}
