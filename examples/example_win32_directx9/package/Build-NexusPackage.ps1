param(
    [string]$Version = '1.0.0',
    # Проект создан в VS 2026 (v145). Для VS 2022 передайте -PlatformToolset v143.
    [string]$PlatformToolset = ''
)
# Собирает архив для Nexus Mods: package\dist\DLTB.RIP-v<Version>.zip
#   DLTB.RIP\DLTB.RIP.exe              — трейнер (способ 2: запуск во время игры)
#   DLTB.RIP\Start DLTB.RIP.bat        — способ 1: игра через Steam + трейнер
#   DLTB.RIP\README.txt
#   DLTB.RIP\items\...                 — рантайм Item Extravaganza (с разрешения автора) + его лицензии
#   DLTB.RIP\licenses\imgui-LICENSE.txt
$ErrorActionPreference = 'Stop'
$proj = Split-Path $PSScriptRoot -Parent
$dist = Join-Path $PSScriptRoot 'dist'
$stage = Join-Path $dist 'DLTB.RIP'
$zip = Join-Path $dist "DLTB.RIP-v$Version.zip"

# Мешает только запущенный exe из ЭТОЙ папки Release (он заблокирован для перезаписи)
$outExe = Join-Path $proj 'Release\DLTB.RIP.exe'
if (Test-Path $outExe) {
    try { [IO.File]::Open($outExe, 'Open', 'ReadWrite', 'None').Dispose() }
    catch { throw "$outExe is in use - close that DLTB.RIP.exe first." }
}

# 1. Трейнер (Release x64 -> Release\DLTB.RIP.exe)
$msb = Get-ChildItem "${env:ProgramFiles}\Microsoft Visual Studio\*\*\MSBuild\Current\Bin\MSBuild.exe" | Sort-Object FullName -Descending | Select-Object -First 1 -ExpandProperty FullName
if (!$msb) { throw 'MSBuild not found.' }
$toolsetArg = if ($PlatformToolset) { "/p:PlatformToolset=$PlatformToolset" } else { '/p:_NoToolsetOverride=1' }
& $msb (Join-Path $proj 'example_win32_directx9.vcxproj') /t:Rebuild /p:Configuration=Release /p:Platform=x64 $toolsetArg /m /nologo /v:minimal
if ($LASTEXITCODE -ne 0) { throw 'Trainer build failed.' }

# 2. Чистая папка
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Force -Path $stage, (Join-Path $stage 'licenses') | Out-Null
Copy-Item (Join-Path $proj 'Release\DLTB.RIP.exe') $stage

# 3. Рантайм Item Extravaganza с мостом -> <stage>\items
& (Join-Path $proj 'item_bridge\Build-DltbItemRuntime.ps1') -OutDir $stage
if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) { throw 'Item runtime build failed.' }

# 4. Батник и README с переводами строк Windows (cmd неверно обрабатывает goto в файлах с одними LF)
foreach ($name in 'Start DLTB.RIP.bat', 'README.txt') {
    $text = [IO.File]::ReadAllText((Join-Path $PSScriptRoot $name)) -replace "`r?`n", "`r`n"
    $enc = if ($name -like '*.bat') { [Text.Encoding]::ASCII } else { New-Object Text.UTF8Encoding($true) }   # README с BOM — Блокнот покажет кириллицу
    [IO.File]::WriteAllText((Join-Path $stage $name), $text, $enc)
}
Copy-Item (Join-Path $proj '..\..\LICENSE.txt') (Join-Path $stage 'licenses\imgui-LICENSE.txt')
Copy-Item (Join-Path $proj 'fonts\Manrope-OFL.txt') (Join-Path $stage 'licenses\Manrope-OFL.txt')
Copy-Item (Join-Path $proj 'fonts\FontAwesome-Free-LICENSE.txt') (Join-Path $stage 'licenses\FontAwesome-Free-LICENSE.txt')

# 5. Проверка состава и архив
$required = 'DLTB.RIP.exe', 'Start DLTB.RIP.bat', 'README.txt', 'items\DLTB_ItemRuntime.dll', 'items\GameItemCatalog.tsv',
            'items\GameAffixCatalog.tsv', 'items\GameBuiltInStats.tsv', 'items\LICENSE.txt', 'items\RIGHTS-AND-ATTRIBUTION.md'
foreach ($r in $required) { if (!(Test-Path (Join-Path $stage $r))) { throw "Missing in package: $r" } }
if (Select-String -Path (Join-Path $stage 'README.txt') -Pattern '<AUTHOR NAME>|<NEXUS LINK>' -Quiet) {
    Write-Warning 'README.txt still has <AUTHOR NAME>/<NEXUS LINK> placeholders - fill them in package\README.txt before uploading.'
}
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path $stage -DestinationPath $zip
Get-ChildItem $stage -Recurse -File | ForEach-Object { '{0,10:N0}  {1}' -f $_.Length, $_.FullName.Substring($dist.Length + 1) }
Write-Host ("Package: {0} ({1:N1} MB)" -f $zip, ((Get-Item $zip).Length / 1MB))
