# Item Extravaganza v1.1: lightweight source

This is the launcher/runtime source used to build the packaged binaries, together with every required local helper, version header and original mod icon resource. It is not needed to run the tool. The full development workspace, game binaries, disassembly, live captures, private paths, saves and third-party tool binaries are not included.

## Build locally

Install Visual Studio/Build Tools with Desktop development with C++ (MSVC x64) and the Windows SDK. Open PowerShell and run:

```powershell
.\Build-ItemExtravaganza.ps1
```

The script finds an installed x64 toolchain or uses an initialized x64 Native Tools environment; it does not download tools or need a game installation. Outputs go to `Source/bin`, intermediates to its `obj` folder. Copy the three TSV catalogues and `ItemExtravaganza.config.ini` beside your locally built EXE/DLL before running. Use a separate folder and fully restart the game to change runtime builds. Local compiler/linker versions and PE timestamps can change hashes; this is not a bit-reproducible build promise.

The release build enables `IE_AFFIX_REMOVE_POC`, `IE_OWNED_AFFIX_EDITOR` and `IE_GIVE_RANK_LEVEL`. The first macro supplies verified record primitives; legacy keyboard POC shortcuts are disabled when the owned editor is compiled. Omitting required flags removes features or is rejected. Keep the EXE/DLL version header and resource paired.

`GiveWeaponLevel.inl` is the guarded creation-time R1-R15 correction, not global player-level enforcement. `OwnedAffixReader.inl` reads weapon/gear instances and actual levels; `OwnedAffixEditor.inl` owns the native record session. Duplicate/repair, float32 conversion, hotkey and window-scope helpers are separate. Catalogue generation/reverse-engineering scripts are intentionally not included; shipped TSVs are needed for the UI.

Native addresses and ABI are tied to the checked retail module. Do not port them to another game build, strip ownership/signature/fault checks, or add speculative fields. Source includes retained compatibility identifiers and internal development comments; these are not additional exposed product features. Inspect the code and safety constraints before reuse; source availability is not a license grant or security certification. See the package's rights and antivirus notices.
