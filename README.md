# DLTB.RIP

Single-player trainer and visual mod for **Dying Light: The Beast** with an in-game overlay menu (Dear ImGui, DirectX 9).

- Nexus Mods page: `<NEXUS PAGE LINK>`
- Ready-to-use download: see the Nexus page. This repository is the full source code, provided for review and local builds.

## Repository layout

| Path | What it is |
|---|---|
| `examples/example_win32_directx9/` | The trainer project (`example_win32_directx9.vcxproj`, output `DLTB.RIP.exe`) |
| `examples/example_win32_directx9/main.cpp` | Overlay, menu and all memory features |
| `examples/example_win32_directx9/fx_*.hpp`, `chip_sound.hpp` | Hitmarkers, kill effects, synthesized sounds |
| `examples/example_win32_directx9/item_menu.hpp` | Items tab: client for the Item Extravaganza runtime |
| `examples/example_win32_directx9/item_bridge/` | Item Extravaganza runtime source + data, plus the DLTB bridge (`runtime/DltbBridge.inl`) |
| `examples/example_win32_directx9/fonts/`, `tools/Embed-Fonts.ps1` | Free fonts and the script that generates `bytes.hpp` from them |
| `examples/example_win32_directx9/package/` | Launcher `.bat`, user README and the release packaging script |
| repository root, `backends/` | Dear ImGui (modified widgets/styling) |

## Building

Requirements:
- Windows 10/11 x64
- Visual Studio 2022 or newer with the **Desktop development with C++** workload and a Windows 10/11 SDK
- PowerShell 5.1 (built into Windows)

### One command (same archive as on Nexus)

```powershell
powershell -ExecutionPolicy Bypass -File examples\example_win32_directx9\package\Build-NexusPackage.ps1 -Version 1.0.0
```

The project was created with Visual Studio 2026 (toolset `v145`). With Visual Studio 2022 add `-PlatformToolset v143`.

The script:
1. builds `DLTB.RIP.exe` (Release | x64, static CRT, no extra DLLs);
2. builds `items\DLTB_ItemRuntime.dll` from `item_bridge\runtime` with `/DIE_DLTB_BRIDGE` (same feature flags as the original Item Extravaganza build);
3. copies the launcher, README and data files and writes `package\dist\DLTB.RIP-v<Version>.zip`.

### Manually

1. Open `examples\example_win32_directx9\example_win32_directx9.vcxproj` in Visual Studio, select **Release | x64**, build. Output: `examples\example_win32_directx9\Release\DLTB.RIP.exe`.
2. Build the item runtime next to it:
   ```powershell
   powershell -ExecutionPolicy Bypass -File examples\example_win32_directx9\item_bridge\Build-DltbItemRuntime.ps1 -OutDir examples\example_win32_directx9\Release
   ```

Binary hashes depend on the exact compiler version, so a local build may not be byte-identical to the uploaded one.

## How it works (for reviewers)

- `DLTB.RIP.exe` is an **external** process: a transparent topmost overlay window. It opens the game process and toggles features with `ReadProcessMemory`/`WriteProcessMemory` (byte patches and small code caves found by signature scan). It requests administrator rights (UAC) for this.
- On exit (menu unload key, closing the game, or starting a second copy) it restores every patch it made.
- The Items tab can load `items\DLTB_ItemRuntime.dll` into the game with `LoadLibraryW` (the same way Item Extravaganza's own launcher does). The DLL exchanges data with the trainer through a named shared-memory section (`item_bridge/DltbItemBridge.h`) and runs the mod's own validated native calls on the game thread. Nothing is written to the game folder.
- No network access, no telemetry. Settings are stored in `configs\` next to the exe.
- Intended for single-player only.

## Credits & licenses

See [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md). Item Extravaganza is included with its author's permission. Not affiliated with Techland, Valve or Nexus Mods.

---

### Кратко по-русски

Исходный код трейнера DLTB.RIP для Dying Light: The Beast. Сборка: Visual Studio 2022+ с «Разработкой классических приложений на C++», затем одна команда — `package\Build-NexusPackage.ps1` (для VS 2022 добавьте `-PlatformToolset v143`). Получится тот же архив, что выложен на Nexus.
