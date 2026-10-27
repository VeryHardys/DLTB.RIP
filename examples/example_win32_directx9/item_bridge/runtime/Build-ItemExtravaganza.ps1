param(
    [string]$OutputDirectory = (Join-Path $PSScriptRoot 'bin'),
    [string]$IntermediateDirectory
)
$ErrorActionPreference = 'Stop'
$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
$intermediateRoot = if ($IntermediateDirectory) {
    [IO.Path]::GetFullPath($IntermediateDirectory)
} else { Join-Path $outputRoot 'obj' }

# Use x64 MSVC + Windows SDK; do not require a game installation or fetch tools.
if (!(Get-Command cl.exe -ErrorAction SilentlyContinue) -or
    !(Get-Command rc.exe -ErrorAction SilentlyContinue) -or
    $env:VSCMD_ARG_TGT_ARCH -ne 'x64') {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (!(Test-Path -LiteralPath $vswhere)) {
        throw 'Install Visual Studio C++ build tools and Windows SDK, or use an x64 Native Tools prompt.'
    }
    $vsRoot = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$vsRoot) { throw 'No installed MSVC x64 toolchain found.' }
    $devCommand = Join-Path $vsRoot 'Common7/Tools/VsDevCmd.bat'
    $environmentLines = & cmd.exe /d /s /c "call `"$devCommand`" -arch=x64 -host_arch=x64 >nul && set"
    if ($LASTEXITCODE -ne 0) { throw 'Could not initialize the MSVC environment.' }
    foreach ($line in $environmentLines) {
        if ($line -match '^([^=]+)=(.*)$') {
            [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
        }
    }
}
New-Item -ItemType Directory -Force -Path $outputRoot,$intermediateRoot | Out-Null
$launcherResource = Join-Path $intermediateRoot 'ItemExtravaganzaLauncher.res'
$runtimeResource = Join-Path $intermediateRoot 'ItemExtravaganzaRuntime.res'
Push-Location $PSScriptRoot
try {
    & rc.exe /nologo "/fo$launcherResource" ItemExtravaganzaIcons.rc
    if ($LASTEXITCODE -ne 0) { throw 'Launcher resource build failed.' }
    & rc.exe /nologo /dIE_RUNTIME_DLL "/fo$runtimeResource" ItemExtravaganzaIcons.rc
    if ($LASTEXITCODE -ne 0) { throw 'Runtime resource build failed.' }
    & cl.exe /nologo /utf-8 /O2 /W4 /EHsc /DIE_AFFIX_REMOVE_POC /DIE_OWNED_AFFIX_EDITOR /DIE_GIVE_RANK_LEVEL /LD "/Fo:$(Join-Path $intermediateRoot 'ItemExtravaganzaRuntime.obj')" "/Fe:$(Join-Path $outputRoot 'ItemExtravaganzaRuntime.dll')" ItemExtravaganzaRuntime.cpp $runtimeResource user32.lib comctl32.lib gdi32.lib
    if ($LASTEXITCODE -ne 0) { throw 'Runtime build failed.' }
    & cl.exe /nologo /utf-8 /O2 /W4 /EHsc "/Fo:$(Join-Path $intermediateRoot 'ItemExtravaganzaLauncher.obj')" "/Fe:$(Join-Path $outputRoot 'ItemExtravaganza.exe')" ItemExtravaganzaLauncher.cpp $launcherResource shell32.lib user32.lib /link /SUBSYSTEM:WINDOWS
    if ($LASTEXITCODE -ne 0) { throw 'Launcher build failed.' }
} finally { Pop-Location }
Write-Host "Built Item Extravaganza v1.1: $outputRoot"
