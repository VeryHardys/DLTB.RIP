// Item Extravaganza -- independent runtime probe and control surface.
// [DLTB.RIP] Local copy: with IE_DLTB_BRIDGE the runtime window stays hidden and
// the DLTB.RIP menu drives it through DltbBridge.inl (see item_bridge/DltbItemBridge.h).
//
// This module is loaded only by Item Extravaganza into the verified retail
// Beast process.  It deliberately has no dependency on DLTBRuntimeBridge,
// ASI loaders, or an app-local proxy DLL.  Native calls remain disabled until
// a build-locked call site is observed and validated.

#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <errno.h>
#include <wctype.h>
#include <initializer_list>
#include "ItemExtravaganzaVersion.h"

#if defined(IE_OWNED_AFFIX_EDITOR) && !defined(IE_AFFIX_REMOVE_POC)
#error The research-only owned affix editor requires the native record POC helpers.
#endif

#ifndef _countof
#define _countof(value) (sizeof(value) / sizeof((value)[0]))
#endif

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

static HMODULE g_module;
static HANDLE g_stop;
static HWND g_window, g_status, g_details;
static HWND g_search, g_search_backdrop, g_category_filter, g_rarity_filter, g_view_filter;
static HWND g_weapon_level_filter, g_item_list;
static HWND g_quantity, g_quantity_backdrop, g_selection, g_give_inventory, g_give_stash;
static HWND g_remove_inventory, g_favorite, g_copy_id, g_quantity_minus, g_quantity_plus;
static HWND g_header, g_subtitle, g_result_count, g_detail_panel, g_brand, g_version;
static HWND g_search_label, g_category_label, g_rarity_label, g_library_label;
static HWND g_weapon_level_label, g_selected_label, g_quantity_label, g_actions_label;
static HWND g_close_button, g_minimize_button, g_maximize_button, g_return_game, g_section_title;
static HWND g_catalog_title, g_catalog_subtitle, g_reset_filters;
static HWND g_tab_all_items, g_tab_favorites;
#ifdef IE_OWNED_AFFIX_EDITOR
static const unsigned kOwnedAffixTestLimit = 32;
static HWND g_tab_owned_weapons, g_tab_owned_gear;
static HWND g_owned_affix_list, g_owned_value_input, g_owned_apply_value;
static HWND g_owned_remove_affix, g_owned_donor_combo, g_owned_add_affix;
static HWND g_owned_clear_affixes, g_owned_duplicate_weapon, g_owned_repair_weapon;
static HWND g_owned_value_backdrop, g_owned_catalog_value_backdrop, g_owned_catalog_search_backdrop;
static HWND g_owned_restore, g_owned_keep, g_owned_value_label, g_owned_donor_label;
static HWND g_owned_catalog_test;
static HWND g_owned_catalog_value_input;
static HWND g_owned_catalog_search;
static HWND g_owned_filter_label, g_owned_format_filter, g_owned_presence_filter;
static bool g_owned_catalog_mode = true;
static RECT g_owned_popup_bounds;
static bool g_owned_popup_prepared;
static unsigned g_owned_catalog_visible[256];
static unsigned g_owned_catalog_visible_count;
static HWND g_owned_affix_detail;
static bool g_owned_mode;
static bool g_owned_gear_mode;
static bool g_owned_equipment_known;
static bool g_owned_extremes_confirmed;
static bool g_owned_panel_rebuilding;
static bool g_owned_view_rebuilding;
static bool g_owned_population_busy;
static HIMAGELIST g_owned_row_images;
#include "OwnedAffixValues.inl"
struct OwnedAffixView {
    char id[128];
    float raw;
    float factor;
    uintptr_t effect_vtable;
    bool stat_change;
};
struct OwnedWeaponView {
    void* instance;
    bool is_gear;
    unsigned worn_slot; // Native outfit slot + 1; zero when not worn/unknown.
    char id[160];
    char name[160];
    char category[64];
    char rarity[32];
    DWORD weapon_level;
    DWORD favorite;
    DWORD recent_order;
    unsigned same_id_ordinal;
    unsigned affix_count;
    OwnedAffixView affixes[32];
};
static OwnedWeaponView g_owned_weapons[512];
static DWORD g_owned_count;
static int g_owned_visible[512];
static DWORD g_owned_visible_count;
struct OwnedDonorChoice {
    void* instance;
    char item_id[160];
    char affix_id[128];
    unsigned affix_index;
    float raw;
};
static OwnedDonorChoice g_owned_donors[16384];
static unsigned g_owned_donor_count;
struct OwnedAffixMetadata {
    char id[128]; char name[160]; char value_format[96]; bool game_localized;
};
static OwnedAffixMetadata g_owned_affix_metadata[256];
static unsigned g_owned_affix_metadata_count;
struct OwnedBuiltInStat {
    char item_id[160];
    char name[160];
    char authored_value[32];
    char unit[32];
    char curve[64];
    bool scalable;
};
static OwnedBuiltInStat g_owned_builtin_stats[512];
static unsigned g_owned_builtin_count;
static void* g_owned_panel_instance;
static char g_owned_selected_affix_id[128];
static int g_owned_selected_builtin = -1;
static void* g_owned_switch_target;
static void* g_owned_switch_offer_target;
enum OwnedEditKind { OWNED_EDIT_VALUE = 1, OWNED_EDIT_REMOVE = 2,
                     OWNED_EDIT_ADD = 3, OWNED_EDIT_RESTORE = 4, OWNED_EDIT_KEEP = 5,
                     OWNED_EDIT_ADD_CATALOG_TEST = 6, OWNED_EDIT_CLEAR = 7,
                     OWNED_EDIT_DUPLICATE = 8, OWNED_EDIT_REPAIR = 9 };
struct OwnedEditRequest {
    OwnedEditKind kind;
    void* target;
    bool is_gear;
    char item_id[160];
    unsigned expected_affix_count;
    OwnedAffixView expected_affixes[32];
    unsigned affix_index;
    char affix_id[128];
    float expected_raw;
    uintptr_t expected_effect_vtable;
    float new_effective;
    OwnedDonorChoice donor;
};
static OwnedEditRequest g_owned_edit_request;
static bool g_owned_edit_pending;
static bool g_owned_edit_dispatched;
static void* g_owned_duplicate_result;
static bool g_owned_edit_faulted;
static bool g_owned_duplicate_faulted;
static bool g_owned_refresh_stale_target;
static bool g_owned_rebound_during_edit;
static void update_owned_affix_panel();
static void owned_affix_selection_changed();
static void populate_owned_donors();
static bool selected_owned_weapon(OwnedWeaponView* out);
static void request_owned_edit(OwnedEditKind kind, bool keep_confirmed = false);
#endif
static HWND g_runtime_log, g_log_filter, g_clear_log, g_console_title;
static HWND g_tooltip;
static HFONT g_font, g_header_font, g_small_font, g_action_font, g_mono_font, g_section_font, g_input_font;
static HFONT g_brand_font, g_nav_font, g_tiny_font;
static HICON g_window_icon_small, g_window_icon_large, g_brand_icon;
static UINT g_ui_dpi = 96;
static HWND g_hot_control;
static BOOL g_first_show = TRUE;
static const UINT WM_IE_MENU_SCOPE = WM_APP + 0x2C6;
static wchar_t g_log_path[MAX_PATH];
static wchar_t g_own_dir[MAX_PATH];
static volatile LONG g_ready;
static HWND g_game_window;
static WNDPROC g_original_game_window_proc;
static volatile LONG g_action_pending;
static volatile LONG g_action_dispatch_sequence;
static ULONGLONG g_last_action_dispatch_ms;
static void* g_cached_sync_action_controller;
static CRITICAL_SECTION g_data_lock;
static HBRUSH g_background_brush, g_panel_brush, g_input_brush, g_status_brush;
static HBRUSH g_sidebar_brush, g_card_brush, g_header_brush, g_console_brush;
static HIMAGELIST g_row_height_images;
static const COLORREF kColorBackground = RGB(8, 10, 13);
static const COLORREF kColorSidebar = RGB(11, 14, 18);
static const COLORREF kColorPanel = RGB(14, 17, 21);
static const COLORREF kColorCard = RGB(16, 20, 24);
static const COLORREF kColorInput = RGB(21, 25, 30);
static const COLORREF kColorStatus = RGB(11, 14, 18);
static const COLORREF kColorText = RGB(238, 242, 245);
static const COLORREF kColorMuted = RGB(145, 155, 166);
static const COLORREF kColorFaint = RGB(88, 98, 108);
static const COLORREF kColorBorder = RGB(36, 42, 49);
static const COLORREF kColorBorderHot = RGB(63, 74, 84);
static const COLORREF kColorAccent = RGB(91, 222, 116);
static const COLORREF kColorAccentDim = RGB(42, 111, 60);
static const COLORREF kColorSelected = RGB(21, 48, 31);
static const COLORREF kColorRemove = RGB(239, 104, 113);
static const COLORREF kColorConsole = RGB(7, 9, 12);
static const COLORREF kColorHeader = RGB(10, 12, 16);

static const int kUiOuterMargin = 16;
static const int kUiPanelGap = 12;
static const int kUiPanelPadding = 16;
static const int kUiPanelRadius = 10;
static const int kUiControlHeight = 36;
static const int kUiCompactButtonHeight = 34;
static const int kUiActionHeight = 40;
static const int kUiTableRowHeight = 34;
static const int kUiHeaderHeight = 72;
static const int kUiStatusHeight = 34;
static const int kUiActionCooldownMs = 650;

#define BSS_MAX_ITEMS 8192
#define BSS_MAX_VISIBLE 8192

struct ItemDefinition {
    char name[160];
    char id[160];
    char category[64];
    char rarity[32];
    char physics[96];
    char localization_key[160];
    char item_type[64];
    char source[240];
    DWORD can_drop; // 0 unspecified, 1 true, 2 false
    DWORD weapon_level; // 0 is not ranked; otherwise R1-R15 from the game ID
    DWORD favorite;
    DWORD recent_order;
};

static ItemDefinition g_items[BSS_MAX_ITEMS];
static DWORD g_item_count;
static int g_visible_items[BSS_MAX_VISIBLE];
static DWORD g_visible_count;
static DWORD g_recent_counter;

static void set_status(const wchar_t* value);
static void rebuild_catalog_filter_choices();

enum {
    IDC_RESCAN = 1001, IDC_COPYLOG = 1002, IDC_TRACE_DROP = 1003,
    IDC_DROP_EQUIPPED = 1004, IDC_GIVE_TEST = 1005,
    IDC_SEARCH = 1010, IDC_CATEGORY = 1011, IDC_VIEW = 1012,
    IDC_ITEM_LIST = 1013, IDC_QUANTITY = 1014,
    IDC_GIVE_INVENTORY = 1015, IDC_GIVE_STASH = 1016,
    IDC_SPAWN_WORLD = 1017, IDC_REMOVE_INVENTORY = 1018,
    IDC_FAVORITE = 1019, IDC_COPY_ID = 1020, IDC_REFRESH_CATALOG = 1021,
    IDC_RARITY = 1022, IDC_WEAPON_LEVEL = 1023,
    IDC_QUANTITY_MINUS = 1024, IDC_QUANTITY_PLUS = 1025,
    IDC_DETAIL_PANEL = 1026, IDC_BRAND = 1027, IDC_CLOSE_BUTTON = 1028,
    IDC_MINIMIZE_BUTTON = 1029, IDC_MAXIMIZE_BUTTON = 1030,
    IDC_RETURN_GAME = 1031, IDC_RESET_FILTERS = 1032,
    IDC_CLEAR_UI_LOG = 1033, IDC_LOG_FILTER = 1034,
    IDC_RESULT_COUNT = 1035, IDC_SELECTED_SUMMARY = 1036,
    IDC_QUANTITY_BACKDROP = 1037,
    IDC_TAB_ALL_ITEMS = 1038, IDC_TAB_FAVORITES = 1039,
    IDC_SEARCH_BACKDROP = 1040,
#ifdef IE_OWNED_AFFIX_EDITOR
    IDC_TAB_OWNED_WEAPONS = 1041,
    IDC_OWNED_AFFIX_LIST = 1042, IDC_OWNED_VALUE_INPUT = 1043,
    IDC_OWNED_APPLY_VALUE = 1044, IDC_OWNED_REMOVE_AFFIX = 1045,
    IDC_OWNED_DONOR_COMBO = 1046, IDC_OWNED_ADD_AFFIX = 1047,
    IDC_OWNED_RESTORE = 1048, IDC_OWNED_KEEP = 1049,
    IDC_OWNED_CATALOG_TEST = 1050, IDC_OWNED_CATALOG_SEARCH = 1051,
    IDC_OWNED_CLEAR_AFFIXES = 1052, IDC_OWNED_DUPLICATE = 1053,
    IDC_OWNED_REPAIR = 1054, IDC_OWNED_VALUE_BACKDROP = 1055,
    IDC_OWNED_CATALOG_VALUE_BACKDROP = 1056, IDC_OWNED_CATALOG_SEARCH_BACKDROP = 1057,
    IDC_TAB_OWNED_GEAR = 1058,
    IDC_OWNED_FORMAT_FILTER = 1059, IDC_OWNED_PRESENCE_FILTER = 1060,
#endif
};

static const UINT WM_BEAST_GIVE_TEST = WM_APP + 0x2B7;
static const UINT WM_BEAST_GIVE_RESULT = WM_APP + 0x2B8;
#ifdef IE_AFFIX_REMOVE_POC
static const UINT WM_BEAST_AFFIX_POC = WM_APP + 0x2B9;
static const UINT WM_BEAST_AFFIX_POC_RESULT = WM_APP + 0x2BA;
#endif
#ifdef IE_OWNED_AFFIX_EDITOR
static const UINT WM_BEAST_OWNED_SCAN = WM_APP + 0x2BB;
static const UINT WM_BEAST_OWNED_SCAN_RESULT = WM_APP + 0x2BC;
static const UINT WM_BEAST_OWNED_EDIT = WM_APP + 0x2BD;
static const UINT WM_BEAST_OWNED_EDIT_RESULT = WM_APP + 0x2BE;
#endif

// Build-locked native item path, reconstructed from the current shipping
// Story::GiveItemLogic/RemoveItemLogic callsites. All values below are RVAs in
// gamedll_ph_x64_rwdi.dll timestamp 0x6AA8CBF7, image size 0x3F0D000.
// The byte checks below reject any later incompatible update before a call.
static const SIZE_T kGetGameDiRva = 0xB347B0;
static const SIZE_T kResolveLocalPlayerRva = 0x15AC280;
static const SIZE_T kGiveInventoryItemRva = 0x116F710;
static const SIZE_T kTtlStringFromViewIatRva = 0x2570948;
static const SIZE_T kTtlStringDestroyIatRva = 0x2570958;
static const SIZE_T kPlayerInventoryOffset = 0x3BE8;
static const SIZE_T kPlayerInventoryAccessRva = 0x1BB059A;
static const SIZE_T kRemoveInventoryItemRva = 0x1BD9770;
static const SIZE_T kForceSendToStashFlagRva = 0x3A6EA6D;
static const SIZE_T kForceSendToStashWriteRva = 0x1BAA09E;
// DropInventoryItem's shipping execute instruction.  Its control flow resolves
// the actor's drop interface through vtable +0x68, then invokes the ID-based
// world-item method at interface vtable +0x48.  We validate both sequences
// before following that exact native route.
static const SIZE_T kDropInventoryInstructionRva = 0x214CC10;
static const SIZE_T kDropInventoryDispatchRva = 0x214CC3F;
// SyncActionController is the concrete implementation used by the shipping
// DropInventoryItem instruction.  Normal gameplay owns a player-bound instance;
// the script-only singleton accessor remains null in a playable save.  The
// inventory helper is a verified secondary base at +0x68; slot +0x48 is the
// ID-based drop routine.
static const SIZE_T kSyncActionControllerVtableRva = 0x29955D8;
static const SIZE_T kSyncActionInventoryVtableRva = 0x2995F20;
static const SIZE_T kSyncActionInventoryOffset = 0x68;
static const SIZE_T kSyncActionDropMethodRva = 0xED8B20;
static const SIZE_T kSyncActionCleanupRva = 0xEC49C0;
static const SIZE_T kSyncActionReadyOffset = 0x218;
static const SIZE_T kSyncActionBusyOffset = 0x228;
static const SIZE_T kSyncActionDropActiveOffset = 0x254;

static const BYTE kGetGameDiBytes[] = {
    0x48, 0x8B, 0x05, 0x39, 0x6D, 0xF9, 0x02, 0xC3
};
static const BYTE kResolveLocalPlayerBytes[] = {
    0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B,
    0x01, 0x48, 0x8B, 0xD9, 0xFF, 0x90, 0x50, 0x08,
    0x00, 0x00
};
static const BYTE kGiveInventoryItemBytes[] = {
    0x48, 0x89, 0x4C, 0x24, 0x08, 0x55, 0x53, 0x56,
    0x57, 0x48, 0x8D, 0xAC, 0x24, 0x88, 0xFE, 0xFF,
    0xFF, 0x48, 0x81, 0xEC, 0x78, 0x02, 0x00, 0x00
};
static const BYTE kRemoveInventoryItemBytes[] = {
    0x48, 0x89, 0x5C, 0x24, 0x18, 0x48, 0x89, 0x74,
    0x24, 0x20, 0x55, 0x41, 0x56, 0x41, 0x57, 0x48,
    0x8D, 0xAC, 0x24, 0x50, 0xFF, 0xFF, 0xFF
};
static const BYTE kForceSendToStashWriteBytes[] = {
    0xC6, 0x05, 0xC8, 0x49, 0xEC, 0x01, 0x01
};
static const BYTE kPlayerInventoryAccessBytes[] = {
    0x48, 0x8B, 0x88, 0xE8, 0x3B, 0x00, 0x00
};
static const BYTE kDropInventoryInstructionBytes[] = {
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x57, 0x48, 0x83,
    0xEC, 0x20, 0x4C, 0x63, 0x41, 0x20, 0x48, 0x8B,
    0xF9, 0x48, 0x8B, 0x42, 0x08
};
static const BYTE kDropInventoryDispatchBytes[] = {
    0x48, 0x8B, 0x03, 0x48, 0x8B, 0xCB, 0xFF, 0x50,
    0x68, 0x48, 0x85, 0xC0, 0x74, 0x17, 0x48, 0x8B,
    0x03, 0x48, 0x8B, 0xCB, 0xFF, 0x50, 0x68, 0x48,
    0x8D, 0x57, 0x28, 0x48, 0x8B, 0xC8, 0x4C, 0x8B,
    0x00, 0x41, 0xFF, 0x50, 0x48
};
static const BYTE kSyncActionDropMethodBytes[] = {
    0x40, 0x55, 0x41, 0x55, 0x41, 0x57, 0x48, 0x8D,
    0xAC, 0x24, 0x20, 0xFE, 0xFF, 0xFF, 0x48, 0x81,
    0xEC, 0xE0, 0x02, 0x00, 0x00, 0x48, 0x83, 0xB9,
    0xB0, 0x01, 0x00, 0x00, 0x00, 0x4C, 0x8B, 0xFA,
    0x4C, 0x8B, 0xE9, 0x0F, 0x84, 0x68, 0x05, 0x00,
    0x00, 0x80, 0xB9, 0xC0, 0x01, 0x00, 0x00, 0x00,
    0x0F, 0x85, 0x5B, 0x05, 0x00, 0x00, 0x48, 0x83,
    0xC1, 0x98, 0x4C, 0x89, 0xA4, 0x24, 0xC8, 0x02,
    0x00, 0x00, 0xE8, 0x59, 0xBE, 0xFE, 0xFF
};
static const BYTE kSyncActionCleanupBytes[] = {
    0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x80, 0xB9,
    0x28, 0x02, 0x00, 0x00, 0x00, 0x48, 0x8B, 0xD9,
    0x74, 0x12, 0x48, 0x81, 0xC1, 0x18, 0x02, 0x00,
    0x00, 0x48, 0x83, 0x39, 0x00, 0x74, 0x05, 0xE8,
    0x8C, 0xE4, 0xEA, 0xFF, 0x48, 0xC7, 0x83, 0x18,
    0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC7,
    0x83, 0x50, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
    0xFF, 0xC6, 0x83, 0x28, 0x02, 0x00, 0x00, 0x00
};

static const SIZE_T kDroppedItemVtableRva = 0x2952578;
// This is the larger inventory-drop dispatcher that calls the previously
// traced MenuInventoryController_PH::DropItem only on one fallback branch.
// A real physical weapon drop bypassed that fallback, so trace the caller.
// The 14-byte prologue below ends on an instruction boundary.
static const SIZE_T kMenuInventoryDropRva = 0xEEED10;
static const BYTE kMenuInventoryDropPrologue[] = {
    0x48, 0x89, 0x5C, 0x24, 0x08,
    0x48, 0x89, 0x6C, 0x24, 0x10,
    0x48, 0x89, 0x74, 0x24, 0x18
};
static BYTE* g_menu_drop_target;
static BYTE* g_menu_drop_trampoline;
static LONG g_menu_drop_trace_state; // 0 idle, 1 armed, 2 captured
static void* g_menu_drop_this;
static void* g_menu_drop_arg1;
static void* g_menu_drop_arg2;
static void* g_menu_drop_arg3;
static void* g_menu_drop_caller;

static bool is_beast_process() {
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(NULL, path, _countof(path));
    const wchar_t* name = wcsrchr(path, L'\\');
    return _wcsicmp(name ? name + 1 : path,
                    L"DyingLightGame_TheBeast_x64_rwdi.exe") == 0;
}

static void log_line(const wchar_t* format, ...) {
    wchar_t message[1024] = {}, line[1152] = {};
    va_list args; va_start(args, format);
    _vsnwprintf_s(message, _countof(message), _TRUNCATE, format, args);
    va_end(args);
    SYSTEMTIME now = {}; GetLocalTime(&now);
    _snwprintf_s(line, _countof(line), _TRUNCATE, L"[%02u:%02u:%02u.%03u] %s\r\n",
                 now.wHour, now.wMinute, now.wSecond, now.wMilliseconds, message);
    HANDLE file = CreateFileW(g_log_path, FILE_APPEND_DATA,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (file != INVALID_HANDLE_VALUE) {
        DWORD written = 0; WriteFile(file, line, (DWORD)(wcslen(line) * sizeof(wchar_t)), &written, NULL);
        CloseHandle(file);
    }
}

static bool is_readable_range(const void* address, SIZE_T length) {
    if (!address || !length) return false;
    const BYTE* cursor = (const BYTE*)address;
    const BYTE* end = cursor + length;
    if (end < cursor) return false;
    while (cursor < end) {
        MEMORY_BASIC_INFORMATION memory = {};
        if (!VirtualQuery(cursor, &memory, sizeof(memory)) || memory.State != MEM_COMMIT ||
            (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
        const BYTE* region_end = (const BYTE*)memory.BaseAddress + memory.RegionSize;
        if (region_end <= cursor) return false;
        cursor = region_end < end ? region_end : end;
    }
    return true;
}

static bool is_executable_address(const void* address) {
    MEMORY_BASIC_INFORMATION memory = {};
    if (!address || !VirtualQuery(address, &memory, sizeof(memory)) || memory.State != MEM_COMMIT ||
        (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
    const DWORD protection = memory.Protect & 0xFF;
    return protection == PAGE_EXECUTE || protection == PAGE_EXECUTE_READ ||
           protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
}

static bool is_sync_action_controller(void* candidate, void* player, BYTE* game_base) {
    if (!is_readable_range(candidate, kSyncActionDropActiveOffset + 1)) return false;
    BYTE* controller = (BYTE*)candidate;
    return *(void**)controller == game_base + kSyncActionControllerVtableRva &&
           *(void**)(controller + 8) == player &&
           *(void**)(controller + kSyncActionInventoryOffset) ==
               game_base + kSyncActionInventoryVtableRva;
}

static void* scan_sync_action_controller_range(void* player, BYTE* game_base,
                                               uintptr_t first, uintptr_t last,
                                               SIZE_T maximum_region_size) {
    const uintptr_t pointer_alignment = sizeof(void*) - 1;
    uintptr_t cursor = first;
    while (cursor < last) {
        MEMORY_BASIC_INFORMATION memory = {};
        if (!VirtualQuery((void*)cursor, &memory, sizeof(memory))) break;
        uintptr_t region_first = (uintptr_t)memory.BaseAddress;
        uintptr_t region_last = region_first + memory.RegionSize;
        if (region_last <= cursor) break;
        const bool readable = memory.State == MEM_COMMIT &&
            !(memory.Protect & (PAGE_GUARD | PAGE_NOACCESS));
        if (readable && (!maximum_region_size || memory.RegionSize <= maximum_region_size)) {
            uintptr_t at = region_first > first ? region_first : first;
            uintptr_t end = region_last < last ? region_last : last;
            at = (at + pointer_alignment) & ~pointer_alignment;
            __try {
                for (; at + kSyncActionDropActiveOffset + 1 <= end; at += sizeof(void*)) {
                    if (*(void**)at == game_base + kSyncActionControllerVtableRva &&
                        is_sync_action_controller((void*)at, player, game_base))
                        return (void*)at;
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                // The region changed while it was being inspected. Continue at
                // the next VirtualQuery boundary instead of risking the game.
            }
        }
        cursor = region_last;
    }
    return NULL;
}

static void* resolve_player_sync_action_controller(void* player, BYTE* game_base) {
    if (is_sync_action_controller(g_cached_sync_action_controller, player, game_base))
        return g_cached_sync_action_controller;

    const ULONGLONG started = GetTickCount64();
    SYSTEM_INFO system = {};
    GetSystemInfo(&system);
    const uintptr_t minimum = (uintptr_t)system.lpMinimumApplicationAddress;
    const uintptr_t maximum = (uintptr_t)system.lpMaximumApplicationAddress;
    const uintptr_t player_address = (uintptr_t)player;
    const uintptr_t near_distance = 128ull * 1024ull * 1024ull;
    const uintptr_t near_first = player_address > near_distance
        ? player_address - near_distance : minimum;
    const uintptr_t near_last = player_address < maximum - near_distance
        ? player_address + near_distance : maximum;

    void* controller = scan_sync_action_controller_range(
        player, game_base, near_first, near_last, 0);
    if (!controller) {
        // The controller is a small private object in this build.  This fallback
        // skips giant streaming allocations, keeping the one-time lookup bounded.
        controller = scan_sync_action_controller_range(
            player, game_base, minimum, maximum, 2ull * 1024ull * 1024ull);
    }
    g_cached_sync_action_controller = controller;
    log_line(L"[WORLD_ITEM] player-bound SyncAction lookup player=%p controller=%p elapsed=%llums",
             player, controller, GetTickCount64() - started);
    return controller;
}

static bool validate_function_bytes(BYTE* base, SIZE_T rva,
                                    const BYTE* expected, SIZE_T expected_size,
                                    const wchar_t* purpose) {
    BYTE* target = base + rva;
    if (!is_readable_range(target, expected_size) || memcmp(target, expected, expected_size)) {
        log_line(L"[ITEM] %s rejected: RVA=0x%llX expected bytes do not match",
                 purpose, (unsigned long long)rva);
        return false;
    }
    return true;
}

static bool validate_give_build(HMODULE game) {
    if (!game || !is_readable_range(game, sizeof(IMAGE_DOS_HEADER))) return false;
    BYTE* base = (BYTE*)game;
    bool identity_ok = false;
    __try {
        IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
        if (dos->e_magic == IMAGE_DOS_SIGNATURE) {
            IMAGE_NT_HEADERS64* nt = (IMAGE_NT_HEADERS64*)(base + dos->e_lfanew);
            identity_ok = nt->Signature == IMAGE_NT_SIGNATURE &&
                nt->FileHeader.TimeDateStamp == 0x6AA8CBF7 &&
                nt->OptionalHeader.SizeOfImage == 0x3F0D000;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) { identity_ok = false; }
    if (!identity_ok) {
        log_line(L"[ITEM] give rejected: loaded gamedll identity does not match the audited retail build");
        return false;
    }
    return validate_function_bytes(base, kGetGameDiRva, kGetGameDiBytes,
                                   sizeof(kGetGameDiBytes), L"GameDI resolver") &&
           validate_function_bytes(base, kResolveLocalPlayerRva, kResolveLocalPlayerBytes,
                                   sizeof(kResolveLocalPlayerBytes), L"local-player resolver") &&
           validate_function_bytes(base, kGiveInventoryItemRva, kGiveInventoryItemBytes,
                                   sizeof(kGiveInventoryItemBytes), L"inventory insertion") &&
           validate_function_bytes(base, kRemoveInventoryItemRva, kRemoveInventoryItemBytes,
                                   sizeof(kRemoveInventoryItemBytes), L"inventory removal") &&
           validate_function_bytes(base, kPlayerInventoryAccessRva,
                                   kPlayerInventoryAccessBytes,
                                   sizeof(kPlayerInventoryAccessBytes),
                                   L"player inventory field") &&
           validate_function_bytes(base, kForceSendToStashWriteRva,
                                   kForceSendToStashWriteBytes,
                                   sizeof(kForceSendToStashWriteBytes),
                                   L"force-stash flag reference");
}

static void classify_item(const char* name, const char* id, char* category, SIZE_T bytes) {
    char folded[384] = {};
    _snprintf_s(folded, _countof(folded), _TRUNCATE, "%s %s", name ? name : "", id ? id : "");
    _strlwr_s(folded, _countof(folded));
    const char* value = "Miscellaneous";
    if (strstr(folded, "outfit") || strstr(folded, "wearable") || strstr(folded, "headwear") ||
        strstr(folded, "_torso") || strstr(folded, "_legs") || strstr(folded, "_shoes") ||
        strstr(folded, "_gloves") || strstr(folded, "bracer") || strstr(folded, "armor"))
        value = "Gear & Outfits";
    else if (strstr(folded, "craftplan") || strstr(folded, "blueprint"))
        value = "Blueprints";
    else if (strstr(folded, "ammo") || strstr(folded, "ammunition"))
        value = "Ammunition";
    else if (strstr(folded, "throwable") || strstr(folded, "grenade") ||
             strstr(folded, "molotov") || strstr(folded, "snowball"))
        value = "Throwables";
    else if (strstr(folded, "charm") || strstr(folded, "accessory") || strstr(folded, "trinket"))
        value = "Charms & Accessories";
    else if (strstr(folded, "collectible") || strstr(folded, "valuable") ||
             strstr(folded, "memento") || strstr(folded, "tape_") || strstr(folded, "note_"))
        value = "Collectibles & Valuables";
    else if (strstr(folded, "craftpart") || strstr(folded, "craftingpart") ||
             strstr(folded, "resource") || strstr(folded, "material") ||
             strstr(folded, "parts") || strstr(folded, "rags"))
        value = "Crafting Resources";
    else if (strstr(folded, "consumable") || strstr(folded, "medicine") ||
             strstr(folded, "bandage") || strstr(folded, "booster") || strstr(folded, "food_"))
        value = "Consumables";
    else if (strstr(folded, "vehicle_") || strstr(folded, "vehicle skin"))
        value = "Vehicles & Skins";
    else if (strstr(folded, "wpn_") || strstr(folded, "weapon") ||
             strstr(folded, "firearm") || strstr(folded, "crossbow") || strstr(folded, "lbow"))
        value = "Weapons";
    strncpy_s(category, bytes, value, _TRUNCATE);
}

static char* trim_ascii(char* value) {
    while (*value == ' ' || *value == '\t' || *value == '\r' || *value == '\n') ++value;
    char* end = value + strlen(value);
    while (end > value && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) --end;
    *end = 0;
    return value;
}

static DWORD parse_weapon_level(const char* id, const char* category) {
    if (!id || !category ||
        (_stricmp(category, "Melee weapons") && _stricmp(category, "Ranged weapons")))
        return 0;
    const SIZE_T length = strlen(id);
    DWORD result = 0;
    for (SIZE_T at = 0; at + 2 < length; ++at) {
        if (id[at] != '_' || (id[at + 1] != 'r' && id[at + 1] != 'R') ||
            id[at + 2] < '0' || id[at + 2] > '9')
            continue;
        DWORD value = (DWORD)(id[at + 2] - '0');
        SIZE_T end = at + 3;
        if (end < length && id[end] >= '0' && id[end] <= '9') {
            value = value * 10 + (DWORD)(id[end] - '0');
            ++end;
        }
        if (value >= 1 && value <= 15 &&
            (end == length || id[end] == '_' || id[end] == '-' || id[end] == '.'))
            result = value;
    }
    return result;
}

static void add_catalog_item(const char* name, const char* id) {
    if (!id || !id[0]) return;
    for (DWORD i = 0; i < g_item_count; ++i) {
        if (!_stricmp(g_items[i].id, id)) {
            if (name && name[0] && !_stricmp(g_items[i].name, g_items[i].id))
                strncpy_s(g_items[i].name, _countof(g_items[i].name), name, _TRUNCATE);
            return;
        }
    }
    if (g_item_count >= BSS_MAX_ITEMS) return;
    ItemDefinition* item = &g_items[g_item_count++];
    ZeroMemory(item, sizeof(*item));
    strncpy_s(item->name, _countof(item->name), name && name[0] ? name : id, _TRUNCATE);
    strncpy_s(item->id, _countof(item->id), id, _TRUNCATE);
    classify_item(item->name, item->id, item->category, _countof(item->category));
    strncpy_s(item->rarity, _countof(item->rarity), "Unspecified", _TRUNCATE);
    item->weapon_level = parse_weapon_level(item->id, item->category);
}

static int split_tab_fields(char* line, char** fields, int capacity) {
    if (!line || !fields || capacity < 1) return 0;
    int count = 0;
    fields[count++] = line;
    for (char* at = line; *at && count < capacity; ++at) {
        if (*at == '\t') {
            *at = 0;
            fields[count++] = at + 1;
        }
    }
    return count;
}

#ifdef IE_OWNED_AFFIX_EDITOR
static void load_owned_affix_metadata() {
    g_owned_affix_metadata_count = 0;
    wchar_t path[MAX_PATH] = {};
    _snwprintf_s(path, _countof(path), _TRUNCATE, L"%sGameAffixCatalog.tsv", g_own_dir);
    FILE* file = NULL;
    _wfopen_s(&file, path, L"rb");
    if (!file) {
        log_line(L"[OWNED_AFFIX] localized affix catalogue missing: %s", path);
        return;
    }
    char line[8192] = {};
    while (fgets(line, sizeof(line), file) &&
           g_owned_affix_metadata_count < _countof(g_owned_affix_metadata)) {
        char* record = trim_ascii(line);
        if (!record[0] || record[0] == '#') continue;
        char* fields[12] = {};
        const int count = split_tab_fields(record, fields, (int)_countof(fields));
        if (count < 4 || !_stricmp(fields[0], "id") || !fields[0][0]) continue;
        OwnedAffixMetadata* entry = &g_owned_affix_metadata[g_owned_affix_metadata_count++];
        strncpy_s(entry->id, _countof(entry->id), fields[0], _TRUNCATE);
        strncpy_s(entry->name, _countof(entry->name),
                  fields[1][0] ? fields[1] : fields[0], _TRUNCATE);
        strncpy_s(entry->value_format, _countof(entry->value_format), fields[3], _TRUNCATE);
        entry->game_localized = count > 11 && !_stricmp(fields[11], "game");
    }
    fclose(file);
    log_line(L"[OWNED_AFFIX] localized affix names loaded: %u", g_owned_affix_metadata_count);
}

static const OwnedAffixMetadata* owned_affix_metadata(const char* id) {
    for (unsigned i = 0; i < g_owned_affix_metadata_count; ++i)
        if (!_stricmp(id, g_owned_affix_metadata[i].id))
            return &g_owned_affix_metadata[i];
    return NULL;
}

static void ascii_to_wide(const char* source, wchar_t* destination, int count);

static void format_owned_affix_value(const OwnedAffixMetadata* metadata,
                                     float magnitude, wchar_t* output,
                                     size_t capacity) {
    if (!output || !capacity) return;
    output[0] = 0;
    // Apply only the game's verified display template. Negative custom rolls
    // are shown numerically rather than inventing a double-sign tooltip.
    if (metadata && magnitude >= 0.0f && metadata->value_format[0]) {
        wchar_t format[120] = {};
        ascii_to_wide(metadata->value_format, format, (int)_countof(format));
        wchar_t* placeholder = wcsstr(format, L"%1s");
        if (placeholder) {
            *placeholder = 0;
            _snwprintf_s(output, capacity, _TRUNCATE, L"%s%.9g%s",
                         format, magnitude, placeholder + 3);
            return;
        }
    }
    _snwprintf_s(output, capacity, _TRUNCATE, L"%.9g", magnitude);
}

static void load_owned_builtin_stats() {
    g_owned_builtin_count = 0;
    wchar_t path[MAX_PATH] = {};
    _snwprintf_s(path, _countof(path), _TRUNCATE, L"%sGameBuiltInStats.tsv", g_own_dir);
    FILE* file = NULL;
    _wfopen_s(&file, path, L"rb");
    if (!file) {
        log_line(L"[OWNED_AFFIX] built-in stat catalogue missing: %s", path);
        return;
    }
    char line[2048] = {};
    while (fgets(line, sizeof(line), file) &&
           g_owned_builtin_count < _countof(g_owned_builtin_stats)) {
        char* record = trim_ascii(line);
        if (!record[0] || record[0] == '#') continue;
        char* fields[8] = {};
        const int count = split_tab_fields(record, fields, (int)_countof(fields));
        if (count < 8 || !_stricmp(fields[0], "id") || !fields[0][0]) continue;
        OwnedBuiltInStat* entry = &g_owned_builtin_stats[g_owned_builtin_count++];
        strncpy_s(entry->item_id, _countof(entry->item_id), fields[0], _TRUNCATE);
        strncpy_s(entry->name, _countof(entry->name), fields[2], _TRUNCATE);
        strncpy_s(entry->authored_value, _countof(entry->authored_value), fields[3], _TRUNCATE);
        strncpy_s(entry->unit, _countof(entry->unit), fields[4], _TRUNCATE);
        entry->scalable = fields[5][0] == '1';
        strncpy_s(entry->curve, _countof(entry->curve), fields[6], _TRUNCATE);
    }
    fclose(file);
    log_line(L"[OWNED_AFFIX] definition-authored UIStats loaded: %u", g_owned_builtin_count);
}

static unsigned owned_builtin_for_item(const char* id, const OwnedBuiltInStat** output,
                                       unsigned capacity) {
    unsigned count = 0;
    for (unsigned i = 0; i < g_owned_builtin_count; ++i) {
        if (_stricmp(g_owned_builtin_stats[i].item_id, id)) continue;
        if (output && count < capacity) output[count] = &g_owned_builtin_stats[i];
        ++count;
    }
    return count;
}
#endif

static void add_game_catalog_item(char** fields, int field_count) {
    if (!fields || field_count < 4 || !fields[0][0] || g_item_count >= BSS_MAX_ITEMS) return;
    for (DWORD i = 0; i < g_item_count; ++i)
        if (!_stricmp(g_items[i].id, fields[0])) return;
    ItemDefinition* item = &g_items[g_item_count++];
    ZeroMemory(item, sizeof(*item));
    strncpy_s(item->id, _countof(item->id), fields[0], _TRUNCATE);
    strncpy_s(item->name, _countof(item->name), fields[1][0] ? fields[1] : fields[0], _TRUNCATE);
    strncpy_s(item->category, _countof(item->category), fields[2][0] ? fields[2] : "Miscellaneous", _TRUNCATE);
    strncpy_s(item->rarity, _countof(item->rarity), fields[3][0] ? fields[3] : "Unspecified", _TRUNCATE);
    if (field_count > 4) {
        item->can_drop = !_stricmp(fields[4], "true") ? 1 : !_stricmp(fields[4], "false") ? 2 : 0;
    }
    if (field_count > 5) strncpy_s(item->physics, _countof(item->physics), fields[5], _TRUNCATE);
    if (field_count > 6) strncpy_s(item->localization_key, _countof(item->localization_key), fields[6], _TRUNCATE);
    if (field_count > 7) strncpy_s(item->item_type, _countof(item->item_type), fields[7], _TRUNCATE);
    if (field_count > 8) strncpy_s(item->source, _countof(item->source), fields[8], _TRUNCATE);
    item->weapon_level = parse_weapon_level(item->id, item->category);
}

static int __cdecl compare_catalog_items(const void* left, const void* right) {
    const ItemDefinition* a = (const ItemDefinition*)left;
    const ItemDefinition* b = (const ItemDefinition*)right;
    int category = _stricmp(a->category, b->category);
    return category ? category : _stricmp(a->name, b->name);
}

static void favorites_path(wchar_t path[MAX_PATH], int format = 0) {
    wchar_t local[MAX_PATH] = {};
    GetEnvironmentVariableW(L"LOCALAPPDATA", local, _countof(local));
    const wchar_t* filename = format == 2 ? L"BeastNativeDevMenu.favorites.txt"
        : format == 1 ? L"BeastNativeDevMenu.ini" : L"ItemExtravaganza.ini";
    _snwprintf_s(path, MAX_PATH, _TRUNCATE, L"%s\\%s", local, filename);
}

static void save_favorites();

static void load_favorites() {
    wchar_t path[MAX_PATH] = {}; favorites_path(path);
    FILE* file = NULL; _wfopen_s(&file, path, L"rb");
    bool legacy = false;
    bool migrate = false;
    if (!file) {
        favorites_path(path, 1);
        _wfopen_s(&file, path, L"rb");
        migrate = file != NULL;
    }
    if (!file) {
        favorites_path(path, 2);
        _wfopen_s(&file, path, L"rb");
        legacy = migrate = file != NULL;
    }
    if (!file) return;
    char line[256] = {};
    bool in_favorites = legacy;
    while (fgets(line, sizeof(line), file)) {
        char* id = trim_ascii(line);
        if (!id[0] || id[0] == ';' || id[0] == '#') continue;
        if (!legacy) {
            if (id[0] == '[') {
                in_favorites = !_stricmp(id, "[Favorites]");
                continue;
            }
            if (!in_favorites) continue;
            char* separator = strchr(id, '=');
            if (!separator) continue;
            *separator++ = 0;
            if (strcmp(trim_ascii(separator), "1")) continue;
            id = trim_ascii(id);
        }
        for (DWORD i = 0; id[0] && i < g_item_count; ++i)
            if (!_stricmp(g_items[i].id, id)) { g_items[i].favorite = 1; break; }
    }
    fclose(file);
    if (migrate) save_favorites();
}

static void save_favorites() {
    wchar_t path[MAX_PATH] = {}; favorites_path(path);
    wchar_t temporary[MAX_PATH] = {};
    _snwprintf_s(temporary, _countof(temporary), _TRUNCATE, L"%s.tmp", path);
    FILE* file = NULL; _wfopen_s(&file, temporary, L"wb");
    if (!file) { log_line(L"[RESOURCE] could not save favorites: %s", path); return; }
    fputs("[Favorites]\r\n", file);
    for (DWORD i = 0; i < g_item_count; ++i)
        if (g_items[i].favorite) fprintf(file, "%s=1\r\n", g_items[i].id);
    fclose(file);
    if (!MoveFileExW(temporary, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        log_line(L"[RESOURCE] could not replace favorites INI: %s error=%lu", path, GetLastError());
}

static void load_item_catalog() {
    EnterCriticalSection(&g_data_lock);
    struct RecentEntry { char id[160]; DWORD order; };
    RecentEntry* recent = (RecentEntry*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                   sizeof(RecentEntry) * g_item_count);
    DWORD recent_count = 0;
    if (recent) for (DWORD i = 0; i < g_item_count; ++i) {
        if (!g_items[i].recent_order) continue;
        strcpy_s(recent[recent_count].id, g_items[i].id);
        recent[recent_count++].order = g_items[i].recent_order;
    }
    ZeroMemory(g_items, sizeof(g_items));
    g_item_count = 0;
    wchar_t path[MAX_PATH] = {};
    _snwprintf_s(path, _countof(path), _TRUNCATE, L"%sGameItemCatalog.tsv", g_own_dir);
    FILE* file = NULL; _wfopen_s(&file, path, L"rb");
    if (file) {
        char line[4096] = {};
        while (fgets(line, sizeof(line), file)) {
            char* record = trim_ascii(line);
            if (!record[0] || record[0] == '#') continue;
            char* fields[9] = {};
            const int count = split_tab_fields(record, fields, (int)_countof(fields));
            for (int i = 0; i < count; ++i) fields[i] = trim_ascii(fields[i]);
            add_game_catalog_item(fields, count);
        }
        fclose(file);
    }
    // The known-good item keeps manual testing possible if the seed file is
    // missing, while a normal installation loads thousands of definitions.
    add_catalog_item("Rusty Pipe", "dlc_ft_wpn_1hb_mace_m_r1");
    qsort(g_items, g_item_count, sizeof(g_items[0]), compare_catalog_items);
    load_favorites();
    if (recent) {
        for (DWORD i = 0; i < g_item_count; ++i)
            for (DWORD j = 0; j < recent_count; ++j)
                if (!_stricmp(g_items[i].id, recent[j].id)) {
                    g_items[i].recent_order = recent[j].order;
                    break;
                }
        HeapFree(GetProcessHeap(), 0, recent);
    }
    const DWORD count = g_item_count;
    LeaveCriticalSection(&g_data_lock);
    log_line(L"[RESOURCE] game-owned item catalog loaded path=%s entries=%lu", path, count);
    rebuild_catalog_filter_choices();
}

static void ascii_to_wide(const char* source, wchar_t* destination, int count) {
    if (!destination || count <= 0) return;
    destination[0] = 0;
    MultiByteToWideChar(CP_UTF8, 0, source ? source : "", -1, destination, count);
    destination[count - 1] = 0;
}

static void selected_filter_text(HWND combo, char* destination, int capacity) {
    if (!destination || capacity < 1) return;
    destination[0] = 0;
    if (!combo) return;
    const LRESULT selected = SendMessageW(combo, CB_GETCURSEL, 0, 0);
    if (selected == CB_ERR || selected == 0) return;
    wchar_t wide[96] = {};
    SendMessageW(combo, CB_GETLBTEXT, (WPARAM)selected, (LPARAM)wide);
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, destination, capacity, NULL, NULL);
    destination[capacity - 1] = 0;
}

static void rebuild_catalog_filter_choices() {
    if (!g_category_filter || !g_rarity_filter) return;
    char selected_category[64] = {}, selected_rarity[32] = {};
    selected_filter_text(g_category_filter, selected_category, (int)_countof(selected_category));
    selected_filter_text(g_rarity_filter, selected_rarity, (int)_countof(selected_rarity));
    SendMessageW(g_category_filter, CB_RESETCONTENT, 0, 0);
    SendMessageW(g_rarity_filter, CB_RESETCONTENT, 0, 0);
    SendMessageW(g_category_filter, CB_ADDSTRING, 0, (LPARAM)L"All categories");
    SendMessageW(g_rarity_filter, CB_ADDSTRING, 0, (LPARAM)L"All rarities");
    int category_choice = 0, rarity_choice = 0;
    EnterCriticalSection(&g_data_lock);
    for (DWORD i = 0; i < g_item_count; ++i) {
        const char* category = g_items[i].category;
        const char* rarity = g_items[i].rarity;
        wchar_t wide[96] = {};
        ascii_to_wide(category, wide, (int)_countof(wide));
        if (SendMessageW(g_category_filter, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)wide) == CB_ERR) {
            const LRESULT index = SendMessageW(g_category_filter, CB_ADDSTRING, 0, (LPARAM)wide);
            if (!_stricmp(category, selected_category)) category_choice = (int)index;
        }
        ascii_to_wide(rarity, wide, (int)_countof(wide));
        if (SendMessageW(g_rarity_filter, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)wide) == CB_ERR) {
            const LRESULT index = SendMessageW(g_rarity_filter, CB_ADDSTRING, 0, (LPARAM)wide);
            if (!_stricmp(rarity, selected_rarity)) rarity_choice = (int)index;
        }
    }
    LeaveCriticalSection(&g_data_lock);
    SendMessageW(g_category_filter, CB_SETCURSEL, category_choice, 0);
    SendMessageW(g_rarity_filter, CB_SETCURSEL, rarity_choice, 0);
}

static void update_selected_item_panel() {
    if (!g_selection || !g_item_list) return;
#ifdef IE_OWNED_AFFIX_EDITOR
    if (g_owned_mode) {
        InvalidateRect(g_selection, NULL, FALSE);
        update_owned_affix_panel();
        return;
    }
#endif
    int row = ListView_GetNextItem(g_item_list, -1, LVNI_SELECTED);
    if (row < 0 || row >= (int)g_visible_count) {
        if (g_favorite) {
            SetWindowTextW(g_favorite, L"Favorite");
            InvalidateRect(g_favorite, NULL, TRUE);
        }
        InvalidateRect(g_selection, NULL, FALSE);
        return;
    }
    ItemDefinition item = {};
    EnterCriticalSection(&g_data_lock);
    item = g_items[g_visible_items[row]];
    LeaveCriticalSection(&g_data_lock);
    if (g_favorite) {
        SetWindowTextW(g_favorite, item.favorite ? L"Unfavorite" : L"Favorite");
        InvalidateRect(g_favorite, NULL, TRUE);
    }
    InvalidateRect(g_selection, NULL, FALSE);
}

#ifdef IE_OWNED_AFFIX_EDITOR
static void refresh_owned_view() {
    if (!g_item_list || !g_search) return;
    void* selected_instance = NULL;
    // Snapshot indices may move after equip/scan; preserve exact identity.
    selected_instance = g_owned_panel_instance;
    if (g_owned_duplicate_result) {
        selected_instance = g_owned_duplicate_result;
        g_owned_duplicate_result = NULL;
    }
    wchar_t wide_query[192] = {}; char query[192] = {};
    GetWindowTextW(g_search, wide_query, _countof(wide_query));
    WideCharToMultiByte(CP_UTF8, 0, wide_query, -1, query, _countof(query), NULL, NULL);
    _strlwr_s(query, _countof(query));
    char category[64] = {}, rarity[32] = {};
    selected_filter_text(g_category_filter, category, (int)_countof(category));
    selected_filter_text(g_rarity_filter, rarity, (int)_countof(rarity));
    const int view_index = g_view_filter
        ? (int)SendMessageW(g_view_filter, CB_GETCURSEL, 0, 0) : 0;
    const int weapon_level = g_weapon_level_filter
        ? (int)SendMessageW(g_weapon_level_filter, CB_GETCURSEL, 0, 0) : 0;
    int restored_row = -1;
    EnterCriticalSection(&g_data_lock);
    g_visible_count = 0;
    DWORD total = 0;
    for (DWORD i = 0; i < g_owned_count; ++i) {
        const OwnedWeaponView* item = &g_owned_weapons[i];
        if (item->is_gear != g_owned_gear_mode) continue;
        ++total;
        if (category[0] && _stricmp(item->category, category)) continue;
        if (rarity[0] && _stricmp(item->rarity, rarity)) continue;
        if (weapon_level > 0 && item->weapon_level != (DWORD)weapon_level) continue;
        if (view_index == 1 && !item->favorite) continue;
        if (view_index == 2 && !item->recent_order) continue;
        if (query[0]) {
            char folded[1024] = {};
            _snprintf_s(folded, _countof(folded), _TRUNCATE, "%s %s %s %s",
                        item->name, item->id, item->category, item->rarity);
            _strlwr_s(folded, _countof(folded));
            bool matches = strstr(folded, query) != NULL;
            for (unsigned j = 0; !matches && j < item->affix_count; ++j) {
                char id[128] = {};
                strcpy_s(id, item->affixes[j].id);
                _strlwr_s(id, _countof(id));
                matches = strstr(id, query) != NULL;
            }
            if (!matches) continue;
        }
        g_visible_items[g_visible_count++] = (int)i;
    }
    // Worn identities come from native outfit slots, not display order or IDs.
    if (g_owned_gear_mode) {
        for (DWORD i = 1; i < g_visible_count; ++i) {
            const int key = g_visible_items[i];
            const unsigned rank = g_owned_weapons[key].worn_slot ?
                g_owned_weapons[key].worn_slot : 7;
            DWORD j = i;
            while (j && (g_owned_weapons[g_visible_items[j-1]].worn_slot ?
                   g_owned_weapons[g_visible_items[j-1]].worn_slot : 7) > rank) {
                g_visible_items[j] = g_visible_items[j-1]; --j;
            }
            g_visible_items[j] = key;
        }
    }
    for (DWORD i = 0; i < g_visible_count; ++i)
        if (g_owned_weapons[g_visible_items[i]].instance == selected_instance)
            restored_row = (int)i;
    const DWORD visible = g_visible_count;
    LeaveCriticalSection(&g_data_lock);
    g_owned_view_rebuilding = true;
    ListView_SetItemCountEx(g_item_list, visible, LVSICF_NOSCROLL);
    ListView_SetItemState(g_item_list, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    if (restored_row >= 0)
        ListView_SetItemState(g_item_list, restored_row, LVIS_SELECTED | LVIS_FOCUSED,
                              LVIS_SELECTED | LVIS_FOCUSED);
    g_owned_view_rebuilding = false;
    if (visible) ListView_RedrawItems(g_item_list, 0, visible - 1);
    update_selected_item_panel();
    if (g_result_count) {
        wchar_t text[80] = {};
        _snwprintf_s(text, _countof(text), _TRUNCATE,
                     L"%lu of %lu %s\r\nCarried inventory", visible, total,
                     g_owned_gear_mode ? L"gear pieces" : L"weapons");
        SetWindowTextW(g_result_count, text);
    }
    if (g_tab_owned_weapons) InvalidateRect(g_tab_owned_weapons, NULL, FALSE);
    if (g_tab_owned_gear) InvalidateRect(g_tab_owned_gear, NULL, FALSE);
}
#endif

static void refresh_item_view() {
    if (!g_item_list || !g_search) return;
#ifdef IE_OWNED_AFFIX_EDITOR
    if (g_owned_mode) { refresh_owned_view(); return; }
#endif
    char selected_id[160] = {};
    const int old_row = ListView_GetNextItem(g_item_list, -1, LVNI_SELECTED);
    if (old_row >= 0 && old_row < (int)g_visible_count) {
        EnterCriticalSection(&g_data_lock);
        strcpy_s(selected_id, g_items[g_visible_items[old_row]].id);
        LeaveCriticalSection(&g_data_lock);
    }
    wchar_t wide_query[192] = {}; char query[192] = {};
    GetWindowTextW(g_search, wide_query, _countof(wide_query));
    WideCharToMultiByte(CP_UTF8, 0, wide_query, -1, query, _countof(query), NULL, NULL);
    _strlwr_s(query, _countof(query));
    char category[64] = {}, rarity[32] = {};
    selected_filter_text(g_category_filter, category, (int)_countof(category));
    selected_filter_text(g_rarity_filter, rarity, (int)_countof(rarity));
    int view_index = g_view_filter ? (int)SendMessageW(g_view_filter, CB_GETCURSEL, 0, 0) : 0;
    int weapon_level = g_weapon_level_filter
        ? (int)SendMessageW(g_weapon_level_filter, CB_GETCURSEL, 0, 0) : 0;
    EnterCriticalSection(&g_data_lock);
    g_visible_count = 0;
    int restored_row = -1;
    for (DWORD i = 0; i < g_item_count && g_visible_count < BSS_MAX_VISIBLE; ++i) {
        const ItemDefinition* item = &g_items[i];
        if (category[0] && _stricmp(item->category, category)) continue;
        if (rarity[0] && _stricmp(item->rarity, rarity)) continue;
        if (weapon_level > 0 && item->weapon_level != (DWORD)weapon_level) continue;
        if (view_index == 1 && !item->favorite) continue;
        if (view_index == 2 && !item->recent_order) continue;
        if (query[0]) {
            char folded[1024] = {};
            _snprintf_s(folded, _countof(folded), _TRUNCATE, "%s %s %s %s %s %s %s",
                        item->name, item->id, item->category, item->rarity,
                        item->localization_key, item->item_type, item->source);
            _strlwr_s(folded, _countof(folded));
            if (!strstr(folded, query)) continue;
        }
        if (selected_id[0] && !_stricmp(item->id, selected_id)) restored_row = (int)g_visible_count;
        g_visible_items[g_visible_count++] = (int)i;
    }
    const DWORD visible = g_visible_count;
    const DWORD total = g_item_count;
    LeaveCriticalSection(&g_data_lock);
    ListView_SetItemCountEx(g_item_list, visible, LVSICF_NOSCROLL);
    ListView_SetItemState(g_item_list, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    if (restored_row >= 0)
        ListView_SetItemState(g_item_list, restored_row, LVIS_SELECTED | LVIS_FOCUSED,
                              LVIS_SELECTED | LVIS_FOCUSED);
    if (visible) ListView_RedrawItems(g_item_list, 0, visible - 1);
    update_selected_item_panel();
    if (g_result_count) {
        wchar_t count[80] = {};
        if (visible == total)
            _snwprintf_s(count, _countof(count), _TRUNCATE, L"%lu items\r\nCurrent catalog", total);
        else
            _snwprintf_s(count, _countof(count), _TRUNCATE, L"%lu of %lu items\r\nFiltered results", visible, total);
        SetWindowTextW(g_result_count, count);
    }
    if (g_tab_all_items) InvalidateRect(g_tab_all_items, NULL, FALSE);
    if (g_tab_favorites) InvalidateRect(g_tab_favorites, NULL, FALSE);
}

static bool selected_item(ItemDefinition* output, int* catalog_index) {
    if (!output) return false;
#ifdef IE_OWNED_AFFIX_EDITOR
    if (g_owned_mode) return false;
#endif
    ZeroMemory(output, sizeof(*output));
    int row = g_item_list ? ListView_GetNextItem(g_item_list, -1, LVNI_SELECTED) : -1;
    if (row >= 0 && row < (int)g_visible_count) {
        const int index = g_visible_items[row];
        EnterCriticalSection(&g_data_lock);
        *output = g_items[index];
        LeaveCriticalSection(&g_data_lock);
        if (catalog_index) *catalog_index = index;
        return output->id[0] != 0;
    }

    // Manual internal-ID escape hatch: a nonmatching search can still be used
    // directly, but whitespace and quotes are rejected.
    wchar_t wide[180] = {}; char id[160] = {};
    if (g_search) GetWindowTextW(g_search, wide, _countof(wide));
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, id, _countof(id), NULL, NULL);
    char* trimmed = trim_ascii(id);
    if (!trimmed[0] || strpbrk(trimmed, " \t\r\n\"'")) return false;
    strncpy_s(output->name, _countof(output->name), trimmed, _TRUNCATE);
    strncpy_s(output->id, _countof(output->id), trimmed, _TRUNCATE);
    classify_item(output->name, output->id, output->category, _countof(output->category));
    output->weapon_level = parse_weapon_level(output->id, output->category);
    if (catalog_index) *catalog_index = -1;
    return true;
}

static bool get_section(HMODULE module, const char* section_name, BYTE** out, SIZE_T* size) {
    BYTE* base = (BYTE*)module;
    __try {
        IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
        IMAGE_NT_HEADERS64* nt = (IMAGE_NT_HEADERS64*)(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) return false;
        IMAGE_SECTION_HEADER* section = IMAGE_FIRST_SECTION(nt);
        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section) {
            char name[9] = {}; memcpy(name, section->Name, 8);
            if (!strcmp(name, section_name)) {
                *out = base + section->VirtualAddress;
                *size = section->Misc.VirtualSize;
                return true;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    return false;
}

static BYTE* find_ascii(BYTE* begin, SIZE_T size, const char* needle) {
    SIZE_T length = strlen(needle);
    if (!length || size < length) return NULL;
    for (SIZE_T at = 0; at <= size - length; ++at)
        if (!memcmp(begin + at, needle, length)) return begin + at;
    return NULL;
}

// Locate RIP-relative LEA references only.  This is an observation tool, not
// a call-signature guesser: every candidate remains a logged lead until its
// caller and arguments have been traced.
static unsigned count_lea_refs(BYTE* text, SIZE_T text_size, BYTE* target, BYTE** first) {
    unsigned count = 0; if (first) *first = NULL;
    for (SIZE_T at = 0; at + 7 <= text_size; ++at) {
        BYTE* p = text + at;
        if (p[0] != 0x48 || p[1] != 0x8D || (p[2] != 0x05 && p[2] != 0x0D && p[2] != 0x15)) continue;
        LONG displacement = *(LONG*)(p + 3);
        if (p + 7 + displacement != target) continue;
        if (first && !*first) *first = p;
        ++count;
    }
    return count;
}

// Like count_lea_refs, but accepts every x64 register encoding.  Constructors
// commonly materialize a vtable through r8/r9 as well as rax/rcx/rdx.
static unsigned count_any_rip_lea_refs(BYTE* text, SIZE_T text_size, BYTE* target, BYTE** first) {
    unsigned count = 0; if (first) *first = NULL;
    for (SIZE_T at = 0; at + 7 <= text_size; ++at) {
        BYTE* p = text + at;
        if ((p[0] < 0x48 || p[0] > 0x4F) || p[1] != 0x8D || (p[2] & 0xC7) != 0x05) continue;
        LONG displacement = *(LONG*)(p + 3);
        if (p + 7 + displacement != target) continue;
        if (first && !*first) *first = p;
        ++count;
    }
    return count;
}

// MSVC x64's CompleteObjectLocator stores image-relative offsets to the
// type descriptor and itself.  Resolving the DroppedItem vtable from these
// records is read-only and lets us find the *actual* construction code before
// any future runtime hook is considered.
static BYTE* find_msvc_vtable(BYTE* base, BYTE* rdata, SIZE_T rdata_size,
                              BYTE* data, SIZE_T data_size, BYTE* text,
                              SIZE_T text_size, const char* type_name) {
    BYTE* name = find_ascii(rdata, rdata_size, type_name);
    if (!name) name = find_ascii(data, data_size, type_name);
    if (!name || name < base + 16) return NULL;
    BYTE* type_descriptor = name - 16;
    const DWORD type_rva = (DWORD)(type_descriptor - base);
    for (SIZE_T at = 0; at + 24 <= rdata_size; at += 4) {
        BYTE* col = rdata + at;
        DWORD* fields = (DWORD*)col;
        // Some shipped classes use the legacy signature form, so pSelf is
        // useful as a discriminator but not a hard requirement.
        if (fields[0] > 1 || fields[3] != type_rva) continue;
        for (SIZE_T ref = 0; ref + 16 <= rdata_size; ref += sizeof(void*)) {
            BYTE* slot = rdata + ref;
            if (*(BYTE**)slot != col) continue;
            BYTE* vtable = slot + sizeof(void*);
            BYTE* first_function = *(BYTE**)vtable;
            if (first_function >= text && first_function < text + text_size) return vtable;
        }
    }
    return NULL;
}

static void trace_dropped_item_construction(BYTE* base, BYTE* rdata, SIZE_T rdata_size,
                                            BYTE* data, SIZE_T data_size, BYTE* text,
                                            SIZE_T text_size) {
    BYTE* vtable = find_msvc_vtable(base, rdata, rdata_size, data, data_size,
                                    text, text_size, ".?AVDroppedItem@@");
    if (!vtable) {
        log_line(L"[WORLD_ITEM] DroppedItem RTTI/vtable could not be resolved on this build");
        return;
    }
    BYTE* first = NULL;
    const unsigned references = count_any_rip_lea_refs(text, text_size, vtable, &first);
    log_line(L"[WORLD_ITEM] DroppedItem vtable_rva=0x%llX construction_lea_refs=%u first_ref_rva=0x%llX",
             (unsigned long long)(vtable - base), references,
             first ? (unsigned long long)(first - base) : 0ull);
    for (unsigned i = 0; i < 12; ++i) {
        BYTE* function = ((BYTE**)vtable)[i];
        if (function < text || function >= text + text_size) break;
        log_line(L"[WORLD_ITEM] DroppedItem vfunc[%u]_rva=0x%llX", i,
                 (unsigned long long)(function - base));
    }
}

static void update_runtime_log_scrollbar() {
    if (!g_runtime_log) return;
    RECT bounds = {};
    GetClientRect(g_runtime_log, &bounds);
    HDC dc = GetDC(g_runtime_log);
    int line_height = 15;
    if (dc) {
        HGDIOBJ previous = SelectObject(dc, g_mono_font ? g_mono_font : GetStockObject(DEFAULT_GUI_FONT));
        TEXTMETRICW metrics = {};
        if (GetTextMetricsW(dc, &metrics))
            line_height = max(1, metrics.tmHeight + metrics.tmExternalLeading);
        SelectObject(dc, previous);
        ReleaseDC(g_runtime_log, dc);
    }
    const int visible_lines = max(1, (bounds.bottom - bounds.top) / line_height);
    const int line_count = (int)SendMessageW(g_runtime_log, EM_GETLINECOUNT, 0, 0);
    ShowScrollBar(g_runtime_log, SB_VERT, line_count > visible_lines);
}

static void append_runtime_message(const wchar_t* value) {
    if (!g_runtime_log || !value || !value[0]) return;
    SYSTEMTIME now = {};
    GetLocalTime(&now);
    wchar_t line[640] = {};
    _snwprintf_s(line, _countof(line), _TRUNCATE, L"[%02u:%02u:%02u] %s\r\n",
                 now.wHour, now.wMinute, now.wSecond, value);
    const int length = GetWindowTextLengthW(g_runtime_log);
    if (length > 12000) SetWindowTextW(g_runtime_log, L"");
    SendMessageW(g_runtime_log, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
    SendMessageW(g_runtime_log, EM_REPLACESEL, FALSE, (LPARAM)line);
    SendMessageW(g_runtime_log, EM_SCROLLCARET, 0, 0);
    update_runtime_log_scrollbar();
}

#ifdef IE_DLTB_BRIDGE
static void bridge_capture_status(const wchar_t* value);
#endif
static void set_status(const wchar_t* value) {
#ifdef IE_DLTB_BRIDGE
    bridge_capture_status(value);
#endif
    if (!g_status) return;
    wchar_t module_path[MAX_PATH] = {};
    GetModuleFileNameW(NULL, module_path, _countof(module_path));
    const wchar_t* process_name = wcsrchr(module_path, L'\\');
    process_name = process_name ? process_name + 1 : module_path;
    wchar_t compact_value[104] = {};
    const wchar_t* source_value = value ? value : L"Ready";
    wcsncpy_s(compact_value, _countof(compact_value), source_value, _TRUNCATE);
    if (wcslen(source_value) >= _countof(compact_value)) {
        compact_value[_countof(compact_value) - 4] = L'.';
        compact_value[_countof(compact_value) - 3] = L'.';
        compact_value[_countof(compact_value) - 2] = L'.';
    }
    wchar_t build[32] = L"Pending";
    HMODULE game = GetModuleHandleW(L"gamedll_ph_x64_rwdi.dll");
    if (game && is_readable_range(game, sizeof(IMAGE_DOS_HEADER))) {
        __try {
            BYTE* base = (BYTE*)game;
            IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
            if (dos->e_magic == IMAGE_DOS_SIGNATURE) {
                IMAGE_NT_HEADERS64* nt = (IMAGE_NT_HEADERS64*)(base + dos->e_lfanew);
                if (nt->Signature == IMAGE_NT_SIGNATURE)
                    _snwprintf_s(build, _countof(build), _TRUNCATE,
                                 L"0x%08X", nt->FileHeader.TimeDateStamp);
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
    wchar_t display[680] = {};
    _snwprintf_s(display, _countof(display), _TRUNCATE,
                 L"Status: %s   |   Process: %s   |   PID: %lu   |   Build: %s",
                 compact_value, process_name, GetCurrentProcessId(), build);
    SetWindowTextW(g_status, display);
    append_runtime_message(value ? value : L"Ready");
}

static COLORREF rarity_color(const char* rarity) {
    if (!_stricmp(rarity, "Uncommon")) return RGB(92, 201, 111);
    if (!_stricmp(rarity, "Rare")) return RGB(79, 155, 235);
    if (!_stricmp(rarity, "Epic")) return RGB(178, 104, 226);
    if (!_stricmp(rarity, "Legendary")) return RGB(241, 150, 60);
    if (!_stricmp(rarity, "Exotic")) return RGB(232, 87, 99);
    if (!_stricmp(rarity, "Platinum")) return RGB(121, 218, 213);
    if (!_stricmp(rarity, "Limited")) return RGB(241, 204, 91);
    if (!_stricmp(rarity, "Common")) return RGB(210, 214, 221);
    return kColorMuted;
}

static int ui_scale(int value) {
    return MulDiv(value, (int)g_ui_dpi, 96);
}

static UINT query_window_dpi(HWND hwnd) {
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    typedef UINT (WINAPI *GetDpiForWindowFn)(HWND);
    GetDpiForWindowFn get_dpi_for_window = user32
        ? (GetDpiForWindowFn)GetProcAddress(user32, "GetDpiForWindow") : NULL;
    if (get_dpi_for_window && hwnd) {
        const UINT dpi = get_dpi_for_window(hwnd);
        if (dpi) return dpi;
    }
    HDC screen = GetDC(hwnd ? hwnd : NULL);
    const int dpi = screen ? GetDeviceCaps(screen, LOGPIXELSX) : 96;
    if (screen) ReleaseDC(hwnd ? hwnd : NULL, screen);
    return dpi > 0 ? (UINT)dpi : 96u;
}

static void destroy_ui_fonts() {
    if (g_input_font) { DeleteObject(g_input_font); g_input_font = NULL; }
    if (g_font) { DeleteObject(g_font); g_font = NULL; }
    if (g_header_font) { DeleteObject(g_header_font); g_header_font = NULL; }
    if (g_small_font) { DeleteObject(g_small_font); g_small_font = NULL; }
    if (g_action_font) { DeleteObject(g_action_font); g_action_font = NULL; }
    if (g_mono_font) { DeleteObject(g_mono_font); g_mono_font = NULL; }
    if (g_section_font) { DeleteObject(g_section_font); g_section_font = NULL; }
    if (g_brand_font) { DeleteObject(g_brand_font); g_brand_font = NULL; }
    if (g_nav_font) { DeleteObject(g_nav_font); g_nav_font = NULL; }
    if (g_tiny_font) { DeleteObject(g_tiny_font); g_tiny_font = NULL; }
}

static void apply_ui_fonts() {
    if (!g_window) return;
    const HWND controls[] = {
        g_search, g_category_filter, g_rarity_filter, g_view_filter, g_weapon_level_filter,
        g_item_list, g_selection, g_quantity, g_status, g_header, g_subtitle, g_result_count,
        g_version, g_search_label, g_category_label, g_rarity_label, g_library_label,
        g_weapon_level_label, g_selected_label, g_quantity_label, g_actions_label,
        g_give_inventory, g_give_stash, g_remove_inventory, g_favorite, g_copy_id,
        g_quantity_minus, g_quantity_plus, g_section_title, g_close_button,
        g_minimize_button, g_maximize_button, g_return_game, g_catalog_title,
        g_catalog_subtitle, g_reset_filters, g_tab_all_items, g_tab_favorites,
#ifdef IE_OWNED_AFFIX_EDITOR
        g_tab_owned_weapons, g_tab_owned_gear,
        g_owned_affix_list, g_owned_value_input, g_owned_apply_value,
        g_owned_remove_affix, g_owned_donor_combo, g_owned_add_affix,
        g_owned_clear_affixes, g_owned_duplicate_weapon, g_owned_repair_weapon,
        g_owned_restore, g_owned_keep, g_owned_catalog_test,
        g_owned_catalog_value_input, g_owned_catalog_search,
        g_owned_value_label, g_owned_donor_label,
        g_owned_affix_detail,
        g_owned_filter_label, g_owned_format_filter, g_owned_presence_filter,
#endif
        g_runtime_log, g_log_filter,
        g_clear_log, g_console_title
    };
    for (HWND control : controls) {
        if (!control) continue;
        HFONT font = g_font;
        if (control == g_header) font = g_header_font;
        else if (control == g_section_title || control == g_catalog_title || control == g_console_title) font = g_section_font;
        else if (control == g_subtitle || control == g_result_count || control == g_version ||
                 control == g_search_label || control == g_category_label ||
                 control == g_rarity_label || control == g_library_label ||
                 control == g_weapon_level_label || control == g_selected_label ||
                 control == g_quantity_label || control == g_actions_label || control == g_status) font = g_small_font;
        else if (control == g_runtime_log) font = g_mono_font;
        else if (control == g_give_inventory || control == g_give_stash ||
                 control == g_remove_inventory || control == g_favorite || control == g_copy_id ||
                 control == g_quantity_minus || control == g_quantity_plus || control == g_close_button ||
                 control == g_minimize_button || control == g_maximize_button ||
                 control == g_return_game || control == g_reset_filters || control == g_clear_log) font = g_action_font;
        if (control == g_tab_all_items || control == g_tab_favorites
#ifdef IE_OWNED_AFFIX_EDITOR
            || control == g_tab_owned_weapons || control == g_tab_owned_gear
#endif
            ) font = g_action_font;
#ifdef IE_OWNED_AFFIX_EDITOR
        if (control == g_owned_value_input || control == g_owned_catalog_value_input ||
            control == g_owned_catalog_search) font = g_input_font;
        if (control == g_owned_value_label || control == g_owned_donor_label || control == g_owned_filter_label)
            font = g_small_font;
        if (control == g_owned_apply_value || control == g_owned_add_affix ||
            control == g_owned_remove_affix || control == g_owned_clear_affixes ||
            control == g_owned_duplicate_weapon || control == g_owned_repair_weapon ||
            control == g_owned_restore || control == g_owned_keep || control == g_owned_catalog_test)
            font = g_action_font;
#endif
        SendMessageW(control, WM_SETFONT, (WPARAM)font, TRUE);
    }
    if (g_item_list) {
        HWND header = ListView_GetHeader(g_item_list);
        if (header) SendMessageW(header, WM_SETFONT, (WPARAM)g_small_font, TRUE);
    }
}

static void rebuild_ui_fonts() {
    destroy_ui_fonts();
    g_input_font = CreateFontW(-ui_scale(15), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_font = CreateFontW(-ui_scale(13), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                         CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_header_font = CreateFontW(-ui_scale(24), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_section_font = CreateFontW(-ui_scale(14), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_small_font = CreateFontW(-ui_scale(11), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_action_font = CreateFontW(-ui_scale(12), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_mono_font = CreateFontW(-ui_scale(11), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_MODERN, L"Consolas");
    g_brand_font = CreateFontW(-ui_scale(17), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_nav_font = CreateFontW(-ui_scale(12), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_tiny_font = CreateFontW(-ui_scale(10), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    apply_ui_fonts();
}

static void rebuild_row_height_image_list() {
    if (!g_item_list) return;
    HIMAGELIST previous = g_row_height_images;
    g_row_height_images = ImageList_Create(1, ui_scale(kUiTableRowHeight), ILC_COLOR32, 1, 1);
    if (g_row_height_images) ListView_SetImageList(g_item_list, g_row_height_images, LVSIL_SMALL);
    if (previous) ImageList_Destroy(previous);
#ifdef IE_OWNED_AFFIX_EDITOR
    if (g_owned_affix_list) {
        previous = g_owned_row_images;
        g_owned_row_images = ImageList_Create(1,ui_scale(28),ILC_COLOR32,1,1);
        if (g_owned_row_images) ListView_SetImageList(g_owned_affix_list,g_owned_row_images,LVSIL_SMALL);
        if (previous) ImageList_Destroy(previous);
    }
#endif
}

static HWND create_dark_button(HWND parent, const wchar_t* text_value, int x, int y,
                               int width, int height, int id) {
    return CreateWindowW(L"BUTTON", text_value,
                         WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_NOTIFY,
                         x, y, width, height, parent, (HMENU)(INT_PTR)id, g_module, NULL);
}

static COLORREF blend_color(COLORREF from, COLORREF to, int amount);
static void fill_round_rect(HDC dc, const RECT& rect, COLORREF fill, COLORREF border,
                            int radius, int border_width);

static void paint_dark_combo(HWND combo, HDC dc) {
    RECT rect = {};
    GetClientRect(combo, &rect);
    // Combo boxes keep their own opaque child surface. Paint the rounded
    // corners explicitly so the stock light control background cannot leak.
    HBRUSH surround = CreateSolidBrush(combo == g_log_filter ? kColorConsole : kColorCard);
    FillRect(dc, &rect, surround);
    DeleteObject(surround);
    const bool enabled = IsWindowEnabled(combo) != FALSE;
    const bool focused = GetFocus() == combo;
    const bool hot = GetPropW(combo, L"BSS.ComboHot") != NULL;
    COLORREF fill = kColorInput;
    if (hot && enabled) fill = blend_color(fill, RGB(255, 255, 255), 4);
    const COLORREF border = focused ? kColorAccent : (hot ? kColorBorderHot : kColorBorder);
    fill_round_rect(dc, rect, fill, border, ui_scale(7), 1);

    wchar_t value[512] = {};
    const LRESULT selected = SendMessageW(combo, CB_GETCURSEL, 0, 0);
    if (selected != CB_ERR && SendMessageW(combo, CB_GETLBTEXTLEN, selected, 0) < (LRESULT)_countof(value))
        SendMessageW(combo, CB_GETLBTEXT, (WPARAM)selected, (LPARAM)value);
#ifdef IE_OWNED_AFFIX_EDITOR
    if (combo == g_owned_donor_combo) {
        if (selected == CB_ERR) wcscpy_s(value,L"No matching affixes");
        wchar_t* separator = wcsstr(value, L"  |  ");
        if (separator) *separator = 0; // Full ID stays in the two-line popup.
    }
#endif

    const int arrow_width = ui_scale(28);
    RECT text_rect = { rect.left + ui_scale(12), rect.top,
                       rect.right - arrow_width - ui_scale(4), rect.bottom };
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, enabled ? kColorText : kColorFaint);
    HGDIOBJ old_font = SelectObject(dc, g_font ? g_font : GetStockObject(DEFAULT_GUI_FONT));
    DrawTextW(dc, value, -1, &text_rect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    SelectObject(dc, old_font);

    const int cx = rect.right - arrow_width / 2;
    const int cy = (rect.top + rect.bottom) / 2;
    HPEN arrow = CreatePen(PS_SOLID, max(1, ui_scale(1)), enabled ? kColorMuted : kColorFaint);
    HGDIOBJ old_pen = SelectObject(dc, arrow);
    MoveToEx(dc, cx - ui_scale(4), cy - ui_scale(2), NULL);
    LineTo(dc, cx, cy + ui_scale(2));
    LineTo(dc, cx + ui_scale(4), cy - ui_scale(2));
    SelectObject(dc, old_pen);
    DeleteObject(arrow);
}

static LRESULT CALLBACK dark_combo_proc(HWND combo, UINT message, WPARAM wparam, LPARAM lparam,
                                        UINT_PTR, DWORD_PTR) {
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT paint = {};
        HDC dc = BeginPaint(combo, &paint);
        paint_dark_combo(combo, dc);
        EndPaint(combo, &paint);
        return 0;
    }
    case WM_PRINTCLIENT:
        paint_dark_combo(combo, (HDC)wparam);
        return 0;
    case WM_MOUSEMOVE: {
        if (!GetPropW(combo, L"BSS.ComboHot")) {
            SetPropW(combo, L"BSS.ComboHot", (HANDLE)1);
            TRACKMOUSEEVENT tracking = { sizeof(tracking), TME_LEAVE, combo, 0 };
            TrackMouseEvent(&tracking);
            InvalidateRect(combo, NULL, FALSE);
        }
        break;
    }
    case WM_MOUSELEAVE:
        RemovePropW(combo, L"BSS.ComboHot");
        InvalidateRect(combo, NULL, FALSE);
        return 0;
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    case WM_ENABLE:
    case WM_LBUTTONUP: {
        const LRESULT result = DefSubclassProc(combo, message, wparam, lparam);
        InvalidateRect(combo, NULL, FALSE);
        return result;
    }
    case CB_SETCURSEL: {
        const LRESULT result = DefSubclassProc(combo, message, wparam, lparam);
        InvalidateRect(combo, NULL, FALSE);
        return result;
    }
    case WM_NCDESTROY:
        RemovePropW(combo, L"BSS.ComboHot");
        RemoveWindowSubclass(combo, dark_combo_proc, 1);
        break;
    }
    return DefSubclassProc(combo, message, wparam, lparam);
}

static void style_dark_combo(HWND combo) {
    if (!combo) return;
    SetWindowTheme(combo, L"", L"");
    SendMessageW(combo, CB_SETITEMHEIGHT, (WPARAM)-1, ui_scale(33));
    SendMessageW(combo, CB_SETITEMHEIGHT, 0, ui_scale(27));
    SendMessageW(combo, CB_SETMINVISIBLE, 8, 0);
    SetWindowSubclass(combo, dark_combo_proc, 1, 0);
}

static void paint_catalog_header(HWND header, HDC dc) {
    RECT bounds = {};
    GetClientRect(header, &bounds);
    HBRUSH background = CreateSolidBrush(RGB(20, 31, 36));
    FillRect(dc, &bounds, background);
    DeleteObject(background);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(176, 195, 200));
    HGDIOBJ old_font = SelectObject(dc, g_small_font ? g_small_font : GetStockObject(DEFAULT_GUI_FONT));
    const int count = Header_GetItemCount(header);
    for (int column = 0; column < count; ++column) {
        RECT cell = {};
        if (!Header_GetItemRect(header, column, &cell)) continue;
        wchar_t label[64] = {};
        HDITEMW item = {};
        item.mask = HDI_TEXT;
        item.pszText = label;
        item.cchTextMax = (int)_countof(label);
        SendMessageW(header, HDM_GETITEMW, column, reinterpret_cast<LPARAM>(&item));
        RECT text_rect = { cell.left + ui_scale(10), cell.top,
                           cell.right - ui_scale(8), cell.bottom };
        DrawTextW(dc, label, -1, &text_rect,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
        if (column + 1 < count) {
            RECT divider = { cell.right - 1, cell.top + ui_scale(6),
                             cell.right, cell.bottom - ui_scale(6) };
            HBRUSH line = CreateSolidBrush(kColorBorder);
            FillRect(dc, &divider, line);
            DeleteObject(line);
        }
    }
    SelectObject(dc, old_font);
    RECT bottom = { bounds.left, bounds.bottom - 1, bounds.right, bounds.bottom };
    HBRUSH line = CreateSolidBrush(kColorBorder);
    FillRect(dc, &bottom, line);
    DeleteObject(line);
}

static LRESULT CALLBACK catalog_header_proc(HWND header, UINT message, WPARAM wparam,
                                            LPARAM lparam, UINT_PTR, DWORD_PTR) {
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT paint = {};
        HDC dc = BeginPaint(header, &paint);
        paint_catalog_header(header, dc);
        EndPaint(header, &paint);
        return 0;
    }
    case WM_PRINTCLIENT:
        paint_catalog_header(header, (HDC)wparam);
        return 0;
    case WM_NCDESTROY:
        RemoveWindowSubclass(header, catalog_header_proc, 1);
        break;
    }
    return DefSubclassProc(header, message, wparam, lparam);
}

static void add_control_tooltip(HWND control, const wchar_t* text_value) {
    if (!g_tooltip || !control || !text_value) return;
    TOOLINFOW tool = { sizeof(tool) };
    tool.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    tool.hwnd = g_window;
    tool.uId = (UINT_PTR)control;
    tool.lpszText = (LPWSTR)text_value;
    SendMessageW(g_tooltip, TTM_ADDTOOLW, 0, (LPARAM)&tool);
}

static COLORREF blend_color(COLORREF from, COLORREF to, int amount) {
    const int inverse = 100 - amount;
    return RGB((GetRValue(from) * inverse + GetRValue(to) * amount) / 100,
               (GetGValue(from) * inverse + GetGValue(to) * amount) / 100,
               (GetBValue(from) * inverse + GetBValue(to) * amount) / 100);
}

static void fill_round_rect(HDC dc, const RECT& rect, COLORREF fill, COLORREF border,
                            int radius, int border_width = 1) {
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, max(1, border_width), border);
    HGDIOBJ old_brush = SelectObject(dc, brush);
    HGDIOBJ old_pen = SelectObject(dc, pen);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(dc, old_pen);
    SelectObject(dc, old_brush);
    DeleteObject(pen);
    DeleteObject(brush);
}

static void draw_button_icon(HDC dc, int id, const RECT& bounds, COLORREF color) {
    const int cx = (bounds.left + bounds.right) / 2;
    const int cy = (bounds.top + bounds.bottom) / 2;
    const int s = ui_scale(7);
    HPEN pen = CreatePen(PS_SOLID, max(1, ui_scale(1)), color);
    HGDIOBJ old_pen = SelectObject(dc, pen);
    HGDIOBJ old_brush = SelectObject(dc, GetStockObject(NULL_BRUSH));

    if (id == IDC_CLOSE_BUTTON) {
        MoveToEx(dc, cx - s, cy - s, NULL); LineTo(dc, cx + s, cy + s);
        MoveToEx(dc, cx + s, cy - s, NULL); LineTo(dc, cx - s, cy + s);
    } else if (id == IDC_MINIMIZE_BUTTON) {
        MoveToEx(dc, cx - s, cy + ui_scale(3), NULL);
        LineTo(dc, cx + s, cy + ui_scale(3));
    } else if (id == IDC_MAXIMIZE_BUTTON) {
        if (g_window && IsZoomed(g_window)) {
            Rectangle(dc, cx - ui_scale(4), cy - s,
                      cx + s, cy + ui_scale(4));
            Rectangle(dc, cx - s, cy - ui_scale(4),
                      cx + ui_scale(4), cy + s);
        } else {
            Rectangle(dc, cx - s, cy - s, cx + s, cy + s);
        }
    } else if (id == IDC_QUANTITY_MINUS || id == IDC_QUANTITY_PLUS) {
        MoveToEx(dc, cx - s, cy, NULL); LineTo(dc, cx + s, cy);
        if (id == IDC_QUANTITY_PLUS) {
            MoveToEx(dc, cx, cy - s, NULL); LineTo(dc, cx, cy + s);
        }
    } else if (id == IDC_REFRESH_CATALOG || id == IDC_RESET_FILTERS) {
        RECT arc = { cx - s, cy - s, cx + s, cy + s };
        Arc(dc, arc.left, arc.top, arc.right, arc.bottom,
            cx + s, cy - ui_scale(1), cx - ui_scale(2), cy - s);
        MoveToEx(dc, cx + s, cy - ui_scale(5), NULL);
        LineTo(dc, cx + s, cy + ui_scale(1));
        LineTo(dc, cx + ui_scale(2), cy - ui_scale(1));
    } else if (id == IDC_COPY_ID
#ifdef IE_OWNED_AFFIX_EDITOR
               || id == IDC_OWNED_DUPLICATE
#endif
    ) {
        Rectangle(dc, cx - s, cy - s + ui_scale(2), cx + ui_scale(4), cy + s);
        Rectangle(dc, cx - ui_scale(3), cy - s, cx + s, cy + ui_scale(4));
    } else if (id == IDC_FAVORITE || id == IDC_TAB_FAVORITES) {
        POINT star[10] = {
            { cx, cy - s }, { cx + ui_scale(2), cy - ui_scale(2) },
            { cx + s, cy - ui_scale(2) }, { cx + ui_scale(3), cy + ui_scale(2) },
            { cx + ui_scale(5), cy + s }, { cx, cy + ui_scale(4) },
            { cx - ui_scale(5), cy + s }, { cx - ui_scale(3), cy + ui_scale(2) },
            { cx - s, cy - ui_scale(2) }, { cx - ui_scale(2), cy - ui_scale(2) }
        };
        Polygon(dc, star, 10);
    } else if (id == IDC_GIVE_INVENTORY) {
        RoundRect(dc, cx - s, cy - ui_scale(4), cx + s, cy + s,
                  ui_scale(4), ui_scale(4));
        Arc(dc, cx - ui_scale(4), cy - s, cx + ui_scale(4), cy,
            cx - ui_scale(4), cy - ui_scale(3), cx + ui_scale(4), cy - ui_scale(3));
        MoveToEx(dc, cx, cy - ui_scale(1), NULL); LineTo(dc, cx, cy + ui_scale(5));
        MoveToEx(dc, cx - ui_scale(3), cy + ui_scale(2), NULL);
        LineTo(dc, cx + ui_scale(3), cy + ui_scale(2));
    } else if (id == IDC_GIVE_STASH) {
        Rectangle(dc, cx - s, cy - ui_scale(4), cx + s, cy + s);
        MoveToEx(dc, cx - s, cy - ui_scale(1), NULL); LineTo(dc, cx + s, cy - ui_scale(1));
        MoveToEx(dc, cx, cy + ui_scale(1), NULL); LineTo(dc, cx, cy + ui_scale(5));
        MoveToEx(dc, cx - ui_scale(3), cy + ui_scale(3), NULL);
        LineTo(dc, cx + ui_scale(3), cy + ui_scale(3));
    } else if (id == IDC_REMOVE_INVENTORY || id == IDC_CLEAR_UI_LOG) {
        Rectangle(dc, cx - ui_scale(5), cy - ui_scale(3), cx + ui_scale(5), cy + s);
        MoveToEx(dc, cx - s, cy - ui_scale(6), NULL); LineTo(dc, cx + s, cy - ui_scale(6));
        MoveToEx(dc, cx - ui_scale(3), cy - s, NULL); LineTo(dc, cx + ui_scale(3), cy - s);
        MoveToEx(dc, cx - ui_scale(2), cy, NULL); LineTo(dc, cx - ui_scale(2), cy + ui_scale(5));
        MoveToEx(dc, cx + ui_scale(2), cy, NULL); LineTo(dc, cx + ui_scale(2), cy + ui_scale(5));
    } else if (id == IDC_RETURN_GAME) {
        Rectangle(dc, cx - ui_scale(2), cy - s, cx + s, cy + s);
        MoveToEx(dc, cx + ui_scale(2), cy, NULL); LineTo(dc, cx - s, cy);
        MoveToEx(dc, cx - s, cy, NULL); LineTo(dc, cx - ui_scale(3), cy - ui_scale(4));
        MoveToEx(dc, cx - s, cy, NULL); LineTo(dc, cx - ui_scale(3), cy + ui_scale(4));
    }

    SelectObject(dc, old_brush);
    SelectObject(dc, old_pen);
    DeleteObject(pen);
}

static bool button_has_icon(int id) {
    switch (id) {
    case IDC_CLOSE_BUTTON:
    case IDC_MINIMIZE_BUTTON:
    case IDC_MAXIMIZE_BUTTON:
    case IDC_QUANTITY_MINUS:
    case IDC_QUANTITY_PLUS:
    case IDC_REFRESH_CATALOG:
    case IDC_CLEAR_UI_LOG:
    case IDC_COPY_ID:
#ifdef IE_OWNED_AFFIX_EDITOR
    case IDC_OWNED_DUPLICATE:
#endif
    case IDC_FAVORITE:
    case IDC_TAB_FAVORITES:
    case IDC_GIVE_INVENTORY:
    case IDC_GIVE_STASH:
    case IDC_REMOVE_INVENTORY:
    case IDC_RETURN_GAME:
        return true;
    default:
        return false;
    }
}

static void draw_dark_button(const DRAWITEMSTRUCT* draw) {
    if (!draw || draw->CtlType != ODT_BUTTON) return;
    RECT rect = draw->rcItem;
    const bool pressed = (draw->itemState & ODS_SELECTED) != 0;
    const bool disabled = (draw->itemState & ODS_DISABLED) != 0;
    const bool focused = (draw->itemState & ODS_FOCUS) != 0;
    const bool hot = draw->hwndItem == g_hot_control;
    const bool primary = draw->CtlID == IDC_GIVE_INVENTORY || draw->CtlID == IDC_GIVE_STASH;
    const bool destructive = draw->CtlID == IDC_REMOVE_INVENTORY;
    const bool close = draw->CtlID == IDC_CLOSE_BUTTON;
    const bool step = draw->CtlID == IDC_QUANTITY_MINUS || draw->CtlID == IDC_QUANTITY_PLUS;
    const bool tab = draw->CtlID == IDC_TAB_ALL_ITEMS || draw->CtlID == IDC_TAB_FAVORITES
#ifdef IE_OWNED_AFFIX_EDITOR
        || draw->CtlID == IDC_TAB_OWNED_WEAPONS || draw->CtlID == IDC_TAB_OWNED_GEAR
#endif
        ;
    const int view = g_view_filter ? (int)SendMessageW(g_view_filter, CB_GETCURSEL, 0, 0) : 0;
    const bool active_tab = (draw->CtlID == IDC_TAB_ALL_ITEMS && view == 0
#ifdef IE_OWNED_AFFIX_EDITOR
                             && !g_owned_mode
#endif
                            ) ||
                            (draw->CtlID == IDC_TAB_FAVORITES && view == 1
#ifdef IE_OWNED_AFFIX_EDITOR
                             && !g_owned_mode
#endif
                            )
#ifdef IE_OWNED_AFFIX_EDITOR
                            || (draw->CtlID == IDC_TAB_OWNED_WEAPONS && g_owned_mode && !g_owned_gear_mode)
                            || (draw->CtlID == IDC_TAB_OWNED_GEAR && g_owned_mode && g_owned_gear_mode)
#endif
                            ;
    const bool has_icon = button_has_icon(draw->CtlID);
    bool icon_only = close || step;

    COLORREF fill = kColorInput;
    COLORREF border = kColorBorder;
    COLORREF text_color = kColorText;
    if (primary) {
        fill = RGB(36, 128, 60);
        border = fill;
        text_color = RGB(245, 255, 247);
    } else if (destructive) {
        fill = RGB(35, 21, 24);
        border = RGB(79, 39, 45);
        text_color = kColorRemove;
    } else if (close) {
        fill = kColorHeader;
        border = kColorHeader;
        text_color = kColorMuted;
    } else if (tab) {
        fill = active_tab ? RGB(23, 41, 38) : kColorPanel;
        border = active_tab ? RGB(56, 115, 91) : kColorBorder;
        text_color = active_tab ? RGB(222, 250, 230) : kColorMuted;
    }

    if (hot && !disabled) {
        if (primary) fill = RGB(45, 151, 72);
        else if (destructive) { fill = RGB(48, 25, 29); border = RGB(113, 49, 57); }
        else if (close) { fill = RGB(112, 40, 45); border = fill; text_color = RGB(255, 240, 242); }
        else { fill = blend_color(fill, RGB(255, 255, 255), 5); border = kColorBorderHot; }
    }
    if (pressed && !disabled) fill = blend_color(fill, RGB(0, 0, 0), 18);
    if (disabled) { fill = RGB(24, 27, 31); border = RGB(31, 35, 40); text_color = kColorFaint; }
    else if (focused && !primary && !close) border = kColorAccentDim;

    const COLORREF surround_color = close ? kColorHeader :
        tab ? kColorPanel :
        draw->CtlID == IDC_REFRESH_CATALOG ? kColorPanel :
        draw->CtlID == IDC_CLEAR_UI_LOG ? kColorConsole : kColorCard;
    HBRUSH surround = CreateSolidBrush(surround_color);
    FillRect(draw->hDC, &rect, surround);
    DeleteObject(surround);
    fill_round_rect(draw->hDC, rect, fill, border, ui_scale(close ? 8 : 7), 1);
    if (active_tab) {
        RECT underline = { rect.left + ui_scale(9), rect.bottom - ui_scale(2),
                           rect.right - ui_scale(9), rect.bottom };
        HBRUSH accent = CreateSolidBrush(kColorAccent);
        FillRect(draw->hDC, &underline, accent);
        DeleteObject(accent);
    }

    wchar_t text_value[128] = {};
    GetWindowTextW(draw->hwndItem, text_value, _countof(text_value));
    if (!text_value[0]) icon_only = true;
    if ((draw->CtlID == IDC_FAVORITE || draw->CtlID == IDC_COPY_ID) &&
        rect.right - rect.left < ui_scale(82)) icon_only = true;

    SetBkMode(draw->hDC, TRANSPARENT);
    SetTextColor(draw->hDC, text_color);
    HGDIOBJ old_font = SelectObject(draw->hDC, g_action_font ? g_action_font : GetStockObject(DEFAULT_GUI_FONT));
    if (icon_only) {
        draw_button_icon(draw->hDC, draw->CtlID, rect, text_color);
    } else if (has_icon) {
        const int icon_width = ui_scale(17);
        SIZE text_size = {};
        GetTextExtentPoint32W(draw->hDC, text_value, (int)wcslen(text_value), &text_size);
        const int content_width = max(0, rect.right - rect.left - ui_scale(20));
        const int group_width = min(content_width, icon_width + ui_scale(7) + text_size.cx);
        const int group_x = rect.left + max(ui_scale(10), (rect.right - rect.left - group_width) / 2);
        RECT icon = { group_x, rect.top, group_x + icon_width, rect.bottom };
        draw_button_icon(draw->hDC, draw->CtlID, icon, text_color);
        RECT label = rect;
        label.left = icon.right + ui_scale(7);
        label.right -= ui_scale(10);
        DrawTextW(draw->hDC, text_value, -1, &label,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    } else {
        rect.left += ui_scale(10);
        rect.right -= ui_scale(10);
        DrawTextW(draw->hDC, text_value, -1, &rect,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }
    SelectObject(draw->hDC, old_font);
}

static void draw_beast_emblem(HDC dc, const RECT& bounds, bool framed) {
    if (framed)
        fill_round_rect(dc, bounds, RGB(8, 25, 21), RGB(36, 91, 57), ui_scale(5));
    const int w = bounds.right - bounds.left;
    const int h = bounds.bottom - bounds.top;
    const int x = bounds.left;
    const int y = bounds.top;
    POINT head[14] = {
        { x + w * 8 / 100, y + h * 8 / 100 },
        { x + w * 33 / 100, y + h * 22 / 100 },
        { x + w * 43 / 100, y + h * 18 / 100 },
        { x + w * 50 / 100, y + h * 10 / 100 },
        { x + w * 57 / 100, y + h * 18 / 100 },
        { x + w * 67 / 100, y + h * 22 / 100 },
        { x + w * 92 / 100, y + h * 8 / 100 },
        { x + w * 84 / 100, y + h * 56 / 100 },
        { x + w * 70 / 100, y + h * 74 / 100 },
        { x + w * 62 / 100, y + h * 80 / 100 },
        { x + w * 50 / 100, y + h * 95 / 100 },
        { x + w * 38 / 100, y + h * 80 / 100 },
        { x + w * 30 / 100, y + h * 74 / 100 },
        { x + w * 16 / 100, y + h * 56 / 100 }
    };
    HBRUSH face = CreateSolidBrush(RGB(15, 54, 34));
    HPEN outline = CreatePen(PS_SOLID, max(1, w / 24), kColorAccent);
    HGDIOBJ old_brush = SelectObject(dc, face);
    HGDIOBJ old_pen = SelectObject(dc, outline);
    Polygon(dc, head, (int)_countof(head));

    HBRUSH accent = CreateSolidBrush(kColorAccent);
    SelectObject(dc, accent);
    POINT left_eye[3] = {
        { x + w * 25 / 100, y + h * 45 / 100 },
        { x + w * 45 / 100, y + h * 49 / 100 },
        { x + w * 35 / 100, y + h * 59 / 100 }
    };
    POINT right_eye[3] = {
        { x + w * 75 / 100, y + h * 45 / 100 },
        { x + w * 55 / 100, y + h * 49 / 100 },
        { x + w * 65 / 100, y + h * 59 / 100 }
    };
    Polygon(dc, left_eye, 3);
    Polygon(dc, right_eye, 3);
    POINT nose[4] = {
        { x + w * 50 / 100, y + h * 60 / 100 },
        { x + w * 59 / 100, y + h * 68 / 100 },
        { x + w * 50 / 100, y + h * 76 / 100 },
        { x + w * 41 / 100, y + h * 68 / 100 }
    };
    Polygon(dc, nose, 4);
    MoveToEx(dc, x + w * 50 / 100, y + h * 76 / 100, NULL);
    LineTo(dc, x + w * 50 / 100, y + h * 86 / 100);
    MoveToEx(dc, x + w * 38 / 100, y + h * 82 / 100, NULL);
    LineTo(dc, x + w * 50 / 100, y + h * 88 / 100);
    LineTo(dc, x + w * 62 / 100, y + h * 82 / 100);

    SelectObject(dc, old_pen);
    SelectObject(dc, old_brush);
    DeleteObject(accent);
    DeleteObject(outline);
    DeleteObject(face);
}

static void draw_brand(const DRAWITEMSTRUCT* draw) {
    if (!draw || draw->CtlType != ODT_STATIC || draw->CtlID != IDC_BRAND) return;
    HBRUSH background = CreateSolidBrush(kColorHeader);
    FillRect(draw->hDC, &draw->rcItem, background);
    DeleteObject(background);
    if (g_brand_icon) {
        DrawIconEx(draw->hDC, draw->rcItem.left, draw->rcItem.top, g_brand_icon,
                   draw->rcItem.right - draw->rcItem.left,
                   draw->rcItem.bottom - draw->rcItem.top, 0, NULL, DI_NORMAL);
    } else {
        draw_beast_emblem(draw->hDC, draw->rcItem, false);
    }
}

static void draw_detail_panel(const DRAWITEMSTRUCT* draw) {
    if (!draw || draw->CtlType != ODT_STATIC || draw->CtlID != IDC_DETAIL_PANEL) return;
    RECT rect = draw->rcItem;
    fill_round_rect(draw->hDC, rect, kColorCard, kColorBorder, ui_scale(kUiPanelRadius));
    RECT rule = { rect.left + ui_scale(16), rect.top + ui_scale(47),
                  rect.right - ui_scale(16), rect.top + ui_scale(48) };
    HBRUSH brush = CreateSolidBrush(RGB(31, 36, 42));
    FillRect(draw->hDC, &rule, brush);
    DeleteObject(brush);
}

static void draw_inspector_text(HDC dc, const wchar_t* value, RECT rect,
                                HFONT font, COLORREF color) {
    HGDIOBJ previous = SelectObject(dc, font ? font : GetStockObject(DEFAULT_GUI_FONT));
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    DrawTextW(dc, value, -1, &rect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    SelectObject(dc, previous);
}

static void draw_selected_summary(const DRAWITEMSTRUCT* draw) {
    if (!draw || draw->CtlType != ODT_STATIC || draw->CtlID != IDC_SELECTED_SUMMARY) return;
    RECT rect = draw->rcItem;
    HBRUSH background = CreateSolidBrush(kColorCard);
    FillRect(draw->hDC, &rect, background);
    DeleteObject(background);
    const int saved = SaveDC(draw->hDC);
    IntersectClipRect(draw->hDC, rect.left, rect.top, rect.right, rect.bottom);
#ifdef IE_OWNED_AFFIX_EDITOR
    if (g_owned_mode) {
        const int selected = g_item_list
            ? ListView_GetNextItem(g_item_list, -1, LVNI_SELECTED) : -1;
        OwnedWeaponView item = {};
        bool valid = false;
        EnterCriticalSection(&g_data_lock);
        if (selected >= 0 && selected < (int)g_visible_count) {
            const int index = g_visible_items[selected];
            if (index >= 0 && index < (int)g_owned_count) {
                item = g_owned_weapons[index];
                valid = true;
            }
        }
        LeaveCriticalSection(&g_data_lock);
        if (!valid) {
            RECT title = { rect.left + ui_scale(8), rect.top + ui_scale(12),
                           rect.right - ui_scale(8), rect.top + ui_scale(42) };
            draw_inspector_text(draw->hDC, g_owned_gear_mode ? L"No owned gear selected" : L"No owned weapon selected", title,
                                g_section_font, kColorText);
            RestoreDC(draw->hDC, saved);
            return;
        }
        wchar_t name[180] = {}, id[180] = {}, line[260] = {};
        ascii_to_wide(item.name, name, (int)_countof(name));
        ascii_to_wide(item.id, id, (int)_countof(id));
        RECT title = { rect.left + ui_scale(8), rect.top + ui_scale(8),
                       rect.right - ui_scale(8), rect.top + ui_scale(38) };
        draw_inspector_text(draw->hDC, name, title, g_section_font, kColorText);
        RECT item_id = { title.left, title.bottom, title.right, title.bottom + ui_scale(24) };
        draw_inspector_text(draw->hDC, id, item_id, g_mono_font, kColorMuted);
        _snwprintf_s(line, _countof(line), _TRUNCATE,
                     L"%sCopy #%u  |  %u/%u native affixes  |  %u built-in",
                     item.is_gear ? (item.worn_slot ? L"Worn  |  " :
                         g_owned_equipment_known ? L"In bag  |  " : L"Status unknown  |  ") : L"",
                     item.same_id_ordinal, item.affix_count,
                     kOwnedAffixTestLimit,
                     owned_builtin_for_item(item.id, NULL, 0));
        RECT identity = { title.left, item_id.bottom + ui_scale(2), title.right,
                          item_id.bottom + ui_scale(25) };
        draw_inspector_text(draw->hDC, line, identity, g_small_font, kColorMuted);
        RestoreDC(draw->hDC, saved);
        return;
    }
#endif
    const int row = g_item_list ? ListView_GetNextItem(g_item_list, -1, LVNI_SELECTED) : -1;
    if (row < 0 || row >= (int)g_visible_count) {
        RECT title = { rect.left + ui_scale(8), rect.top + ui_scale(12),
                       rect.right - ui_scale(8), rect.top + ui_scale(42) };
        draw_inspector_text(draw->hDC, L"No item selected", title, g_section_font, kColorText);
        RECT help = { rect.left + ui_scale(8), title.bottom + ui_scale(4),
                      rect.right - ui_scale(8), title.bottom + ui_scale(32) };
        draw_inspector_text(draw->hDC, L"Choose an item in the catalog", help, g_small_font, kColorMuted);
        RestoreDC(draw->hDC, saved);
        return;
    }
    ItemDefinition item = {};
    EnterCriticalSection(&g_data_lock);
    item = g_items[g_visible_items[row]];
    LeaveCriticalSection(&g_data_lock);

    wchar_t name[180] = {}, id[180] = {}, category[80] = {}, rarity[48] = {};
    wchar_t type[80] = {}, source[260] = {}, key[180] = {}, level[24] = L"—";
    ascii_to_wide(item.name, name, (int)_countof(name));
    ascii_to_wide(item.id, id, (int)_countof(id));
    ascii_to_wide(item.category, category, (int)_countof(category));
    ascii_to_wide(item.rarity, rarity, (int)_countof(rarity));
    ascii_to_wide(item.item_type, type, (int)_countof(type));
    ascii_to_wide(item.source, source, (int)_countof(source));
    ascii_to_wide(item.localization_key, key, (int)_countof(key));
    if (item.weapon_level)
        _snwprintf_s(level, _countof(level), _TRUNCATE, L"R%lu", item.weapon_level);

    RECT title = { rect.left + ui_scale(8), rect.top + ui_scale(8),
                   rect.right - ui_scale(8), rect.top + ui_scale(38) };
    draw_inspector_text(draw->hDC, name, title, g_section_font, kColorText);
    HGDIOBJ previous_font = SelectObject(draw->hDC, g_small_font);
    SIZE rarity_size = {};
    GetTextExtentPoint32W(draw->hDC, rarity, lstrlenW(rarity), &rarity_size);
    SelectObject(draw->hDC, previous_font);
    const int badge_width = min(max(rarity_size.cx + ui_scale(24), ui_scale(68)), ui_scale(120));
    RECT badge = { title.left, title.bottom + ui_scale(5),
                   min(title.left + badge_width, rect.right - ui_scale(8)),
                   title.bottom + ui_scale(27) };
    const COLORREF color = rarity_color(item.rarity);
    fill_round_rect(draw->hDC, badge, blend_color(kColorCard, color, 12), color, ui_scale(4));
    previous_font = SelectObject(draw->hDC, g_small_font);
    SetBkMode(draw->hDC, TRANSPARENT);
    SetTextColor(draw->hDC, color);
    DrawTextW(draw->hDC, rarity, -1, &badge,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    SelectObject(draw->hDC, previous_font);
    RECT category_line = { badge.right + ui_scale(10), badge.top,
                           rect.right - ui_scale(8), badge.bottom };
    draw_inspector_text(draw->hDC, category, category_line, g_small_font, kColorMuted);
    RECT divider = { rect.left + ui_scale(8), badge.bottom + ui_scale(14),
                     rect.right - ui_scale(8), badge.bottom + ui_scale(15) };
    HBRUSH rule = CreateSolidBrush(kColorBorder);
    FillRect(draw->hDC, &divider, rule);
    DeleteObject(rule);

    const wchar_t* labels[] = { L"Item ID", L"Category", L"Rarity", L"Weapon level",
                                L"Item type", L"Can drop", L"Source", L"Localization" };
    const wchar_t* values[] = { id, category, rarity, level, type[0] ? type : L"—",
                                item.can_drop == 1 ? L"Yes" : item.can_drop == 2 ? L"No" : L"Unknown",
                                source[0] ? source : L"—", key[0] ? key : L"—" };
    const int start_y = divider.bottom + ui_scale(12);
    for (int index = 0; index < (int)_countof(labels); ++index) {
        const int top = start_y + ui_scale(index * 30);
        if (top + ui_scale(25) > rect.bottom) break;
        RECT label = { rect.left + ui_scale(8), top,
                       rect.left + ui_scale(105), top + ui_scale(25) };
        RECT value = { label.right + ui_scale(6), top,
                       rect.right - ui_scale(8), top + ui_scale(25) };
        draw_inspector_text(draw->hDC, labels[index], label, g_small_font, kColorMuted);
        draw_inspector_text(draw->hDC, values[index], value,
                            index == 0 ? g_mono_font : g_font,
                            index == 2 ? color : kColorText);
    }
    RestoreDC(draw->hDC, saved);
}

static void paint_catalog_row(HDC dc, int row) {
    if (!g_item_list || row < 0 || row >= (int)g_visible_count) return;
    RECT bounds = {}, client = {};
    if (!ListView_GetItemRect(g_item_list, row, &bounds, LVIR_BOUNDS)) return;
    GetClientRect(g_item_list, &client);
    const int saved = SaveDC(dc);
    IntersectClipRect(dc, client.left, client.top, client.right, client.bottom);

    const bool selected = (ListView_GetItemState(g_item_list, row, LVIS_SELECTED) & LVIS_SELECTED) != 0;
    const COLORREF base = selected ? RGB(24, 43, 49) :
                          ((row & 1) ? RGB(16, 19, 23) : kColorPanel);
    HBRUSH brush = CreateSolidBrush(base);
    FillRect(dc, &bounds, brush);
    DeleteObject(brush);
    if (selected) {
        RECT sheen = { bounds.left, bounds.top + 1, bounds.right,
                       bounds.top + (bounds.bottom - bounds.top) / 2 };
        brush = CreateSolidBrush(RGB(33, 59, 65));
        FillRect(dc, &sheen, brush);
        DeleteObject(brush);
        RECT top_edge = { bounds.left, bounds.top, bounds.right, bounds.top + 1 };
        brush = CreateSolidBrush(RGB(78, 151, 155));
        FillRect(dc, &top_edge, brush);
        DeleteObject(brush);
        RECT marker = { bounds.left, bounds.top + ui_scale(3),
                        bounds.left + ui_scale(3), bounds.bottom - ui_scale(3) };
        brush = CreateSolidBrush(RGB(111, 224, 213));
        FillRect(dc, &marker, brush);
        DeleteObject(brush);
    } else {
        RECT separator = { bounds.left + ui_scale(8), bounds.bottom - 1,
                           bounds.right - ui_scale(8), bounds.bottom };
        brush = CreateSolidBrush(RGB(29, 34, 40));
        FillRect(dc, &separator, brush);
        DeleteObject(brush);
    }

    ItemDefinition item = {};
#ifdef IE_OWNED_AFFIX_EDITOR
    bool gear_view = false, worn = false, equipment_known = false;
#endif
    EnterCriticalSection(&g_data_lock);
#ifdef IE_OWNED_AFFIX_EDITOR
    if (g_owned_mode) {
        const OwnedWeaponView* owned = &g_owned_weapons[g_visible_items[row]];
        gear_view = owned->is_gear;
        worn = owned->worn_slot != 0;
        equipment_known = g_owned_equipment_known;
        _snprintf_s(item.name, _countof(item.name), _TRUNCATE,
                    "%s  #%u", owned->name, owned->same_id_ordinal);
        strcpy_s(item.id, owned->id);
        strcpy_s(item.category, owned->category);
        strcpy_s(item.rarity, owned->rarity);
        item.weapon_level = owned->weapon_level;
        _snprintf_s(item.source, _countof(item.source), _TRUNCATE,
                    "%u affixes", owned->affix_count);
    } else
#endif
    item = g_items[g_visible_items[row]];
    LeaveCriticalSection(&g_data_lock);
    char level[16] = {};
    if (item.weapon_level)
        _snprintf_s(level, _countof(level), _TRUNCATE, "R%lu", item.weapon_level);
#ifdef IE_OWNED_AFFIX_EDITOR
    if (gear_view) strcpy_s(level, worn ? "Worn" : equipment_known ? "Bag" : "?");
#endif
    const char* source_name = strrchr(item.source, '/');
    source_name = source_name ? source_name + 1 : item.source;
    const char* fields[] = { item.name, item.id, item.category, item.rarity, level, source_name };
    SetBkMode(dc, TRANSPARENT);
    for (int column = 0; column < (int)_countof(fields); ++column) {
        RECT cell = {};
        if (!ListView_GetSubItemRect(g_item_list, row, column, LVIR_BOUNDS, &cell)) continue;
        if (column == 0) cell.right = cell.left + ListView_GetColumnWidth(g_item_list, 0);
        if (cell.right <= cell.left) continue;
        wchar_t value[260] = {};
        ascii_to_wide(fields[column], value, (int)_countof(value));
        if (column == 3) {
            const COLORREF color = rarity_color(item.rarity);
            RECT badge = { cell.left + ui_scale(7), cell.top + ui_scale(5),
                           min(cell.right - ui_scale(7), cell.left + ui_scale(82)),
                           cell.bottom - ui_scale(5) };
            if (badge.right > badge.left) {
                fill_round_rect(dc, badge, blend_color(base, color, 12), color, ui_scale(4));
                HGDIOBJ previous_font = SelectObject(dc, g_small_font);
                SetTextColor(dc, color);
                DrawTextW(dc, value, -1, &badge,
                          DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
                SelectObject(dc, previous_font);
            }
            continue;
        }
        RECT text_rect = { cell.left + ui_scale(7), cell.top,
                           cell.right - ui_scale(6), cell.bottom };
        if (column == 0 && item.favorite) {
            RECT star = { text_rect.left, cell.top,
                          text_rect.left + ui_scale(14), cell.bottom };
            draw_button_icon(dc, IDC_FAVORITE, star, RGB(238, 194, 85));
            text_rect.left = star.right + ui_scale(4);
        }
        HGDIOBJ previous_font = SelectObject(dc,
            selected && column == 0 ? g_action_font : g_font);
        SetTextColor(dc, selected ? RGB(243, 251, 251) :
                     (column == 1 || column == 4 ? kColorMuted : kColorText));
#ifdef IE_OWNED_AFFIX_EDITOR
        if (gear_view && worn && column == 4) SetTextColor(dc, kColorAccent);
#endif
        DrawTextW(dc, value, -1, &text_rect,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
        SelectObject(dc, previous_font);
    }
    RestoreDC(dc, saved);
}

#ifdef IE_OWNED_AFFIX_EDITOR
static void paint_owned_affix_row(HDC dc, int row) {
    RECT bounds = {}, client = {};
    if (!ListView_GetItemRect(g_owned_affix_list, row, &bounds, LVIR_BOUNDS)) return;
    GetClientRect(g_owned_affix_list, &client);
    const int saved = SaveDC(dc);
    IntersectClipRect(dc, client.left, client.top, client.right, client.bottom);
    const bool selected = (ListView_GetItemState(g_owned_affix_list, row, LVIS_SELECTED) & LVIS_SELECTED) != 0;
    HBRUSH brush = CreateSolidBrush(selected ? RGB(24,43,49) :
        (row & 1) ? RGB(16,19,23) : kColorPanel);
    FillRect(dc, &bounds, brush); DeleteObject(brush);
    RECT edge = { bounds.left + ui_scale(7), bounds.bottom-1,
                  bounds.right-ui_scale(7), bounds.bottom };
    brush = CreateSolidBrush(kColorBorder); FillRect(dc, &edge, brush); DeleteObject(brush);
    if (selected) {
        RECT marker = { bounds.left, bounds.top+ui_scale(3), bounds.left+ui_scale(3), bounds.bottom-ui_scale(3) };
        brush = CreateSolidBrush(RGB(111,224,213)); FillRect(dc, &marker, brush); DeleteObject(brush);
    }
    OwnedWeaponView item = {};
    const bool builtin = selected_owned_weapon(&item) && (unsigned)row >= item.affix_count;
    SetBkMode(dc, TRANSPARENT);
    for (int column=0; column<2; ++column) {
        RECT cell = {};
        ListView_GetSubItemRect(g_owned_affix_list, row, column, LVIR_BOUNDS, &cell);
        if (!column) cell.right=cell.left+ListView_GetColumnWidth(g_owned_affix_list,0);
        cell.left += ui_scale(8); cell.right -= ui_scale(8);
        wchar_t text[220] = {};
        ListView_GetItemText(g_owned_affix_list,row,column,text,_countof(text));
        SelectObject(dc, column ? g_input_font : g_font);
        SetTextColor(dc, builtin ? kColorMuted : selected ? RGB(243,251,251) : kColorText);
        DrawTextW(dc,text,-1,&cell,(column ? DT_RIGHT : DT_LEFT) |
            DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }
    RestoreDC(dc,saved);
}
#endif

static void draw_quantity_backdrop(const DRAWITEMSTRUCT* draw) {
    if (!draw) return;
    fill_round_rect(draw->hDC, draw->rcItem, kColorInput, kColorBorder,
                    ui_scale(5), 1);
}

static void draw_result_summary(const DRAWITEMSTRUCT* draw) {
    if (!draw || draw->CtlType != ODT_STATIC || draw->CtlID != IDC_RESULT_COUNT) return;
    RECT rect = draw->rcItem;
    HBRUSH background = CreateSolidBrush(kColorPanel);
    FillRect(draw->hDC, &rect, background);
    DeleteObject(background);

    wchar_t value[96] = {};
    GetWindowTextW(draw->hwndItem, value, _countof(value));
    wchar_t* second = wcsstr(value, L"\r\n");
    if (second) { *second = L'\0'; second += 2; }
    SetBkMode(draw->hDC, TRANSPARENT);
    HGDIOBJ old_font = SelectObject(draw->hDC, g_small_font ? g_small_font : GetStockObject(DEFAULT_GUI_FONT));
    RECT first_line = { rect.left, rect.top + ui_scale(2), rect.right, rect.top + ui_scale(19) };
    SetTextColor(draw->hDC, kColorText);
    DrawTextW(draw->hDC, value, -1, &first_line,
              DT_RIGHT | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    if (second && *second) {
        RECT second_line = { rect.left, rect.top + ui_scale(19), rect.right, rect.bottom };
        SetTextColor(draw->hDC, kColorMuted);
        DrawTextW(draw->hDC, second, -1, &second_line,
                  DT_RIGHT | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }
    SelectObject(draw->hDC, old_font);
}

static void open_filter_dropdown(HWND control) {
    if (!control) return;
    SetFocus(control);
    SendMessageW(control, CB_SHOWDROPDOWN, TRUE, 0);
}

struct CatalogueLayout {
    RECT client;
    RECT header_card;
    RECT filter_card;
    RECT list_card;
    RECT detail_card;
    RECT status_card;
    RECT console_card;
    int left_width;
    int detail_width;
    int gap;
};

static RECT make_rect(int left, int top, int right, int bottom) {
    RECT value = { left, top, right, bottom };
    return value;
}

static void compute_catalogue_layout(HWND window, CatalogueLayout* layout) {
    if (!layout) return;
    ZeroMemory(layout, sizeof(*layout));
    GetClientRect(window, &layout->client);
    const int width = layout->client.right;
    const int height = layout->client.bottom;
    const int margin = ui_scale(kUiOuterMargin);
    layout->gap = ui_scale(kUiPanelGap);

    layout->header_card = make_rect(0, 0, width, ui_scale(kUiHeaderHeight));

    int console_height = max(ui_scale(132), min(ui_scale(168), height * 16 / 100));
#ifdef IE_OWNED_AFFIX_EDITOR
    // Keep one scrollable affix row and every action usable at minimum height.
    // Compact the independent log instead of overlapping the inspector controls.
    if (g_owned_mode && height < ui_scale(860)) console_height = ui_scale(80);
#endif
    layout->console_card = make_rect(margin, height - margin - console_height,
                                     width - margin, height - margin);
    const int status_height = ui_scale(kUiStatusHeight);
    layout->status_card = make_rect(margin, layout->console_card.top - ui_scale(8) - status_height,
                                    width - margin, layout->console_card.top - ui_scale(8));

    const int content_top = layout->header_card.bottom + ui_scale(12);
    const int content_bottom = layout->status_card.top - layout->gap;
    const int content_width = width - margin * 2;
    layout->left_width = max(ui_scale(228), min(ui_scale(258), content_width * 19 / 100));
    layout->detail_width = max(ui_scale(332), min(ui_scale(382), content_width * 27 / 100));
    const int catalog_left = margin + layout->left_width + layout->gap;
    const int catalog_right = width - margin - layout->detail_width - layout->gap;
    layout->filter_card = make_rect(margin, content_top, catalog_left - layout->gap, content_bottom);
    layout->list_card = make_rect(catalog_left, content_top, catalog_right, content_bottom);
    layout->detail_card = make_rect(catalog_right + layout->gap, content_top,
                                    width - margin, content_bottom);
}

static void defer_catalogue_control(HDWP* batch, HWND control, int x, int y,
                                    int width, int height) {
    if (!control) return;
    width = max(1, width);
    height = max(1, height);
    if (batch && *batch) {
        HDWP next = DeferWindowPos(*batch, control, NULL, x, y, width, height,
                                   SWP_NOZORDER | SWP_NOACTIVATE);
        if (next) { *batch = next; return; }
        *batch = NULL;
    }
    SetWindowPos(control, NULL, x, y, width, height,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

static void layout_item_catalogue(HWND window) {
    CatalogueLayout layout = {};
    compute_catalogue_layout(window, &layout);
    const int header_x = ui_scale(20);
    const int header_y = ui_scale(13);
    const int close_size = ui_scale(42);
    const int header_right = layout.header_card.right;
    const int left_x = layout.filter_card.left + ui_scale(kUiPanelPadding);
    const int left_w = layout.filter_card.right - layout.filter_card.left - ui_scale(kUiPanelPadding * 2);
    HDWP batch = BeginDeferWindowPos(40);

    defer_catalogue_control(&batch, g_brand, header_x, header_y, ui_scale(42), ui_scale(42));
    // Give the font its full ascent and descender space (notably the 'g' in Extravaganza).
    defer_catalogue_control(&batch, g_header, header_x + ui_scale(56), ui_scale(10),
                            ui_scale(420), ui_scale(48));
    defer_catalogue_control(&batch, g_close_button, header_right - close_size - ui_scale(10), ui_scale(10),
                            close_size, close_size);
    defer_catalogue_control(&batch, g_version, header_right - close_size - ui_scale(168), ui_scale(18),
                            ui_scale(146), ui_scale(18));
    if (g_return_game) ShowWindow(g_return_game, SW_HIDE);

    int y = layout.filter_card.top + ui_scale(17);
    defer_catalogue_control(&batch, g_section_title, left_x, y, left_w - ui_scale(80), ui_scale(22));
    defer_catalogue_control(&batch, g_reset_filters, left_x + left_w - ui_scale(68), y - ui_scale(4),
                            ui_scale(68), ui_scale(30));
    y += ui_scale(42);
    defer_catalogue_control(&batch, g_search_label, left_x, y, left_w, ui_scale(17));
    y += ui_scale(23);
    defer_catalogue_control(&batch, g_search_backdrop, left_x + ui_scale(1), y,
                            left_w - ui_scale(2), ui_scale(32));
    defer_catalogue_control(&batch, g_search, left_x + ui_scale(11), y + ui_scale(5),
                            left_w - ui_scale(22), ui_scale(22));
    y += ui_scale(45);

    const HWND filter_labels[] = { g_category_label, g_rarity_label, g_library_label, g_weapon_level_label };
    const HWND filter_controls[] = { g_category_filter, g_rarity_filter, g_view_filter, g_weapon_level_filter };
    for (int i = 0; i < 4; ++i) {
        defer_catalogue_control(&batch, filter_labels[i], left_x, y, left_w, ui_scale(17));
        y += ui_scale(22);
        defer_catalogue_control(&batch, filter_controls[i], left_x, y, left_w, ui_scale(230));
        y += ui_scale(
#ifdef IE_OWNED_AFFIX_EDITOR
            g_owned_mode ? 37 :
#endif
            47);
    }
#ifdef IE_OWNED_AFFIX_EDITOR
    defer_catalogue_control(&batch,g_owned_filter_label,left_x,y+ui_scale(8),left_w,ui_scale(18));
    defer_catalogue_control(&batch,g_owned_format_filter,left_x,y+ui_scale(32),left_w,ui_scale(220));
    defer_catalogue_control(&batch,g_owned_presence_filter,left_x,y+ui_scale(77),left_w,ui_scale(220));
#endif

    const int catalog_x = layout.list_card.left + ui_scale(kUiPanelPadding);
    const int catalog_w = layout.list_card.right - layout.list_card.left - ui_scale(kUiPanelPadding * 2);
    const int catalog_y = layout.list_card.top + ui_scale(kUiPanelPadding);
    HWND refresh_button = GetDlgItem(window, IDC_REFRESH_CATALOG);
    const int refresh_w = ui_scale(106);
    const int count_w = ui_scale(148);
    const int count_x = layout.list_card.right - ui_scale(kUiPanelPadding) - refresh_w
        - ui_scale(12) - count_w;
    const int heading_w = max(ui_scale(100), count_x - catalog_x - ui_scale(12));
    defer_catalogue_control(&batch, g_catalog_title, catalog_x, catalog_y, ui_scale(220), ui_scale(22));
    defer_catalogue_control(&batch, g_catalog_subtitle, catalog_x, catalog_y + ui_scale(23),
                            heading_w, ui_scale(17));
    defer_catalogue_control(&batch, refresh_button, layout.list_card.right - ui_scale(kUiPanelPadding) - refresh_w,
                            catalog_y + ui_scale(3), refresh_w, ui_scale(34));
    defer_catalogue_control(&batch, g_result_count, count_x, catalog_y, count_w, ui_scale(40));
    defer_catalogue_control(&batch, g_tab_all_items, catalog_x, catalog_y + ui_scale(48),
                            ui_scale(105), ui_scale(28));
    defer_catalogue_control(&batch, g_tab_favorites, catalog_x + ui_scale(111),
                            catalog_y + ui_scale(48), ui_scale(118), ui_scale(28));
#ifdef IE_OWNED_AFFIX_EDITOR
    defer_catalogue_control(&batch, g_tab_owned_weapons, catalog_x + ui_scale(235),
                            catalog_y + ui_scale(48), ui_scale(146), ui_scale(28));
    defer_catalogue_control(&batch, g_tab_owned_gear, catalog_x + ui_scale(387),
                            catalog_y + ui_scale(48), ui_scale(112), ui_scale(28));
#endif
    defer_catalogue_control(&batch, g_item_list, catalog_x, catalog_y + ui_scale(82), catalog_w,
                            layout.list_card.bottom - (catalog_y + ui_scale(82)) - ui_scale(kUiPanelPadding));

    if (g_item_list && catalog_w > ui_scale(420)) {
        const int usable = catalog_w - ui_scale(18);
        ListView_SetColumnWidth(g_item_list, 0, usable * 28 / 100);
        ListView_SetColumnWidth(g_item_list, 1, usable * 24 / 100);
        ListView_SetColumnWidth(g_item_list, 2, usable * 16 / 100);
        ListView_SetColumnWidth(g_item_list, 3, usable * 12 / 100);
        ListView_SetColumnWidth(g_item_list, 4, usable * 7 / 100);
        ListView_SetColumnWidth(g_item_list, 5, usable - (usable * 87 / 100));
    }

    const int panel_x = layout.detail_card.left;
    const int panel_top = layout.detail_card.top;
    const int panel_bottom = layout.detail_card.bottom;
    const int panel_w = layout.detail_card.right - layout.detail_card.left;
    const int inner_x = panel_x + ui_scale(kUiPanelPadding);
    const int inner_w = panel_w - ui_scale(kUiPanelPadding * 2);
    defer_catalogue_control(&batch, g_detail_panel, panel_x, panel_top, panel_w, panel_bottom - panel_top);
    defer_catalogue_control(&batch, g_selected_label, inner_x, panel_top + ui_scale(16), inner_w, ui_scale(20));
#ifdef IE_OWNED_AFFIX_EDITOR
    defer_catalogue_control(&batch, g_owned_duplicate_weapon,
                            inner_x + inner_w - ui_scale(142), panel_top + ui_scale(10),
                            ui_scale(142), ui_scale(30));
    if (g_owned_mode && !g_owned_gear_mode)
        defer_catalogue_control(&batch, g_selected_label, inner_x,
                                panel_top + ui_scale(16), max(1, inner_w - ui_scale(150)), ui_scale(20));
#endif

    const int action_h = ui_scale(kUiActionHeight);
    const int action_gap = ui_scale(8);
    const int remove_y = panel_bottom - ui_scale(kUiPanelPadding) - action_h;
    const int utility_y = remove_y - action_gap - ui_scale(kUiCompactButtonHeight);
    const int give_y = utility_y - action_gap - action_h;
    const int actions_label_y = give_y - ui_scale(24);
    const int quantity_controls_y = actions_label_y - ui_scale(47);
    const int quantity_label_y = quantity_controls_y + ui_scale(9);

    int summary_height = max(ui_scale(118),
        quantity_controls_y - (panel_top + ui_scale(68)));
#ifdef IE_OWNED_AFFIX_EDITOR
    if (g_owned_mode) summary_height = ui_scale(88);
#endif
    defer_catalogue_control(&batch, g_selection, inner_x, panel_top + ui_scale(56),
                            inner_w, summary_height);
#ifdef IE_OWNED_AFFIX_EDITOR
    defer_catalogue_control(&batch, g_owned_repair_weapon, inner_x,
                            panel_top + ui_scale(150), inner_w, ui_scale(30));
    const int owned_list_y = panel_top + ui_scale(g_owned_gear_mode ? 150 : 188);
    const int owned_list_bottom = panel_bottom - ui_scale(305);
    defer_catalogue_control(&batch, g_owned_affix_list, inner_x, owned_list_y,
                            inner_w, max(ui_scale(45), owned_list_bottom - owned_list_y));
    if (g_owned_affix_list) {
        ListView_SetColumnWidth(g_owned_affix_list, 0, max(ui_scale(130), inner_w - ui_scale(128)));
        ListView_SetColumnWidth(g_owned_affix_list, 1, ui_scale(126));
    }
    defer_catalogue_control(&batch, g_owned_affix_detail, inner_x,
                            panel_bottom - ui_scale(297), inner_w, ui_scale(48));
    const int owned_value_y = panel_bottom - ui_scale(218);
    defer_catalogue_control(&batch, g_owned_value_label, inner_x,
                            owned_value_y - ui_scale(21), inner_w, ui_scale(18));
    const int owned_field_w = max(ui_scale(90), inner_w - ui_scale(122));
    defer_catalogue_control(&batch, g_owned_value_backdrop, inner_x, owned_value_y,
                            owned_field_w, ui_scale(36));
    defer_catalogue_control(&batch, g_owned_value_input, inner_x + ui_scale(10), owned_value_y + ui_scale(8),
                            owned_field_w - ui_scale(20), ui_scale(20));
    defer_catalogue_control(&batch, g_owned_apply_value,
                            inner_x + inner_w - ui_scale(112), owned_value_y,
                            ui_scale(112), ui_scale(36));
    defer_catalogue_control(&batch, g_owned_donor_label, inner_x,
                            panel_bottom - ui_scale(174),
                            max(ui_scale(100), inner_w - ui_scale(142)), ui_scale(18));
    defer_catalogue_control(&batch, g_owned_catalog_search_backdrop, inner_x,
                            panel_bottom - ui_scale(174),
                            max(ui_scale(100), inner_w - ui_scale(142)), ui_scale(34));
    defer_catalogue_control(&batch, g_owned_catalog_search, inner_x + ui_scale(10),
                            panel_bottom - ui_scale(167),
                            max(ui_scale(80), inner_w - ui_scale(162)), ui_scale(20));
    defer_catalogue_control(&batch, g_owned_catalog_test,
                            inner_x + inner_w - ui_scale(138),
                            panel_bottom - ui_scale(174), ui_scale(138), ui_scale(34));
    defer_catalogue_control(&batch, g_owned_donor_combo, inner_x,
                            panel_bottom - ui_scale(132),
                            g_owned_catalog_mode ? max(ui_scale(120), inner_w - ui_scale(138)) : inner_w,
                            ui_scale(220));
    defer_catalogue_control(&batch, g_owned_catalog_value_backdrop,
                            inner_x + inner_w - ui_scale(130),
                            panel_bottom - ui_scale(132), ui_scale(130), ui_scale(34));
    defer_catalogue_control(&batch, g_owned_catalog_value_input,
                            inner_x + inner_w - ui_scale(120),
                            panel_bottom - ui_scale(125), ui_scale(110), ui_scale(20));
    const int owned_button_gap = ui_scale(8);
    const int owned_button_w = (inner_w - owned_button_gap) / 2;
    defer_catalogue_control(&batch, g_owned_add_affix, inner_x,
                            panel_bottom - ui_scale(84),
                            (inner_w - ui_scale(16)) / 3, ui_scale(34));
    defer_catalogue_control(&batch, g_owned_remove_affix,
                            inner_x + (inner_w - ui_scale(16)) / 3 + owned_button_gap,
                            panel_bottom - ui_scale(84),
                            (inner_w - ui_scale(16)) / 3, ui_scale(34));
    defer_catalogue_control(&batch, g_owned_clear_affixes,
                            inner_x + 2 * ((inner_w - ui_scale(16)) / 3 + owned_button_gap),
                            panel_bottom - ui_scale(84),
                            inner_w - 2 * ((inner_w - ui_scale(16)) / 3 + owned_button_gap),
                            ui_scale(34));
    defer_catalogue_control(&batch, g_owned_restore, inner_x,
                            panel_bottom - ui_scale(42), owned_button_w, ui_scale(34));
    defer_catalogue_control(&batch, g_owned_keep,
                            inner_x + owned_button_w + owned_button_gap,
                            panel_bottom - ui_scale(42), owned_button_w, ui_scale(34));
#endif
    defer_catalogue_control(&batch, g_quantity_label, inner_x, quantity_label_y,
                            max(ui_scale(60), inner_w - ui_scale(196)), ui_scale(18));
    const int stepper_width = min(ui_scale(190), inner_w * 58 / 100);
    const int stepper_x = inner_x + inner_w - stepper_width;
    const int step_button = ui_scale(34);
    const int quantity_field_x = stepper_x + step_button + ui_scale(6);
    const int quantity_field_w = stepper_width - step_button * 2 - ui_scale(12);
    defer_catalogue_control(&batch, g_quantity_minus, stepper_x, quantity_controls_y,
                            step_button, ui_scale(34));
    defer_catalogue_control(&batch, g_quantity_backdrop, quantity_field_x, quantity_controls_y,
                            quantity_field_w, ui_scale(34));
    defer_catalogue_control(&batch, g_quantity, quantity_field_x + ui_scale(4),
                            quantity_controls_y + ui_scale(6),
                            quantity_field_w - ui_scale(8), ui_scale(22));
    defer_catalogue_control(&batch, g_quantity_plus, stepper_x + stepper_width - step_button,
                            quantity_controls_y, step_button, ui_scale(34));

    const int action_w = (inner_w - action_gap) / 2;
    defer_catalogue_control(&batch, g_actions_label, inner_x, actions_label_y, inner_w, ui_scale(18));
    defer_catalogue_control(&batch, g_give_inventory, inner_x, give_y, action_w, action_h);
    defer_catalogue_control(&batch, g_give_stash, inner_x + action_w + action_gap, give_y,
                            inner_w - action_w - action_gap, action_h);
    defer_catalogue_control(&batch, g_favorite, inner_x, utility_y,
                            (inner_w - action_gap) / 2, ui_scale(kUiCompactButtonHeight));
    defer_catalogue_control(&batch, g_copy_id, inner_x + (inner_w + action_gap) / 2, utility_y,
                            (inner_w - action_gap) / 2, ui_scale(kUiCompactButtonHeight));
    defer_catalogue_control(&batch, g_remove_inventory, inner_x, remove_y, inner_w, action_h);

    defer_catalogue_control(&batch, g_status, layout.status_card.left + ui_scale(28), layout.status_card.top,
                            layout.status_card.right - layout.status_card.left - ui_scale(40),
                            layout.status_card.bottom - layout.status_card.top);
    defer_catalogue_control(&batch, g_console_title, layout.console_card.left + ui_scale(16),
                            layout.console_card.top + ui_scale(11), ui_scale(210), ui_scale(18));
    defer_catalogue_control(&batch, g_log_filter, layout.console_card.right - ui_scale(254),
                            layout.console_card.top + ui_scale(7), ui_scale(142), ui_scale(210));
    defer_catalogue_control(&batch, g_clear_log, layout.console_card.right - ui_scale(100),
                            layout.console_card.top + ui_scale(7), ui_scale(84), ui_scale(30));
    defer_catalogue_control(&batch, g_runtime_log, layout.console_card.left + ui_scale(16),
                            layout.console_card.top + ui_scale(42),
                            layout.console_card.right - layout.console_card.left - ui_scale(32),
                            layout.console_card.bottom - layout.console_card.top - ui_scale(56));

    if (batch) EndDeferWindowPos(batch);
    update_runtime_log_scrollbar();
    InvalidateRect(window, NULL, FALSE);
}

static void draw_parent_text(HDC dc, const wchar_t* text_value, RECT rect, HFONT font,
                             COLORREF color, UINT flags) {
    HFONT selected_font = font ? font : (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HGDIOBJ old_font = SelectObject(dc, selected_font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    DrawTextW(dc, text_value, -1, &rect, flags);
    SelectObject(dc, old_font);
}

static void paint_catalogue_chrome(HWND window, HDC dc) {
    CatalogueLayout layout = {};
    compute_catalogue_layout(window, &layout);
    FillRect(dc, &layout.client, g_background_brush);

    HBRUSH header = CreateSolidBrush(kColorHeader);
    FillRect(dc, &layout.header_card, header);
    DeleteObject(header);
    RECT header_rule = { layout.header_card.left, layout.header_card.bottom - 1,
                         layout.header_card.right, layout.header_card.bottom };
    HBRUSH rule = CreateSolidBrush(RGB(28, 33, 39));
    FillRect(dc, &header_rule, rule);
    DeleteObject(rule);

    fill_round_rect(dc, layout.filter_card, kColorCard, kColorBorder, ui_scale(kUiPanelRadius));
    fill_round_rect(dc, layout.list_card, kColorPanel, kColorBorder, ui_scale(kUiPanelRadius));
    fill_round_rect(dc, layout.detail_card, kColorCard, kColorBorder, ui_scale(kUiPanelRadius));
    fill_round_rect(dc, layout.status_card, kColorStatus, RGB(28, 33, 39), ui_scale(8));
    fill_round_rect(dc, layout.console_card, kColorConsole, RGB(30, 35, 41), ui_scale(kUiPanelRadius));

    RECT filter_rule = make_rect(layout.filter_card.left + ui_scale(16), layout.filter_card.top + ui_scale(48),
                                 layout.filter_card.right - ui_scale(16), layout.filter_card.top + ui_scale(49));
    rule = CreateSolidBrush(RGB(31, 36, 42));
    FillRect(dc, &filter_rule, rule);
    DeleteObject(rule);

    RECT status_dot = make_rect(layout.status_card.left + ui_scale(12),
                                (layout.status_card.top + layout.status_card.bottom) / 2 - ui_scale(4),
                                layout.status_card.left + ui_scale(20),
                                (layout.status_card.top + layout.status_card.bottom) / 2 + ui_scale(4));
    HBRUSH live = CreateSolidBrush(kColorAccent);
    HGDIOBJ old_brush = SelectObject(dc, live);
    Ellipse(dc, status_dot.left, status_dot.top, status_dot.right, status_dot.bottom);
    SelectObject(dc, old_brush);
    DeleteObject(live);
}

static HICON create_brand_icon(int size) {
    HDC screen = GetDC(NULL);
    if (!screen) return NULL;
    HDC color_dc = CreateCompatibleDC(screen);
    HDC mask_dc = CreateCompatibleDC(screen);
    HBITMAP color_bitmap = CreateCompatibleBitmap(screen, size, size);
    HBITMAP mask_bitmap = CreateBitmap(size, size, 1, 1, NULL);
    HICON icon = NULL;
    if (color_dc && mask_dc && color_bitmap && mask_bitmap) {
        HGDIOBJ old_color = SelectObject(color_dc, color_bitmap);
        HGDIOBJ old_mask = SelectObject(mask_dc, mask_bitmap);
        RECT icon_rect = { 0, 0, size, size };
        HBRUSH background = CreateSolidBrush(kColorSidebar);
        FillRect(color_dc, &icon_rect, background);
        DeleteObject(background);
        PatBlt(mask_dc, 0, 0, size, size, BLACKNESS);

        RECT emblem_rect = { 1, 1, size - 1, size - 1 };
        draw_beast_emblem(color_dc, emblem_rect, false);

        SelectObject(color_dc, old_color);
        SelectObject(mask_dc, old_mask);
        ICONINFO information = {};
        information.fIcon = TRUE;
        information.hbmColor = color_bitmap;
        information.hbmMask = mask_bitmap;
        icon = CreateIconIndirect(&information);
    }
    if (color_bitmap) DeleteObject(color_bitmap);
    if (mask_bitmap) DeleteObject(mask_bitmap);
    if (color_dc) DeleteDC(color_dc);
    if (mask_dc) DeleteDC(mask_dc);
    ReleaseDC(NULL, screen);
    return icon;
}

static void rebuild_window_icons() {
    HICON small_icon = (HICON)LoadImageW(g_module, MAKEINTRESOURCEW(101), IMAGE_ICON,
                                        ui_scale(16), ui_scale(16), LR_DEFAULTCOLOR);
    HICON large_icon = (HICON)LoadImageW(g_module, MAKEINTRESOURCEW(101), IMAGE_ICON,
                                        ui_scale(32), ui_scale(32), LR_DEFAULTCOLOR);
    HICON brand_icon = (HICON)LoadImageW(g_module, MAKEINTRESOURCEW(101), IMAGE_ICON,
                                        ui_scale(64), ui_scale(64), LR_DEFAULTCOLOR);
    if (!small_icon) small_icon = create_brand_icon(ui_scale(16));
    if (!large_icon) large_icon = create_brand_icon(ui_scale(32));
    if (!brand_icon) brand_icon = create_brand_icon(ui_scale(64));
    if (small_icon) {
        SendMessageW(g_window, WM_SETICON, ICON_SMALL, (LPARAM)small_icon);
        if (g_window_icon_small) DestroyIcon(g_window_icon_small);
        g_window_icon_small = small_icon;
    }
    if (large_icon) {
        SendMessageW(g_window, WM_SETICON, ICON_BIG, (LPARAM)large_icon);
        if (g_window_icon_large) DestroyIcon(g_window_icon_large);
        g_window_icon_large = large_icon;
    }
    if (brand_icon) {
        if (g_brand_icon) DestroyIcon(g_brand_icon);
        g_brand_icon = brand_icon;
        if (g_brand) InvalidateRect(g_brand, NULL, FALSE);
    }
}

static bool arm_normal_drop_trace();

static void trace_drop_path() {
    HMODULE game = GetModuleHandleW(L"gamedll_ph_x64_rwdi.dll");
    BYTE *rdata = NULL, *data = NULL, *text = NULL;
    SIZE_T rdata_size = 0, data_size = 0, text_size = 0;
    if (!game || !get_section(game, ".rdata", &rdata, &rdata_size) ||
        !get_section(game, ".data", &data, &data_size) ||
        !get_section(game, ".text", &text, &text_size)) {
        log_line(L"[WORLD_ITEM] trace failed: required game sections are unavailable");
        set_status(L"The game module is not ready; try the trace again after loading your save.");
        return;
    }
    arm_normal_drop_trace();
}

static void WINAPI record_menu_inventory_drop(void* self, void* arg1, void* arg2,
                                              void* arg3, void* caller) {
    // This callback runs at the verified game action entry.  It deliberately
    // does no allocation, file I/O, or game mutation on the game thread.
    if (InterlockedCompareExchange(&g_menu_drop_trace_state, 2, 1) != 1) return;
    g_menu_drop_this = self;
    g_menu_drop_arg1 = arg1;
    g_menu_drop_arg2 = arg2;
    g_menu_drop_arg3 = arg3;
    g_menu_drop_caller = caller;
}

static BYTE* emit_bytes(BYTE*& at, const void* source, SIZE_T count) {
    memcpy(at, source, count); at += count; return at;
}

static void emit_qword(BYTE*& at, uintptr_t value) { emit_bytes(at, &value, sizeof(value)); }

// Emits an RIP-indirect absolute jump.  Unlike `mov rax, address; jmp rax`,
// this does not destroy RAX, which is live at a generic function boundary.
static void emit_absolute_jump(BYTE*& at, void* destination) {
    const BYTE jump[] = { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00 };
    emit_bytes(at, jump, sizeof(jump)); emit_qword(at, (uintptr_t)destination);
}

static bool validate_menu_drop_target(BYTE* target, BYTE* text, SIZE_T text_size) {
    if (!target || target < text || target + sizeof(kMenuInventoryDropPrologue) > text + text_size)
        return false;
    return !memcmp(target, kMenuInventoryDropPrologue, sizeof(kMenuInventoryDropPrologue));
}

static bool arm_normal_drop_trace() {
    // Test state before looking at code bytes: an armed trace deliberately
    // changed the target's first 14 bytes, so a second click is not a build
    // mismatch.  This makes the UI idempotent instead of misleading.
    const LONG current_state = InterlockedCompareExchange(&g_menu_drop_trace_state, 1, 1);
    if (current_state != 0) {
        set_status(current_state >= 2
            ? L"Capture already completed. Restart the game before another capture."
            : L"Capture is already armed. The panel will close; now make one normal inventory drop.");
        if (current_state == 1 && g_window) ShowWindow(g_window, SW_HIDE);
        return false;
    }
    HMODULE game = GetModuleHandleW(L"gamedll_ph_x64_rwdi.dll");
    BYTE* text = NULL; SIZE_T text_size = 0;
    if (!game || !get_section(game, ".text", &text, &text_size)) {
        log_line(L"[ITEM] normal-drop trace rejected: game module/text unavailable");
        set_status(L"Game module is not ready; load your save before arming the trace.");
        return false;
    }
    BYTE* target = (BYTE*)game + kMenuInventoryDropRva;
    if (!validate_menu_drop_target(target, text, text_size)) {
        log_line(L"[ITEM] normal-drop trace rejected: RVA=0x%llX byte validation failed",
                 (unsigned long long)kMenuInventoryDropRva);
        set_status(L"Drop trace rejected: this game build's target bytes do not match.");
        return false;
    }
    if (InterlockedCompareExchange(&g_menu_drop_trace_state, 1, 0) != 0) return arm_normal_drop_trace();

    // The trampoline contains the six whole instructions displaced by the
    // 14-byte patch, followed by an RAX-preserving return jump.
    BYTE* trampoline = (BYTE*)VirtualAlloc(NULL, 64, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    BYTE* detour = (BYTE*)VirtualAlloc(NULL, 160, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!trampoline || !detour) {
        if (trampoline) VirtualFree(trampoline, 0, MEM_RELEASE);
        if (detour) VirtualFree(detour, 0, MEM_RELEASE);
        InterlockedExchange(&g_menu_drop_trace_state, 0);
        log_line(L"[ITEM] normal-drop trace rejected: trampoline allocation failed");
        set_status(L"Drop trace allocation failed; no game code was changed.");
        return false;
    }
    BYTE* write = trampoline;
    emit_bytes(write, target, sizeof(kMenuInventoryDropPrologue));
    emit_absolute_jump(write, target + sizeof(kMenuInventoryDropPrologue));

    // Save flags and every volatile register used for the capture.  At the
    // call, [rsp+20h] is the fifth ABI argument and [rsp+68h] is the original
    // return address.  The callback only copies values to globals.
    write = detour;
    const BYTE save[] = { 0x9C, 0x50, 0x51, 0x52, 0x41, 0x50, 0x41, 0x51, 0x41, 0x52, 0x41, 0x53 };
    emit_bytes(write, save, sizeof(save));
    const BYTE reserve[] = { 0x48, 0x83, 0xEC, 0x28 };
    emit_bytes(write, reserve, sizeof(reserve));
    // rcx=self, rdx=arg1, r8=arg2, r9=arg3, stack arg=caller.
    const BYTE load_args[] = {
        0x48, 0x8B, 0x4C, 0x24, 0x50, 0x48, 0x8B, 0x54, 0x24, 0x48,
        0x4C, 0x8B, 0x44, 0x24, 0x40, 0x4C, 0x8B, 0x4C, 0x24, 0x38,
        0x48, 0x8B, 0x44, 0x24, 0x68, 0x48, 0x89, 0x44, 0x24, 0x20
    };
    emit_bytes(write, load_args, sizeof(load_args));
    const BYTE call[] = { 0xFF, 0x15, 0x00, 0x00, 0x00, 0x00 };
    emit_bytes(write, call, sizeof(call)); emit_qword(write, (uintptr_t)&record_menu_inventory_drop);
    const BYTE release_and_restore[] = {
        0x48, 0x83, 0xC4, 0x28, 0x41, 0x5B, 0x41, 0x5A, 0x41, 0x59,
        0x41, 0x58, 0x5A, 0x59, 0x58, 0x9D
    };
    emit_bytes(write, release_and_restore, sizeof(release_and_restore));
    emit_absolute_jump(write, trampoline);

    DWORD old_protect = 0;
    if (!VirtualProtect(target, sizeof(kMenuInventoryDropPrologue), PAGE_EXECUTE_READWRITE, &old_protect)) {
        VirtualFree(trampoline, 0, MEM_RELEASE); VirtualFree(detour, 0, MEM_RELEASE);
        InterlockedExchange(&g_menu_drop_trace_state, 0);
        log_line(L"[ITEM] normal-drop trace rejected: VirtualProtect failed error=%lu", GetLastError());
        set_status(L"Drop trace protection change failed; no game code was changed.");
        return false;
    }
    BYTE patch[sizeof(kMenuInventoryDropPrologue)] = { 0xFF, 0x25, 0, 0, 0, 0 };
    *(uintptr_t*)(patch + 6) = (uintptr_t)detour;
    memcpy(target, patch, sizeof(patch));
    FlushInstructionCache(GetCurrentProcess(), target, sizeof(patch));
    DWORD ignored = 0; VirtualProtect(target, sizeof(kMenuInventoryDropPrologue), old_protect, &ignored);
    g_menu_drop_target = target;
    g_menu_drop_trampoline = trampoline;
    log_line(L"[ITEM] inventory-removal boundary trace armed module=%p rva=0x%llX target=%p trampoline=%p detour=%p",
             game, (unsigned long long)kMenuInventoryDropRva, target, trampoline, detour);
    set_status(L"Capture armed. This panel is closing; now make one normal inventory drop.");
    if (g_window) ShowWindow(g_window, SW_HIDE);
    return true;
}

static void poll_normal_drop_trace() {
    if (InterlockedCompareExchange(&g_menu_drop_trace_state, 2, 2) != 2) return;
    // State 3 means the external UI thread already recorded the one capture.
    if (InterlockedCompareExchange(&g_menu_drop_trace_state, 3, 2) != 2) return;
    HMODULE game = GetModuleHandleW(L"gamedll_ph_x64_rwdi.dll");
    const SIZE_T caller_rva = game && g_menu_drop_caller >= game
        ? (SIZE_T)((BYTE*)g_menu_drop_caller - (BYTE*)game) : 0;
    log_line(L"[ITEM] normal-drop trace captured this=%p arg1=%p arg2=%p arg3=%p caller=%p caller_rva=0x%llX",
             g_menu_drop_this, g_menu_drop_arg1, g_menu_drop_arg2, g_menu_drop_arg3,
             g_menu_drop_caller, (unsigned long long)caller_rva);
    set_status(L"Normal drop captured. Open the log; restart the game before a new capture.");
}

static void run_probe() {
    HMODULE game = GetModuleHandleW(L"gamedll_ph_x64_rwdi.dll");
    if (!game) {
        InterlockedExchange(&g_ready, 0);
        log_line(L"[NATIVE] gamedll_ph_x64_rwdi.dll is not loaded yet");
        set_status(L"Waiting for the game module to load.");
        return;
    }
    BYTE* base = (BYTE*)game;
    IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
    IMAGE_NT_HEADERS64* nt = (IMAGE_NT_HEADERS64*)(base + dos->e_lfanew);
    log_line(L"[NATIVE] game module base=%p timestamp=0x%08X image=0x%X", game,
             nt->FileHeader.TimeDateStamp, nt->OptionalHeader.SizeOfImage);
    if (!validate_give_build(game)) {
        InterlockedExchange(&g_ready, 0);
        log_line(L"[NATIVE] unsupported retail build or item callsites changed; native actions locked");
        set_status(L"Game update not yet compatible: native item call sites did not validate.");
        return;
    }
    InterlockedExchange(&g_ready, 1);
    set_status(L"Game build verified. Item actions ready.");
}

static void copy_log_path() {
    if (!OpenClipboard(g_window)) return;
    EmptyClipboard(); SIZE_T bytes = (wcslen(g_log_path) + 1) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (memory) { memcpy(GlobalLock(memory), g_log_path, bytes); GlobalUnlock(memory); SetClipboardData(CF_UNICODETEXT, memory); }
    CloseClipboard(); set_status(L"Native log path copied to clipboard.");
}

static BOOL CALLBACK find_game_window_proc(HWND hwnd, LPARAM result) {
    if (hwnd == g_window || !IsWindowVisible(hwnd) || GetWindow(hwnd, GW_OWNER)) return TRUE;
    DWORD pid = 0; GetWindowThreadProcessId(hwnd, &pid);
    if (pid != GetCurrentProcessId()) return TRUE;
    *(HWND*)result = hwnd;
    return FALSE;
}

static HWND find_game_window() {
    HWND result = NULL;
    EnumWindows(find_game_window_proc, (LPARAM)&result);
    return result;
}

static void position_catalogue_for_first_show() {
    if (!g_window || !g_first_show) return;
    g_first_show = FALSE;
    HWND game_window = find_game_window();
    RECT anchor = {};
    if (!game_window || !GetWindowRect(game_window, &anchor)) {
        MONITORINFO monitor = { sizeof(monitor) };
        HMONITOR target = MonitorFromWindow(g_window, MONITOR_DEFAULTTONEAREST);
        if (!GetMonitorInfoW(target, &monitor)) return;
        anchor = monitor.rcWork;
    }
    RECT current = {};
    GetWindowRect(g_window, &current);
    const int width = current.right - current.left;
    const int height = current.bottom - current.top;
    HMONITOR monitor_handle = MonitorFromRect(&anchor, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitor = { sizeof(monitor) };
    if (!GetMonitorInfoW(monitor_handle, &monitor)) return;
    const RECT work = monitor.rcWork;
    int x = anchor.left + ((anchor.right - anchor.left) - width) / 2;
    int y = anchor.top + ((anchor.bottom - anchor.top) - height) / 2;
    x = max(work.left, min(x, work.right - width));
    y = max(work.top, min(y, work.bottom - height));
    SetWindowPos(g_window, NULL, x, y, width, height,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

static void focus_game_after_catalogue() {
    HWND game_window = find_game_window();
    if (!game_window || !IsWindow(game_window)) return;
    const DWORD current_thread = GetCurrentThreadId();
    const DWORD game_thread = GetWindowThreadProcessId(game_window, NULL);
    const BOOL attached = current_thread != game_thread &&
        AttachThreadInput(current_thread, game_thread, TRUE);
    ShowWindow(game_window, SW_SHOW);
    BringWindowToTop(game_window);
    SetForegroundWindow(game_window);
    SetActiveWindow(game_window);
    SetFocus(game_window);
    if (attached) AttachThreadInput(current_thread, game_thread, FALSE);
}

#include "RuntimeWindowScope.inl"

enum ActionResult {
    GIVE_RESULT_OK = 1,
    GIVE_RESULT_MODULE_NOT_READY = 2,
    GIVE_RESULT_BUILD_REJECTED = 3,
    GIVE_RESULT_IMPORT_REJECTED = 4,
    GIVE_RESULT_NO_PLAYER = 5,
    GIVE_RESULT_NO_INVENTORY = 6,
    GIVE_RESULT_NATIVE_REJECTED = 7,
    GIVE_RESULT_EXCEPTION = 8,
    GIVE_RESULT_DISPATCH_FAILED = 9,
    GIVE_RESULT_LEVEL_BLOCKED = 10,
    GIVE_RESULT_LEVEL_UNVERIFIED = 11
};

enum ItemAction {
    ITEM_ACTION_GIVE_INVENTORY = 1,
    ITEM_ACTION_GIVE_STASH = 2,
    ITEM_ACTION_REMOVE_INVENTORY = 3,
    ITEM_ACTION_SPAWN_WORLD = 4
};

struct ItemActionRequest {
    ItemAction action;
    char id[160];
    char name[160];
    char category[64];
    int quantity;
    DWORD dispatch_sequence;
};

static ItemActionRequest g_action_request = {};

struct NativeStringView {
    const char* data;
    DWORD length;
    DWORD reserved;
};

struct NativeItemDescriptor {
    unsigned __int64 id;
    DWORD generation_value;
    DWORD generation_flags;
};

static_assert(sizeof(NativeStringView) == 16, "native string view layout changed");
static_assert(sizeof(NativeItemDescriptor) == 16, "native item descriptor layout changed");

typedef void* (__fastcall *GetGameDiFn)();
typedef void* (__fastcall *ResolveLocalPlayerFn)(void* game_di);
typedef void (__fastcall *TtlStringFromViewFn)(void* destination,
                                               const NativeStringView* source,
                                               int allocator_type);
typedef void (__fastcall *TtlStringDestroyFn)(void* string, bool deallocate);
typedef bool (__fastcall *GiveInventoryItemFn)(void* inventory, void* player,
                                               const NativeItemDescriptor* descriptor,
                                               int count, bool reject_stash_fallback,
                                               void* result_item, bool* native_detail,
                                               bool force_equip, bool unknown_flag);
typedef bool (__fastcall *RemoveInventoryItemFn)(void* player, const void* item_id,
                                                  int count, bool remove_equipped,
                                                  void* result);
typedef void (__fastcall *DropInventoryIdFn)(void* drop_interface, const void* item_id);

#ifdef IE_AFFIX_REMOVE_POC
#include "AffixRemovalPoc.inl"
#endif
#ifdef IE_OWNED_AFFIX_EDITOR
#include "OwnedAffixReader.inl"
#ifdef IE_AFFIX_REMOVE_POC
#include "OwnedAffixEditor.inl"
#endif
#endif
#ifdef IE_GIVE_RANK_LEVEL
#if !defined(IE_OWNED_AFFIX_EDITOR) || !defined(IE_AFFIX_REMOVE_POC)
#error IE_GIVE_RANK_LEVEL requires the verified owned-item reader/editor build
#endif
#include "GiveWeaponLevel.inl"
#endif

static const wchar_t* action_label(ItemAction action) {
    switch (action) {
    case ITEM_ACTION_GIVE_INVENTORY: return L"give to inventory";
    case ITEM_ACTION_GIVE_STASH: return L"give to stash";
    case ITEM_ACTION_REMOVE_INVENTORY: return L"remove from inventory";
    case ITEM_ACTION_SPAWN_WORLD: return L"spawn in world";
    default: return L"item action";
    }
}

static ActionResult perform_item_action_on_game_thread(ItemActionRequest* completed) {
    ItemActionRequest request = {};
    EnterCriticalSection(&g_data_lock);
    request = g_action_request;
    LeaveCriticalSection(&g_data_lock);
    if (completed) *completed = request;

    HMODULE game = GetModuleHandleW(L"gamedll_ph_x64_rwdi.dll");
    if (!game) {
        log_line(L"[ITEM] %s rejected: gamedll is not loaded", action_label(request.action));
        return GIVE_RESULT_MODULE_NOT_READY;
    }
    if (!validate_give_build(game)) return GIVE_RESULT_BUILD_REJECTED;

    BYTE* base = (BYTE*)game;
    if (!is_readable_range(base + kTtlStringFromViewIatRva, sizeof(void*)) ||
        !is_readable_range(base + kTtlStringDestroyIatRva, sizeof(void*))) {
        log_line(L"[ITEM] give rejected: TTL string import slots are unreadable");
        return GIVE_RESULT_IMPORT_REJECTED;
    }

    TtlStringFromViewFn string_from_view =
        *(TtlStringFromViewFn*)(base + kTtlStringFromViewIatRva);
    TtlStringDestroyFn destroy_string =
        *(TtlStringDestroyFn*)(base + kTtlStringDestroyIatRva);
    GetGameDiFn get_game_di = (GetGameDiFn)(base + kGetGameDiRva);
    ResolveLocalPlayerFn resolve_player = (ResolveLocalPlayerFn)(base + kResolveLocalPlayerRva);
    GiveInventoryItemFn give_item = (GiveInventoryItemFn)(base + kGiveInventoryItemRva);
    RemoveInventoryItemFn remove_item = (RemoveInventoryItemFn)(base + kRemoveInventoryItemRva);
    if (!is_executable_address((void*)string_from_view) ||
        !is_executable_address((void*)destroy_string) ||
        !is_executable_address((void*)get_game_di) ||
        !is_executable_address((void*)resolve_player) ||
        !is_executable_address((void*)give_item) ||
        !is_executable_address((void*)remove_item)) {
        log_line(L"[ITEM] give rejected: one or more native targets are not executable");
        return GIVE_RESULT_IMPORT_REJECTED;
    }

    log_line(L"[ITEM] request sequence=%lu route=%s id=%S requested=%d dispatch_count=1 thread=%lu module=%p player_resolver=%p (RVA 0x%llX) give=%p (RVA 0x%llX) remove=%p (RVA 0x%llX)",
             request.dispatch_sequence, action_label(request.action), request.id, request.quantity, GetCurrentThreadId(), game,
             (void*)resolve_player, (unsigned long long)kResolveLocalPlayerRva,
             (void*)give_item, (unsigned long long)kGiveInventoryItemRva,
             (void*)remove_item, (unsigned long long)kRemoveInventoryItemRva);

    void* game_di = NULL;
    void* player = NULL;
    void* inventory = NULL;
    void* drop_interface = NULL;
    void* drop_dispatch = NULL;
    NativeItemDescriptor descriptor = {};
    NativeStringView view = { request.id, (DWORD)strlen(request.id), 0 };
    bool string_constructed = false;
    bool native_result = false;
    bool native_detail = false;
    bool stash_flag_changed = false;
    BYTE previous_stash_flag = 0;
    DWORD trapped_exception = 0;
    ActionResult result = GIVE_RESULT_EXCEPTION;
#ifdef IE_GIVE_RANK_LEVEL
    const DWORD requested_rank = request.action == ITEM_ACTION_GIVE_INVENTORY
        ? parse_weapon_level(request.id, request.category) : 0;
    RankGiveSnapshot rank_snapshot = {};
#endif

    __try {
        game_di = get_game_di();
        if (!is_readable_range(game_di, sizeof(void*))) {
            result = GIVE_RESULT_NO_PLAYER;
        } else {
            player = resolve_player(game_di);
            if (!is_readable_range(player, kPlayerInventoryOffset + sizeof(void*))) {
                result = GIVE_RESULT_NO_PLAYER;
            } else {
                inventory = *(void**)((BYTE*)player + kPlayerInventoryOffset);
                const bool requires_inventory = request.action == ITEM_ACTION_GIVE_INVENTORY ||
                                                request.action == ITEM_ACTION_GIVE_STASH ||
                                                request.action == ITEM_ACTION_SPAWN_WORLD;
                if (requires_inventory &&
                    !is_readable_range(inventory, sizeof(void*))) {
                    result = GIVE_RESULT_NO_INVENTORY;
                } else {
#ifdef IE_GIVE_RANK_LEVEL
                    if (requested_rank && !rank_give_prepare(base, player, request.quantity, &rank_snapshot)) {
                        log_line(L"[GIVE_LEVEL] blocked before Give id=%S rank=%lu requested=%d dispatch_count=0; signature/ownership/bounds check failed",
                                 request.id, requested_rank, request.quantity);
                        return GIVE_RESULT_LEVEL_BLOCKED;
                    }
#endif
                    // Story::GiveItemLogic constructs this exact 16-byte descriptor:
                    // TTL item-id string, generation value 0, generation flags 0xEF.
                    descriptor.generation_value = 0;
                    descriptor.generation_flags = 0xEF;
                    string_from_view(&descriptor.id, &view, 0);
                    string_constructed = true;

                    if (request.action == ITEM_ACTION_REMOVE_INVENTORY) {
                        // Story::RemoveItemLogic passes player, TTL item ID, count,
                        // its remove-equipped flag, and a null result pointer.
                        native_result = remove_item(player, &descriptor.id, request.quantity,
                                                    false, NULL);
                    } else if (request.action == ITEM_ACTION_SPAWN_WORLD) {
                        void* controller = resolve_player_sync_action_controller(player, base);
                        void** controller_vtable = is_readable_range(controller, 0x70)
                            ? *(void***)controller : NULL;
                        const void* expected_controller_vtable =
                            base + kSyncActionControllerVtableRva;
                        if (controller_vtable != expected_controller_vtable) {
                            log_line(L"[WORLD_ITEM] player-bound SyncActionController unavailable or wrong type controller=%p vtable=%p expected=%p player=%p",
                                     controller, controller_vtable,
                                     expected_controller_vtable,
                                     player);
                            result = GIVE_RESULT_NATIVE_REJECTED;
                        } else {
                            drop_interface = (BYTE*)controller + kSyncActionInventoryOffset;
                            void** drop_vtable = is_readable_range(drop_interface, sizeof(void*))
                                ? *(void***)drop_interface : NULL;
                            void** expected_drop_vtable =
                                (void**)(base + kSyncActionInventoryVtableRva);
                            if (drop_vtable != expected_drop_vtable ||
                                !is_readable_range(drop_vtable, 0x50)) {
                                log_line(L"[WORLD_ITEM] SyncAction inventory helper validation failed controller=%p interface=%p vtable=%p expected=%p",
                                         controller, drop_interface, drop_vtable,
                                         expected_drop_vtable);
                                result = GIVE_RESULT_NATIVE_REJECTED;
                            } else {
                                DropInventoryIdFn drop_by_id =
                                    (DropInventoryIdFn)drop_vtable[0x48 / sizeof(void*)];
                                drop_dispatch = (void*)drop_by_id;
                                const void* expected_dispatch =
                                    base + kSyncActionDropMethodRva;
                                if ((void*)drop_by_id != expected_dispatch ||
                                    !is_executable_address((void*)drop_by_id)) {
                                    log_line(L"[WORLD_ITEM] SyncAction ID-drop slot validation failed interface=%p target=%p expected=%p",
                                             drop_interface, (void*)drop_by_id,
                                             expected_dispatch);
                                    result = GIVE_RESULT_NATIVE_REJECTED;
                                } else {
                                    // This is the same inventory-to-world transaction used by
                                    // shipping SyncActionEventDropInventoryItem. Stage one real
                                    // carried item, then hand its TTL ID to the controller's
                                    // inventory helper. It never swaps or force-equips a weapon.
                                    BYTE* force_stash = base + kForceSendToStashFlagRva;
                                    if (!is_readable_range(force_stash, 1)) {
                                        result = GIVE_RESULT_BUILD_REJECTED;
                                    } else {
                                        previous_stash_flag = *force_stash;
                                        *force_stash = 0;
                                        stash_flag_changed = true;
                                        native_result = true;
                                        for (int index = 0; index < request.quantity; ++index) {
                                            const bool staged = give_item(inventory, player,
                                                &descriptor, 1, true, NULL, &native_detail,
                                                false, false);
                                            if (!staged) {
                                                native_result = false;
                                                log_line(L"[WORLD_ITEM] staging grant rejected id=%S item=%d/%d player=%p inventory=%p",
                                                         request.id, index + 1,
                                                         request.quantity, player, inventory);
                                                break;
                                            }
                                            void** ready = (void**)((BYTE*)controller + kSyncActionReadyOffset);
                                            BYTE* busy = (BYTE*)controller + kSyncActionBusyOffset;
                                            BYTE* drop_active = (BYTE*)controller + kSyncActionDropActiveOffset;
                                            if (!is_readable_range(ready, sizeof(*ready)) ||
                                                !is_readable_range(busy, 1) ||
                                                !is_readable_range(drop_active, 1) || *busy) {
                                                native_result = false;
                                                log_line(L"[WORLD_ITEM] SyncAction controller cannot accept drop item=%d/%d ready=%p busy=%u active=%u",
                                                         index + 1, request.quantity,
                                                         is_readable_range(ready, sizeof(*ready)) ? *ready : NULL,
                                                         is_readable_range(busy, 1) ? (unsigned)*busy : 0xFFu,
                                                         is_readable_range(drop_active, 1) ? (unsigned)*drop_active : 0xFFu);
                                                break;
                                            }

                                            // DropInventoryItem's first instruction checks this
                                            // action-ready slot, then its verified cleanup routine
                                            // clears it before resolving the ID or creating the
                                            // pickup.  A player-bound controller is dormant outside
                                            // a scripted SyncAction, so activate only that one-shot
                                            // gate.  The busy byte must remain zero, which guarantees
                                            // cleanup never dereferences or releases this sentinel.
                                            const bool primed = *ready == NULL;
                                            if (primed) *ready = controller;
                                            drop_by_id(drop_interface, &descriptor.id);
                                            if (primed && *ready == controller) *ready = NULL;
                                            log_line(L"[WORLD_ITEM] SyncAction drop dispatched id=%S item=%d/%d controller=%p interface=%p target=%p ready_after=%p busy=%u active=%u",
                                                     request.id, index + 1,
                                                     request.quantity, controller,
                                                     drop_interface, (void*)drop_by_id,
                                                     *ready, (unsigned)*busy,
                                                     (unsigned)*drop_active);
                                        }
                                        *force_stash = previous_stash_flag;
                                        stash_flag_changed = false;
                                    }
                                }
                            }
                        }
                    } else {
                        BYTE* force_stash = base + kForceSendToStashFlagRva;
                        if (!is_readable_range(force_stash, 1)) {
                            result = GIVE_RESULT_BUILD_REJECTED;
                        } else {
                            previous_stash_flag = *force_stash;
                            *force_stash = request.action == ITEM_ACTION_GIVE_STASH ? 1 : 0;
                            stash_flag_changed = true;
                            const bool force_equip = false;
                            // The fifth argument is the native "do not use the
                            // fallback destination" switch. A real pickup callsite
                            // passes true so a full carried category is rejected
                            // instead of silently overflowing to stash. Story rewards
                            // pass false because they are allowed to use that fallback.
                            const bool reject_stash_fallback =
                                request.action == ITEM_ACTION_GIVE_INVENTORY;
                            native_result = give_item(inventory, player, &descriptor,
                                                      request.quantity, reject_stash_fallback,
                                                      NULL, &native_detail,
                                                      force_equip, false);
                            log_line(L"[ITEM] insertion result route=%s accepted=%d native_detail=%d previous_force_stash=%u strict_carried=%d",
                                     action_label(request.action), native_result ? 1 : 0,
                                     native_detail ? 1 : 0, (unsigned)previous_stash_flag,
                                     reject_stash_fallback ? 1 : 0);
                            *force_stash = previous_stash_flag;
                            stash_flag_changed = false;
#ifdef IE_GIVE_RANK_LEVEL
                            if (native_result && requested_rank &&
                                !rank_give_apply(base, player, &rank_snapshot, request.id,
                                                 requested_rank, request.quantity)) {
                                g_rank_give_faulted = true;
                                result = GIVE_RESULT_LEVEL_UNVERIFIED;
                                log_line(L"[GIVE_LEVEL] correction not verified AFTER Give id=%S requested=%d; items may already exist; no retry",
                                         request.id, request.quantity);
                            }
                            if (!native_result && requested_rank &&
                                !rank_give_unchanged(base, player, &rank_snapshot)) {
                                g_rank_give_faulted = true;
                                result = GIVE_RESULT_LEVEL_UNVERIFIED;
                                log_line(L"[GIVE_LEVEL] native rejection changed inventory; items may already exist; no correction/retry");
                            }
#endif
                        }
                    }
                    if (result != GIVE_RESULT_BUILD_REJECTED &&
                        result != GIVE_RESULT_NATIVE_REJECTED &&
                        result != GIVE_RESULT_LEVEL_UNVERIFIED)
                        result = native_result ? GIVE_RESULT_OK : GIVE_RESULT_NATIVE_REJECTED;
                }
            }
        }
    } __except (trapped_exception = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {
        result = GIVE_RESULT_EXCEPTION;
#ifdef IE_GIVE_RANK_LEVEL
        if (requested_rank) g_rank_give_faulted = true;
#endif
    }

    if (stash_flag_changed) {
        __try { *(base + kForceSendToStashFlagRva) = previous_stash_flag; }
        __except (trapped_exception = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {
            result = GIVE_RESULT_EXCEPTION;
        }
    }

    if (string_constructed) {
        __try { destroy_string(&descriptor.id, false); }
        __except (trapped_exception = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {
            result = GIVE_RESULT_EXCEPTION;
        }
    }

    if (result == GIVE_RESULT_OK) {
        if (request.action == ITEM_ACTION_SPAWN_WORLD)
            log_line(L"[WORLD_ITEM] native ID-based spawn dispatched id=%S count=%d player=%p interface=%p target=%p",
                     request.id, request.quantity, player, drop_interface, drop_dispatch);
        else
            log_line(L"[ITEM] native %s accepted id=%S count=%d player=%p inventory=%p",
                     action_label(request.action), request.id, request.quantity, player, inventory);
    } else if (result == GIVE_RESULT_NO_PLAYER) {
        log_line(L"[ITEM] give failed at local-player resolution GameDI=%p player=%p", game_di, player);
    } else if (result == GIVE_RESULT_NO_INVENTORY) {
        log_line(L"[ITEM] give failed: player resolved but inventory +0x%llX is invalid player=%p inventory=%p",
                 (unsigned long long)kPlayerInventoryOffset, player, inventory);
    } else if (result == GIVE_RESULT_NATIVE_REJECTED) {
        log_line(L"[ITEM] native %s rejected id=%S count=%d player=%p inventory=%p",
                 action_label(request.action), request.id, request.quantity, player, inventory);
    } else if (result == GIVE_RESULT_EXCEPTION) {
        log_line(L"[ITEM] native %s trapped exception=0x%08X id=%S player=%p inventory=%p",
                 action_label(request.action), trapped_exception, request.id, player, inventory);
    }
    return result;
}

static LRESULT CALLBACK game_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
#ifdef IE_OWNED_AFFIX_EDITOR
    if (msg == WM_BEAST_OWNED_EDIT) {
        OwnedEditRequest request = {};
        EnterCriticalSection(&g_data_lock);
        if (!g_owned_edit_pending || g_owned_edit_dispatched) {
            LeaveCriticalSection(&g_data_lock);
            return 0;
        }
        g_owned_edit_dispatched = true;
        request = g_owned_edit_request;
        LeaveCriticalSection(&g_data_lock);
        int result = 99;
        __try {
            if (g_owned_edit_faulted) result = 96;
            else {
                HMODULE game = GetModuleHandleW(L"gamedll_ph_x64_rwdi.dll");
                if (game && validate_give_build(game)) {
                    BYTE* base = (BYTE*)game;
                    GetGameDiFn get_di = (GetGameDiFn)(base + kGetGameDiRva);
                    ResolveLocalPlayerFn get_player = (ResolveLocalPlayerFn)(base + kResolveLocalPlayerRva);
                    void* di = get_di();
                    void* player = is_readable_range(di, 8) ? get_player(di) : NULL;
                    result = is_readable_range(player, 0x458)
                        ? owned_execute_edit(base, player, &request) : 98;
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            result = 97;
            g_owned_edit_faulted = true;
        }
        log_line(L"[OWNED_AFFIX] action=%d result=%d target=%p item=%S affix=%S index=%u snapshot=%d",
                 (int)request.kind, result, request.target, request.item_id,
                 request.affix_id, request.affix_index, g_owned_transaction ? 1 : 0);
        if (g_window) PostMessageW(g_window, WM_BEAST_OWNED_EDIT_RESULT,
                                   (WPARAM)result, (LPARAM)request.kind);
        return 0;
    }
    if (msg == WM_BEAST_OWNED_SCAN) {
        int result = 99;
        bool snapshot_expired = false;
        __try {
            HMODULE game = GetModuleHandleW(L"gamedll_ph_x64_rwdi.dll");
            if (game && validate_give_build(game)) {
                BYTE* base = (BYTE*)game;
                GetGameDiFn get_di = (GetGameDiFn)(base + kGetGameDiRva);
                ResolveLocalPlayerFn get_player = (ResolveLocalPlayerFn)(base + kResolveLocalPlayerRva);
                void* di = get_di();
                void* player = is_readable_range(di, 8) ? get_player(di) : NULL;
                result = is_readable_range(player, 0x458)
                    ? owned_scan_weapons(base, player) : 98;
                if (!result && g_owned_transaction &&
                    !owned_active_target_unchanged(base, player)) {
                    owned_release_snapshot(base);
                    snapshot_expired = true;
                    log_line(L"[OWNED_AFFIX] stale edit snapshot expired; item instance or affix records changed outside editor");
                }
                if (!result && !g_owned_lookup_probe_done) {
                    g_owned_lookup_probe_done = true;
                    const int lookup = owned_probe_catalog_lookup(base, player);
                    g_owned_lookup_probe_passed = lookup == 0;
                    log_line(L"[OWNED_AFFIX] read-only catalogue lookup result=%d", lookup);
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            result = 97;
        }
        log_line(L"[OWNED_AFFIX] scan result=%d carried_weapon_and_gear_instances=%lu", result, g_owned_count);
        if (g_window) PostMessageW(g_window, WM_BEAST_OWNED_SCAN_RESULT,
                                   (WPARAM)result, snapshot_expired ? 1 : 0);
        return 0;
    }
#endif
#if defined(IE_AFFIX_REMOVE_POC) && !defined(IE_OWNED_AFFIX_EDITOR)
    if (msg == WM_BEAST_AFFIX_POC) {
        int result = 99;
        __try {
            HMODULE game = GetModuleHandleW(L"gamedll_ph_x64_rwdi.dll");
            if (game && validate_give_build(game)) {
                BYTE* base = (BYTE*)game;
                GetGameDiFn get_di = (GetGameDiFn)(base + kGetGameDiRva);
                ResolveLocalPlayerFn get_player = (ResolveLocalPlayerFn)(base + kResolveLocalPlayerRva);
                void* di = get_di();
                void* player = is_readable_range(di, 8) ? get_player(di) : NULL;
                result = is_readable_range(player, 0x458)
                    ? (wparam == 2 ? poc_add_donor_affix(base, player)
                       : poc_change_affix(base, player, wparam == 1)) : 98;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            result = 97;
        }
        log_line(L"[AFFIX_POC] operation=%s result=%d saved_records=%u mode=%u item=%p",
                 wparam == 2 ? L"add donor" : (wparam == 1 ? L"restore" : L"remove"), result,
                 g_poc_saved_count, g_poc_mode, g_poc_saved_item);
        if (g_window) PostMessageW(g_window, WM_BEAST_AFFIX_POC_RESULT,
                                   (WPARAM)result, wparam);
        return 0;
    }
#endif
    if (msg == WM_BEAST_GIVE_TEST) {
        ItemActionRequest completed = {};
        const ActionResult result = perform_item_action_on_game_thread(&completed);
        if (!g_window || !PostMessageW(g_window, WM_BEAST_GIVE_RESULT,
                                       (WPARAM)result, (LPARAM)completed.action))
            InterlockedExchange(&g_action_pending, 0);
        return 0;
    }
    return g_original_game_window_proc
        ? CallWindowProcW(g_original_game_window_proc, hwnd, msg, wparam, lparam)
        : DefWindowProcW(hwnd, msg, wparam, lparam);
}

static bool install_game_thread_dispatch(HWND game_window) {
    if (!game_window || !IsWindow(game_window)) return false;
    // Another cooperating tool can be above us in the same window's chain.
    // Reinstalling there would replace our predecessor with that tool and form
    // Item -> Prop -> Item recursion. Retain the original predecessor instead.
    if (g_game_window == game_window && g_original_game_window_proc)
        return true;

    WNDPROC current = (WNDPROC)GetWindowLongPtrW(game_window, GWLP_WNDPROC);
    if (!current || current == game_window_proc) return false;
    g_game_window = game_window;
    g_original_game_window_proc = current;
    SetLastError(ERROR_SUCCESS);
    WNDPROC replaced = (WNDPROC)SetWindowLongPtrW(game_window, GWLP_WNDPROC,
                                                  (LONG_PTR)game_window_proc);
    if (!replaced && GetLastError() != ERROR_SUCCESS) {
        log_line(L"[ITEM] game-thread dispatch install failed error=%lu", GetLastError());
        g_game_window = NULL;
        g_original_game_window_proc = NULL;
        return false;
    }
    if (replaced) g_original_game_window_proc = replaced;
    DWORD owner_thread = GetWindowThreadProcessId(game_window, NULL);
    log_line(L"[ITEM] game-thread dispatch installed hwnd=%p owner_thread=%lu tool_thread=%lu",
             game_window, owner_thread, GetCurrentThreadId());
    return true;
}

static bool read_item_quantity(int* output) {
    if (!output) return false;
    wchar_t text[24] = L"1";
    if (g_quantity) GetWindowTextW(g_quantity, text, _countof(text));
    wchar_t* end = NULL;
    const unsigned __int64 value = _wcstoui64(text, &end, 10);
    while (end && (*end == L' ' || *end == L'\t')) ++end;
    if (!text[0] || !end || *end || value < 1 || value > 0x7FFFFFFFull)
        return false;
    *output = (int)value;
    return true;
}

static void step_item_quantity(int delta) {
    int quantity = 1;
    if (!read_item_quantity(&quantity)) quantity = 1;
    if (delta > 0 && quantity < 0x7FFFFFFF) ++quantity;
    else if (delta < 0 && quantity > 1) --quantity;
    wchar_t text[24] = {};
    _snwprintf_s(text, _countof(text), _TRUNCATE, L"%d", quantity);
    SetWindowTextW(g_quantity, text);
}

static void set_item_actions_enabled(bool enabled) {
    if (g_give_inventory) EnableWindow(g_give_inventory, enabled ? TRUE : FALSE);
    if (g_give_stash) EnableWindow(g_give_stash, enabled ? TRUE : FALSE);
    if (g_remove_inventory) EnableWindow(g_remove_inventory, enabled ? TRUE : FALSE);
}

static void queue_item_action(ItemAction action) {
    if (!InterlockedCompareExchange(&g_ready, 1, 1)) {
        set_status(GetModuleHandleW(L"gamedll_ph_x64_rwdi.dll")
            ? L"Native item calls are locked: this game build has not passed validation."
            : L"Waiting for the game module to load.");
        return;
    }
    ItemDefinition item = {}; int catalog_index = -1;
    if (!selected_item(&item, &catalog_index)) {
        set_status(L"Select an item, or enter one exact internal ID in Search.");
        return;
    }
    int quantity = 1;
    if (!read_item_quantity(&quantity)) {
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
    log_line(L"[ITEM] queued sequence=%lu route=%s item=%S requested=%d dispatch_count=1",
             g_action_request.dispatch_sequence, action_label(action), item.id, quantity);
    HWND game_window = find_game_window();
    if (!install_game_thread_dispatch(game_window) ||
        !PostMessageW(game_window, WM_BEAST_GIVE_TEST, 0, 0)) {
        InterlockedExchange(&g_action_pending, 0);
        set_item_actions_enabled(true);
        log_line(L"[ITEM] %s dispatch failed hwnd=%p error=%lu",
                 action_label(action), game_window, GetLastError());
        set_status(L"Could not dispatch the item action on the game thread; see the log.");
        return;
    }
    wchar_t status[300] = {}, name[180] = {};
    ascii_to_wide(item.name, name, _countof(name));
    _snwprintf_s(status, _countof(status), _TRUNCATE, L"Running %s: %s x%d…",
                 action_label(action), name, quantity);
    set_status(status);
}

static void copy_selected_item_id() {
    ItemDefinition item = {};
    if (!selected_item(&item, NULL)) { set_status(L"Select an item first."); return; }
    wchar_t id[180] = {}; ascii_to_wide(item.id, id, _countof(id));
    if (!OpenClipboard(g_window)) { set_status(L"Clipboard is currently unavailable."); return; }
    EmptyClipboard();
    const SIZE_T bytes = (wcslen(id) + 1) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (memory) {
        void* destination = GlobalLock(memory);
        if (destination) { memcpy(destination, id, bytes); GlobalUnlock(memory); SetClipboardData(CF_UNICODETEXT, memory); }
    }
    CloseClipboard();
    set_status(L"Internal item ID copied.");
}

static void toggle_selected_favorite() {
    ItemDefinition item = {}; int index = -1;
    if (!selected_item(&item, &index) || index < 0) {
        set_status(L"Select a catalog item before changing favorites.");
        return;
    }
    EnterCriticalSection(&g_data_lock);
    g_items[index].favorite = !g_items[index].favorite;
    const bool favorite = g_items[index].favorite != 0;
    LeaveCriticalSection(&g_data_lock);
    save_favorites();
    if (g_view_filter && SendMessageW(g_view_filter, CB_GETCURSEL, 0, 0) == 1)
        refresh_item_view();
    else {
        update_selected_item_panel();
        InvalidateRect(g_item_list, NULL, FALSE);
    }
    set_status(favorite ? L"Added to favorites." : L"Removed from favorites.");
}

#ifdef IE_OWNED_AFFIX_EDITOR
static const wchar_t kOwnedAffixSearchHint[] = L"Search Affix...";

static bool draw_owned_affix_search_hint(HWND edit,HDC dc) {
    if (!dc || GetWindowTextLengthW(edit)) return false;
    RECT bounds={}; GetClientRect(edit,&bounds);
    const LRESULT margins=SendMessageW(edit,EM_GETMARGINS,0,0);
    bounds.left+=LOWORD(margins); bounds.right-=HIWORD(margins);
    const int saved=SaveDC(dc);
    SelectObject(dc,g_input_font ? g_input_font : GetStockObject(DEFAULT_GUI_FONT));
    SetBkMode(dc,TRANSPARENT); SetTextColor(dc,kColorMuted);
    DrawTextW(dc,kOwnedAffixSearchHint,-1,&bounds,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    RestoreDC(dc,saved);
    return true;
}

static LRESULT CALLBACK owned_affix_search_proc(HWND edit,UINT message,WPARAM wparam,LPARAM lparam,
                                                UINT_PTR,DWORD_PTR) {
    // Paint our hint rather than relying on theme/activation-context cue banner
    // support inside the host game. It never becomes the edit's actual value.
    const LRESULT result=DefSubclassProc(edit,message,wparam,lparam);
    if (message==WM_PAINT) {
        HDC dc=GetDC(edit); draw_owned_affix_search_hint(edit,dc); ReleaseDC(edit,dc);
    } else if (message==WM_PRINTCLIENT || message==WM_PRINT) {
        draw_owned_affix_search_hint(edit,(HDC)wparam);
    } else if (message==WM_SETTEXT || message==WM_SETFOCUS || message==WM_KILLFOCUS) {
        InvalidateRect(edit,NULL,TRUE);
    } else if (message==WM_NCDESTROY) {
        RemoveWindowSubclass(edit,owned_affix_search_proc,1);
    }
    return result;
}

static bool selected_owned_weapon(OwnedWeaponView* out) {
    if (!g_owned_mode || !out || !g_item_list) return false;
    const int row = ListView_GetNextItem(g_item_list, -1, LVNI_SELECTED);
    EnterCriticalSection(&g_data_lock);
    const bool valid = row >= 0 && row < (int)g_visible_count &&
        g_visible_items[row] >= 0 && g_visible_items[row] < (int)g_owned_count;
    if (valid) *out = g_owned_weapons[g_visible_items[row]];
    LeaveCriticalSection(&g_data_lock);
    return valid;
}

static bool select_owned_instance(void* instance) {
    int row = -1;
    EnterCriticalSection(&g_data_lock);
    for (DWORD i = 0; i < g_visible_count; ++i)
        if (g_visible_items[i] >= 0 && g_visible_items[i] < (int)g_owned_count &&
            g_owned_weapons[g_visible_items[i]].instance == instance) row = (int)i;
    LeaveCriticalSection(&g_data_lock);
    if (row < 0) return false;
    g_owned_view_rebuilding = true;
    ListView_SetItemState(g_item_list,-1,0,LVIS_SELECTED | LVIS_FOCUSED);
    ListView_SetItemState(g_item_list,row,LVIS_SELECTED | LVIS_FOCUSED,LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(g_item_list,row,FALSE);
    g_owned_view_rebuilding = false;
    update_owned_affix_panel();
    populate_owned_donors();
    return true;
}

static void update_owned_add_button() {
    OwnedWeaponView target = {};
    const int choice = (int)SendMessageW(g_owned_donor_combo,CB_GETCURSEL,0,0);
    const char* id = NULL;
    if (g_owned_catalog_mode && choice >= 0 && choice < (int)g_owned_catalog_visible_count)
        id = g_owned_affix_metadata[g_owned_catalog_visible[choice]].id;
    else if (!g_owned_catalog_mode && choice >= 0 && choice < (int)g_owned_donor_count)
        id = g_owned_donors[choice].affix_id;
    const wchar_t* reason = NULL;
    if (!selected_owned_weapon(&target)) reason = L"Select Item";
    else if (g_owned_edit_faulted) reason = L"Restart";
    else if (g_owned_edit_pending || g_owned_switch_target) reason = L"Please Wait";
    else if (g_owned_transaction && g_owned_saved_item != target.instance) reason = L"Finish Edit";
    else if (!id) reason = L"Choose Affix";
    else if (target.affix_count >= kOwnedAffixTestLimit) reason = L"32 Max";
    for (unsigned i = 0; !reason && i < target.affix_count; ++i)
        if (!_stricmp(target.affixes[i].id,id)) reason = L"Already On";
    const bool allowed = reason == NULL;
    SetWindowTextW(g_owned_add_affix, reason ? reason :
                   g_owned_catalog_mode ? L"Add Affix" : L"Copy Affix");
    EnableWindow(g_owned_add_affix,allowed);
}

static void update_owned_affix_panel() {
    if (!g_owned_affix_list) return;
    int old_affix = -1;
    g_owned_panel_rebuilding = true;
    SendMessageW(g_owned_affix_list, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(g_owned_affix_list);
    EnableWindow(g_owned_restore, g_owned_transaction && !g_owned_edit_pending && !g_owned_switch_target && !g_owned_edit_faulted);
    EnableWindow(g_owned_keep, g_owned_transaction && !g_owned_edit_pending && !g_owned_switch_target && !g_owned_edit_faulted);
    OwnedWeaponView weapon = {};
    if (!selected_owned_weapon(&weapon)) {
        g_owned_panel_instance = NULL;
        g_owned_selected_affix_id[0] = 0;
        g_owned_selected_builtin = -1;
        SetWindowTextW(g_owned_value_input, L"");
        SetWindowTextW(g_owned_affix_detail, L"Select an owned item to inspect affixes and built-in stats.");
        EnableWindow(g_owned_apply_value, FALSE);
        EnableWindow(g_owned_remove_affix, FALSE);
        EnableWindow(g_owned_clear_affixes, FALSE);
        EnableWindow(g_owned_duplicate_weapon, FALSE);
        EnableWindow(g_owned_repair_weapon, FALSE);
        update_owned_add_button();
        g_owned_panel_rebuilding = false;
        SendMessageW(g_owned_affix_list, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(g_owned_affix_list,NULL,FALSE);
        return;
    }
    if (g_owned_panel_instance != weapon.instance) {
        g_owned_selected_affix_id[0] = 0;
        g_owned_selected_builtin = -1;
    }
    g_owned_panel_instance = weapon.instance;
    const bool other_session = g_owned_transaction && g_owned_saved_item != weapon.instance;
    EnableWindow(g_owned_duplicate_weapon, !weapon.is_gear && !g_owned_edit_pending && !g_owned_switch_target && !g_owned_edit_faulted && !g_owned_duplicate_faulted);
    EnableWindow(g_owned_repair_weapon, !weapon.is_gear && !g_owned_edit_pending && !g_owned_switch_target && !g_owned_edit_faulted);
    EnableWindow(g_owned_clear_affixes, weapon.affix_count && !other_session && !g_owned_edit_pending && !g_owned_switch_target && !g_owned_edit_faulted);
    update_owned_add_button();
    for (unsigned i = 0; i < weapon.affix_count; ++i) {
        if (g_owned_selected_affix_id[0] &&
            !_stricmp(g_owned_selected_affix_id, weapon.affixes[i].id)) old_affix = (int)i;
        wchar_t name[180] = {}, magnitude[48] = {};
        const OwnedAffixMetadata* metadata = owned_affix_metadata(weapon.affixes[i].id);
        ascii_to_wide(metadata ? metadata->name : weapon.affixes[i].id,
                      name, (int)_countof(name));
        if (weapon.affixes[i].stat_change)
            format_owned_affix_value(metadata,
                weapon.affixes[i].raw * weapon.affixes[i].factor,
                magnitude, _countof(magnitude));
        else
            wcscpy_s(magnitude, L"Unknown");
        LVITEMW row = {};
        row.mask = LVIF_TEXT;
        row.iItem = (int)i;
        row.pszText = name;
        ListView_InsertItem(g_owned_affix_list, &row);
        ListView_SetItemText(g_owned_affix_list, (int)i, 1, magnitude);
    }
    const OwnedBuiltInStat* built_in[16] = {};
    const unsigned built_in_count = owned_builtin_for_item(weapon.id, built_in,
                                                          (unsigned)_countof(built_in));
    if (g_owned_selected_builtin >= 0 && g_owned_selected_builtin < (int)built_in_count)
        old_affix = (int)weapon.affix_count + g_owned_selected_builtin;
    for (unsigned i = 0; i < built_in_count && i < _countof(built_in); ++i) {
        const OwnedBuiltInStat* stat = built_in[i];
        wchar_t name[200] = {}, value[48] = {};
        wchar_t localized[170] = {};
        ascii_to_wide(stat->name, localized, (int)_countof(localized));
        _snwprintf_s(name, _countof(name), _TRUNCATE, L"Built-in: %s", localized);
        wchar_t authored[32] = {};
        ascii_to_wide(stat->authored_value, authored, (int)_countof(authored));
        const wchar_t* suffix = !_stricmp(stat->unit, "Percent") ? L"%" :
                                !_stricmp(stat->unit, "Seconds") ? L"s" : L"";
        _snwprintf_s(value, _countof(value), _TRUNCATE, L"%s%s%s", authored,
                     suffix, stat->scalable ? L"*" : L"");
        LVITEMW row = {};
        row.mask = LVIF_TEXT;
        row.iItem = (int)(weapon.affix_count + i);
        row.pszText = name;
        ListView_InsertItem(g_owned_affix_list, &row);
        ListView_SetItemText(g_owned_affix_list, row.iItem, 1, value);
    }
    g_owned_panel_rebuilding = false;
    if (old_affix >= 0 && old_affix < (int)(weapon.affix_count + built_in_count)) {
        ListView_SetItemState(g_owned_affix_list, old_affix,
                              LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        owned_affix_selection_changed();
    } else {
        g_owned_selected_affix_id[0] = 0;
        g_owned_selected_builtin = -1;
        SetWindowTextW(g_owned_value_input, L"");
        SetWindowTextW(g_owned_affix_detail,
            weapon.affix_count ? L"Select an editable affix or a read-only built-in stat."
                               : L"No editable affixes. Built-in stats are read-only.");
        EnableWindow(g_owned_apply_value, FALSE);
        EnableWindow(g_owned_remove_affix, FALSE);
    }
    SendMessageW(g_owned_affix_list,WM_SETREDRAW,TRUE,0);
    InvalidateRect(g_owned_affix_list,NULL,FALSE);
    if (g_owned_edit_pending || g_owned_switch_target || g_owned_edit_faulted) {
        SetWindowTextW(g_owned_affix_detail,g_owned_edit_faulted ?
            L"Native action faulted. Restart game and tool before editing." :
            L"Applying one action; please wait.");
        EnableWindow(g_owned_apply_value,FALSE);
        EnableWindow(g_owned_remove_affix,FALSE);
    } else if (other_session) {
        SetWindowTextW(g_owned_affix_detail,L"Finish the previous item's edit first:\r\nKeep Changes or Restore Original, then edit this piece.");
        EnableWindow(g_owned_apply_value,FALSE);
        EnableWindow(g_owned_remove_affix,FALSE);
    }
}

static bool owned_text_contains(const char* haystack, const char* needle) {
    if (!needle || !needle[0]) return true;
    if (!haystack) return false;
    const size_t length = strlen(needle);
    for (const char* at = haystack; *at; ++at)
        if (!_strnicmp(at, needle, length)) return true;
    return false;
}

static bool owned_affix_passes_filters(const char* id, const OwnedWeaponView* selected) {
    const OwnedAffixMetadata* metadata = owned_affix_metadata(id);
    const int format = (int)SendMessageW(g_owned_format_filter,CB_GETCURSEL,0,0);
    const bool percent = metadata && strstr(metadata->value_format,"%1s%") != NULL;
    if ((format==1 && !percent) || (format==2 && percent)) return false;
    const int presence = (int)SendMessageW(g_owned_presence_filter,CB_GETCURSEL,0,0);
    if (presence>0) {
        if (!selected || !selected->instance) return false;
        bool found=false;
        for (unsigned i=0;i<selected->affix_count;++i)
            if (!_stricmp(selected->affixes[i].id,id)) { found=true; break; }
        if ((presence==1 && found) || (presence==2 && !found)) return false;
    }
    return true;
}

static void populate_owned_donors() {
    if (!g_owned_donor_combo) return;
    g_owned_population_busy = true;
    OwnedWeaponView target = {};
    selected_owned_weapon(&target);
    const int previous_catalogue_selection = g_owned_catalog_mode
        ? (int)SendMessageW(g_owned_donor_combo, CB_GETCURSEL, 0, 0) : -1;
    const unsigned previous_catalogue_id = previous_catalogue_selection >= 0 &&
        previous_catalogue_selection < (int)g_owned_catalog_visible_count
        ? g_owned_catalog_visible[previous_catalogue_selection] : (unsigned)-1;
    const int previous_donor_selection = !g_owned_catalog_mode
        ? (int)SendMessageW(g_owned_donor_combo,CB_GETCURSEL,0,0) : -1;
    OwnedDonorChoice previous_donor = {};
    if (previous_donor_selection >= 0 && previous_donor_selection < (int)g_owned_donor_count)
        previous_donor = g_owned_donors[previous_donor_selection];
    wchar_t query_wide[96] = {};
    char query[256] = {};
    if (g_owned_catalog_search)
        GetWindowTextW(g_owned_catalog_search, query_wide, (int)_countof(query_wide));
    WideCharToMultiByte(CP_UTF8,0,query_wide,-1,query,(int)_countof(query),NULL,NULL);
    SendMessageW(g_owned_donor_combo, CB_RESETCONTENT, 0, 0);
    g_owned_donor_count = 0;
    g_owned_catalog_visible_count = 0;
    if (g_owned_catalog_mode) {
        for (unsigned i = 0; i < g_owned_affix_metadata_count; ++i) {
            const OwnedAffixMetadata* entry = &g_owned_affix_metadata[i];
            if (!owned_affix_passes_filters(entry->id,&target)) continue;
            if (!owned_text_contains(entry->name, query) &&
                !owned_text_contains(entry->id, query)) continue;
            wchar_t name[170] = {}, id[138] = {}, label[340] = {};
            ascii_to_wide(entry->name, name, (int)_countof(name));
            ascii_to_wide(entry->id, id, (int)_countof(id));
            _snwprintf_s(label, _countof(label), _TRUNCATE, L"%s  |  %s%s",
                         name, id, entry->game_localized ? L"" : L"  [ID label]");
            SendMessageW(g_owned_donor_combo, CB_ADDSTRING, 0, (LPARAM)label);
            g_owned_catalog_visible[g_owned_catalog_visible_count++] = i;
        }
        if (g_owned_catalog_visible_count) {
            unsigned selected = 0;
            for (unsigned i = 0; i < g_owned_catalog_visible_count; ++i)
                if (g_owned_catalog_visible[i] == previous_catalogue_id) {
                    selected = i; break;
                }
            SendMessageW(g_owned_donor_combo, CB_SETCURSEL, selected, 0);
        }
        g_owned_population_busy = false;
        update_owned_add_button();
        return;
    }
    unsigned restored_donor = 0;
    EnterCriticalSection(&g_data_lock);
    for (DWORD i = 0; i < g_owned_count; ++i) {
        const OwnedWeaponView* weapon = &g_owned_weapons[i];
        for (unsigned j = 0; j < weapon->affix_count &&
             g_owned_donor_count < _countof(g_owned_donors); ++j) {
            const OwnedAffixView* affix = &weapon->affixes[j];
            if (!affix->stat_change) continue;
            if (!owned_affix_passes_filters(affix->id,&target)) continue;
            const OwnedAffixMetadata* metadata = owned_affix_metadata(affix->id);
            if (!owned_text_contains(metadata ? metadata->name : affix->id,query) &&
                !owned_text_contains(affix->id,query) &&
                !owned_text_contains(weapon->name,query) &&
                !owned_text_contains(weapon->id,query)) continue;
            OwnedDonorChoice* choice = &g_owned_donors[g_owned_donor_count++];
            choice->instance = weapon->instance;
            strcpy_s(choice->item_id, weapon->id);
            strcpy_s(choice->affix_id, affix->id);
            choice->affix_index = j;
            choice->raw = affix->raw;
            if (choice->instance == previous_donor.instance &&
                !_stricmp(choice->affix_id,previous_donor.affix_id))
                restored_donor = g_owned_donor_count - 1;
            wchar_t name[180] = {}, affix_name[180] = {}, affix_id[140] = {}, text[512] = {};
            ascii_to_wide(weapon->name, name, (int)_countof(name));
            ascii_to_wide(metadata ? metadata->name : affix->id,
                          affix_name, (int)_countof(affix_name));
            ascii_to_wide(affix->id,affix_id,(int)_countof(affix_id));
            _snwprintf_s(text, _countof(text), _TRUNCATE,
                L"%s  |  %s  ·  %.9g  ·  %s #%u", affix_name,affix_id,
                affix->raw * affix->factor,
                name, weapon->same_id_ordinal);
            SendMessageW(g_owned_donor_combo, CB_ADDSTRING, 0, (LPARAM)text);
        }
    }
    LeaveCriticalSection(&g_data_lock);
    if (g_owned_donor_count) SendMessageW(g_owned_donor_combo, CB_SETCURSEL, restored_donor, 0);
    g_owned_population_busy = false;
    update_owned_add_button();
}

static void set_owned_add_source(bool catalogue) {
    g_owned_catalog_mode = catalogue;
    if (g_owned_donor_label)
        ShowWindow(g_owned_donor_label,SW_HIDE);
    if (g_owned_catalog_search)
        ShowWindow(g_owned_catalog_search,
                   g_owned_mode ? SW_SHOW : SW_HIDE);
    ShowWindow(g_owned_catalog_search_backdrop, g_owned_mode ? SW_SHOW : SW_HIDE);
    ShowWindow(g_owned_catalog_value_backdrop, g_owned_mode && catalogue ? SW_SHOW : SW_HIDE);
    if (g_owned_catalog_test)
        SetWindowTextW(g_owned_catalog_test, catalogue ? L"Copy Owned" : L"All Affixes");
    if (g_owned_add_affix)
        SetWindowTextW(g_owned_add_affix, catalogue ? L"Add Affix" : L"Copy Affix");
    if (g_owned_catalog_value_input)
        ShowWindow(g_owned_catalog_value_input,
                   g_owned_mode && catalogue ? SW_SHOW : SW_HIDE);
    populate_owned_donors();
    if (g_window) layout_item_catalogue(g_window);
}

static void set_owned_view(bool enabled, bool gear = false) {
    if (g_item_list) ListView_SetItemState(g_item_list, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    if (enabled && (!g_owned_mode || gear != g_owned_gear_mode)) {
        // Enter the owned view unfiltered so duplicate instances cannot look missing.
        SetWindowTextW(g_search, L"");
        SendMessageW(g_category_filter, CB_SETCURSEL, 0, 0);
        SendMessageW(g_rarity_filter, CB_SETCURSEL, 0, 0);
        SendMessageW(g_view_filter, CB_SETCURSEL, 0, 0);
        SendMessageW(g_weapon_level_filter, CB_SETCURSEL, 0, 0);
    }
    g_owned_mode = enabled;
    g_owned_gear_mode = enabled && gear;
    const HWND affix_filters[] = {g_owned_filter_label,g_owned_format_filter,g_owned_presence_filter};
    for (HWND filter : affix_filters)
        if (filter) ShowWindow(filter,enabled ? SW_SHOW : SW_HIDE);
    const HWND catalog_actions[] = {
        g_quantity_label, g_quantity_minus, g_quantity_backdrop, g_quantity,
        g_quantity_plus, g_actions_label, g_give_inventory, g_give_stash,
        g_favorite, g_copy_id, g_remove_inventory
    };
    for (HWND control : catalog_actions)
        if (control) ShowWindow(control, enabled ? SW_HIDE : SW_SHOW);
    const HWND editor_controls[] = { g_owned_affix_list, g_owned_affix_detail,
        g_owned_value_label, g_owned_value_backdrop,
        g_owned_value_input, g_owned_apply_value, g_owned_donor_label,
        g_owned_donor_combo, g_owned_add_affix, g_owned_catalog_test,
        g_owned_remove_affix, g_owned_clear_affixes, g_owned_duplicate_weapon, g_owned_repair_weapon,
        g_owned_restore, g_owned_keep };
    for (HWND control : editor_controls)
        if (control) ShowWindow(control, enabled ? SW_SHOW : SW_HIDE);
    ShowWindow(g_owned_duplicate_weapon, enabled && !gear ? SW_SHOW : SW_HIDE);
    ShowWindow(g_owned_repair_weapon, enabled && !gear ? SW_SHOW : SW_HIDE);
    if (g_owned_donor_label)
        ShowWindow(g_owned_donor_label,SW_HIDE);
    if (g_owned_catalog_search)
        ShowWindow(g_owned_catalog_search,
                   enabled ? SW_SHOW : SW_HIDE);
    if (g_owned_catalog_value_input)
        ShowWindow(g_owned_catalog_value_input,
                   enabled && g_owned_catalog_mode ? SW_SHOW : SW_HIDE);
    ShowWindow(g_owned_catalog_search_backdrop, enabled ? SW_SHOW : SW_HIDE);
    ShowWindow(g_owned_catalog_value_backdrop, enabled && g_owned_catalog_mode ? SW_SHOW : SW_HIDE);
    HWND refresh = g_window ? GetDlgItem(g_window, IDC_REFRESH_CATALOG) : NULL;
    if (refresh) SetWindowTextW(refresh, enabled ? L"Scan" : L"Refresh");
    layout_item_catalogue(g_window);
    if (g_catalog_title) SetWindowTextW(g_catalog_title,
                                        enabled ? (gear ? L"Owned Gear" : L"Owned Weapons") : L"Item Catalogue");
    if (g_catalog_subtitle) SetWindowTextW(g_catalog_subtitle,
        enabled ? (gear ? L"Worn gear first; exact carried instances and native affixes" : L"Live carried weapons, editable affixes, and built-in stats")
                : L"Browse, inspect, and manage game items");
    if (g_selected_label) SetWindowTextW(g_selected_label,
                                         enabled ? (gear ? L"Owned Gear" : L"Owned Weapon") : L"Item Details");
    HWND header = g_item_list ? ListView_GetHeader(g_item_list) : NULL;
    if (header) {
        HDITEMW column = {};
        column.mask = HDI_TEXT;
        column.pszText = enabled ? (LPWSTR)L"Affixes" : (LPWSTR)L"Source";
        Header_SetItem(header, 5, &column);
        column.pszText = enabled && gear ? (LPWSTR)L"State" : (LPWSTR)L"Lvl";
        Header_SetItem(header,4,&column);
    }
    if (g_tab_all_items) InvalidateRect(g_tab_all_items, NULL, FALSE);
    if (g_tab_favorites) InvalidateRect(g_tab_favorites, NULL, FALSE);
    if (g_tab_owned_weapons) InvalidateRect(g_tab_owned_weapons, NULL, FALSE);
    if (g_tab_owned_gear) InvalidateRect(g_tab_owned_gear, NULL, FALSE);
    refresh_item_view();
}

static bool request_owned_scan() {
    HWND game_window = find_game_window();
    if (!game_window || !install_game_thread_dispatch(game_window) ||
        !PostMessageW(game_window, WM_BEAST_OWNED_SCAN, 0, 0)) {
        set_status(L"Owned item scan could not reach the game window.");
        return false;
    }
    set_status(L"Reading carried weapon and gear instances and native affixes.");
    return true;
}

static void owned_affix_selection_changed() {
    if (g_owned_panel_rebuilding) return;
    OwnedWeaponView weapon = {};
    const int affix = g_owned_affix_list
        ? ListView_GetNextItem(g_owned_affix_list, -1, LVNI_SELECTED) : -1;
    if (!selected_owned_weapon(&weapon) || affix < 0) {
        g_owned_selected_affix_id[0] = 0;
        g_owned_selected_builtin = -1;
        SetWindowTextW(g_owned_value_input, L"");
        SetWindowTextW(g_owned_affix_detail, L"Select an affix to inspect its exact ID and raw value.");
        EnableWindow(g_owned_apply_value, FALSE);
        EnableWindow(g_owned_remove_affix, FALSE);
        return;
    }
    g_owned_selected_affix_id[0] = 0;
    g_owned_selected_builtin = -1;
    if (affix < (int)weapon.affix_count)
        strcpy_s(g_owned_selected_affix_id, weapon.affixes[affix].id);
    else g_owned_selected_builtin = affix - (int)weapon.affix_count;
    if (g_owned_edit_pending || g_owned_switch_target || g_owned_edit_faulted ||
        (g_owned_transaction && g_owned_saved_item != weapon.instance)) {
        SetWindowTextW(g_owned_affix_detail,(g_owned_edit_pending || g_owned_switch_target) ? L"Applying one action; please wait." :
            g_owned_edit_faulted ? L"Native action faulted. Restart game and tool before editing." :
            L"Finish the previous item's edit first:\r\nKeep Changes or Restore Original, then edit this piece.");
        EnableWindow(g_owned_apply_value,FALSE); EnableWindow(g_owned_remove_affix,FALSE);
        return;
    }
    if (affix >= (int)weapon.affix_count) {
        const OwnedBuiltInStat* built_in[16] = {};
        const unsigned count = owned_builtin_for_item(weapon.id, built_in,
                                                     (unsigned)_countof(built_in));
        const unsigned index = (unsigned)affix - weapon.affix_count;
        wchar_t detail[360] = {}, name[170] = {}, value[40] = {};
        if (index < count && index < _countof(built_in)) {
            const OwnedBuiltInStat* stat = built_in[index];
            ascii_to_wide(stat->name, name, (int)_countof(name));
            ascii_to_wide(stat->authored_value, value, (int)_countof(value));
            _snwprintf_s(detail, _countof(detail), _TRUNCATE,
                L"Built-in: %s\r\nAuthored %s%s; read-only.", name, value,
                stat->scalable ? L" (game-scaled)" : L"");
        } else {
            wcscpy_s(detail, L"Built-in stat unavailable.");
        }
        SetWindowTextW(g_owned_affix_detail, detail);
        SetWindowTextW(g_owned_value_input, L"");
        EnableWindow(g_owned_apply_value, FALSE);
        EnableWindow(g_owned_remove_affix, FALSE);
        return;
    }
    const OwnedAffixView* current = &weapon.affixes[affix];
    const OwnedAffixMetadata* metadata = owned_affix_metadata(current->id);
    wchar_t id[160] = {}, detail[360] = {};
    ascii_to_wide(current->id, id, (int)_countof(id));
    if (!current->stat_change) {
        _snwprintf_s(detail, _countof(detail), _TRUNCATE,
            L"%s\r\nValue type unknown; removal is available.", id);
        SetWindowTextW(g_owned_affix_detail, detail);
        SetWindowTextW(g_owned_value_input, L"");
        EnableWindow(g_owned_apply_value, FALSE);
        EnableWindow(g_owned_remove_affix, TRUE);
        return;
    }
    EnableWindow(g_owned_apply_value, TRUE);
    EnableWindow(g_owned_remove_affix, TRUE);
    wchar_t shown[56] = {};
    format_owned_affix_value(metadata, current->raw * current->factor,
                             shown, _countof(shown));
    _snwprintf_s(detail, _countof(detail), _TRUNCATE,
        L"%s%s\r\n%s  |  raw %g x factor %g", id,
        metadata && !metadata->game_localized ? L" (ID label)" : L"",
        shown, current->raw, current->factor);
    SetWindowTextW(g_owned_affix_detail, detail);
    wchar_t text[48] = {};
    _snwprintf_s(text, _countof(text), _TRUNCATE, L"%.9g",
                 current->raw * current->factor);
    SetWindowTextW(g_owned_value_input, text);
}

static bool read_owned_magnitude(HWND control,float* value) {
    wchar_t input[64] = {};
    GetWindowTextW(control,input,_countof(input));
    wchar_t* end=NULL;
    errno=0;
    const double parsed=wcstod(input,&end);
    if (end==input || errno==ERANGE || !_finite(parsed)) return false;
    while (*end && iswspace(*end)) ++end;
    const float result=(float)parsed;
    if (*end || !_finite(result) || (parsed!=0.0 && result==0.0f)) return false;
    *value=result;
    return true;
}

static void request_owned_edit(OwnedEditKind kind, bool keep_confirmed) {
    if (g_owned_edit_pending) { set_status(L"Affix action is already in progress."); return; }
    if (g_owned_edit_faulted) { set_status(L"Restart game and tool before another native action."); return; }
    if (kind == OWNED_EDIT_DUPLICATE && g_owned_duplicate_faulted) {
        set_status(L"Further copies are blocked after verification failed. Restart game/tool before duplicating again."); return;
    }
    if (kind == OWNED_EDIT_KEEP && !keep_confirmed && MessageBoxW(g_window,
        L"Keep the current live affixes and discard Restore Original for this edit session? This does not itself save the game. Keep a separate save backup before testing extreme values.",
        L"Finish affix edit", MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2) != IDYES)
        return;
    OwnedEditRequest request = {};
    request.kind = kind;
    if (kind != OWNED_EDIT_KEEP && kind != OWNED_EDIT_RESTORE) {
        OwnedWeaponView weapon = {};
        if (!selected_owned_weapon(&weapon)) {
            set_status(L"Select an owned item instance first."); return;
        }
        request.target = weapon.instance;
        request.is_gear = weapon.is_gear;
        strcpy_s(request.item_id, weapon.id);
        request.expected_affix_count = weapon.affix_count;
        memcpy(request.expected_affixes, weapon.affixes,
               sizeof(request.expected_affixes));
        if (kind == OWNED_EDIT_ADD_CATALOG_TEST) {
            if (!g_owned_lookup_probe_passed) {
                set_status(L"Catalogue add requires a successful game-definition lookup.");
                return;
            }
            const int selected = (int)SendMessageW(g_owned_donor_combo, CB_GETCURSEL, 0, 0);
            if (!g_owned_catalog_mode || selected < 0 ||
                selected >= (int)g_owned_catalog_visible_count) {
                set_status(L"Select a game-sourced affix from the catalogue first."); return;
            }
            strcpy_s(request.affix_id,
                     g_owned_affix_metadata[g_owned_catalog_visible[selected]].id);
            float value=0.0f;
            if (!read_owned_magnitude(g_owned_catalog_value_input,&value)) {
                set_status(L"Enter a finite float32 magnitude; scientific notation is supported."); return;
            }
            request.new_effective = value;
        } else if (kind == OWNED_EDIT_ADD) {
            const int donor = (int)SendMessageW(g_owned_donor_combo, CB_GETCURSEL, 0, 0);
            if (donor < 0 || donor >= (int)g_owned_donor_count) {
                set_status(L"Select a real affix donor first."); return;
            }
            request.donor = g_owned_donors[donor];
        } else if (kind == OWNED_EDIT_CLEAR) {
            if (!weapon.affix_count) {
                set_status(L"This item already has no native affixes."); return;
            }
            const int confirm = MessageBoxW(g_window,
                L"Remove every native affix from this item? Restore Original remains available only while this exact item instance stays carried. The game may save changes independently.",
                L"Clear item affixes", MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
            if (confirm != IDYES) return;
        } else if (kind == OWNED_EDIT_DUPLICATE || kind == OWNED_EDIT_REPAIR) {
            if (weapon.is_gear) { set_status(L"Duplicate and Repair are verified only for weapons."); return; }
            // Full native copy needs only the exact owned-instance snapshot,
            // not an affix-row selection or a catalogue Give request.
            if (g_owned_edit_faulted) {
                set_status(L"Restart game and tool before another native action."); return;
            }
        } else {
            const int affix = ListView_GetNextItem(g_owned_affix_list, -1, LVNI_SELECTED);
            if (affix < 0 || affix >= (int)weapon.affix_count ||
                (kind == OWNED_EDIT_VALUE && !weapon.affixes[affix].stat_change)) {
                set_status(L"Select a native affix line first."); return;
            }
            request.affix_index = (unsigned)affix;
            strcpy_s(request.affix_id, weapon.affixes[affix].id);
            request.expected_raw = weapon.affixes[affix].raw;
            request.expected_effect_vtable = weapon.affixes[affix].effect_vtable;
            if (kind == OWNED_EDIT_VALUE) {
                float number=0.0f, encoded=0.0f;
                if (!read_owned_magnitude(g_owned_value_input,&number) ||
                    !owned_encode_magnitude(number,weapon.affixes[affix].factor,&encoded)) {
                    set_status(L"Value exceeds float32 storage after this affix's factor conversion."); return;
                }
                request.new_effective = number;
            }
        }
    }
    if ((kind==OWNED_EDIT_VALUE || kind==OWNED_EDIT_ADD_CATALOG_TEST) &&
        (request.new_effective>1000.0f || request.new_effective<-1000.0f) &&
        !g_owned_extremes_confirmed) {
        if (MessageBoxW(g_window,
            L"Extreme-value testing: float32 storage is not a safe gameplay range. Large rolls can overflow later game calculations, destabilize gameplay or be saved automatically. Use disposable gear and a backed-up save. Continue this session with values beyond 1000?",
            L"Experimental affix values",MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2)!=IDYES) return;
        g_owned_extremes_confirmed=true;
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
    update_owned_affix_panel();
    set_status(kind == OWNED_EDIT_DUPLICATE
        ? L"Duplicating one selected weapon with its native instance state."
        : kind == OWNED_EDIT_REPAIR ? L"Repairing the selected weapon using one native repair slot."
        : L"Applying one native affix action to the selected instance.");
}

// The backend owns a single deep rollback snapshot. Never silently keep it or
// apply it to another instance just because the user selects another row.
static OwnedEditKind owned_switch_operation(int answer) {
    return answer == IDYES ? OWNED_EDIT_KEEP : answer == IDNO ? OWNED_EDIT_RESTORE : (OwnedEditKind)0;
}

static void offer_owned_item_switch() {
    OwnedWeaponView next = {};
    if (!selected_owned_weapon(&next)) return;
    if (g_owned_switch_target) {
        // The user's latest selection wins while the approved finish completes.
        g_owned_switch_target = next.instance;
        return;
    }
    if (g_owned_edit_pending) {
        g_owned_switch_offer_target = next.instance;
        return;
    }
    g_owned_switch_offer_target = NULL;
    if (!g_owned_transaction || g_owned_edit_faulted ||
        g_owned_saved_item == next.instance) return;
    wchar_t previous[180] = {};
    ascii_to_wide(g_owned_saved_id,previous,_countof(previous));
    EnterCriticalSection(&g_data_lock);
    for (DWORD i=0;i<g_owned_count;++i)
        if (g_owned_weapons[i].instance==g_owned_saved_item)
            ascii_to_wide(g_owned_weapons[i].name,previous,_countof(previous));
    LeaveCriticalSection(&g_data_lock);
    wchar_t prompt[640] = {};
    _snwprintf_s(prompt,_countof(prompt),_TRUNCATE,
        L"Finish editing %s before editing another item.\r\n\r\nYes: Keep its current affixes and discard its rollback snapshot.\r\nNo: Restore its original affixes.\r\nCancel: Return to the previous item.\r\n\r\nThis does not save the game. The newly selected item will not be changed.",previous);
    const OwnedEditKind operation = owned_switch_operation(MessageBoxW(g_window,prompt,
        L"Switch affix target",MB_YESNOCANCEL | MB_ICONQUESTION | MB_DEFBUTTON3));
    if (!operation) {
        if (!select_owned_instance(g_owned_saved_item))
            set_status(L"Previous edited item is hidden by filters. Keep Changes or Restore Original finishes its edit.");
        return;
    }
    g_owned_switch_target = next.instance;
    request_owned_edit(operation,true);
    if (!g_owned_edit_pending) {
        g_owned_switch_target = NULL;
        select_owned_instance(g_owned_saved_item);
    }
}

static RECT owned_affix_popup_rect(const RECT& anchor,const RECT& work,int width,int count) {
    const int margin=ui_scale(12),gap=ui_scale(4);
    const int available_width=max(1,work.right-work.left-2*margin);
    width=max(1,min(width,available_width));
    const int left=max(work.left+margin,min(anchor.right-width,work.right-width-margin));
    const int desired=ui_scale(48)*max(1,min(count,8))+ui_scale(4);
    const int above=max(0,anchor.top-work.top-margin-gap);
    const int below=max(0,work.bottom-anchor.bottom-margin-gap);
    const bool up=above>=min(desired,ui_scale(240)) || above>=below;
    const int height=max(1,min(desired,up ? above : below));
    const int top=up ? anchor.top-height-gap : anchor.bottom+gap;
    RECT bounds={left,top,left+width,top+height};
    return bounds;
}

static void prepare_owned_affix_popup() {
    g_owned_popup_prepared=false;
    if (!g_owned_donor_combo) return;
    RECT anchor = {}; GetWindowRect(g_owned_donor_combo,&anchor);
    MONITORINFO monitor = {sizeof(monitor)};
    if (!GetMonitorInfoW(MonitorFromWindow(g_window,MONITOR_DEFAULTTONEAREST),&monitor)) return;
    const int count=(int)SendMessageW(g_owned_donor_combo,CB_GETCOUNT,0,0);
    HDC dc=GetDC(g_owned_donor_combo);
    HGDIOBJ previous=SelectObject(dc,g_input_font);
    int width=ui_scale(640);
    for (int i=0;i<count;++i) {
        wchar_t text[512] = {};
        if (SendMessageW(g_owned_donor_combo,CB_GETLBTEXTLEN,i,0)>=(LRESULT)_countof(text)) continue;
        SendMessageW(g_owned_donor_combo,CB_GETLBTEXT,i,(LPARAM)text);
        wchar_t* second=wcsstr(text,L"  |  ");
        if (second) { *second=0; second+=5; }
        SIZE size={}; GetTextExtentPoint32W(dc,text,(int)wcslen(text),&size);
        width=max(width,size.cx+ui_scale(44));
        if (second) {
            SelectObject(dc,g_mono_font);
            GetTextExtentPoint32W(dc,second,(int)wcslen(second),&size);
            width=max(width,size.cx+ui_scale(44));
            SelectObject(dc,g_input_font);
        }
    }
    SelectObject(dc,previous); ReleaseDC(g_owned_donor_combo,dc);
    g_owned_popup_bounds=owned_affix_popup_rect(anchor,monitor.rcWork,width,count);
    g_owned_popup_prepared=true;
    SendMessageW(g_owned_donor_combo,CB_SETDROPPEDWIDTH,
        g_owned_popup_bounds.right-g_owned_popup_bounds.left,0);
}

static LRESULT CALLBACK owned_affix_popup_proc(HWND list,UINT message,WPARAM wparam,LPARAM lparam,
                                               UINT_PTR,DWORD_PTR) {
    if (message==WM_WINDOWPOSCHANGING) {
        // Amend the native list's initial placement BEFORE it becomes visible.
        // No posted correction, second SetWindowPos, or focus activation.
        const LRESULT result=DefSubclassProc(list,message,wparam,lparam);
        WINDOWPOS* position=(WINDOWPOS*)lparam;
        if (position && !(position->flags & SWP_HIDEWINDOW) &&
            ((position->flags & SWP_SHOWWINDOW) || IsWindowVisible(list))) {
            if (!g_owned_popup_prepared) prepare_owned_affix_popup();
            if (g_owned_popup_prepared) {
                position->x=g_owned_popup_bounds.left;
                position->y=g_owned_popup_bounds.top;
                position->cx=g_owned_popup_bounds.right-g_owned_popup_bounds.left;
                position->cy=g_owned_popup_bounds.bottom-g_owned_popup_bounds.top;
                position->flags &= ~(SWP_NOMOVE | SWP_NOSIZE);
            }
        }
        return result;
    }
    if (message==WM_NCDESTROY) RemoveWindowSubclass(list,owned_affix_popup_proc,1);
    return DefSubclassProc(list,message,wparam,lparam);
}

static bool install_owned_affix_popup_style() {
    COMBOBOXINFO info={sizeof(info)};
    return GetComboBoxInfo(g_owned_donor_combo,&info) && info.hwndList &&
        SetWindowSubclass(info.hwndList,owned_affix_popup_proc,1,0);
}

static void draw_owned_affix_choice(const DRAWITEMSTRUCT* draw) {
    const int saved=SaveDC(draw->hDC);
    const bool selected=(draw->itemState & ODS_SELECTED)!=0;
    HBRUSH brush=CreateSolidBrush(selected ? RGB(24,43,49) : kColorPanel);
    FillRect(draw->hDC,&draw->rcItem,brush); DeleteObject(brush);
    wchar_t text[512] = {};
    if (draw->itemID!=(UINT)-1 &&
        SendMessageW(g_owned_donor_combo,CB_GETLBTEXTLEN,draw->itemID,0)<(LRESULT)_countof(text))
        SendMessageW(g_owned_donor_combo,CB_GETLBTEXT,draw->itemID,(LPARAM)text);
    wchar_t* second=wcsstr(text,L"  |  ");
    if (second) { *second=0; second+=5; }
    RECT name=draw->rcItem; name.left+=ui_scale(12); name.right-=ui_scale(12);
    SetBkMode(draw->hDC,TRANSPARENT);
    SelectObject(draw->hDC,g_input_font); SetTextColor(draw->hDC,kColorText);
    if (draw->itemState & ODS_COMBOBOXEDIT) {
        DrawTextW(draw->hDC,text,-1,&name,DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
    } else {
        name.top+=ui_scale(5); name.bottom=name.top+ui_scale(19);
        DrawTextW(draw->hDC,text,-1,&name,DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
        if (second) {
            RECT id=name; id.top=name.bottom+ui_scale(2); id.bottom=id.top+ui_scale(17);
            SelectObject(draw->hDC,g_mono_font); SetTextColor(draw->hDC,selected ? RGB(151,217,213) : kColorMuted);
            DrawTextW(draw->hDC,second,-1,&id,DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
        }
    }
    RestoreDC(draw->hDC,saved);
}
#endif

#ifdef IE_DLTB_BRIDGE
#include "DltbBridge.inl"
#endif

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_IE_MENU_SCOPE: handle_catalogue_scope_request((unsigned)wparam); return 0;
    case WM_SHOWWINDOW:
        // Owner minimize/restore and automatic scope changes aren't user closes.
        if (!g_menu_scope_transition && lparam == 0) g_menu_open_requested = wparam != 0;
        break;
    case WM_ACTIVATEAPP:
        if (!wparam) PostMessageW(hwnd,WM_IE_MENU_SCOPE,0,0);
        break;
#ifdef IE_OWNED_AFFIX_EDITOR
    case WM_MEASUREITEM: {
        MEASUREITEMSTRUCT* measure=(MEASUREITEMSTRUCT*)lparam;
        if (measure && measure->CtlID==IDC_OWNED_DONOR_COMBO) {
            measure->itemHeight=ui_scale(48); return TRUE;
        }
        break;
    }
    case WM_BEAST_OWNED_SCAN_RESULT:
        if (wparam == 0) {
            if (g_owned_switch_target && !g_owned_edit_pending && !g_owned_transaction) {
                g_owned_panel_instance = g_owned_switch_target;
                g_owned_switch_target = NULL;
                g_owned_selected_affix_id[0] = 0;
                g_owned_selected_builtin = -1;
            }
            if (g_owned_mode) refresh_owned_view();
            populate_owned_donors();
            EnableWindow(g_owned_restore, g_owned_transaction ? TRUE : FALSE);
            EnableWindow(g_owned_keep,
                         g_owned_transaction ? TRUE : FALSE);
            wchar_t status[128] = {};
            if (g_owned_refresh_stale_target) {
                wcscpy_s(status, L"Weapon changed in-game. Select its refreshed row and retry; no edit was made.");
                g_owned_refresh_stale_target = false;
            } else if (lparam)
                wcscpy_s(status, L"Weapon instance or affixes changed; stale rollback discarded. Current state is a new baseline.");
            else
                _snwprintf_s(status, _countof(status), _TRUNCATE,
                             L"Read %lu carried weapon and gear instances and their native affixes.",
                             g_owned_count);
            set_status(status);
        } else {
            if (!g_owned_edit_pending) g_owned_switch_target = NULL;
            g_owned_refresh_stale_target = false;
            EnterCriticalSection(&g_data_lock);
            g_owned_count = 0;
            LeaveCriticalSection(&g_data_lock);
            populate_owned_donors();
            if (g_owned_mode) refresh_owned_view();
            set_status(L"Owned item scan rejected the current game state. See [OWNED_AFFIX] in the log.");
        }
        return 0;
    case WM_BEAST_OWNED_EDIT_RESULT: {
        g_owned_edit_pending = false;
        if (wparam) g_owned_switch_target = NULL;
        EnableWindow(g_owned_restore, g_owned_transaction ? TRUE : FALSE);
        EnableWindow(g_owned_keep,
                     g_owned_transaction ? TRUE : FALSE);
        if (wparam == 0) {
            if (g_owned_rebound_during_edit) {
                // The internal safety scan replaced the snapshot under the old
                // row indices. Do not retain a selection by an unrelated index.
                ListView_SetItemState(g_item_list, -1, 0,
                                      LVIS_SELECTED | LVIS_FOCUSED);
                g_owned_rebound_during_edit = false;
            }
            const wchar_t* action = (OwnedEditKind)lparam == OWNED_EDIT_VALUE ? L"Value changed" :
                (OwnedEditKind)lparam == OWNED_EDIT_DUPLICATE ? L"One independent weapon copy created; original unchanged" :
                (OwnedEditKind)lparam == OWNED_EDIT_REPAIR ? L"Weapon repaired to full durability; one repair slot used" :
                (OwnedEditKind)lparam == OWNED_EDIT_REMOVE ? L"Affix removed" :
                (OwnedEditKind)lparam == OWNED_EDIT_CLEAR ? L"All native affixes removed" :
                (OwnedEditKind)lparam == OWNED_EDIT_ADD ? L"Affix copied" :
                (OwnedEditKind)lparam == OWNED_EDIT_ADD_CATALOG_TEST ? L"Catalogue affix added" :
                (OwnedEditKind)lparam == OWNED_EDIT_RESTORE ? L"Original affixes restored" :
                L"Change kept";
            wchar_t status[240] = {};
            _snwprintf_s(status, _countof(status), _TRUNCATE,
                g_owned_gear_mode ? L"%s. Refresh inventory; unequip/re-equip gear to refresh active bonuses."
                                  : L"%s. Close and reopen game inventory to verify the tooltip.", action);
            set_status(status);
            if (!request_owned_scan()) g_owned_switch_target = NULL;
        } else {
            g_owned_rebound_during_edit = false;
            wchar_t status[200] = {};
            if (wparam == 53)
                wcscpy_s(status, L"Weapon inventory is full. Make room before duplicating; no copy was created.");
            else if (wparam == 55)
                wcscpy_s(status, L"Native inventory declined the copy; original unchanged. No retry was queued.");
            else if (wparam == 56)
                wcscpy_s(status, g_owned_edit_faulted
                    ? L"Copy isolation failed. Restart game/tool before editing; no retry queued."
                    : L"Copy verification failed; a copy may exist. Further copies blocked. Scan and verify the copy before editing it.");
            else if (wparam == 73)
                wcscpy_s(status, L"Weapon already has full durability. No repair slot used.");
            else if (wparam == 74)
                wcscpy_s(status, L"Weapon cannot be repaired or has no remaining repairs. Nothing changed.");
            else if (wparam == 75)
                wcscpy_s(status, L"Repair verification failed. Restart game/tool before another edit; no retry queued.");
            else if (wparam == 57)
                wcscpy_s(status, L"Further copies are blocked after failed verification. Restart game/tool before duplicating.");
            else if (wparam == 76)
                wcscpy_s(status, L"Duplicate and Repair are verified only for weapons; gear was not changed.");
            else if (wparam == 78)
                wcscpy_s(status, L"Gear effect ownership is shared or unreadable. Edit refused; nothing changed.");
            else if (wparam == 52 || wparam == 54)
                wcscpy_s(status, L"This weapon's native state is not supported by the copy verifier. Nothing was duplicated.");
            else if (wparam == 31)
                wcscpy_s(status, L"Previous item changed; its stale rollback was discarded. Scan and select the current instance.");
            else if (wparam == 5)
                wcscpy_s(status, L"Finish the previous item's edit: Keep Changes or Restore Original before editing another piece.");
            else if (wparam == 9)
                wcscpy_s(status, L"32 native affixes reached. Remove an affix before adding another; Keep does not reset this count.");
            else if (wparam == 11)
                wcscpy_s(status, L"That affix ID is already present. Edit its value or use the Not on selected item filter.");
            else if (wparam == 13 || wparam == 24 || wparam == 25 || wparam == 27)
                wcscpy_s(status,L"Magnitude cannot be stored as a finite float32 after native factor conversion. Nothing changed.");
            else if (wparam == 22 || wparam == 23)
                wcscpy_s(status,L"This affix's native effect layout is not supported by the verified constructor. Nothing changed.");
            else if (wparam == 33) {
                wcscpy_s(status, L"Weapon changed or matching copies are ambiguous. Refreshing; select the current instance before retrying.");
                g_owned_refresh_stale_target = true;
                ListView_SetItemState(g_item_list, -1, 0,
                                      LVIS_SELECTED | LVIS_FOCUSED);
                request_owned_scan();
            }
            else
                _snwprintf_s(status, _countof(status), _TRUNCATE,
                    wparam == 97 || wparam == 96
                        ? L"Affix action faulted (%u). Restart game and tool before another edit."
                        : L"Affix action stopped at safety check %u. No further edit queued.",
                    (unsigned)wparam);
            set_status(status);
        }
        EnableWindow(g_owned_duplicate_weapon, !g_owned_gear_mode && !g_owned_edit_faulted && !g_owned_duplicate_faulted && g_owned_panel_instance != NULL);
        EnableWindow(g_owned_repair_weapon, !g_owned_gear_mode && !g_owned_edit_faulted && g_owned_panel_instance != NULL);
        update_owned_affix_panel();
        if (g_owned_switch_offer_target) {
            OwnedWeaponView selected = {};
            const bool still_selected = selected_owned_weapon(&selected) &&
                selected.instance == g_owned_switch_offer_target;
            g_owned_switch_offer_target = NULL;
            if (still_selected) offer_owned_item_switch();
        }
        return 0;
    }
#endif
#if defined(IE_AFFIX_REMOVE_POC) && !defined(IE_OWNED_AFFIX_EDITOR)
    case WM_BEAST_AFFIX_POC_RESULT:
        if (wparam == 0)
            set_status(lparam == 2 ? L"Donor affix added to disposable copy. Close and reopen inventory to inspect; Ctrl+Shift+F11 restores it."
                       : (lparam ? L"Disposable affixes restored from native snapshot."
                                 : L"Disposable's Damage vs Humans affix removed. Close and reopen inventory to inspect; Ctrl+Shift+F11 restores it."));
        else
            set_status(L"Affix removal test rejected by a safety check. See [AFFIX_POC] in the log.");
        return 0;
#endif
    case WM_NOTIFY: {
        NMHDR* notification = (NMHDR*)lparam;
        if (!notification) break;
#ifdef IE_OWNED_AFFIX_EDITOR
        if (notification->hwndFrom==g_owned_affix_list && notification->code==NM_CUSTOMDRAW) {
            NMLVCUSTOMDRAW* custom=(NMLVCUSTOMDRAW*)lparam;
            if (custom->nmcd.dwDrawStage==CDDS_PREPAINT) return CDRF_NOTIFYITEMDRAW;
            if (custom->nmcd.dwDrawStage==CDDS_ITEMPREPAINT) {
                paint_owned_affix_row(custom->nmcd.hdc,(int)custom->nmcd.dwItemSpec);
                return CDRF_SKIPDEFAULT;
            }
        }
        if (notification->hwndFrom == g_owned_affix_list &&
            notification->code == LVN_GETINFOTIPW) {
            NMLVGETINFOTIPW* tip = (NMLVGETINFOTIPW*)lparam;
            OwnedWeaponView weapon = {};
            if (tip->pszText && tip->cchTextMax > 0 &&
                selected_owned_weapon(&weapon) && tip->iItem >= 0) {
                const unsigned index = (unsigned)tip->iItem;
                if (index < weapon.affix_count) {
                    const OwnedAffixView* affix = &weapon.affixes[index];
                    const OwnedAffixMetadata* metadata = owned_affix_metadata(affix->id);
                    wchar_t name[170] = {}, id[140] = {}, value[56] = {};
                    ascii_to_wide(metadata ? metadata->name : affix->id,
                                  name, (int)_countof(name));
                    ascii_to_wide(affix->id, id, (int)_countof(id));
                    if (affix->stat_change)
                        format_owned_affix_value(metadata, affix->raw * affix->factor,
                                                 value, _countof(value));
                    else wcscpy_s(value, L"Effect type unknown");
                    _snwprintf_s(tip->pszText, tip->cchTextMax, _TRUNCATE,
                        L"%s: %s\nID: %s\nRaw %g x factor %g%s", name, value, id,
                        affix->raw, affix->factor,
                        metadata && !metadata->game_localized ? L"\nName is an ID-based label" : L"");
                } else {
                    const OwnedBuiltInStat* built_in[16] = {};
                    const unsigned n = owned_builtin_for_item(weapon.id, built_in,
                                                              (unsigned)_countof(built_in));
                    const unsigned builtin_index = index - weapon.affix_count;
                    if (builtin_index < n && builtin_index < _countof(built_in)) {
                        const OwnedBuiltInStat* stat = built_in[builtin_index];
                        wchar_t name[170] = {}, value[40] = {};
                        ascii_to_wide(stat->name, name, (int)_countof(name));
                        ascii_to_wide(stat->authored_value, value, (int)_countof(value));
                        _snwprintf_s(tip->pszText, tip->cchTextMax, _TRUNCATE,
                            L"%s: authored %s%s\nDefinition stat; read-only%s", name,
                            value, !_stricmp(stat->unit, "Percent") ? L"%" :
                                   !_stricmp(stat->unit, "Seconds") ? L"s" : L"",
                            stat->scalable ? L"; game-scaled" : L"");
                    }
                }
            }
            return 0;
        }
        if (notification->hwndFrom == g_owned_affix_list &&
            notification->code == LVN_ITEMCHANGED) {
            const NMLISTVIEW* change = (const NMLISTVIEW*)lparam;
            if ((change->uChanged & LVIF_STATE) &&
                ((change->uOldState ^ change->uNewState) & LVIS_SELECTED))
                owned_affix_selection_changed();
            return 0;
        }
#endif
        HWND list_header = g_item_list ? ListView_GetHeader(g_item_list) : NULL;
        if (notification->hwndFrom == list_header &&
            (notification->code == HDN_ITEMCLICKW ||
             notification->code == HDN_ITEMCLICKA)) {
            const NMHEADERW* header_click = (const NMHEADERW*)lparam;
            if (header_click->iItem == 0 || header_click->iItem == 1) SetFocus(g_search);
            else if (header_click->iItem == 2) open_filter_dropdown(g_category_filter);
            else if (header_click->iItem == 3) open_filter_dropdown(g_rarity_filter);
            else if (header_click->iItem == 4) open_filter_dropdown(g_weapon_level_filter);
            return 0;
        }
        if (notification->hwndFrom != g_item_list) break;
        if (notification->code == NM_CUSTOMDRAW) {
            NMLVCUSTOMDRAW* custom = (NMLVCUSTOMDRAW*)lparam;
            if (custom->nmcd.dwDrawStage == CDDS_PREPAINT) return CDRF_NOTIFYITEMDRAW;
            if (custom->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
                paint_catalog_row(custom->nmcd.hdc, (int)custom->nmcd.dwItemSpec);
                return CDRF_SKIPDEFAULT;
            }
        }
        if (notification->code == LVN_GETDISPINFOW) {
            NMLVDISPINFOW* info = (NMLVDISPINFOW*)lparam;
            const int row = info->item.iItem;
            if ((info->item.mask & LVIF_TEXT) && row >= 0 && row < (int)g_visible_count) {
                ItemDefinition item = {};
                char state[16] = {};
                EnterCriticalSection(&g_data_lock);
#ifdef IE_OWNED_AFFIX_EDITOR
                if (g_owned_mode) {
                    const OwnedWeaponView* owned = &g_owned_weapons[g_visible_items[row]];
                    _snprintf_s(item.name, _countof(item.name), _TRUNCATE,
                                "%s  #%u", owned->name, owned->same_id_ordinal);
                    strcpy_s(item.id, owned->id);
                    strcpy_s(item.category, owned->category);
                    strcpy_s(item.rarity, owned->rarity);
                    item.weapon_level = owned->weapon_level;
                    if (owned->is_gear)
                        strcpy_s(state,g_owned_equipment_known ? (owned->worn_slot ? "Worn" : "Bag") : "Unknown");
                    _snprintf_s(item.source, _countof(item.source), _TRUNCATE,
                                "%u affixes", owned->affix_count);
                } else
#endif
                item = g_items[g_visible_items[row]];
                LeaveCriticalSection(&g_data_lock);
                char level[16] = {};
                if (item.weapon_level)
                    _snprintf_s(level, _countof(level), _TRUNCATE, "R%lu", item.weapon_level);
                if (state[0]) strcpy_s(level,state);
                const char* source_name = strrchr(item.source, '/');
                source_name = source_name ? source_name + 1 : item.source;
                const char* text = info->item.iSubItem == 0 ? item.name :
                                   info->item.iSubItem == 1 ? item.id :
                                   info->item.iSubItem == 2 ? item.category :
                                   info->item.iSubItem == 3 ? item.rarity :
                                   info->item.iSubItem == 4 ? level : source_name;
                MultiByteToWideChar(CP_UTF8, 0, text, -1,
                                    info->item.pszText, info->item.cchTextMax);
            }
            return 0;
        }
        if (notification->code == LVN_ITEMCHANGED) {
            NMLISTVIEW* change = (NMLISTVIEW*)lparam;
            if ((change->uChanged & LVIF_STATE) &&
                ((change->uOldState ^ change->uNewState) & LVIS_SELECTED)) {
#ifdef IE_OWNED_AFFIX_EDITOR
                // A list rebuild temporarily deselects every row. Neither that
                // transition nor the deselection half of a user switch is a new target.
                if (g_owned_mode && (g_owned_view_rebuilding || !(change->uNewState & LVIS_SELECTED)))
                    return 0;
                if (g_owned_mode) offer_owned_item_switch();
#endif
                update_selected_item_panel();
#ifdef IE_OWNED_AFFIX_EDITOR
                if (g_owned_mode && !g_owned_population_busy &&
                    SendMessageW(g_owned_presence_filter,CB_GETCURSEL,0,0)>0)
                    populate_owned_donors();
#endif
                InvalidateRect(g_item_list, NULL, FALSE);
            }
            return 0;
        }
        break;
    }
    case WM_COMMAND:
#ifdef IE_OWNED_AFFIX_EDITOR
        if (LOWORD(wparam)==IDC_OWNED_DONOR_COMBO && HIWORD(wparam)==CBN_DROPDOWN) {
            prepare_owned_affix_popup(); return 0;
        }
        if (LOWORD(wparam)==IDC_OWNED_DONOR_COMBO && HIWORD(wparam)==CBN_CLOSEUP) {
            g_owned_popup_prepared=false; return 0;
        }
        if (LOWORD(wparam)==IDC_OWNED_DONOR_COMBO && HIWORD(wparam)==CBN_SELCHANGE) {
            update_owned_add_button(); return 0;
        }
        if ((LOWORD(wparam)==IDC_OWNED_FORMAT_FILTER || LOWORD(wparam)==IDC_OWNED_PRESENCE_FILTER) &&
            HIWORD(wparam)==CBN_SELCHANGE) {
            populate_owned_donors(); return 0;
        }
#endif
        if (LOWORD(wparam) == IDC_SEARCH && HIWORD(wparam) == EN_CHANGE) { refresh_item_view(); return 0; }
        if ((LOWORD(wparam) == IDC_TAB_ALL_ITEMS || LOWORD(wparam) == IDC_TAB_FAVORITES) &&
            HIWORD(wparam) == BN_CLICKED) {
            SendMessageW(g_view_filter, CB_SETCURSEL,
                         LOWORD(wparam) == IDC_TAB_FAVORITES ? 1 : 0, 0);
#ifdef IE_OWNED_AFFIX_EDITOR
            set_owned_view(false);
#else
            refresh_item_view();
#endif
            return 0;
        }
#ifdef IE_OWNED_AFFIX_EDITOR
        if ((LOWORD(wparam) == IDC_TAB_OWNED_WEAPONS || LOWORD(wparam) == IDC_TAB_OWNED_GEAR) && HIWORD(wparam) == BN_CLICKED) {
            set_owned_view(true, LOWORD(wparam) == IDC_TAB_OWNED_GEAR);
            request_owned_scan();
            return 0;
        }
        if (LOWORD(wparam) == IDC_OWNED_CATALOG_SEARCH &&
            HIWORD(wparam) == EN_CHANGE) {
            InvalidateRect(g_owned_catalog_search,NULL,TRUE);
            populate_owned_donors();
            return 0;
        }
        if (LOWORD(wparam) == IDC_OWNED_CATALOG_TEST && HIWORD(wparam) == BN_CLICKED) {
            set_owned_add_source(!g_owned_catalog_mode);
            return 0;
        }
        if (HIWORD(wparam) == BN_CLICKED) {
            OwnedEditKind kind = (OwnedEditKind)0;
            switch (LOWORD(wparam)) {
            case IDC_OWNED_APPLY_VALUE: kind = OWNED_EDIT_VALUE; break;
            case IDC_OWNED_REMOVE_AFFIX: kind = OWNED_EDIT_REMOVE; break;
            case IDC_OWNED_CLEAR_AFFIXES: kind = OWNED_EDIT_CLEAR; break;
            case IDC_OWNED_DUPLICATE: kind = OWNED_EDIT_DUPLICATE; break;
            case IDC_OWNED_REPAIR: kind = OWNED_EDIT_REPAIR; break;
            case IDC_OWNED_ADD_AFFIX:
                kind = g_owned_catalog_mode ? OWNED_EDIT_ADD_CATALOG_TEST : OWNED_EDIT_ADD;
                break;
            case IDC_OWNED_RESTORE: kind = OWNED_EDIT_RESTORE; break;
            case IDC_OWNED_KEEP: kind = OWNED_EDIT_KEEP; break;
            }
            if (kind) { request_owned_edit(kind); return 0; }
        }
#endif
        if (LOWORD(wparam) == IDC_QUANTITY && HIWORD(wparam) == EN_KILLFOCUS) {
            int normalized = 1;
            if (!read_item_quantity(&normalized)) SetWindowTextW(g_quantity, L"1");
            return 0;
        }
        if ((LOWORD(wparam) == IDC_CATEGORY || LOWORD(wparam) == IDC_RARITY ||
             LOWORD(wparam) == IDC_VIEW || LOWORD(wparam) == IDC_WEAPON_LEVEL) &&
            HIWORD(wparam) == CBN_SELCHANGE) { refresh_item_view(); return 0; }
        if (LOWORD(wparam) == IDC_GIVE_INVENTORY && HIWORD(wparam) == BN_CLICKED) {
            queue_item_action(ITEM_ACTION_GIVE_INVENTORY);
            return 0;
        }
        if (LOWORD(wparam) == IDC_GIVE_STASH && HIWORD(wparam) == BN_CLICKED) {
            queue_item_action(ITEM_ACTION_GIVE_STASH);
            return 0;
        }
        if (LOWORD(wparam) == IDC_REMOVE_INVENTORY && HIWORD(wparam) == BN_CLICKED) {
            queue_item_action(ITEM_ACTION_REMOVE_INVENTORY);
            return 0;
        }
        if (LOWORD(wparam) == IDC_QUANTITY_MINUS && HIWORD(wparam) == BN_CLICKED) {
            step_item_quantity(-1);
            return 0;
        }
        if (LOWORD(wparam) == IDC_QUANTITY_PLUS && HIWORD(wparam) == BN_CLICKED) {
            step_item_quantity(1);
            return 0;
        }
        if (LOWORD(wparam) == IDC_FAVORITE && HIWORD(wparam) == BN_CLICKED) {
            toggle_selected_favorite();
            return 0;
        }
        if (LOWORD(wparam) == IDC_COPY_ID) { copy_selected_item_id(); return 0; }
        if (LOWORD(wparam) == IDC_RESET_FILTERS) {
            SetWindowTextW(g_search, L"");
            SendMessageW(g_category_filter, CB_SETCURSEL, 0, 0);
            SendMessageW(g_rarity_filter, CB_SETCURSEL, 0, 0);
            SendMessageW(g_view_filter, CB_SETCURSEL, 0, 0);
            SendMessageW(g_weapon_level_filter, CB_SETCURSEL, 0, 0);
#ifdef IE_OWNED_AFFIX_EDITOR
            SendMessageW(g_owned_format_filter,CB_SETCURSEL,0,0);
            SendMessageW(g_owned_presence_filter,CB_SETCURSEL,0,0);
            SetWindowTextW(g_owned_catalog_search,L"");
            if (g_owned_mode) populate_owned_donors();
#endif
            refresh_item_view();
            set_status(L"Filters reset.");
            return 0;
        }
        if (LOWORD(wparam) == IDC_CLEAR_UI_LOG) {
            set_status(L"Runtime log cleared.");
            if (g_runtime_log) {
                SetWindowTextW(g_runtime_log, L"");
                ShowScrollBar(g_runtime_log, SB_VERT, FALSE);
            }
            return 0;
        }
        if (LOWORD(wparam) == IDC_CLOSE_BUTTON || LOWORD(wparam) == IDC_RETURN_GAME) {
            close_catalogue(true);
            return 0;
        }
        if (LOWORD(wparam) == IDC_MINIMIZE_BUTTON) {
            ShowWindow(hwnd, SW_MINIMIZE);
            focus_game_after_catalogue();
            return 0;
        }
        if (LOWORD(wparam) == IDC_MAXIMIZE_BUTTON) {
            ShowWindow(hwnd, IsZoomed(hwnd) ? SW_RESTORE : SW_MAXIMIZE);
            return 0;
        }
        if (LOWORD(wparam) == IDC_REFRESH_CATALOG) {
#ifdef IE_OWNED_AFFIX_EDITOR
            if (g_owned_mode) { request_owned_scan(); return 0; }
#endif
            ListView_SetItemState(g_item_list, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
            g_visible_count = 0; // Old row indexes cannot be reused after a rebuilt catalog.
            load_item_catalog();
            refresh_item_view();
            set_status(L"Catalog refreshed from installed game definitions.");
            return 0;
        }
        if (LOWORD(wparam) == IDC_RESCAN) { run_probe(); return 0; }
        if (LOWORD(wparam) == IDC_COPYLOG) { copy_log_path(); return 0; }
        if (LOWORD(wparam) == IDC_TRACE_DROP) { trace_drop_path(); return 0; }
        break;
    case WM_BEAST_GIVE_RESULT:
        {
        ItemActionRequest request = {};
        EnterCriticalSection(&g_data_lock); request = g_action_request; LeaveCriticalSection(&g_data_lock);
        wchar_t name[180] = {}, status[420] = {};
        ascii_to_wide(request.name, name, _countof(name));
        switch ((ActionResult)wparam) {
        case GIVE_RESULT_OK:
            if ((ItemAction)lparam == ITEM_ACTION_GIVE_STASH)
                _snwprintf_s(status, _countof(status), _TRUNCATE, L"Game accepted stash request: %s x%d. Check the stash to confirm delivery.", name, request.quantity);
            else if ((ItemAction)lparam == ITEM_ACTION_REMOVE_INVENTORY)
                _snwprintf_s(status, _countof(status), _TRUNCATE, L"Removed from player inventory: %s x%d", name, request.quantity);
            else if ((ItemAction)lparam == ITEM_ACTION_SPAWN_WORLD)
                _snwprintf_s(status, _countof(status), _TRUNCATE, L"Spawned through the native ID-based world-item path: %s x%d", name, request.quantity);
            else
                _snwprintf_s(status, _countof(status), _TRUNCATE, L"Added to player inventory: %s x%d", name, request.quantity);
            set_status(status);
            break;
        case GIVE_RESULT_NO_PLAYER:
            set_status(L"Native path ran, but no playable local player resolved. See [ITEM] in the log.");
            break;
        case GIVE_RESULT_NO_INVENTORY:
            set_status(L"Player resolved, but the inventory component was invalid. See [ITEM] in the log.");
            break;
        case GIVE_RESULT_NATIVE_REJECTED:
            if ((ItemAction)lparam == ITEM_ACTION_GIVE_INVENTORY)
                set_status(L"Carried inventory rejected this item. Make room in that category, or use Give to Stash.");
            else if ((ItemAction)lparam == ITEM_ACTION_SPAWN_WORLD)
                set_status(L"World spawn failed: the player-bound drop controller rejected this item or is busy. See the last [WORLD_ITEM] line.");
            else
                set_status(L"Player resolved, but the native item routine rejected this ID or quantity. See [ITEM] in the log.");
            break;
        case GIVE_RESULT_BUILD_REJECTED:
            set_status(L"Give blocked: this loaded game module failed exact build/byte validation.");
            break;
        case GIVE_RESULT_LEVEL_BLOCKED:
            set_status(L"Ranked Give blocked before creating anything. Restart after a level fault; check [GIVE_LEVEL] in the log.");
            break;
        case GIVE_RESULT_LEVEL_UNVERIFIED:
            set_status(L"Give ran, but requested level could not be verified. Items may already exist; do not retry. Check inventory and [GIVE_LEVEL] in the log.");
            break;
        case GIVE_RESULT_EXCEPTION:
            set_status(L"Give stopped by crash protection. The exception stage is recorded in the log.");
            break;
        default:
            set_status(L"Item action could not run. See the last [ITEM] line in ItemExtravaganza.log.");
            break;
        }
        InterlockedExchange(&g_action_pending, 0);
        set_item_actions_enabled(true);
        return 0;
        }
    case WM_DRAWITEM: {
        const DRAWITEMSTRUCT* draw = (const DRAWITEMSTRUCT*)lparam;
        if (!draw) return FALSE;
#ifdef IE_OWNED_AFFIX_EDITOR
        if (draw->CtlID==IDC_OWNED_DONOR_COMBO && draw->CtlType==ODT_COMBOBOX) {
            draw_owned_affix_choice(draw); return TRUE;
        }
#endif
        if (draw->CtlID == IDC_BRAND) draw_brand(draw);
        else if (draw->CtlID == IDC_RESULT_COUNT) draw_result_summary(draw);
        else if (draw->CtlID == IDC_SELECTED_SUMMARY) draw_selected_summary(draw);
        else if (draw->CtlID == IDC_QUANTITY_BACKDROP || draw->CtlID == IDC_SEARCH_BACKDROP
#ifdef IE_OWNED_AFFIX_EDITOR
                 || draw->CtlID == IDC_OWNED_VALUE_BACKDROP ||
                 draw->CtlID == IDC_OWNED_CATALOG_VALUE_BACKDROP || draw->CtlID == IDC_OWNED_CATALOG_SEARCH_BACKDROP
#endif
                 )
            draw_quantity_backdrop(draw);
        else if (draw->CtlType == ODT_STATIC) draw_detail_panel(draw);
        else draw_dark_button(draw);
        return TRUE;
    }
    case WM_PAINT: {
        PAINTSTRUCT paint = {};
        HDC target = BeginPaint(hwnd, &paint);
        RECT client = {};
        GetClientRect(hwnd, &client);
        HDC buffer = CreateCompatibleDC(target);
        HBITMAP bitmap = CreateCompatibleBitmap(target, max(1, client.right), max(1, client.bottom));
        if (buffer && bitmap) {
            HGDIOBJ previous = SelectObject(buffer, bitmap);
            paint_catalogue_chrome(hwnd, buffer);
            BitBlt(target, 0, 0, client.right, client.bottom, buffer, 0, 0, SRCCOPY);
            SelectObject(buffer, previous);
        } else {
            paint_catalogue_chrome(hwnd, target);
        }
        if (bitmap) DeleteObject(bitmap);
        if (buffer) DeleteDC(buffer);
        EndPaint(hwnd, &paint);
        return 0;
    }
    case WM_NCCALCSIZE:
        if (wparam) {
            NCCALCSIZE_PARAMS* sizing = (NCCALCSIZE_PARAMS*)lparam;
            if (IsZoomed(hwnd) && sizing) {
                MONITORINFO monitor = { sizeof(monitor) };
                HMONITOR target = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                if (GetMonitorInfoW(target, &monitor)) sizing->rgrc[0] = monitor.rcWork;
            }
            return 0;
        }
        break;
    case WM_NCHITTEST: {
        POINT point = { (short)LOWORD(lparam), (short)HIWORD(lparam) };
        RECT window_rect = {}; GetWindowRect(hwnd, &window_rect);
        const int border = ui_scale(8);
        const bool left = point.x >= window_rect.left && point.x < window_rect.left + border;
        const bool right = point.x < window_rect.right && point.x >= window_rect.right - border;
        const bool top = point.y >= window_rect.top && point.y < window_rect.top + border;
        const bool bottom = point.y < window_rect.bottom && point.y >= window_rect.bottom - border;
        if (top && left) return HTTOPLEFT;
        if (top && right) return HTTOPRIGHT;
        if (bottom && left) return HTBOTTOMLEFT;
        if (bottom && right) return HTBOTTOMRIGHT;
        if (left) return HTLEFT;
        if (right) return HTRIGHT;
        if (top) return HTTOP;
        if (bottom) return HTBOTTOM;
        const HWND title_buttons[] = { g_close_button };
        for (HWND button : title_buttons) {
            RECT button_rect = {};
            if (button && GetWindowRect(button, &button_rect) && PtInRect(&button_rect, point))
                return HTCLIENT;
        }
        if (point.y < window_rect.top + ui_scale(78)) return HTCAPTION;
        return HTCLIENT;
    }
    case WM_SETCURSOR: {
        HWND candidate = (HWND)wparam;
        HWND next_hot = NULL;
        if (candidate && candidate != hwnd && GetParent(candidate) == hwnd) {
            wchar_t class_name[24] = {};
            GetClassNameW(candidate, class_name, _countof(class_name));
            if (!_wcsicmp(class_name, L"Button")) next_hot = candidate;
        }
        if (next_hot != g_hot_control) {
            HWND previous = g_hot_control;
            g_hot_control = next_hot;
            if (previous) InvalidateRect(previous, NULL, TRUE);
            if (next_hot) InvalidateRect(next_hot, NULL, TRUE);
        }
        break;
    }
    case WM_ACTIVATE:
        if (LOWORD(wparam) == WA_INACTIVE) PostMessageW(hwnd,WM_IE_MENU_SCOPE,0,0);
        if (LOWORD(wparam) == WA_INACTIVE && g_hot_control) {
            HWND previous = g_hot_control;
            g_hot_control = NULL;
            InvalidateRect(previous, NULL, TRUE);
        }
        break;
    case WM_ERASEBKGND: {
        RECT rect = {}; GetClientRect(hwnd, &rect);
        FillRect((HDC)wparam, &rect, g_background_brush);
        return 1;
    }
    case WM_CTLCOLORSTATIC: {
        HDC dc = (HDC)wparam;
        HWND control = (HWND)lparam;
        SetBkMode(dc, TRANSPARENT);
        const bool muted = control == g_subtitle || control == g_version ||
            control == g_search_label || control == g_category_label ||
            control == g_rarity_label || control == g_library_label ||
            control == g_weapon_level_label ||
            control == g_quantity_label || control == g_actions_label || control == g_catalog_subtitle
#ifdef IE_OWNED_AFFIX_EDITOR
            || control == g_owned_value_label || control == g_owned_donor_label || control == g_owned_filter_label
#endif
            ;
        const bool section = control == g_section_title || control == g_catalog_title || control == g_console_title;
        SetTextColor(dc, control == g_status ? kColorMuted :
                     control == g_result_count ? kColorAccent : section ? kColorText :
                     muted ? kColorMuted : kColorText);
        const bool header = control == g_header || control == g_subtitle ||
                            control == g_version;
        const bool filter = control == g_section_title || control == g_result_count ||
                            control == g_search_label || control == g_category_label ||
                            control == g_rarity_label || control == g_library_label ||
                            control == g_weapon_level_label
#ifdef IE_OWNED_AFFIX_EDITOR
                            || control == g_owned_filter_label
#endif
                            ;
        const bool detail = control == g_selected_label || control == g_quantity_label ||
                            control == g_actions_label
#ifdef IE_OWNED_AFFIX_EDITOR
                            || control == g_owned_value_label || control == g_owned_donor_label || control == g_owned_affix_detail
#endif
                            ;
        const bool catalog = control == g_catalog_title || control == g_catalog_subtitle;
        if (control == g_status) return (LRESULT)g_status_brush;
        if (header) return (LRESULT)g_header_brush;
        if (control == g_result_count) return (LRESULT)g_panel_brush;
        if (filter || detail) return (LRESULT)g_card_brush;
        if (catalog) return (LRESULT)g_panel_brush;
        if (control == g_console_title) return (LRESULT)g_console_brush;
        return (LRESULT)g_background_brush;
    }
    case WM_CTLCOLOREDIT: {
        HDC dc = (HDC)wparam;
        HWND control = (HWND)lparam;
        const bool details = control == g_selection;
        const bool console = control == g_runtime_log;
        SetBkColor(dc, console ? kColorConsole : (details ? kColorPanel : kColorInput));
        SetTextColor(dc, kColorText);
        if (console) return (LRESULT)g_console_brush;
        return (LRESULT)(details ? g_panel_brush : g_input_brush);
    }
    case WM_CTLCOLORLISTBOX: {
        HDC dc = (HDC)wparam;
        SetBkColor(dc, kColorPanel);
        SetTextColor(dc, kColorText);
        return (LRESULT)g_panel_brush;
    }
    case WM_SIZE:
        if (wparam != SIZE_MINIMIZED) {
            layout_item_catalogue(hwnd);
            if (g_maximize_button) InvalidateRect(g_maximize_button, NULL, TRUE);
        }
        return 0;
    case WM_DPICHANGED: {
        const UINT dpi = HIWORD(wparam);
        if (dpi) g_ui_dpi = dpi;
        rebuild_ui_fonts();
        rebuild_row_height_image_list();
        rebuild_window_icons();
        RECT* suggested = (RECT*)lparam;
        if (suggested) SetWindowPos(hwnd, NULL, suggested->left, suggested->top,
                                    suggested->right - suggested->left, suggested->bottom - suggested->top,
                                    SWP_NOZORDER | SWP_NOACTIVATE);
        layout_item_catalogue(hwnd);
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }
    case WM_GETMINMAXINFO: {
        MINMAXINFO* limits = (MINMAXINFO*)lparam;
        limits->ptMinTrackSize.x = ui_scale(1160);
        limits->ptMinTrackSize.y = ui_scale(760);
#ifdef IE_OWNED_AFFIX_EDITOR
        limits->ptMinTrackSize.y = ui_scale(800); // Keep the affix list and actions separate.
#endif
        return 0;
    }
    case WM_TIMER: poll_normal_drop_trace(); return 0;
    case WM_CLOSE:
        close_catalogue(true);
        return 0;
    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE) {
            close_catalogue(true);
            return 0;
        }
        break;
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static DWORD WINAPI runtime_thread(void*) {
    INITCOMMONCONTROLSEX controls = {
        sizeof(controls), ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES
    };
    InitCommonControlsEx(&controls);
    g_background_brush = CreateSolidBrush(kColorBackground);
    g_panel_brush = CreateSolidBrush(kColorPanel);
    g_input_brush = CreateSolidBrush(kColorInput);
    g_status_brush = CreateSolidBrush(kColorStatus);
    g_sidebar_brush = CreateSolidBrush(kColorSidebar);
    g_card_brush = CreateSolidBrush(kColorCard);
    g_header_brush = CreateSolidBrush(kColorHeader);
    g_console_brush = CreateSolidBrush(kColorConsole);
    WNDCLASSW wc = {}; wc.hInstance = g_module; wc.lpfnWndProc = window_proc;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW); wc.hbrBackground = g_background_brush;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpszClassName = L"ItemExtravaganzaWindow"; RegisterClassW(&wc);
    g_window = CreateWindowExW(WS_EX_TOOLWINDOW, wc.lpszClassName, L"Item Extravaganza",
        WS_POPUP | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, ui_scale(1420), ui_scale(900),
        find_game_window(), NULL, g_module, NULL);
    g_ui_dpi = query_window_dpi(g_window);
    MONITORINFO initial_monitor = { sizeof(initial_monitor) };
    GetMonitorInfoW(MonitorFromWindow(g_window, MONITOR_DEFAULTTONEAREST), &initial_monitor);
    const int work_width = initial_monitor.rcWork.right - initial_monitor.rcWork.left;
    const int work_height = initial_monitor.rcWork.bottom - initial_monitor.rcWork.top;
    const int initial_width = min(ui_scale(1420), max(ui_scale(1160), work_width - ui_scale(32)));
    const int initial_height = min(ui_scale(900), max(ui_scale(760), work_height - ui_scale(32)));
    SetWindowPos(g_window, NULL, 0, 0, initial_width, initial_height,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    BOOL dark_title = TRUE;
    DwmSetWindowAttribute(g_window, 20, &dark_title, sizeof(dark_title));
    DwmSetWindowAttribute(g_window, 35, &kColorBackground, sizeof(kColorBackground));
    DwmSetWindowAttribute(g_window, 36, &kColorText, sizeof(kColorText));
    const DWORD rounded_corners = 2;
    DwmSetWindowAttribute(g_window, 33, &rounded_corners, sizeof(rounded_corners));
    const DWORD no_border_color = 0xFFFFFFFE;
    DwmSetWindowAttribute(g_window, 34, &no_border_color, sizeof(no_border_color));
    SetWindowTheme(g_window, L"DarkMode_Explorer", NULL);
    rebuild_ui_fonts();
    rebuild_window_icons();
    g_brand = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                            ui_scale(24), ui_scale(24), ui_scale(46), ui_scale(46),
                            g_window, (HMENU)IDC_BRAND, g_module, NULL);
    g_header = CreateWindowW(L"STATIC", L"Item Extravaganza",
                             WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE | SS_NOPREFIX,
                             20, 12, 330, 34, g_window, NULL, g_module, NULL);
    g_result_count = CreateWindowW(L"STATIC", L"Loading items…",
                                   WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                   1010, 20, 190, 56, g_window,
                                   (HMENU)IDC_RESULT_COUNT, g_module, NULL);
    g_version = CreateWindowW(L"STATIC", L"v" IE_VERSION_WTEXT, WS_CHILD | WS_VISIBLE | SS_RIGHT,
                              1010, 44, 190, 18, g_window, NULL, g_module, NULL);
    g_minimize_button = NULL;
    g_maximize_button = NULL;
    g_close_button = create_dark_button(g_window, L"", 1208, 18, 44, 44, IDC_CLOSE_BUTTON);
    g_return_game = create_dark_button(g_window, L"Return to Game", 18, 720, 162, 38, IDC_RETURN_GAME);
    g_section_title = CreateWindowW(L"STATIC", L"Filter Items", WS_CHILD | WS_VISIBLE,
                                    24, 92, 240, 26, g_window, NULL, g_module, NULL);
    g_reset_filters = create_dark_button(g_window, L"Reset", 24, 92, 82, 28, IDC_RESET_FILTERS);
    g_search_label = CreateWindowW(L"STATIC", L"Search", WS_CHILD | WS_VISIBLE,
                                   20, 68, 260, 18, g_window, NULL, g_module, NULL);
    g_category_label = CreateWindowW(L"STATIC", L"Category", WS_CHILD | WS_VISIBLE,
                                     392, 68, 180, 18, g_window, NULL, g_module, NULL);
    g_rarity_label = CreateWindowW(L"STATIC", L"Rarity", WS_CHILD | WS_VISIBLE,
                                   584, 68, 120, 18, g_window, NULL, g_module, NULL);
    g_library_label = CreateWindowW(L"STATIC", L"View", WS_CHILD | WS_VISIBLE,
                                    716, 68, 140, 18, g_window, NULL, g_module, NULL);
    g_weapon_level_label = CreateWindowW(L"STATIC", L"Weapon Level", WS_CHILD | WS_VISIBLE,
                                         878, 68, 120, 18, g_window, NULL, g_module, NULL);
    g_search_backdrop = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                      20, 88, 360, 28, g_window,
                                      (HMENU)IDC_SEARCH_BACKDROP, g_module, NULL);
    g_search = CreateWindowExW(0, L"EDIT", L"",
                               WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                20, 88, 360, 28, g_window, (HMENU)IDC_SEARCH, g_module, NULL);
    SendMessageW(g_search, EM_SETCUEBANNER, TRUE, (LPARAM)L"Search items...");
    g_category_filter = CreateWindowW(WC_COMBOBOXW, L"",
                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                                      392, 88, 180, 360, g_window, (HMENU)IDC_CATEGORY, g_module, NULL);
    SendMessageW(g_category_filter, CB_ADDSTRING, 0, (LPARAM)L"All categories");
    SendMessageW(g_category_filter, CB_SETCURSEL, 0, 0);
    g_rarity_filter = CreateWindowW(WC_COMBOBOXW, L"",
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                                    584, 88, 120, 280, g_window, (HMENU)IDC_RARITY, g_module, NULL);
    SendMessageW(g_rarity_filter, CB_ADDSTRING, 0, (LPARAM)L"All rarities");
    SendMessageW(g_rarity_filter, CB_SETCURSEL, 0, 0);
    g_view_filter = CreateWindowW(WC_COMBOBOXW, L"",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
                                  716, 88, 150, 160, g_window, (HMENU)IDC_VIEW, g_module, NULL);
    SendMessageW(g_view_filter, CB_ADDSTRING, 0, (LPARAM)L"All items");
    SendMessageW(g_view_filter, CB_ADDSTRING, 0, (LPARAM)L"Favorites");
    SendMessageW(g_view_filter, CB_ADDSTRING, 0, (LPARAM)L"Recently used");
    SendMessageW(g_view_filter, CB_SETCURSEL, 0, 0);
    g_weapon_level_filter = CreateWindowW(WC_COMBOBOXW, L"",
                                          WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                                          878, 88, 110, 360, g_window,
                                          (HMENU)IDC_WEAPON_LEVEL, g_module, NULL);
    SendMessageW(g_weapon_level_filter, CB_ADDSTRING, 0, (LPARAM)L"All");
    for (int level = 1; level <= 15; ++level) {
        wchar_t label[16] = {};
        _snwprintf_s(label, _countof(label), _TRUNCATE, L"R%d", level);
        SendMessageW(g_weapon_level_filter, CB_ADDSTRING, 0, (LPARAM)label);
    }
    SendMessageW(g_weapon_level_filter, CB_SETCURSEL, 0, 0);
    create_dark_button(g_window, L"Refresh", 1000, 87, 204, 30, IDC_REFRESH_CATALOG);
    g_catalog_title = CreateWindowW(L"STATIC", L"Item Catalogue", WS_CHILD | WS_VISIBLE,
                                    20, 130, 240, 22, g_window, NULL, g_module, NULL);
    g_catalog_subtitle = CreateWindowW(L"STATIC", L"Browse, inspect, and manage game items", WS_CHILD | WS_VISIBLE,
                                       20, 152, 280, 18, g_window, NULL, g_module, NULL);
    g_tab_all_items = create_dark_button(g_window, L"All Items", 20, 180, 105, 28, IDC_TAB_ALL_ITEMS);
    g_tab_favorites = create_dark_button(g_window, L"Favorites", 131, 180, 118, 28, IDC_TAB_FAVORITES);
#ifdef IE_OWNED_AFFIX_EDITOR
    g_tab_owned_weapons = create_dark_button(g_window, L"Owned Weapons", 255, 180, 146, 28,
                                             IDC_TAB_OWNED_WEAPONS);
    g_tab_owned_gear = create_dark_button(g_window, L"Owned Gear", 407, 180, 112, 28,
                                         IDC_TAB_OWNED_GEAR);
#endif

    g_item_list = CreateWindowExW(0, WC_LISTVIEWW, L"",
                                  WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_OWNERDATA |
                                  LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_TABSTOP,
                                  20, 132, 780, 532, g_window, (HMENU)IDC_ITEM_LIST, g_module, NULL);
    ListView_SetExtendedListViewStyle(g_item_list,
        LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
    ListView_SetBkColor(g_item_list, kColorPanel);
    ListView_SetTextBkColor(g_item_list, kColorPanel);
    ListView_SetTextColor(g_item_list, kColorText);
    SetWindowTheme(g_item_list, L"DarkMode_Explorer", NULL);
    g_row_height_images = ImageList_Create(1, ui_scale(kUiTableRowHeight), ILC_COLOR32, 1, 1);
    if (g_row_height_images) ListView_SetImageList(g_item_list, g_row_height_images, LVSIL_SMALL);
    const wchar_t* columns[] = { L"Item", L"ID", L"Category", L"Rarity", L"Lvl", L"Source" };
    const int widths[] = { 170, 185, 110, 90, 88, 110 };
    for (int i = 0; i < 6; ++i) {
        LVCOLUMNW column = { LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM };
        column.pszText = (LPWSTR)columns[i]; column.cx = widths[i]; column.iSubItem = i;
        ListView_InsertColumn(g_item_list, i, &column);
    }
    SetWindowTheme(ListView_GetHeader(g_item_list), L"DarkMode_Explorer", NULL);
    SetWindowSubclass(ListView_GetHeader(g_item_list), catalog_header_proc, 1, 0);

    g_detail_panel = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                   818, 132, 386, 532, g_window,
                                   (HMENU)IDC_DETAIL_PANEL, g_module, NULL);
    g_selected_label = CreateWindowW(L"STATIC", L"Item Details", WS_CHILD | WS_VISIBLE,
                                     834, 148, 180, 18, g_window, NULL, g_module, NULL);
    g_selection = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                834, 172, 354, 280, g_window,
                                (HMENU)IDC_SELECTED_SUMMARY, g_module, NULL);
#ifdef IE_OWNED_AFFIX_EDITOR
    g_owned_affix_list = CreateWindowExW(0, WC_LISTVIEWW, L"",
        WS_CHILD | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_TABSTOP,
        834, 280, 354, 170, g_window, (HMENU)IDC_OWNED_AFFIX_LIST, g_module, NULL);
    ListView_SetExtendedListViewStyle(g_owned_affix_list,
        LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP | LVS_EX_INFOTIP);
    ListView_SetBkColor(g_owned_affix_list, kColorPanel);
    ListView_SetTextBkColor(g_owned_affix_list, kColorPanel);
    ListView_SetTextColor(g_owned_affix_list, kColorText);
    SetWindowTheme(g_owned_affix_list, L"DarkMode_Explorer", NULL);
    SetWindowSubclass(ListView_GetHeader(g_owned_affix_list),catalog_header_proc,1,0);
    g_owned_row_images=ImageList_Create(1,ui_scale(28),ILC_COLOR32,1,1);
    if (g_owned_row_images) ListView_SetImageList(g_owned_affix_list,g_owned_row_images,LVSIL_SMALL);
    const wchar_t* affix_columns[] = { L"Affix / built-in stat", L"Value" };
    for (int i = 0; i < 2; ++i) {
        LVCOLUMNW column = { LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM };
        column.pszText = (LPWSTR)affix_columns[i];
        column.cx = i ? 90 : 260;
        column.iSubItem = i;
        ListView_InsertColumn(g_owned_affix_list, i, &column);
    }
    g_owned_affix_detail = CreateWindowW(L"STATIC", L"Select an affix or a built-in stat.",
        WS_CHILD, 834, 454, 350, 30, g_window, NULL, g_module, NULL);
    g_owned_value_label = CreateWindowW(L"STATIC", L"Magnitude · float32 (units vary by affix)",
        WS_CHILD, 834, 460, 350, 18, g_window, NULL, g_module, NULL);
    g_owned_value_backdrop = CreateWindowW(L"STATIC", L"", WS_CHILD | SS_OWNERDRAW,
        834, 480, 212, 36, g_window, (HMENU)IDC_OWNED_VALUE_BACKDROP, g_module, NULL);
    g_owned_value_input = CreateWindowExW(0, L"EDIT", L"",
        WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_CENTER,
        834, 480, 212, 32, g_window, (HMENU)IDC_OWNED_VALUE_INPUT, g_module, NULL);
    SendMessageW(g_owned_value_input, EM_SETLIMITTEXT, 30, 0);
    g_owned_apply_value = create_dark_button(g_window, L"Set Value", 1054, 480,
        132, 32, IDC_OWNED_APPLY_VALUE);
    g_owned_donor_label = CreateWindowW(L"STATIC", L"Add from a currently owned item",
        WS_CHILD, 834, 520, 350, 18, g_window, NULL, g_module, NULL);
    g_owned_catalog_test = create_dark_button(g_window, L"Copy Owned",
        1046, 516, 138, 25, IDC_OWNED_CATALOG_TEST);
    g_owned_catalog_search_backdrop = CreateWindowW(L"STATIC", L"", WS_CHILD | SS_OWNERDRAW,
        834, 516, 204, 30, g_window, (HMENU)IDC_OWNED_CATALOG_SEARCH_BACKDROP, g_module, NULL);
    g_owned_catalog_search = CreateWindowExW(0, L"EDIT", L"",
        WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
        834, 516, 204, 25, g_window, (HMENU)IDC_OWNED_CATALOG_SEARCH, g_module, NULL);
    SetWindowSubclass(g_owned_catalog_search,owned_affix_search_proc,1,0);
    SendMessageW(g_owned_catalog_search, EM_SETLIMITTEXT, 60, 0);
    g_owned_donor_combo = CreateWindowW(WC_COMBOBOXW, L"",
        WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL,
        834, 540, 350, 230, g_window, (HMENU)IDC_OWNED_DONOR_COMBO, g_module, NULL);
    g_owned_catalog_value_backdrop = CreateWindowW(L"STATIC", L"", WS_CHILD | SS_OWNERDRAW,
        1092, 540, 90, 30, g_window, (HMENU)IDC_OWNED_CATALOG_VALUE_BACKDROP, g_module, NULL);
    g_owned_catalog_value_input = CreateWindowExW(0, L"EDIT", L"10",
        WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_CENTER,
        1092, 540, 90, 32, g_window, NULL, g_module, NULL);
    SendMessageW(g_owned_catalog_value_input, EM_SETLIMITTEXT, 30, 0);
    g_owned_add_affix = create_dark_button(g_window, L"Add Affix", 834, 584,
        110, 34, IDC_OWNED_ADD_AFFIX);
    g_owned_remove_affix = create_dark_button(g_window, L"Remove", 952, 584,
        110, 34, IDC_OWNED_REMOVE_AFFIX);
    g_owned_clear_affixes = create_dark_button(g_window, L"Clear All", 1070, 584,
        110, 34, IDC_OWNED_CLEAR_AFFIXES);
    g_owned_duplicate_weapon = create_dark_button(g_window, L"Duplicate", 1040, 180,
        142, 30, IDC_OWNED_DUPLICATE);
    EnableWindow(g_owned_duplicate_weapon, FALSE);
    g_owned_repair_weapon = create_dark_button(g_window, L"Repair Weapon", 834, 286,
        350, 30, IDC_OWNED_REPAIR);
    EnableWindow(g_owned_repair_weapon, FALSE);
    g_owned_restore = create_dark_button(g_window, L"Restore Original", 834, 626,
        171, 34, IDC_OWNED_RESTORE);
    g_owned_keep = create_dark_button(g_window, L"Keep Changes", 1013, 626,
        171, 34, IDC_OWNED_KEEP);
    EnableWindow(g_owned_restore, FALSE);
    EnableWindow(g_owned_keep, FALSE);
    g_owned_filter_label=CreateWindowW(L"STATIC",L"Affix Filters",WS_CHILD,
        30,520,200,18,g_window,NULL,g_module,NULL);
    g_owned_format_filter=CreateWindowW(WC_COMBOBOXW,L"",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
        30,544,220,220,g_window,(HMENU)IDC_OWNED_FORMAT_FILTER,g_module,NULL);
    g_owned_presence_filter=CreateWindowW(WC_COMBOBOXW,L"",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
        30,589,220,220,g_window,(HMENU)IDC_OWNED_PRESENCE_FILTER,g_module,NULL);
    for (const wchar_t* label : {L"All value formats",L"Percentage display",L"Raw / unspecified units"})
        SendMessageW(g_owned_format_filter,CB_ADDSTRING,0,(LPARAM)label);
    for (const wchar_t* label : {L"All matching affixes",L"Not on selected item",L"Already on selected item"})
        SendMessageW(g_owned_presence_filter,CB_ADDSTRING,0,(LPARAM)label);
    SendMessageW(g_owned_format_filter,CB_SETCURSEL,0,0);
    SendMessageW(g_owned_presence_filter,CB_SETCURSEL,0,0);
    style_dark_combo(g_owned_format_filter);
    style_dark_combo(g_owned_presence_filter);
    const HWND owned_controls[] = { g_owned_affix_list, g_owned_affix_detail, g_owned_value_label,
        g_owned_value_input, g_owned_apply_value, g_owned_donor_label,
        g_owned_donor_combo, g_owned_add_affix, g_owned_catalog_test,
        g_owned_catalog_value_input, g_owned_catalog_search, g_owned_remove_affix,
        g_owned_clear_affixes, g_owned_duplicate_weapon, g_owned_repair_weapon,
        g_owned_value_backdrop, g_owned_catalog_value_backdrop, g_owned_catalog_search_backdrop,
        g_owned_restore, g_owned_keep };
    for (HWND control : owned_controls) if (control) ShowWindow(control, SW_HIDE);
#endif
    g_quantity_label = CreateWindowW(L"STATIC", L"Quantity",
                                     WS_CHILD | WS_VISIBLE,
                                     834, 470, 260, 18, g_window, NULL, g_module, NULL);
    g_quantity_minus = create_dark_button(g_window, L"−", 834, 490, 32, 30, IDC_QUANTITY_MINUS);
    g_quantity_backdrop = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                        872, 490, 116, 30, g_window,
                                        (HMENU)IDC_QUANTITY_BACKDROP, g_module, NULL);
    g_quantity = CreateWindowExW(0, L"EDIT", L"1",
                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_CENTER,
                                  872, 490, 116, 30, g_window, (HMENU)IDC_QUANTITY, g_module, NULL);
    SendMessageW(g_quantity, EM_SETLIMITTEXT, 10, 0);
    g_quantity_plus = create_dark_button(g_window, L"+", 994, 490, 32, 30, IDC_QUANTITY_PLUS);
    g_favorite = create_dark_button(g_window, L"Favorite", 1038, 490, 72, 30, IDC_FAVORITE);
    g_copy_id = create_dark_button(g_window, L"Copy ID", 1116, 490, 72, 30, IDC_COPY_ID);

    g_actions_label = CreateWindowW(L"STATIC", L"Actions", WS_CHILD | WS_VISIBLE,
                                    834, 542, 180, 18, g_window, NULL, g_module, NULL);
    g_give_inventory = create_dark_button(g_window, L"Give to Inventory", 834, 562, 110, 32, IDC_GIVE_INVENTORY);
    g_give_stash = create_dark_button(g_window, L"Give to Stash", 951, 562, 110, 32, IDC_GIVE_STASH);
    g_remove_inventory = create_dark_button(g_window, L"Remove from Inventory", 1068, 562, 120, 32, IDC_REMOVE_INVENTORY);
    g_status = CreateWindowExW(0, L"STATIC", L"Loading game item database…",
                               WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
                               20, 682, 1184, 38, g_window, NULL, g_module, NULL);
    g_console_title = CreateWindowW(L"STATIC", L"Runtime Log", WS_CHILD | WS_VISIBLE,
                                    20, 730, 210, 18, g_window, NULL, g_module, NULL);
    g_log_filter = CreateWindowW(WC_COMBOBOXW, L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                                 1020, 730, 130, 180, g_window, (HMENU)IDC_LOG_FILTER, g_module, NULL);
    SendMessageW(g_log_filter, CB_ADDSTRING, 0, (LPARAM)L"All Messages");
    SendMessageW(g_log_filter, CB_SETCURSEL, 0, 0);
    g_clear_log = create_dark_button(g_window, L"Clear", 1160, 730, 76, 26, IDC_CLEAR_UI_LOG);
    g_runtime_log = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY |
                                    ES_AUTOVSCROLL | WS_VSCROLL,
                                    20, 760, 1184, 120, g_window, NULL, g_module, NULL);

    style_dark_combo(g_category_filter);
    style_dark_combo(g_rarity_filter);
    style_dark_combo(g_view_filter);
    style_dark_combo(g_weapon_level_filter);
    style_dark_combo(g_log_filter);
#ifdef IE_OWNED_AFFIX_EDITOR
    style_dark_combo(g_owned_donor_combo);
    if (!install_owned_affix_popup_style())
        log_line(L"[AFFIX_UI] Could not install pre-show picker positioning; native dropdown placement remains active.");
    // The adjacent catalogue magnitude field is 30px high, not the normal
    // filter's 39px combo. Keep the closed selection and value on one row.
    SendMessageW(g_owned_donor_combo, CB_SETITEMHEIGHT, (WPARAM)-1, ui_scale(28));
    SendMessageW(g_owned_donor_combo, CB_SETITEMHEIGHT, 0, ui_scale(48));
    SendMessageW(g_owned_donor_combo, CB_SETDROPPEDWIDTH, ui_scale(680), 0);
    SendMessageW(g_owned_value_input, EM_SETMARGINS, EC_LEFTMARGIN|EC_RIGHTMARGIN,0);
    SendMessageW(g_owned_catalog_value_input, EM_SETMARGINS, EC_LEFTMARGIN|EC_RIGHTMARGIN,0);
#endif

    g_tooltip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, NULL,
                                WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                g_window, NULL, g_module, NULL);
    if (g_tooltip) {
        SetWindowPos(g_tooltip, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        SetWindowTheme(g_tooltip, L"DarkMode_Explorer", NULL);
        SendMessageW(g_tooltip, TTM_SETTIPBKCOLOR, (WPARAM)kColorPanel, 0);
        SendMessageW(g_tooltip, TTM_SETTIPTEXTCOLOR, (WPARAM)kColorText, 0);
        SendMessageW(g_tooltip, TTM_SETMAXTIPWIDTH, 0, ui_scale(340));
        add_control_tooltip(g_minimize_button, L"Minimize catalogue");
        add_control_tooltip(g_maximize_button, L"Maximize or restore catalogue");
        add_control_tooltip(g_close_button, L"Close catalogue and return focus to the game");
        add_control_tooltip(g_return_game, L"Hide the catalogue and return keyboard focus to the game");
        add_control_tooltip(GetDlgItem(g_window, IDC_REFRESH_CATALOG), L"Reload GameItemCatalog.tsv");
        add_control_tooltip(g_favorite, L"Add or remove the selected item from Favorites");
        add_control_tooltip(g_copy_id, L"Copy the selected internal item identifier");
        add_control_tooltip(g_give_inventory, L"Give the selected item to the local player");
        add_control_tooltip(g_give_stash, L"Send the selected item directly to the player stash");
        add_control_tooltip(g_remove_inventory, L"Remove the selected quantity from player inventory");
        add_control_tooltip(g_quantity, L"Quantity: 1 to 2,147,483,647 (signed 4-byte maximum)");
        add_control_tooltip(g_reset_filters, L"Reset search and all catalogue filters");
        add_control_tooltip(g_clear_log, L"Clear the runtime action log");
#ifdef IE_OWNED_AFFIX_EDITOR
        add_control_tooltip(g_owned_catalog_test,
            L"All Affixes is the default. Copy Owned switches to real donor records from carried weapons/gear; All Affixes switches back. Search works in both modes.");
        add_control_tooltip(g_owned_catalog_value_input,
            L"Float32 magnitude, not necessarily a percentage. Scientific notation is accepted. Max finite storage is about 3.402823466e38; the affix factor may require a lower magnitude. Extreme values are not proven safe.");
        add_control_tooltip(g_owned_value_input,
            L"Edit the selected affix magnitude. Positive/negative and scientific notation are supported. Values are rounded to the game's float32 storage; overflowing factor conversions are refused.");
        add_control_tooltip(g_owned_format_filter,L"Filter the affix picker using verified game display formats, not guessed gameplay applicability.");
        add_control_tooltip(g_owned_keep,L"Finish the previous item's edit and discard its rollback snapshot. Does not save the game. Afterward another item can be edited.");
        add_control_tooltip(g_owned_restore,L"Restore the previous edited item's original affixes, not the newly selected item's. Ends that rollback session.");
        add_control_tooltip(g_owned_add_affix,L"Add the picker affix to this exact item. Already On means that ID is present; choose a different ID or edit its existing value.");
        add_control_tooltip(g_owned_presence_filter,L"Show all affixes, only new IDs, or IDs already attached to the exact selected item.");
        add_control_tooltip(g_owned_catalog_search,
            L"Search affixes by name or internal ID in either source mode. Copy Owned also matches the donor item's name and ID.");
        add_control_tooltip(g_owned_clear_affixes,
            L"Remove all native affixes from the selected item. Restore Original remains available while this instance stays carried.");
        add_control_tooltip(g_owned_duplicate_weapon,
            L"Create ONE native copy of this exact weapon, including affixes, attachment definitions and instance settings. Does not edit the original. Verify the copy in-game before clearing either weapon.");
        add_control_tooltip(g_owned_repair_weapon,
            L"Restore full durability through the native repair routine. Uses one remaining repair slot, no materials. Does not reset repair limits or edit affixes. Live validation pending.");
#endif
    }

    EnumChildWindows(g_window, [](HWND child, LPARAM) -> BOOL {
        wchar_t class_name[32] = {}; GetClassNameW(child, class_name, _countof(class_name));
        if (!_wcsicmp(class_name, WC_COMBOBOXW) || !_wcsicmp(class_name, L"Edit"))
            SetWindowTheme(child, L"DarkMode_Explorer", NULL);
        return TRUE;
    }, 0);
    apply_ui_fonts();
    layout_item_catalogue(g_window);
    load_item_catalog();
#ifdef IE_OWNED_AFFIX_EDITOR
    load_owned_affix_metadata();
    load_owned_builtin_stats();
#endif
    refresh_item_view();
    run_probe();
    ShowWindow(g_window, SW_HIDE);
#ifdef IE_DLTB_BRIDGE
    bridge_attach_window();
#endif
    MSG message;
    while (WaitForSingleObject(g_stop, 0) == WAIT_TIMEOUT && GetMessageW(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message); DispatchMessageW(&message);
    }
    if (g_window) DestroyWindow(g_window);
    destroy_ui_fonts();
    if (g_row_height_images) ImageList_Destroy(g_row_height_images);
#ifdef IE_OWNED_AFFIX_EDITOR
    if (g_owned_row_images) ImageList_Destroy(g_owned_row_images);
#endif
    if (g_background_brush) DeleteObject(g_background_brush);
    if (g_panel_brush) DeleteObject(g_panel_brush);
    if (g_input_brush) DeleteObject(g_input_brush);
    if (g_status_brush) DeleteObject(g_status_brush);
    if (g_sidebar_brush) DeleteObject(g_sidebar_brush);
    if (g_card_brush) DeleteObject(g_card_brush);
    if (g_header_brush) DeleteObject(g_header_brush);
    if (g_console_brush) DeleteObject(g_console_brush);
    if (g_window_icon_small) DestroyIcon(g_window_icon_small);
    if (g_window_icon_large) DestroyIcon(g_window_icon_large);
    if (g_brand_icon) DestroyIcon(g_brand_icon);
    return 0;
}

#include "RuntimeHotkeys.inl"

static DWORD WINAPI hotkey_thread(void*) {
    wchar_t config[MAX_PATH] = {};
    _snwprintf_s(config,_countof(config),_TRUNCATE,L"%sItemExtravaganza.config.ini",g_own_dir);
    const UINT toggle_vk = load_menu_toggle_vk(config);
    bool toggle_down = (GetAsyncKeyState(toggle_vk) & 0x8000) != 0;
    bool escape_down = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
    while (WaitForSingleObject(g_stop, 50) == WAIT_TIMEOUT) {
        const bool toggle_pressed = menu_hotkey_edge(GetAsyncKeyState(toggle_vk),&toggle_down);
        const bool escape_pressed = menu_hotkey_edge(GetAsyncKeyState(VK_ESCAPE),&escape_down);
        DWORD foreground_pid = 0;
        GetWindowThreadProcessId(GetForegroundWindow(),&foreground_pid);
        const bool foreground = foreground_pid == GetCurrentProcessId();
        // Also handles desktop/task switching while no hotkey is pressed.
        if (g_window) PostMessageW(g_window,WM_IE_MENU_SCOPE,0,0);
#if defined(IE_AFFIX_REMOVE_POC) && !defined(IE_OWNED_AFFIX_EDITOR)
        if ((GetAsyncKeyState(VK_F10) & 1) &&
            (GetAsyncKeyState(VK_CONTROL) & 0x8000) &&
            (GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
            HWND game_window = find_game_window();
            if (install_game_thread_dispatch(game_window))
                PostMessageW(game_window, WM_BEAST_AFFIX_POC, 0, 0);
        }
        if ((GetAsyncKeyState(VK_F11) & 1) &&
            (GetAsyncKeyState(VK_CONTROL) & 0x8000) &&
            (GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
            HWND game_window = find_game_window();
            if (install_game_thread_dispatch(game_window))
                PostMessageW(game_window, WM_BEAST_AFFIX_POC, 1, 0);
        }
        if ((GetAsyncKeyState(VK_F12) & 1) &&
            (GetAsyncKeyState(VK_CONTROL) & 0x8000) &&
            (GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
            HWND game_window = find_game_window();
            if (install_game_thread_dispatch(game_window))
                PostMessageW(game_window, WM_BEAST_AFFIX_POC, 2, 0);
        }
#endif
        if (toggle_pressed && foreground && g_window) {
            PostMessageW(g_window,WM_IE_MENU_SCOPE,1,0);
        }
        if (escape_pressed && foreground && g_window && IsWindowVisible(g_window)) {
            PostMessageW(g_window,WM_IE_MENU_SCOPE,2,0);
        }
    }
    return 0;
}

// Steam exposes the process a little before gamedll finishes loading. The
// launcher injects as soon as the real process exists, so discovery retries
// here instead of producing a one-shot false "module not loaded" result.
static DWORD WINAPI delayed_probe_thread(void*) {
    for (unsigned attempt = 0; attempt < 120 && WaitForSingleObject(g_stop, 500) == WAIT_TIMEOUT; ++attempt) {
        if (GetModuleHandleW(L"gamedll_ph_x64_rwdi.dll")) {
            if (!InterlockedCompareExchange(&g_ready, 1, 1)) run_probe();
            return 0;
        }
    }
    log_line(L"[NATIVE] gamedll did not load within 60 seconds; retry with Rescan native discovery.");
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        if (!is_beast_process()) return TRUE;
        g_module = instance;
        InitializeCriticalSection(&g_data_lock);
        wchar_t module_path[MAX_PATH] = {}; GetModuleFileNameW(instance, module_path, _countof(module_path));
        wchar_t* slash = wcsrchr(module_path, L'\\');
        if (slash) { slash[1] = 0; wcscpy_s(g_own_dir, _countof(g_own_dir), module_path); }
        wchar_t local[MAX_PATH] = {}; GetEnvironmentVariableW(L"LOCALAPPDATA", local, _countof(local));
        _snwprintf_s(g_log_path, _countof(g_log_path), _TRUNCATE, L"%s\\ItemExtravaganza.log", local);
        g_stop = CreateEventW(NULL, TRUE, FALSE, NULL);
        log_line(L"[LOADER] Item Extravaganza v%s runtime attached; pid=%lu", IE_VERSION_WTEXT, GetCurrentProcessId());
        HANDLE thread = CreateThread(NULL, 0, runtime_thread, NULL, 0, NULL); if (thread) CloseHandle(thread);
#ifdef IE_DLTB_BRIDGE
        bridge_start();   // горячая клавиша рантайма не нужна: Insert открывает меню DLTB.RIP
#else
        thread = CreateThread(NULL, 0, hotkey_thread, NULL, 0, NULL); if (thread) CloseHandle(thread);
#endif
        thread = CreateThread(NULL, 0, delayed_probe_thread, NULL, 0, NULL); if (thread) CloseHandle(thread);
    } else if (reason == DLL_PROCESS_DETACH) {
        if (g_stop) SetEvent(g_stop);
    }
    return TRUE;
}
