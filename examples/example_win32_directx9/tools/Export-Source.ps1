param(
    [string]$Destination = (Join-Path $PSScriptRoot '..\..\..\..\DLTB.RIP-source')
)
# Copies a clean, buildable source tree for GitHub (whitelist only: no backups, builds,
# archives or local configs). Layout is preserved because the project includes Dear ImGui
# via ..\..\ paths.
$ErrorActionPreference = 'Stop'
$imguiRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$project = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$dest = [IO.Path]::GetFullPath($Destination)
if (Test-Path $dest) { Remove-Item $dest -Recurse -Force }

function Copy-Rel([string]$root, [string[]]$relPaths, [string]$destRoot) {
    foreach ($rel in $relPaths) {
        $src = Join-Path $root $rel
        if (!(Test-Path $src)) { throw "Missing source file: $src" }
        $dst = Join-Path $destRoot $rel
        New-Item -ItemType Directory -Force (Split-Path $dst) | Out-Null
        Copy-Item $src $dst
    }
}

# Dear ImGui (repo root) - only what the project compiles/includes
Copy-Rel $imguiRoot @(
    'imgui.cpp', 'imgui.h', 'imgui_internal.h', 'imgui_draw.cpp', 'imgui_widgets.cpp', 'imgui_tables.cpp', 'imgui_demo.cpp',
    'imconfig.h', 'imstb_rectpack.h', 'imstb_textedit.h', 'imstb_truetype.h', 'LICENSE.txt',
    'backends\imgui_impl_dx9.cpp', 'backends\imgui_impl_dx9.h', 'backends\imgui_impl_win32.cpp', 'backends\imgui_impl_win32.h',
    'misc\debuggers\imgui.natvis', 'misc\debuggers\imgui.natstepfilter', 'examples\README.txt'
) $dest

# The trainer project
$projRel = 'examples\example_win32_directx9'
Copy-Rel $project @(
    'example_win32_directx9.vcxproj', 'example_win32_directx9.vcxproj.filters',
    'main.cpp', 'gui.cpp', 'gui.hpp', 'color_t.hpp', 'hashes.hpp', 'bytes.hpp',
    'blur.hpp', 'blur_x.hpp', 'blur_y.hpp', 'imgui_freetype.cpp', 'imgui_freetype.hpp',
    'fx_config.hpp', 'fx_system.hpp', 'chip_sound.hpp', 'item_menu.hpp',
    'fonts\Manrope-Medium.ttf', 'fonts\Manrope-ExtraBold.ttf', 'fonts\Manrope-OFL.txt',
    'fonts\fa-solid-900.ttf', 'fonts\FontAwesome-Free-LICENSE.txt',
    'tools\Embed-Fonts.ps1', 'tools\Export-Source.ps1',
    'tools\repo\README.md', 'tools\repo\.gitignore', 'tools\repo\THIRD-PARTY-NOTICES.md',
    'package\Start DLTB.RIP.bat', 'package\README.txt', 'package\Build-NexusPackage.ps1',
    'item_bridge\DltbItemBridge.h', 'item_bridge\Build-DltbItemRuntime.ps1'
) (Join-Path $dest $projRel)

# Item Extravaganza runtime source + data (all files in those two folders)
foreach ($sub in 'item_bridge\runtime', 'item_bridge\data') {
    $files = Get-ChildItem (Join-Path $project $sub) -File | ForEach-Object { Join-Path $sub $_.Name }
    Copy-Rel $project $files (Join-Path $dest $projRel)
}

# Repo files (README, .gitignore, notices) live next to this script
foreach ($name in 'README.md', '.gitignore', 'THIRD-PARTY-NOTICES.md') {
    Copy-Item (Join-Path $PSScriptRoot "repo\$name") (Join-Path $dest $name)
}
$count = (Get-ChildItem $dest -Recurse -File).Count
$size = (Get-ChildItem $dest -Recurse -File | Measure-Object Length -Sum).Sum / 1MB
Write-Host ("Exported {0} files ({1:N1} MB) -> {2}" -f $count, $size, $dest)
