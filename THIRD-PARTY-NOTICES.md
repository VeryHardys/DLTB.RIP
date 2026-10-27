# Third-party components

| Component | Where | License |
|---|---|---|
| Dear ImGui (Omar Cornut and contributors), with local modifications to widgets and styling | repository root, `backends/` | MIT — `LICENSE.txt` |
| Manrope font (The Manrope Project Authors, Mikhail Sharanda) | `examples/example_win32_directx9/fonts/Manrope-*.ttf`, embedded in `bytes.hpp` | SIL Open Font License 1.1 — `fonts/Manrope-OFL.txt` |
| Font Awesome 5 Free, Solid (Fonticons, Inc.) | `fonts/fa-solid-900.ttf`, embedded in `bytes.hpp` | Font: SIL OFL 1.1 — `fonts/FontAwesome-Free-LICENSE.txt` |
| Font Awesome icon codepoint definitions (`hashes.hpp`) | `examples/example_win32_directx9/hashes.hpp` | Generated icon-constant header (IconFontCppHeaders style) |
| Item Extravaganza v1.1 by `<AUTHOR NAME>` (`<NEXUS LINK>`) | `examples/example_win32_directx9/item_bridge/runtime`, `item_bridge/data` | Included **with the author's permission**. No open-source license is granted; see `item_bridge/data/LICENSE.txt`, `COPYRIGHT.txt`, `RIGHTS-AND-ATTRIBUTION.md`. The `.tsv` catalogues contain game-derived identifiers and names owned by their rights holders. |

DLTB.RIP-specific changes to Item Extravaganza are limited to the `IE_DLTB_BRIDGE` hooks in `ItemExtravaganzaRuntime.cpp` and the new file `runtime/DltbBridge.inl`.

Dying Light: The Beast and related names are trademarks of Techland. This project is not affiliated with Techland, Valve or Nexus Mods.
