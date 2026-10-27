#pragma once
// ============================================================================
//  Items -> Item Extravaganza: клиент моста + вкладки меню.
//
//  Движок — рантайм Item Extravaganza (items\DLTB_ItemRuntime.dll, собран из
//  его исходников с IE_DLTB_BRIDGE). Трейнер подгружает его в игру один раз
//  за сессию, читает каталог/предметы/статус из общей памяти и отправляет
//  команды. Все проверки безопасности и нативные вызовы остаются в рантайме.
//
//  Требует (объявлены в main.cpp до подключения): GameMemory, gui, MenuVars,
//  TopNotificationSystem, ICON_FA_*.
// ============================================================================
#include "item_bridge/DltbItemBridge.h"
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <cfloat>
#include <cerrno>
#include <algorithm>

namespace ItemBridge {
    static HANDLE mapping = nullptr;
    static DltbBridgeShared* view = nullptr;
    static DWORD mappedPid = 0;
    static ULONGLONG lastOpenTry = 0, lastBeatTick = 0;
    static long lastBeat = -1;
    static bool injecting = false;
    static std::string injectError;

    // локальные копии (обновляются по версиям)
    static std::vector<DltbCatalogItem> catalog;
    static std::vector<DltbOwnedItem> owned;
    static std::vector<DltbAffixMeta> affixMeta;
    static std::vector<DltbBuiltInStat> builtIn;
    static long catalogVersion = -1, ownedVersion = -1;
    static bool catalogChanged = false, ownedChanged = false;

    struct Flags { int ready, moduleLoaded, actionPending, editPending, transaction, editFaulted, duplicateFaulted, lookupOk, extremesConfirmed; uint64_t transactionItem; char transactionItemId[160]; };
    static Flags flags = {};
    static char status[512] = "";
    static std::vector<std::string> logLines;
    static long logSeen = 0;

    static std::string Utf8(const wchar_t* w) {
        char buf[1024] = {};
        WideCharToMultiByte(CP_UTF8, 0, w ? w : L"", -1, buf, sizeof(buf) - 1, nullptr, nullptr);
        return buf;
    }

    static std::wstring RuntimePath() {
        wchar_t path[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        wchar_t* slash = wcsrchr(path, L'\\');
        if (slash) slash[1] = 0;
        return std::wstring(path) + L"items\\DLTB_ItemRuntime.dll";
    }
    static bool RuntimeFilesPresent() { return GetFileAttributesW(RuntimePath().c_str()) != INVALID_FILE_ATTRIBUTES; }

    static void Unmap() {
        if (view) UnmapViewOfFile(view);
        if (mapping) CloseHandle(mapping);
        view = nullptr; mapping = nullptr; mappedPid = 0;
        catalog.clear(); owned.clear(); affixMeta.clear(); builtIn.clear();
        catalogVersion = ownedVersion = -1; lastBeat = -1; logSeen = 0; logLines.clear();
        flags = {}; status[0] = 0;
    }

    static bool TryOpen(DWORD pid) {
        wchar_t name[64] = {};
        _snwprintf_s(name, _countof(name), _TRUNCATE, DLTB_BRIDGE_NAME_FMT, pid);
        HANDLE h = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, name);
        if (!h) return false;
        auto* v = (DltbBridgeShared*)MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(DltbBridgeShared));
        if (!v) { CloseHandle(h); return false; }
        mapping = h; view = v; mappedPid = pid;
        return true;
    }

    // Рантайм жив: магия совпала и сердцебиение шло последние ~2 с
    static bool Connected() {
        return view && view->magic == DLTB_BRIDGE_MAGIC && view->version == DLTB_BRIDGE_VERSION && GetTickCount64() - lastBeatTick < 2000;
    }

    // LoadLibraryW в игре — так же, как это делает лаунчер Item Extravaganza
    static bool Inject(HANDLE process) {
        injectError.clear();
        const std::wstring path = RuntimePath();
        if (!RuntimeFilesPresent()) { injectError = "items\\DLTB_ItemRuntime.dll not found next to the trainer"; return false; }
        const SIZE_T bytes = (path.size() + 1) * sizeof(wchar_t);
        void* remote = VirtualAllocEx(process, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!remote) { injectError = "VirtualAllocEx failed"; return false; }
        bool ok = false;
        if (WriteProcessMemory(process, remote, path.c_str(), bytes, nullptr)) {
            FARPROC loadLibrary = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
            HANDLE thread = CreateRemoteThread(process, nullptr, 0, (LPTHREAD_START_ROUTINE)loadLibrary, remote, 0, nullptr);
            if (thread) {
                if (WaitForSingleObject(thread, 8000) == WAIT_OBJECT_0) {
                    DWORD code = 0; GetExitCodeThread(thread, &code);
                    ok = code != 0;
                    if (!ok) injectError = "the game refused to load the runtime DLL";
                }
                else injectError = "timed out waiting for the runtime to load";
                CloseHandle(thread);
            }
            else injectError = "CreateRemoteThread failed";
        }
        else injectError = "WriteProcessMemory failed";
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        return ok;
    }

    static void Update(DWORD pid) {
        catalogChanged = ownedChanged = false;
        if (pid == 0) { if (view) Unmap(); return; }
        if (view && mappedPid != pid) Unmap();
        if (!view) {
            ULONGLONG now = GetTickCount64();
            if (now - lastOpenTry < 500) return;
            lastOpenTry = now;
            if (!TryOpen(pid)) return;
        }
        if (view->magic != DLTB_BRIDGE_MAGIC) return;
        if (view->heartbeat != lastBeat) { lastBeat = view->heartbeat; lastBeatTick = GetTickCount64(); }

        // seqlock: читаем согласованный снимок, иначе пробуем в следующем кадре
        const long seq0 = view->writeSeq;
        if (seq0 & 1) return;
        MemoryBarrier();
        Flags f = {};
        f.ready = view->ready; f.moduleLoaded = view->gameModuleLoaded; f.actionPending = view->actionPending;
        f.editPending = view->editPending; f.transaction = view->transaction; f.editFaulted = view->editFaulted;
        f.duplicateFaulted = view->duplicateFaulted; f.lookupOk = view->lookupProbePassed; f.extremesConfirmed = view->extremesConfirmed;
        f.transactionItem = view->transactionItem;
        memcpy(f.transactionItemId, view->transactionItemId, sizeof(f.transactionItemId));
        std::string st = Utf8(view->status);
        const long cv = view->catalogVersion, ov = view->ownedVersion;
        std::vector<DltbCatalogItem> newCatalog; std::vector<DltbAffixMeta> newMeta; std::vector<DltbBuiltInStat> newBuilt;
        std::vector<DltbOwnedItem> newOwned;
        if (cv != catalogVersion) {
            newCatalog.assign(view->catalog, view->catalog + (view->catalogCount < DLTB_MAX_CATALOG ? view->catalogCount : DLTB_MAX_CATALOG));
            newMeta.assign(view->affixMeta, view->affixMeta + (view->affixMetaCount < DLTB_MAX_AFFIX_META ? view->affixMetaCount : DLTB_MAX_AFFIX_META));
            newBuilt.assign(view->builtIn, view->builtIn + (view->builtInCount < DLTB_MAX_BUILTIN ? view->builtInCount : DLTB_MAX_BUILTIN));
        }
        if (ov != ownedVersion)
            newOwned.assign(view->owned, view->owned + (view->ownedCount < DLTB_MAX_OWNED ? view->ownedCount : DLTB_MAX_OWNED));
        MemoryBarrier();
        if (view->writeSeq != seq0) return;   // рантайм писал во время чтения

        flags = f;
        snprintf(status, sizeof(status), "%s", st.c_str());
        if (cv != catalogVersion) { catalog.swap(newCatalog); affixMeta.swap(newMeta); builtIn.swap(newBuilt); catalogVersion = cv; catalogChanged = true; }
        if (ov != ownedVersion) { owned.swap(newOwned); ownedVersion = ov; ownedChanged = true; }

        const long total = view->logCount;
        if (total - logSeen > DLTB_LOG_LINES) logSeen = total - DLTB_LOG_LINES;
        for (; logSeen < total; ++logSeen) {
            logLines.push_back(Utf8(view->log[logSeen % DLTB_LOG_LINES]));
            if (logLines.size() > 200) logLines.erase(logLines.begin());
        }
    }

    static bool Busy() { return !view || view->cmdSeq != view->ackSeq; }

    static bool Send(const DltbCommand& cmd) {
        if (!Connected() || Busy()) return false;
        view->cmd = cmd;
        MemoryBarrier();
        InterlockedIncrement(&view->cmdSeq);
        return true;
    }
}

namespace ItemMenu {
    using namespace ImGui;

    // ---- общие помощники --------------------------------------------------
    static bool ContainsI(const char* hay, const char* needle) {
        if (!needle || !needle[0]) return true;
        if (!hay) return false;
        const size_t n = strlen(needle);
        for (const char* p = hay; *p; ++p) if (!_strnicmp(p, needle, n)) return true;
        return false;
    }

    static ImVec4 RarityColor(const char* rarity) {
        if (ContainsI(rarity, "legend")) return ImVec4(1.00f, 0.66f, 0.18f, 1.0f);
        if (ContainsI(rarity, "epic")) return ImVec4(0.74f, 0.45f, 1.00f, 1.0f);
        if (ContainsI(rarity, "rare")) return ImVec4(0.36f, 0.62f, 1.00f, 1.0f);
        if (ContainsI(rarity, "uncommon")) return ImVec4(0.42f, 0.86f, 0.45f, 1.0f);
        if (ContainsI(rarity, "unique") || ContainsI(rarity, "gold")) return ImVec4(1.00f, 0.84f, 0.30f, 1.0f);
        return ImVec4(gui.text_disabled.r, gui.text_disabled.g, gui.text_disabled.b, 1.0f);
    }

    static ImVec4 Accent(float a = 1.0f) { return ImVec4(gui.accent_color.r, gui.accent_color.g, gui.accent_color.b, a); }

    // Строка списка: выделение в цвете темы, текст слева, бейдж справа
    static bool Row(const char* id, bool selected, const char* text, ImVec4 textCol, const char* badge, ImVec4 badgeCol) {
        ImGuiWindow* window = GetCurrentWindow();
        ImVec2 pos = window->DC.CursorPos;
        const float w = GetContentRegionAvail().x, h = 24.0f;
        ImRect bb(pos, ImVec2(pos.x + w, pos.y + h));
        ImGuiID gid = window->GetID(id);
        ItemSize(bb);
        if (!ItemAdd(bb, gid)) return false;
        bool hovered, held;
        bool pressed = ButtonBehavior(bb, gid, &hovered, &held);
        ImDrawList* dl = window->DrawList;
        if (selected) {
            dl->AddRectFilled(bb.Min, bb.Max, gui.frame_active.to_im_color(), 4.0f);
            dl->AddRectFilled(ImVec2(bb.Min.x, bb.Min.y + 5), ImVec2(bb.Min.x + 2.5f, bb.Max.y - 5), gui.accent_color.to_im_color(), 1.0f);
        }
        else if (hovered) dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(255, 255, 255, 10), 4.0f);
        float badgeW = badge && badge[0] ? CalcTextSize(badge).x + 8.0f : 0.0f;
        ImVec2 ts = CalcTextSize(text);
        float y = std::floor(bb.Min.y + (h - ts.y) * 0.5f);
        gui.side_label(dl, ImVec2(bb.Min.x + 8, y), bb.Max.x - badgeW - 6, ImGui::ColorConvertFloat4ToU32(ImVec4(textCol.x, textCol.y, textCol.z, textCol.w * GetStyle().Alpha)), text);
        if (badgeW > 0.0f)
            dl->AddText(ImVec2(bb.Max.x - badgeW, y), ImGui::ColorConvertFloat4ToU32(ImVec4(badgeCol.x, badgeCol.y, badgeCol.z, badgeCol.w * GetStyle().Alpha)), badge);
        return pressed;
    }

    static void StatusLine() {
        if (!ItemBridge::status[0]) return;
        bool bad = ContainsI(ItemBridge::status, "fail") || ContainsI(ItemBridge::status, "reject") || ContainsI(ItemBridge::status, "could not") ||
                   ContainsI(ItemBridge::status, "blocked") || ContainsI(ItemBridge::status, "locked");
        PushStyleColor(ImGuiCol_Text, bad ? ImVec4(1.0f, 0.55f, 0.45f, 1.0f) : Accent());
        PushTextWrapPos(GetWindowWidth() - 4.0f);
        TextWrapped("%s", ItemBridge::status);
        PopTextWrapPos();
        PopStyleColor();
    }

    // ---- подключение ------------------------------------------------------
    // Возвращает true, когда рантайм подключён и можно рисовать вкладку
    static bool ConnectionPanel(bool gameConnected) {
        if (ItemBridge::Connected()) return true;
        const float w = GetWindowWidth();
        gui.group_box(ICON_FA_PALLET " Item Extravaganza", ImVec2(w, 250)); {
            if (!gameConnected) {
                TextDisabled("Start the game and load your save first.");
            }
            else if (!ItemBridge::RuntimeFilesPresent()) {
                TextColored(ImVec4(1.0f, 0.55f, 0.45f, 1.0f), "items\\DLTB_ItemRuntime.dll is missing next to the trainer.");
                TextDisabled("Build it with item_bridge\\Build-DltbItemRuntime.ps1.");
            }
            else if (ItemBridge::view && !ItemBridge::Connected()) {
                TextDisabled("Runtime loaded, waiting for it to respond...");
            }
            else {
                TextWrapped("Loads the Item Extravaganza runtime into the game (once per game session). "
                            "Its own window stays hidden; everything is controlled from this tab.");
                TextDisabled("Back up your save. Single-player only. Changes may autosave.");
                TextDisabled("Don't also run the original ItemExtravaganza.exe in the same session.");
                Spacing();
                if (Button(ICON_FA_BOLT " Load Item Extravaganza", ImVec2(GetWindowWidth(), 28)) && !ItemBridge::injecting) {
                    ItemBridge::injecting = true;
                    if (ItemBridge::Inject(GameMemory::hProcess))
                        TopNotificationSystem::Push("Item Extravaganza", "Runtime loaded into the game", ImVec4(0.30f, 0.85f, 0.45f, 1.0f), 3.0f, ICON_FA_PALLET);
                    else
                        TopNotificationSystem::Push("Item Extravaganza", ItemBridge::injectError.c_str(), ImVec4(1.0f, 0.4f, 0.35f, 1.0f), 4.0f, ICON_FA_EXCLAMATION_TRIANGLE);
                    ItemBridge::injecting = false;
                }
                Checkbox("Load automatically when the game starts", &MenuVars::itemsAutoLoad);
                if (!ItemBridge::injectError.empty()) TextColored(ImVec4(1.0f, 0.55f, 0.45f, 1.0f), "Last error: %s", ItemBridge::injectError.c_str());
            }
        } gui.end_group_box();
        return false;
    }

    // ---- каталог -----------------------------------------------------------
    struct CatalogUi {
        char search[192] = "";
        int category = 0, rarity = 0, viewMode = 0, rank = 0;
        std::vector<std::string> categories{ "All categories" }, rarities{ "All rarities" };
        std::vector<int> visible;
        bool dirty = true;
        std::string selectedId;
        char qty[16] = "1";
        int lastVisibleCatalogVersion = -2;
    };
    static CatalogUi cat;

    static void RebuildCatalogChoices() {
        cat.categories.assign(1, "All categories");
        cat.rarities.assign(1, "All rarities");
        for (auto& it : ItemBridge::catalog) {
            bool f = false;
            for (auto& c : cat.categories) if (!_stricmp(c.c_str(), it.category)) { f = true; break; }
            if (!f) cat.categories.push_back(it.category);
            f = false;
            for (auto& r : cat.rarities) if (!_stricmp(r.c_str(), it.rarity)) { f = true; break; }
            if (!f) cat.rarities.push_back(it.rarity);
        }
        if (cat.category >= (int)cat.categories.size()) cat.category = 0;
        if (cat.rarity >= (int)cat.rarities.size()) cat.rarity = 0;
    }

    static void RefilterCatalog() {
        cat.visible.clear();
        const char* c = cat.category > 0 ? cat.categories[cat.category].c_str() : nullptr;
        const char* r = cat.rarity > 0 ? cat.rarities[cat.rarity].c_str() : nullptr;
        for (int i = 0; i < (int)ItemBridge::catalog.size(); ++i) {
            const DltbCatalogItem& it = ItemBridge::catalog[i];
            if (c && _stricmp(it.category, c)) continue;
            if (r && _stricmp(it.rarity, r)) continue;
            if (cat.rank > 0 && it.weaponLevel != (uint32_t)cat.rank) continue;
            if (cat.viewMode == 1 && !it.favorite) continue;
            if (cat.viewMode == 2 && !it.recentOrder) continue;
            if (cat.search[0] && !ContainsI(it.name, cat.search) && !ContainsI(it.id, cat.search) && !ContainsI(it.category, cat.search) &&
                !ContainsI(it.rarity, cat.search) && !ContainsI(it.itemType, cat.search) && !ContainsI(it.source, cat.search)) continue;
            cat.visible.push_back(i);
        }
        cat.dirty = false;
    }

    static const DltbCatalogItem* FindCatalog(const std::string& id) {
        for (auto& it : ItemBridge::catalog) if (!_stricmp(it.id, id.c_str())) return &it;
        return nullptr;
    }

    // Как в оригинале: без выбора можно ввести точный внутренний ID в поиск
    static bool ExactIdFromSearch(std::string& out) {
        std::string s = cat.search;
        while (!s.empty() && (s.back() == ' ')) s.pop_back();
        while (!s.empty() && s.front() == ' ') s.erase(s.begin());
        if (s.empty() || s.find_first_of(" \t\"'") != std::string::npos) return false;
        out = s;
        return true;
    }

    static void ComboStrings(const char* label, int* value, const std::vector<std::string>& items) {
        std::vector<const char*> ptrs;
        for (auto& s : items) ptrs.push_back(s.c_str());
        if (Combo(label, value, ptrs.data(), (int)ptrs.size())) cat.dirty = true;
    }

    static void SendItemAction(int action, const std::string& id) {
        const DltbCatalogItem* it = FindCatalog(id);
        int q = atoi(cat.qty);
        DltbCommand cmd = {};
        cmd.type = DLTB_CMD_ITEM_ACTION;
        cmd.action = action;
        cmd.quantity = q;
        strncpy_s(cmd.itemId, id.c_str(), _TRUNCATE);
        if (it) { strncpy_s(cmd.itemName, it->name, _TRUNCATE); strncpy_s(cmd.category, it->category, _TRUNCATE); }
        if (!ItemBridge::Send(cmd)) TopNotificationSystem::Push("Item Extravaganza", "Busy — wait for the previous action", ImVec4(1.0f, 0.7f, 0.3f, 1.0f), 2.5f, ICON_FA_EXCLAMATION_TRIANGLE);
    }

    static void DrawCatalog() {
        if (ItemBridge::catalogChanged || cat.lastVisibleCatalogVersion != (int)ItemBridge::catalogVersion) {
            RebuildCatalogChoices();
            cat.dirty = true;
            cat.lastVisibleCatalogVersion = (int)ItemBridge::catalogVersion;
        }
        if (cat.dirty) RefilterCatalog();
        const float halfW = GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2;

        gui.group_box(ICON_FA_SEARCH " Item Catalog", ImVec2(halfW, 440)); {
            PushItemWidth(GetWindowWidth());
            if (InputTextWithHint("##iesearch", "Search name, ID, category...", cat.search, sizeof(cat.search))) cat.dirty = true;
            PopItemWidth();
            ComboStrings("Category", &cat.category, cat.categories);
            ComboStrings("Rarity", &cat.rarity, cat.rarities);
            static const char* viewNames[] = { "All Items", "Favorites", "Recent" };
            if (Combo("View", &cat.viewMode, viewNames, 3)) cat.dirty = true;
            static const char* rankNames[] = { "Any Rank", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15" };
            if (Combo("Weapon Rank", &cat.rank, rankNames, 16)) cat.dirty = true;
            TextDisabled("%d of %d items", (int)cat.visible.size(), (int)ItemBridge::catalog.size());
            BeginChild("##ielist", ImVec2(GetWindowWidth(), 0), false);
            ImGuiListClipper clip;
            clip.Begin((int)cat.visible.size(), 24.0f + GetStyle().ItemSpacing.y);
            while (clip.Step()) {
                for (int row = clip.DisplayStart; row < clip.DisplayEnd; ++row) {
                    const DltbCatalogItem& it = ItemBridge::catalog[cat.visible[row]];
                    char id[32]; snprintf(id, sizeof(id), "##ie%d", cat.visible[row]);
                    char badge[24] = "";
                    if (it.weaponLevel) snprintf(badge, sizeof(badge), "R%u", it.weaponLevel);
                    if (it.favorite) strncat_s(badge, " " ICON_FA_STAR, _TRUNCATE);
                    if (Row(id, cat.selectedId == it.id, it.name, RarityColor(it.rarity), badge, Accent()))
                        cat.selectedId = it.id;
                }
            }
            EndChild();
        } gui.end_group_box();
        SameLine();
        gui.group_box(ICON_FA_PALLET " Selected Item", ImVec2(halfW, 440)); {
            const DltbCatalogItem* it = cat.selectedId.empty() ? nullptr : FindCatalog(cat.selectedId);
            std::string useId;
            if (it) {
                useId = it->id;
                PushStyleColor(ImGuiCol_Text, RarityColor(it->rarity));
                TextWrapped("%s", it->name);
                PopStyleColor();
                TextDisabled("%s  |  %s", it->category, it->rarity);
                if (it->weaponLevel) { SameLine(); TextColored(Accent(), "R%u", it->weaponLevel); }
                PushTextWrapPos(GetWindowWidth() - 4.0f);
                TextDisabled("ID: %s", it->id);
                if (it->itemType[0]) TextDisabled("Type: %s", it->itemType);
                if (it->canDrop) TextDisabled("Can drop: %s", it->canDrop == 1 ? "yes" : "no");
                PopTextWrapPos();
            }
            else if (ExactIdFromSearch(useId)) {
                TextWrapped("No item selected.");
                TextDisabled("Using the exact internal ID from Search:");
                TextColored(Accent(), "%s", useId.c_str());
            }
            else TextDisabled("Select an item, or type one exact internal ID into Search.");
            Separator();
            if (InputText("Quantity", cat.qty, sizeof(cat.qty), ImGuiInputTextFlags_CharsDecimal)) {}
            {
                const char* quick[] = { "1", "10", "50", "999" };
                const float bw = (GetWindowWidth() - 3 * GetStyle().ItemSpacing.x) / 4;
                for (int i = 0; i < 4; ++i) { if (i) SameLine(); if (Button(quick[i], ImVec2(bw, 22))) snprintf(cat.qty, sizeof(cat.qty), "%s", quick[i]); }
            }
            const long long q = _atoi64(cat.qty);
            const bool qtyOk = q >= 1 && q <= 0x7FFFFFFFll;
            const bool can = !useId.empty() && qtyOk && ItemBridge::flags.ready && !ItemBridge::flags.actionPending;
            if (!qtyOk) TextColored(ImVec4(1.0f, 0.55f, 0.45f, 1.0f), "Quantity must be 1 .. 2,147,483,647");
            BeginDisabled(!can);
            if (Button(ICON_FA_PLUS " Give to Inventory", ImVec2(GetWindowWidth(), 26))) SendItemAction(1, useId);
            if (Button(ICON_FA_SAVE " Give to Stash", ImVec2(GetWindowWidth() / 2 - 4, 24))) SendItemAction(2, useId);
            SameLine();
            if (Button(ICON_FA_TIMES " Remove", ImVec2(GetWindowWidth() / 2 - 4, 24))) SendItemAction(3, useId);
            EndDisabled();
            TextDisabled("Stash: an accepted request alone doesn't prove delivery.");
            BeginDisabled(!it);
            if (Button(it && it->favorite ? ICON_FA_STAR " Unfavorite" : ICON_FA_STAR " Favorite", ImVec2(GetWindowWidth() / 2 - 4, 22)) && it) {
                DltbCommand cmd = {}; cmd.type = DLTB_CMD_TOGGLE_FAVORITE; strncpy_s(cmd.itemId, it->id, _TRUNCATE); ItemBridge::Send(cmd);
            }
            EndDisabled();
            SameLine();
            BeginDisabled(useId.empty());
            if (Button(ICON_FA_CODE " Copy ID", ImVec2(GetWindowWidth() / 2 - 4, 22))) SetClipboardText(useId.c_str());
            EndDisabled();
            Separator();
            if (!ItemBridge::flags.ready)
                TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "%s", ItemBridge::flags.moduleLoaded ? "Native calls locked: game build not verified." : "Waiting for the game module...");
            StatusLine();
        } gui.end_group_box();
    }

}
