param(
    # Папка трейнера (рядом с exe); runtime и данные кладутся в <OutDir>\items
    [string]$OutDir = (Join-Path $PSScriptRoot '..\Release')
)
# Собирает рантайм Item Extravaganza с мостом DLTB (IE_DLTB_BRIDGE) по образцу
# исходного Build-ItemExtravaganza.ps1: те же флаги функций + мост.
$ErrorActionPreference = 'Stop'
$itemsDir = Join-Path ([IO.Path]::GetFullPath($OutDir)) 'items'
$objDir = Join-Path $PSScriptRoot 'obj'

if (!(Get-Command cl.exe -ErrorAction SilentlyContinue) -or $env:VSCMD_ARG_TGT_ARCH -ne 'x64') {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (!(Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio C++ build tools not found.' }
    $vsRoot = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$vsRoot) { throw 'No installed MSVC x64 toolchain found.' }
    $devCommand = Join-Path $vsRoot 'Common7/Tools/VsDevCmd.bat'
    $environmentLines = & cmd.exe /d /s /c "call `"$devCommand`" -arch=x64 -host_arch=x64 >nul 2>nul && set"
    if ($LASTEXITCODE -ne 0) { throw 'Could not initialize the MSVC environment.' }
    foreach ($line in $environmentLines) {
        if ($line -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process') }
    }
}
New-Item -ItemType Directory -Force -Path $itemsDir, $objDir | Out-Null
$res = Join-Path $objDir 'DltbItemRuntime.res'
Push-Location (Join-Path $PSScriptRoot 'runtime')
try {
    & rc.exe /nologo /dIE_RUNTIME_DLL "/fo$res" ItemExtravaganzaIcons.rc
    if ($LASTEXITCODE -ne 0) { throw 'Runtime resource build failed.' }
    & cl.exe /nologo /utf-8 /O2 /W4 /EHsc /DIE_AFFIX_REMOVE_POC /DIE_OWNED_AFFIX_EDITOR /DIE_GIVE_RANK_LEVEL /DIE_DLTB_BRIDGE /LD `
        "/Fo:$(Join-Path $objDir 'DltbItemRuntime.obj')" "/Fe:$(Join-Path $itemsDir 'DLTB_ItemRuntime.dll')" `
        ItemExtravaganzaRuntime.cpp $res user32.lib comctl32.lib gdi32.lib
    if ($LASTEXITCODE -ne 0) { throw 'Runtime build failed.' }
} finally { Pop-Location }
# Рантайм читает каталоги из своей папки
Copy-Item (Join-Path $PSScriptRoot 'data\*') $itemsDir -Force
Remove-Item (Join-Path $itemsDir 'DLTB_ItemRuntime.exp'), (Join-Path $itemsDir 'DLTB_ItemRuntime.lib') -ErrorAction SilentlyContinue
Write-Host "Built DLTB_ItemRuntime.dll -> $itemsDir"
