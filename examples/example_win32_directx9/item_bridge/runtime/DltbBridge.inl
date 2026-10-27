// ============================================================================
//  DltbBridge.inl — мост между рантаймом Item Extravaganza и меню DLTB.RIP.
//  Включается в ItemExtravaganzaRuntime.cpp только при IE_DLTB_BRIDGE.
//
//  Окно рантайма создаётся как обычно, но никогда не показывается (горячая
//  клавиша рантайма не запускается — Insert принадлежит меню трейнера).
//  Команды трейнера выполняются в потоке этого окна теми же путями, что и
//  кнопки: те же проверки, тот же запрос в игровой поток, те же обработчики
//  результата. Подтверждения (очистка аффиксов, значения > 1000) спрашивает
//  меню трейнера и передаёт флагом.
// ============================================================================
#include "../DltbItemBridge.h"

static HANDLE g_bridge_mapping;
static DltbBridgeShared* g_bridge;
static CRITICAL_SECTION g_bridge_text_lock;
static bool g_bridge_text_lock_ready;
static wchar_t g_bridge_status[320] = L"Starting Item Extravaganza runtime...";
static volatile LONG g_bridge_catalog_dirty = 1;
static volatile LONG g_bridge_owned_dirty = 1;
static volatile LONG g_bridge_cmd_inflight;
static volatile LONG g_bridge_attached;   // подкласс окна установлен — команды можно отправлять
static DltbCommand g_bridge_cmd;
static long g_bridge_cmd_seq;
static const UINT WM_DLTB_BRIDGE_CMD = WM_APP + 0x3D1;

// Вызывается из set_status(): статус и строка журнала уходят в меню трейнера.
static void bridge_capture_status(const wchar_t* value) {
    if (!g_bridge_text_lock_ready) return;
    const wchar_t* text = value ? value : L"Ready";
    EnterCriticalSection(&g_bridge_text_lock);
    wcsncpy_s(g_bridge_status, _countof(g_bridge_status), text, _TRUNCATE);
    if (g_bridge) {
        SYSTEMTIME now = {};
        GetLocalTime(&now);
        const long index = g_bridge->logCount % DLTB_LOG_LINES;
        _snwprintf_s(g_bridge->log[index], _countof(g_bridge->log[index]), _TRUNCATE,
                     L"[%02u:%02u:%02u] %s", now.wHour, now.wMinute, now.wSecond, text);
        InterlockedIncrement(&g_bridge->logCount);
    }
    LeaveCriticalSection(&g_bridge_text_lock);
}

static void bridge_copy_owned(DltbBridgeShared* b) {
    EnterCriticalSection(&g_data_lock);
    const DWORD count = g_owned_count < DLTB_MAX_OWNED ? g_owned_count : DLTB_MAX_OWNED;
    for (DWORD i = 0; i < count; ++i) {
        const OwnedWeaponView& src = g_owned_weapons[i];
        DltbOwnedItem& dst = b->owned[i];
        ZeroMemory(&dst, sizeof(dst));
        dst.instance = (uint64_t)(uintptr_t)src.instance;
        dst.isGear = src.is_gear ? 1 : 0;
        dst.wornSlot = src.worn_slot;
        strcpy_s(dst.id, src.id);
        strcpy_s(dst.name, src.name);
        strcpy_s(dst.category, src.category);
        strcpy_s(dst.rarity, src.rarity);
        dst.weaponLevel = src.weapon_level;
        dst.sameIdOrdinal = src.same_id_ordinal;
        dst.affixCount = src.affix_count < DLTB_MAX_AFFIXES ? src.affix_count : DLTB_MAX_AFFIXES;
        for (unsigned j = 0; j < dst.affixCount; ++j) {
            const OwnedAffixView& a = src.affixes[j];
            strcpy_s(dst.affixes[j].id, a.id);
            dst.affixes[j].raw = a.raw;
            dst.affixes[j].factor = a.factor;
            dst.affixes[j].statChange = a.stat_change ? 1 : 0;
            format_owned_affix_value(owned_affix_metadata(a.id), a.raw * a.factor,
                                     dst.affixes[j].shown, _countof(dst.affixes[j].shown));
        }
    }
    b->ownedCount = count;
    LeaveCriticalSection(&g_data_lock);
}

static void bridge_copy_catalog(DltbBridgeShared* b) {
    EnterCriticalSection(&g_data_lock);
    const DWORD count = g_item_count < DLTB_MAX_CATALOG ? g_item_count : DLTB_MAX_CATALOG;
    for (DWORD i = 0; i < count; ++i) {
        const ItemDefinition& src = g_items[i];
        DltbCatalogItem& dst = b->catalog[i];
        strcpy_s(dst.name, src.name);
        strcpy_s(dst.id, src.id);
        strcpy_s(dst.category, src.category);
        strcpy_s(dst.rarity, src.rarity);
        strncpy_s(dst.itemType, src.item_type, _TRUNCATE);
        strncpy_s(dst.source, src.source, _TRUNCATE);
        dst.canDrop = src.can_drop;
        dst.weaponLevel = src.weapon_level;
        dst.favorite = src.favorite;
        dst.recentOrder = src.recent_order;
    }
    b->catalogCount = count;
    LeaveCriticalSection(&g_data_lock);

    const unsigned metaCount = g_owned_affix_metadata_count < DLTB_MAX_AFFIX_META ? g_owned_affix_metadata_count : DLTB_MAX_AFFIX_META;
    for (unsigned i = 0; i < metaCount; ++i) {
        const OwnedAffixMetadata& src = g_owned_affix_metadata[i];
        DltbAffixMeta& dst = b->affixMeta[i];
        strcpy_s(dst.id, src.id);
        strcpy_s(dst.name, src.name);
        strcpy_s(dst.valueFormat, src.value_format);
        dst.gameLocalized = src.game_localized ? 1 : 0;
    }
    b->affixMetaCount = metaCount;

    const unsigned builtCount = g_owned_builtin_count < DLTB_MAX_BUILTIN ? g_owned_builtin_count : DLTB_MAX_BUILTIN;
    for (unsigned i = 0; i < builtCount; ++i) {
        const OwnedBuiltInStat& src = g_owned_builtin_stats[i];
        DltbBuiltInStat& dst = b->builtIn[i];
        strcpy_s(dst.itemId, src.item_id);
        strcpy_s(dst.name, src.name);
        strcpy_s(dst.authoredValue, src.authored_value);
        dst.scalable = src.scalable ? 1 : 0;
    }
    b->builtInCount = builtCount;
}

static void bridge_publish() {
    DltbBridgeShared* b = g_bridge;
    if (!b) return;
    InterlockedIncrement(&b->writeSeq);   // нечётный: идёт запись
    b->ready = InterlockedCompareExchange(&g_ready, 1, 1) ? 1 : 0;
    b->gameModuleLoaded = GetModuleHandleW(L"gamedll_ph_x64_rwdi.dll") ? 1 : 0;
    b->actionPending = InterlockedCompareExchange(&g_action_pending, 0, 0) ? 1 : 0;
    b->editPending = g_owned_edit_pending ? 1 : 0;
    b->transaction = g_owned_transaction ? 1 : 0;
    b->editFaulted = g_owned_edit_faulted ? 1 : 0;
    b->duplicateFaulted = g_owned_duplicate_faulted ? 1 : 0;
    b->lookupProbePassed = g_owned_lookup_probe_passed ? 1 : 0;
    b->extremesConfirmed = g_owned_extremes_confirmed ? 1 : 0;
    b->transactionItem = (uint64_t)(uintptr_t)g_owned_saved_item;
    strcpy_s(b->transactionItemId, g_owned_saved_id);
    EnterCriticalSection(&g_bridge_text_lock);
    wcscpy_s(b->status, g_bridge_status);
    LeaveCriticalSection(&g_bridge_text_lock);
    if (InterlockedExchange(&g_bridge_owned_dirty, 0)) {
        bridge_copy_owned(b);
        InterlockedIncrement(&b->ownedVersion);
    }
    if (InterlockedExchange(&g_bridge_catalog_dirty, 0)) {
        bridge_copy_catalog(b);
        InterlockedIncrement(&b->catalogVersion);
    }
    InterlockedIncrement(&b->writeSeq);   // чётный: данные согласованы
}

// --- команды (поток окна рантайма) -------------------------------------------

static bool bridge_find_owned(uint64_t instance, OwnedWeaponView* out) {
    bool found = false;
    EnterCriticalSection(&g_data_lock);
    for (DWORD i = 0; i < g_owned_count; ++i)
        if ((uint64_t)(uintptr_t)g_owned_weapons[i].instance == instance) { *out = g_owned_weapons[i]; found = true; break; }
    LeaveCriticalSection(&g_data_lock);
    return found;
}

// Копия queue_item_action(), но предмет приходит из меню, а не из списка окна.
static void bridge_queue_item_action(const DltbCommand& cmd) {
    if (!InterlockedCompareExchange(&g_ready, 1, 1)) {
        set_status(GetModuleHandleW(L"gamedll_ph_x64_rwdi.dll")
            ? L"Native item calls are locked: this game build has not passed validation."
            : L"Waiting for the game module to load.");
        return;
    }
    if (cmd.action < ITEM_ACTION_GIVE_INVENTORY || cmd.action > ITEM_ACTION_REMOVE_INVENTORY) return;
    const ItemAction action = (ItemAction)cmd.action;
    ItemDefinition item = {}; int catalog_index = -1;
    char id[160] = {};
    strncpy_s(id, cmd.itemId, _TRUNCATE);
    char* trimmed = trim_ascii(id);
    if (!trimmed[0] || strpbrk(trimmed, " \t\r\n\"'")) {
        set_status(L"Select an item, or enter one exact internal ID in Search.");
        return;
    }
    EnterCriticalSection(&g_data_lock);
    for (DWORD i = 0; i < g_item_count; ++i)
        if (!_stricmp(g_items[i].id, trimmed)) { item = g_items[i]; catalog_index = (int)i; break; }
    LeaveCriticalSection(&g_data_lock);
    if (catalog_index < 0) {   // точный внутренний ID, которого нет в каталоге
        strncpy_s(item.name, _countof(item.name), trimmed, _TRUNCATE);
        strncpy_s(item.id, _countof(item.id), trimmed, _TRUNCATE);
        classify_item(item.name, item.id, item.category, _countof(item.category));
        item.weapon_level = parse_weapon_level(item.id, item.category);
    }
    const int quantity = cmd.quantity;
    if (quantity < 1) {
        set_status(L"Quantity must be between 1 and 2,147,483,647 (signed 4-byte maximum).");
        return;
    }
    const ULONGLONG now = GetTickCount64();
    if (g_last_action_dispatch_ms && now - g_last_action_dispatch_ms < (ULONGLONG)kUiActionCooldownMs) {
        set_status(L"Duplicate click ignored; the previous item action already ran.");
        return;
    }
    if (InterlockedCompareExchange(&g_action_pending, 1, 0) != 0) {
        set_status(L"An item action is already running.");
        return;
    }
    g_last_action_dispatch_ms = now;
    set_item_actions_enabled(false);
    EnterCriticalSection(&g_data_lock);
    g_action_request.action = action;
    strncpy_s(g_action_request.id, _countof(g_action_request.id), item.id, _TRUNCATE);
    strncpy_s(g_action_request.name, _countof(g_action_request.name), item.name, _TRUNCATE);
    strncpy_s(g_action_request.category, _countof(g_action_request.category), item.category, _TRUNCATE);
    g_action_request.quantity = quantity;
    g_action_request.dispatch_sequence = (DWORD)InterlockedIncrement(&g_action_dispatch_sequence);
    if (catalog_index >= 0) g_items[catalog_index].recent_order = ++g_recent_counter;
    LeaveCriticalSection(&g_data_lock);
    InterlockedExchange(&g_bridge_catalog_dirty, 1);   // обновился "Recent"
    log_line(L"[ITEM] queued sequence=%lu route=%s item=%S requested=%d dispatch_count=1 source=DLTB",
             g_action_request.dispatch_sequence, action_label(action), item.id, quantity);
    HWND game_window = find_game_window();
    if (!install_game_thread_dispatch(game_window) ||
        !PostMessageW(game_window, WM_BEAST_GIVE_TEST, 0, 0)) {
        InterlockedExchange(&g_action_pending, 0);
        set_item_actions_enabled(true);
        log_line(L"[ITEM] %s dispatch failed hwnd=%p error=%lu", action_label(action), game_window, GetLastError());
        set_status(L"Could not dispatch the item action on the game thread; see the log.");
        return;
    }
    wchar_t status[300] = {}, name[180] = {};
    ascii_to_wide(item.name, name, _countof(name));
    _snwprintf_s(status, _countof(status), _TRUNCATE, L"Running %s: %s x%d…", action_label(action), name, quantity);
    set_status(status);
}

static bool bridge_has_affix(const OwnedWeaponView& weapon, const char* id) {
    for (unsigned i = 0; i < weapon.affix_count; ++i)
        if (!_stricmp(weapon.affixes[i].id, id)) return true;
    return false;
}

// Копия request_owned_edit(): цель, аффикс и значение приходят из меню.
static void bridge_request_owned_edit(const DltbCommand& cmd) {
    const OwnedEditKind kind = (OwnedEditKind)cmd.editKind;
    if (kind < OWNED_EDIT_VALUE || kind > OWNED_EDIT_REPAIR) return;
    if (g_owned_edit_pending) { set_status(L"Affix action is already in progress."); return; }
    if (g_owned_edit_faulted) { set_status(L"Restart game and tool before another native action."); return; }
    if (kind == OWNED_EDIT_DUPLICATE && g_owned_duplicate_faulted) {
        set_status(L"Further copies are blocked after verification failed. Restart game/tool before duplicating again."); return;
    }
    OwnedEditRequest request = {};
    request.kind = kind;
    if (kind != OWNED_EDIT_KEEP && kind != OWNED_EDIT_RESTORE) {
        OwnedWeaponView weapon = {};
        if (!bridge_find_owned(cmd.target, &weapon)) {
            set_status(L"Select an owned item instance first (scan again if it changed)."); return;
        }
        if (g_owned_transaction && g_owned_saved_item != weapon.instance) {
            set_status(L"Finish the previous item's edit first: Keep Changes or Restore Original."); return;
        }
        request.target = weapon.instance;
        request.is_gear = weapon.is_gear;
        strcpy_s(request.item_id, weapon.id);
        request.expected_affix_count = weapon.affix_count;
        memcpy(request.expected_affixes, weapon.affixes, sizeof(request.expected_affixes));
        if (kind == OWNED_EDIT_ADD_CATALOG_TEST) {
            if (!g_owned_lookup_probe_passed) { set_status(L"Catalogue add requires a successful game-definition lookup."); return; }
            if (!owned_affix_metadata(cmd.affixId)) { set_status(L"Select a game-sourced affix from the catalogue first."); return; }
            if (weapon.affix_count >= kOwnedAffixTestLimit) { set_status(L"This item already has the 32-affix safety maximum."); return; }
            if (bridge_has_affix(weapon, cmd.affixId)) { set_status(L"That affix is already on this item; edit its value instead."); return; }
            if (!_finite(cmd.value)) { set_status(L"Enter a finite float32 magnitude; scientific notation is supported."); return; }
            strcpy_s(request.affix_id, cmd.affixId);
            request.new_effective = cmd.value;
        } else if (kind == OWNED_EDIT_ADD) {
            OwnedWeaponView donor = {};
            if (!bridge_find_owned(cmd.donorInstance, &donor) || cmd.donorAffixIndex >= donor.affix_count ||
                _stricmp(donor.affixes[cmd.donorAffixIndex].id, cmd.affixId) || !donor.affixes[cmd.donorAffixIndex].stat_change) {
                set_status(L"Select a real affix donor first."); return;
            }
            if (weapon.affix_count >= kOwnedAffixTestLimit) { set_status(L"This item already has the 32-affix safety maximum."); return; }
            if (bridge_has_affix(weapon, cmd.affixId)) { set_status(L"That affix is already on this item; edit its value instead."); return; }
            request.donor.instance = donor.instance;
            strcpy_s(request.donor.item_id, donor.id);
            strcpy_s(request.donor.affix_id, donor.affixes[cmd.donorAffixIndex].id);
            request.donor.affix_index = cmd.donorAffixIndex;
            request.donor.raw = donor.affixes[cmd.donorAffixIndex].raw;
        } else if (kind == OWNED_EDIT_CLEAR) {
            if (!weapon.affix_count) { set_status(L"This item already has no native affixes."); return; }
        } else if (kind == OWNED_EDIT_DUPLICATE || kind == OWNED_EDIT_REPAIR) {
            if (weapon.is_gear) { set_status(L"Duplicate and Repair are verified only for weapons."); return; }
        } else {
            const unsigned affix = cmd.affixIndex;
            if (affix >= weapon.affix_count || _stricmp(weapon.affixes[affix].id, cmd.affixId) ||
                (kind == OWNED_EDIT_VALUE && !weapon.affixes[affix].stat_change)) {
                set_status(L"Select a native affix line first."); return;
            }
            request.affix_index = affix;
            strcpy_s(request.affix_id, weapon.affixes[affix].id);
            request.expected_raw = weapon.affixes[affix].raw;
            request.expected_effect_vtable = weapon.affixes[affix].effect_vtable;
            if (kind == OWNED_EDIT_VALUE) {
                float encoded = 0.0f;
                if (!_finite(cmd.value) || !owned_encode_magnitude(cmd.value, weapon.affixes[affix].factor, &encoded)) {
                    set_status(L"Value exceeds float32 storage after this affix's factor conversion."); return;
                }
                request.new_effective = cmd.value;
            }
        }
        g_owned_panel_instance = weapon.instance;
    }
    if ((kind == OWNED_EDIT_VALUE || kind == OWNED_EDIT_ADD_CATALOG_TEST) &&
        (request.new_effective > 1000.0f || request.new_effective < -1000.0f) && !g_owned_extremes_confirmed) {
        if (!cmd.confirmExtremes) { set_status(L"Values beyond 1000 need the extreme-value confirmation."); return; }
        g_owned_extremes_confirmed = true;
    }
    HWND game_window = find_game_window();
    if (!game_window || !install_game_thread_dispatch(game_window)) {
        set_status(L"Affix action could not reach the game window."); return;
    }
    EnterCriticalSection(&g_data_lock);
    g_owned_edit_request = request;
    g_owned_edit_pending = true;
    g_owned_edit_dispatched = false;
    if (kind == OWNED_EDIT_DUPLICATE) g_owned_duplicate_result = NULL;
    LeaveCriticalSection(&g_data_lock);
    if (!PostMessageW(game_window, WM_BEAST_OWNED_EDIT, 0, 0)) {
        g_owned_edit_pending = false;
        set_status(L"Affix action could not be queued.");
        return;
    }
    set_status(kind == OWNED_EDIT_DUPLICATE ? L"Duplicating one selected weapon with its native instance state."
        : kind == OWNED_EDIT_REPAIR ? L"Repairing the selected weapon using one native repair slot."
        : kind == OWNED_EDIT_KEEP ? L"Keeping the current affixes and ending the edit session."
        : kind == OWNED_EDIT_RESTORE ? L"Restoring the edited item's original affixes."
        : L"Applying one native affix action to the selected instance.");
}

static void bridge_toggle_favorite(const DltbCommand& cmd) {
    int index = -1; bool favorite = false;
    EnterCriticalSection(&g_data_lock);
    for (DWORD i = 0; i < g_item_count; ++i)
        if (!_stricmp(g_items[i].id, cmd.itemId)) { index = (int)i; break; }
    if (index >= 0) { g_items[index].favorite = !g_items[index].favorite; favorite = g_items[index].favorite != 0; }
    LeaveCriticalSection(&g_data_lock);
    if (index < 0) { set_status(L"Select a catalog item before changing favorites."); return; }
    save_favorites();
    refresh_item_view();
    InterlockedExchange(&g_bridge_catalog_dirty, 1);
    set_status(favorite ? L"Added to favorites." : L"Removed from favorites.");
}

static void bridge_execute(const DltbCommand& cmd) {
    switch (cmd.type) {
    case DLTB_CMD_ITEM_ACTION: bridge_queue_item_action(cmd); break;
    case DLTB_CMD_OWNED_SCAN: request_owned_scan(); break;
    case DLTB_CMD_OWNED_EDIT: bridge_request_owned_edit(cmd); break;
    case DLTB_CMD_TOGGLE_FAVORITE: bridge_toggle_favorite(cmd); break;
    case DLTB_CMD_RELOAD_CATALOG:
        ListView_SetItemState(g_item_list, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
        g_visible_count = 0;
        load_item_catalog();
        refresh_item_view();
        InterlockedExchange(&g_bridge_catalog_dirty, 1);
        set_status(L"Catalog refreshed from installed game definitions.");
        break;
    default: break;
    }
}

// Подкласс скрытого окна рантайма: команды моста + отметка, что снимок предметов обновился.
static LRESULT CALLBACK bridge_window_subclass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR, DWORD_PTR) {
    if (msg == WM_DLTB_BRIDGE_CMD) {
        DltbCommand cmd = g_bridge_cmd;
        const long seq = g_bridge_cmd_seq;
        __try { bridge_execute(cmd); }
        __except (EXCEPTION_EXECUTE_HANDLER) { set_status(L"Bridge command stopped by crash protection; see the log."); }
        if (g_bridge) InterlockedExchange(&g_bridge->ackSeq, seq);
        InterlockedExchange(&g_bridge_cmd_inflight, 0);
        return 0;
    }
    const LRESULT result = DefSubclassProc(hwnd, msg, wparam, lparam);
    if (msg == WM_BEAST_OWNED_SCAN_RESULT || msg == WM_BEAST_OWNED_EDIT_RESULT || msg == WM_BEAST_GIVE_RESULT)
        InterlockedExchange(&g_bridge_owned_dirty, 1);
    return result;
}

// Вызывается из runtime_thread() в потоке окна, перед циклом сообщений.
static void bridge_attach_window() {
    if (g_window && SetWindowSubclass(g_window, bridge_window_subclass, 0xD17B, 0))
        InterlockedExchange(&g_bridge_attached, 1);
    else log_line(L"[DLTB] bridge could not subclass the runtime window");
    InterlockedExchange(&g_bridge_catalog_dirty, 1);
    InterlockedExchange(&g_bridge_owned_dirty, 1);
}

static DWORD WINAPI bridge_thread(void*) {
    wchar_t name[64] = {};
    _snwprintf_s(name, _countof(name), _TRUNCATE, DLTB_BRIDGE_NAME_FMT, GetCurrentProcessId());
    g_bridge_mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)sizeof(DltbBridgeShared), name);
    if (!g_bridge_mapping) { log_line(L"[DLTB] bridge mapping failed error=%lu", GetLastError()); return 0; }
    g_bridge = (DltbBridgeShared*)MapViewOfFile(g_bridge_mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(DltbBridgeShared));
    if (!g_bridge) { log_line(L"[DLTB] bridge view failed error=%lu", GetLastError()); return 0; }
    g_bridge->version = DLTB_BRIDGE_VERSION;
    g_bridge->runtimePid = GetCurrentProcessId();
    g_bridge->ackSeq = g_bridge->cmdSeq;
    g_bridge->magic = DLTB_BRIDGE_MAGIC;
    log_line(L"[DLTB] bridge ready mapping=%s bytes=%llu", name, (unsigned long long)sizeof(DltbBridgeShared));
    while (WaitForSingleObject(g_stop, 50) == WAIT_TIMEOUT) {
        InterlockedIncrement(&g_bridge->heartbeat);
        const long seq = g_bridge->cmdSeq;
        if (seq != g_bridge->ackSeq && g_window && g_bridge_attached && !InterlockedCompareExchange(&g_bridge_cmd_inflight, 1, 0)) {
            MemoryBarrier();
            g_bridge_cmd = g_bridge->cmd;
            g_bridge_cmd_seq = seq;
            if (!PostMessageW(g_window, WM_DLTB_BRIDGE_CMD, 0, 0)) InterlockedExchange(&g_bridge_cmd_inflight, 0);
        }
        if (g_window && g_bridge_attached) {
            if (IsWindowVisible(g_window)) ShowWindowAsync(g_window, SW_HIDE);   // окно рантайма всегда скрыто — UI в меню DLTB
            bridge_publish();
        }
    }
    return 0;
}

static void bridge_start() {
    InitializeCriticalSection(&g_bridge_text_lock);
    g_bridge_text_lock_ready = true;
    HANDLE thread = CreateThread(NULL, 0, bridge_thread, NULL, 0, NULL);
    if (thread) CloseHandle(thread);
}
