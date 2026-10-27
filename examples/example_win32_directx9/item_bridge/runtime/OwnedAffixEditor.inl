// Development-only native edits. Every request is resolved again from the
// player's carried weapon vector on the game thread. One pending transaction
// is retained for Restore/Keep; never publish before the general UI is tested.

struct OwnedEditFingerprint {
    char id[128];
    float raw;
    uintptr_t effect_vtable;
    bool stat_change;
};
alignas(16) static BYTE g_owned_saved_records[kOwnedAffixTestLimit][0x20] = {};
static unsigned g_owned_saved_count = 0;
static void* g_owned_saved_item = NULL;
static char g_owned_saved_id[160] = {};
static uint64_t g_owned_saved_flags = 0;
static bool g_owned_transaction = false;
static OwnedEditFingerprint g_owned_after[kOwnedAffixTestLimit] = {};
static unsigned g_owned_after_count = 0;
static bool g_owned_lookup_probe_done = false;
static bool g_owned_lookup_probe_passed = false;
static void* owned_locate_carried(BYTE* base, void* player, void* target,
                                 const char* expected_id);
static bool owned_vector(void* item, PocRecordVector** out, BYTE** records,
                         unsigned* count);

static void* owned_find_catalog_definition(BYTE* base, const char* id) {
    if (!id || !id[0]) return NULL;
    typedef void* (__fastcall *RegistryFn)();
    typedef void* (__fastcall *FindFn)(void*, const uint64_t*);
    void* registry = ((RegistryFn)(base + 0xDBAD70))();
    if (!is_readable_range(registry, 0x10)) return NULL;
    const uint64_t view = (1ull << 60) |
        ((uint64_t)(uintptr_t)id & 0x00FFFFFFFFFFFFFFull);
    return ((FindFn)(base + 0xDA6DA0))(registry, &view);
}

// Research-only, read-only check before attempting arbitrary-ID construction.
// The game's saved-affix loader calls these exact two functions and passes an
// engine string to the second one. A tagged, null-terminated string view uses
// the comparator's non-owning representation; no inventory object is changed.
static int owned_probe_catalog_lookup(BYTE* base, void* player) {
    static const BYTE registry_bytes[] = { 0x48, 0x83, 0xEC, 0x28 };
    static const BYTE find_bytes[] = { 0xE9, 0x0B, 0x00, 0x00, 0x00 };
    if (!validate_give_build((HMODULE)base) ||
        !validate_function_bytes(base, 0xDBAD70, registry_bytes,
                                 sizeof(registry_bytes), L"affix registry") ||
        !validate_function_bytes(base, 0xDA6DA0, find_bytes,
                                 sizeof(find_bytes), L"affix definition lookup")) return 1;
    if (!owned_find_catalog_definition(base, "Weapon_Damage_vs_Humans")) return 2;
    unsigned known = 0;
    for (DWORD i = 0; i < g_owned_count && known < 3; ++i) {
        const OwnedWeaponView* weapon = &g_owned_weapons[i];
        if (!weapon->affix_count) continue;
        void* item = owned_locate_carried(base, player, weapon->instance, weapon->id);
        PocRecordVector* vector = NULL; BYTE* records = NULL; unsigned count = 0;
        if (!item || !owned_vector(item, &vector, &records, &count)) return 3;
        for (unsigned j = 0; j < count && known < 3; ++j) {
            const void* actual = (const void*)(*(const uintptr_t*)(records + j * 0x20) &
                                               0xFFFFFFFFFFFFull);
            const void* found = owned_find_catalog_definition(base, weapon->affixes[j].id);
            if (!found || found != actual) return 4;
            ++known;
        }
    }
    if (!known) return 5;
    const char* candidates[] = {
        "Weapon_Damage_vs_Infected", "Weapon_Damage_at_Night", "Stamina_Regen_on_Hit"
    };
    for (const char* id : candidates) {
        const BYTE* definition = (const BYTE*)owned_find_catalog_definition(base, id);
        if (!is_readable_range(definition, 0x30)) return 6;
        const uint64_t packed = *(const uint64_t*)(definition + 0x20);
        const unsigned inline_count = (unsigned)(packed >> 56);
        const unsigned count = inline_count ? inline_count - 1 :
            *(const unsigned*)(definition + 0x28);
        const void* effects = (const void*)(packed & 0xFFFFFFFFFFFFull);
        if (!count || count > 16 || !is_readable_range(effects, count * 8)) return 7;
        log_line(L"[OWNED_AFFIX] catalogue lookup id=%S definition=%p configured_effects=%u",
                 id, definition, count);
    }
    log_line(L"[OWNED_AFFIX] catalogue lookup matched %u existing native definitions; no writes",
             known);
    return 0;
}

static void* owned_locate_carried(BYTE* base, void* player, void* target,
                                 const char* expected_id) {
    if (!target || !expected_id || !expected_id[0]) return NULL;
    void* instances[_countof(g_owned_weapons)] = {};
    bool gear_flags[_countof(g_owned_weapons)] = {};
    unsigned count = 0;
    if (owned_collect_instances(base, player, instances, gear_flags, _countof(instances), &count)) return NULL;
    bool present = false;
    for (unsigned i = 0; i < count; ++i) present |= instances[i] == target;
    if (!present || !is_readable_range(target, 0x118) ||
        *(void**)target != base + kPocItemVtableRva) return NULL;
    void* descriptor = *(void**)((BYTE*)target + 0x58);
    if (!is_readable_range((BYTE*)descriptor + 8, 8)) return NULL;
    const uintptr_t address = *(const uintptr_t*)((BYTE*)descriptor + 8) & 0xFFFFFFFFFFFFull;
    char id[160] = {};
    if (!owned_read_ascii((const char*)address, id, _countof(id)) ||
        _stricmp(id, expected_id)) return NULL;
    return target;
}

static bool owned_vector(void* item, PocRecordVector** out, BYTE** records,
                         unsigned* count) {
    if (!is_readable_range(item, 0x118)) return false;
    PocRecordVector* vector = (PocRecordVector*)((BYTE*)item + 0x108);
    const unsigned n = poc_vector_count(vector);
    if (n > kOwnedAffixTestLimit || (!(vector->packed >> 56) &&
        (vector->capacity > 64 || n > vector->capacity)))
        return false;
    BYTE* data = (BYTE*)(vector->packed & 0xFFFFFFFFFFFFull);
    if (n && !is_readable_range(data, n * 0x20)) return false;
    *out = vector;
    *records = data;
    *count = n;
    return true;
}

static bool owned_gear_effects_isolated(BYTE* base, void* player, void* item,
                                       BYTE* records, unsigned count) {
    void* instances[_countof(g_owned_weapons)] = {};
    bool gear_flags[_countof(g_owned_weapons)] = {};
    unsigned total = 0;
    if (owned_collect_instances(base, player, instances, gear_flags, _countof(instances), &total)) return false;
    for (unsigned i = 0; i < total; ++i) {
        if (instances[i] == item) continue;
        PocRecordVector* other_vector = NULL; BYTE* other_records = NULL; unsigned n = 0;
        if (!owned_vector(instances[i], &other_vector, &other_records, &n)) return false;
        for (unsigned a = 0; a < count; ++a)
            for (unsigned b = 0; b < n; ++b)
                if ((*(uintptr_t*)(records + a*0x20 + 0x10) & 0xFFFFFFFFFFFFull) ==
                    (*(uintptr_t*)(other_records + b*0x20 + 0x10) & 0xFFFFFFFFFFFFull)) return false;
    }
    return true;
}

static bool owned_record_matches(BYTE* base, const BYTE* records, unsigned count,
                                 unsigned index, const char* id, float expected_raw) {
    if (index >= count) return false;
    OwnedAffixView observed = {};
    return owned_read_affix(base, records + index * 0x20, &observed) &&
           !_stricmp(observed.id, id) && observed.stat_change &&
           observed.raw == expected_raw;
}

static bool owned_snapshot(BYTE* base, void* item, const char* id,
                           PocRecordVector* vector, BYTE* records, unsigned count) {
    if (g_owned_transaction || count > kOwnedAffixTestLimit || !id) return false;
    PocRecordCopyFn copy = (PocRecordCopyFn)(base + kPocRecordCopyRva);
    g_owned_saved_flags = vector->packed;
    g_owned_saved_item = item;
    strcpy_s(g_owned_saved_id, id);
    for (unsigned i = 0; i < count; ++i) {
        copy(g_owned_saved_records[i], records + i * 0x20);
        g_owned_saved_count = i + 1;
    }
    g_owned_transaction = true;
    return true;
}

static bool owned_capture_after(BYTE* base, void* item) {
    PocRecordVector* vector = NULL; BYTE* records = NULL; unsigned count = 0;
    if (!owned_vector(item, &vector, &records, &count)) return false;
    for (unsigned i = 0; i < count; ++i) {
        OwnedAffixView view = {};
        if (!owned_read_affix(base, records + i * 0x20, &view)) return false;
        strcpy_s(g_owned_after[i].id, view.id);
        g_owned_after[i].raw = view.raw;
        g_owned_after[i].effect_vtable = view.effect_vtable;
        g_owned_after[i].stat_change = view.stat_change;
    }
    g_owned_after_count = count;
    return true;
}

static bool owned_matches_after(BYTE* base, BYTE* records, unsigned count) {
    if (!g_owned_transaction || count != g_owned_after_count) return false;
    for (unsigned i = 0; i < count; ++i) {
        OwnedAffixView view = {};
        if (!owned_read_affix(base, records + i * 0x20, &view) ||
            _stricmp(view.id, g_owned_after[i].id) ||
            view.effect_vtable != g_owned_after[i].effect_vtable ||
            view.stat_change != g_owned_after[i].stat_change ||
            (view.stat_change && view.raw != g_owned_after[i].raw)) return false;
    }
    return true;
}

static bool owned_active_target_unchanged(BYTE* base, void* player) {
    if (!g_owned_transaction || !g_owned_saved_item) return false;
    void* item = owned_locate_carried(base, player, g_owned_saved_item,
                                     g_owned_saved_id);
    PocRecordVector* vector = NULL; BYTE* records = NULL; unsigned count = 0;
    return item && owned_vector(item, &vector, &records, &count) &&
           owned_matches_after(base, records, count);
}

static bool owned_request_affixes_match(const OwnedEditRequest* request,
                                        const OwnedAffixView* current,
                                        unsigned count) {
    if (!request || count != request->expected_affix_count ||
        count > kOwnedAffixTestLimit) return false;
    for (unsigned i = 0; i < count; ++i) {
        const OwnedAffixView* expected = &request->expected_affixes[i];
        const OwnedAffixView* observed = &current[i];
        if (_stricmp(expected->id, observed->id) ||
            expected->effect_vtable != observed->effect_vtable ||
            expected->stat_change != observed->stat_change ||
            (observed->stat_change &&
             (expected->raw != observed->raw ||
              expected->factor != observed->factor))) return false;
    }
    return true;
}

static bool owned_request_item_matches(BYTE* base, void* item,
                                       const OwnedEditRequest* request) {
    if (!request || !is_readable_range((BYTE*)item + 0x58, 8)) return false;
    const BYTE* descriptor = *(BYTE**)((BYTE*)item + 0x58);
    if (!is_readable_range(descriptor, 8) ||
        ((*(void* const*)descriptor == base + 0x28963D0) != request->is_gear)) return false;
    PocRecordVector* vector = NULL; BYTE* records = NULL; unsigned count = 0;
    if (!owned_vector(item, &vector, &records, &count) ||
        count != request->expected_affix_count) return false;
    OwnedAffixView current[kOwnedAffixTestLimit] = {};
    for (unsigned i = 0; i < count; ++i)
        if (!owned_read_affix(base, records + i * 0x20, &current[i])) return false;
    return owned_request_affixes_match(request, current, count);
}

// A sleep/save can replace an InventoryItem without changing its affixes.
// Rebind only to a single carried instance with the same ID AND every native
// record matching the UI snapshot. Duplicate indistinguishable copies fail
// closed so an action cannot silently target another weapon.
static void* owned_resolve_request_target(BYTE* base, void* player,
                                          const OwnedEditRequest* request) {
    void* item = owned_locate_carried(base, player, request->target,
                                     request->item_id);
    if (item && owned_request_item_matches(base, item, request)) return item;
    if (owned_scan_weapons(base, player)) return NULL;
    void* match = NULL;
    for (unsigned i = 0; i < g_owned_count; ++i) {
        const OwnedWeaponView* weapon = &g_owned_weapons[i];
        if (weapon->is_gear != request->is_gear || _stricmp(weapon->id, request->item_id) ||
            !owned_request_affixes_match(request, weapon->affixes,
                                         weapon->affix_count)) continue;
        if (match) return NULL;
        match = weapon->instance;
    }
    if (match && match != request->target) {
        g_owned_rebound_during_edit = true;
        log_line(L"[OWNED_AFFIX] rebound exact matching carried item old=%p new=%p id=%S",
                 request->target, match, request->item_id);
    }
    return match;
}

static void owned_release_snapshot(BYTE* base) {
    PocRecordDestroyFn destroy = (PocRecordDestroyFn)(base + kPocRecordDestroyRva);
    for (unsigned i = 0; i < g_owned_saved_count; ++i)
        destroy(g_owned_saved_records[i]);
    g_owned_saved_count = 0;
    g_owned_saved_item = NULL;
    g_owned_saved_id[0] = 0;
    g_owned_transaction = false;
    g_owned_after_count = 0;
}

// An operation that cannot be verified returns the whole item to its original
// session snapshot rather than leaving an uncertain intermediate edit behind.
static int owned_abort_transaction(BYTE* base, PocRecordVector* vector,
                                   int error) {
    if (!g_owned_transaction || !vector) return error;
    PocRecordVector original = poc_make_vector(g_owned_saved_flags,
                                               g_owned_saved_records,
                                               g_owned_saved_count);
    ((PocVectorAssignFn)(base + kPocVectorAssignRva))(vector, &original);
    if (poc_vector_count(vector) != g_owned_saved_count) return 30;
    owned_release_snapshot(base);
    return error;
}

static int owned_build_catalog_test_record(BYTE* base, const char* id,
                                           float effective, BYTE* record,
                                           bool* initialized) {
    static const BYTE init_bytes[] = { 0x48, 0x8D, 0x41, 0x10, 0x48, 0xC7, 0x01 };
    static const BYTE clone_bytes[] = { 0x48, 0x89, 0x5C, 0x24, 0x10, 0x48 };
    if (!initialized || !record || !g_owned_lookup_probe_passed ||
        !owned_affix_metadata(id) ||
        !validate_function_bytes(base, 0xD33300, init_bytes,
                                 sizeof(init_bytes), L"affix record initialize") ||
        !validate_function_bytes(base, 0xD77CE0, clone_bytes,
                                 sizeof(clone_bytes), L"affix definition clone")) return 20;
    *initialized = false;
    const BYTE* definition = (const BYTE*)owned_find_catalog_definition(base, id);
    if (!is_readable_range(definition, 0x30)) return 21;
    const uint64_t configured = *(const uint64_t*)(definition + 0x20);
    const unsigned inline_count = (unsigned)(configured >> 56);
    const unsigned effects = inline_count ? inline_count - 1 :
        *(const unsigned*)(definition + 0x28);
    if (effects != 1 || !is_readable_range(
            (const void*)(configured & 0xFFFFFFFFFFFFull), sizeof(void*))) return 22;
    typedef void* (__fastcall *InitFn)(void*);
    typedef void (__fastcall *CloneFn)(void*, const void*);
    ((InitFn)(base + 0xD33300))(record);
    *initialized = true;
    ((CloneFn)(base + 0xD77CE0))(record, definition);
    const uint64_t packed = *(const uint64_t*)(record + 8);
    const unsigned stored = (unsigned)(packed >> 56);
    const unsigned cloned_effects = stored ? stored - 1 :
        *(const unsigned*)(record + 0x10);
    if (cloned_effects != 1) return 23;
    OwnedAffixView view = {};
    if (!owned_read_affix(base, record, &view) || !view.stat_change ||
        _stricmp(view.id, id) || !is_readable_range(
            (const void*)(*(const uintptr_t*)(record + 0x10) &
                          0xFFFFFFFFFFFFull), 0x18) ||
        !_finite(effective)) return 24;
    float raw = 0.0f;
    if (!owned_encode_magnitude(effective, view.factor, &raw)) return 25;
    BYTE* effect = (BYTE*)(*(const uintptr_t*)(record + 0x10) & 0xFFFFFFFFFFFFull);
    *(float*)(effect + 0x10) = raw;
    OwnedAffixView after = {};
    if (!owned_read_affix(base, record, &after) || after.raw != raw ||
        _stricmp(after.id, id)) return 26;
    return 0;
}

#include "OwnedWeaponDuplicate.inl"
#include "OwnedWeaponRepair.inl"

static int owned_execute_edit(BYTE* base, void* player,
                              const OwnedEditRequest* request) {
    if (!request || !validate_give_build((HMODULE)base) ||
        !validate_function_bytes(base, kPocRecordCopyRva, kPocRecordCopyBytes,
                                 sizeof(kPocRecordCopyBytes), L"affix record copy") ||
        !validate_function_bytes(base, kPocRecordDestroyRva, kPocRecordDestroyBytes,
                                 sizeof(kPocRecordDestroyBytes), L"affix record destroy") ||
        !validate_function_bytes(base, kPocVectorAssignRva, kPocVectorAssignBytes,
                                 sizeof(kPocVectorAssignBytes), L"affix vector assign")) return 1;
    if (request->is_gear && (request->kind == OWNED_EDIT_DUPLICATE ||
        request->kind == OWNED_EDIT_REPAIR)) return 76;
    if (request->kind == OWNED_EDIT_DUPLICATE)
        return owned_duplicate_weapon(base, player, request);
    if (request->kind == OWNED_EDIT_REPAIR)
        return owned_repair_weapon(base, player, request);
    if (g_owned_transaction && !owned_active_target_unchanged(base, player)) {
        // The game may replace an InventoryItem after sleep/save/refresh.
        // Its old snapshot cannot safely be applied to the new instance.
        owned_release_snapshot(base);
        if (request->kind == OWNED_EDIT_RESTORE ||
            request->kind == OWNED_EDIT_KEEP) return 31;
    }
    if (request->kind == OWNED_EDIT_KEEP) {
        if (!g_owned_transaction) return 2;
        owned_release_snapshot(base);
        return 0;
    }
    if (request->kind == OWNED_EDIT_RESTORE) {
        if (!g_owned_transaction || !g_owned_saved_item ||
            !owned_locate_carried(base, player, g_owned_saved_item,
                                  g_owned_saved_id)) return 2;
        PocRecordVector* vector = NULL; BYTE* records = NULL; unsigned count = 0;
        if (!owned_vector(g_owned_saved_item, &vector, &records, &count) ||
            !owned_matches_after(base, records, count)) return 3;
        PocRecordVector original = poc_make_vector(g_owned_saved_flags,
                                                   g_owned_saved_records,
                                                   g_owned_saved_count);
        PocVectorAssignFn assign = (PocVectorAssignFn)(base + kPocVectorAssignRva);
        assign(vector, &original);
        if (poc_vector_count(vector) != g_owned_saved_count) return 4;
        owned_release_snapshot(base);
        return 0;
    }
    if (request->kind != OWNED_EDIT_VALUE && request->kind != OWNED_EDIT_REMOVE &&
        request->kind != OWNED_EDIT_ADD && request->kind != OWNED_EDIT_CLEAR &&
        request->kind != OWNED_EDIT_ADD_CATALOG_TEST) return 16;
    void* item = owned_resolve_request_target(base, player, request);
    if (!item) return 33;
    PocRecordVector* vector = NULL; BYTE* records = NULL; unsigned count = 0;
    if (!owned_vector(item, &vector, &records, &count)) return 7;
    if (request->is_gear && !owned_gear_effects_isolated(base, player, item, records, count)) return 78;
    // This editor's temporary source vector uses the verified heap-backed
    // representation. An inline-packed vector needs a different constructor.
    if ((vector->packed >> 56) != 0) return 32;
    if (g_owned_transaction &&
        (item != g_owned_saved_item || _stricmp(request->item_id, g_owned_saved_id) ||
         !owned_matches_after(base, records, count))) return 5;
    if (request->kind == OWNED_EDIT_VALUE || request->kind == OWNED_EDIT_REMOVE) {
        OwnedAffixView observed = {};
        if (request->affix_index >= count ||
            !owned_read_affix(base, records + request->affix_index * 0x20,
                              &observed) ||
            _stricmp(observed.id, request->affix_id) ||
            observed.effect_vtable != request->expected_effect_vtable ||
            (observed.stat_change && observed.raw != request->expected_raw) ||
            (request->kind == OWNED_EDIT_VALUE && !observed.stat_change)) return 8;
    }
    if ((request->kind == OWNED_EDIT_ADD ||
         request->kind == OWNED_EDIT_ADD_CATALOG_TEST) &&
        count >= kOwnedAffixTestLimit) return 9;
    if (request->kind == OWNED_EDIT_REMOVE && request->affix_index >= count) return 15;
    if (request->kind == OWNED_EDIT_CLEAR && count == 0) return 15;
    float new_raw = 0.0f;
    if (request->kind == OWNED_EDIT_VALUE) {
        OwnedAffixView current = {};
        if (!owned_read_affix(base, records + request->affix_index * 0x20,
                              &current) || !current.stat_change ||
            !owned_encode_magnitude(request->new_effective, current.factor, &new_raw)) return 13;
        const uintptr_t effect = *(const uintptr_t*)(
            records + request->affix_index * 0x20 + 0x10) & 0xFFFFFFFFFFFFull;
        if (!is_readable_range((const void*)(effect + 0x10), 4)) return 13;
    }
    BYTE* donor_records = NULL; unsigned donor_count = 0;
    PocRecordVector* donor_vector = NULL;
    if (request->kind == OWNED_EDIT_ADD) {
        void* donor = owned_locate_carried(base, player, request->donor.instance,
                                           request->donor.item_id);
        if (!donor || !owned_vector(donor, &donor_vector, &donor_records, &donor_count) ||
            !owned_record_matches(base, donor_records, donor_count,
                                  request->donor.affix_index,
                                  request->donor.affix_id, request->donor.raw)) return 10;
        // Duplicate native IDs may have special engine semantics; gate until tested.
        for (unsigned i = 0; i < count; ++i) {
            OwnedAffixView existing = {};
            if (!owned_read_affix(base, records + i * 0x20, &existing)) return 10;
            if (!_stricmp(existing.id, request->donor.affix_id)) return 11;
        }
    }
    if (request->kind == OWNED_EDIT_ADD_CATALOG_TEST) {
        // The selected carried instance and its current records are checked
        // above. Catalogue IDs must come from the game-sourced list.
        if (!owned_affix_metadata(request->affix_id) ||
            !_finite(request->new_effective)) return 27;
        for (unsigned i = 0; i < count; ++i) {
            OwnedAffixView existing = {};
            if (!owned_read_affix(base, records + i * 0x20, &existing) ||
                !_stricmp(existing.id, request->affix_id)) return 11;
        }
    }
    alignas(16) BYTE catalog_record[0x20] = {};
    bool catalog_initialized = false;
    if (request->kind == OWNED_EDIT_ADD_CATALOG_TEST) {
        const int built = owned_build_catalog_test_record(base, request->affix_id,
            request->new_effective, catalog_record, &catalog_initialized);
        if (built) {
            if (catalog_initialized)
                ((PocRecordDestroyFn)(base + kPocRecordDestroyRva))(catalog_record);
            return built;
        }
    }
    if (!g_owned_transaction &&
        !owned_snapshot(base, item, request->item_id, vector, records, count)) {
        if (catalog_initialized)
            ((PocRecordDestroyFn)(base + kPocRecordDestroyRva))(catalog_record);
        return 12;
    }
    if (request->kind == OWNED_EDIT_VALUE) {
        BYTE* record = records + request->affix_index * 0x20;
        const uintptr_t effect = *(const uintptr_t*)(record + 0x10) & 0xFFFFFFFFFFFFull;
        *(float*)(effect + 0x10) = new_raw;
        return owned_capture_after(base, item) ? 0 :
            owned_abort_transaction(base, vector, 14);
    }
    PocRecordVector changed = {};
    alignas(16) BYTE source[kOwnedAffixTestLimit][0x20] = {};
    unsigned source_count = 0;
    PocRecordCopyFn copy = (PocRecordCopyFn)(base + kPocRecordCopyRva);
    if (request->kind == OWNED_EDIT_CLEAR) {
        // Empty source vector asks the game's native assignment routine to
        // destroy all destination records. Snapshot remains for restoration.
    } else if (request->kind == OWNED_EDIT_REMOVE) {
        if (request->affix_index >= count) return 15;
        for (unsigned i = 0; i < count; ++i)
            if (i != request->affix_index)
                copy(source[source_count++], records + i * 0x20);
    } else if (request->kind == OWNED_EDIT_ADD ||
               request->kind == OWNED_EDIT_ADD_CATALOG_TEST) {
        for (unsigned i = 0; i < count; ++i)
            copy(source[source_count++], records + i * 0x20);
        if (request->kind == OWNED_EDIT_ADD) {
            copy(source[source_count], donor_records + request->donor.affix_index * 0x20);
        } else {
            copy(source[source_count], catalog_record);
        }
        ++source_count;
    } else return 16;
    changed = poc_make_vector(vector->packed,
                              source_count ? (void*)source : NULL, source_count);
    PocVectorAssignFn assign = (PocVectorAssignFn)(base + kPocVectorAssignRva);
    assign(vector, &changed);
    PocRecordDestroyFn destroy = (PocRecordDestroyFn)(base + kPocRecordDestroyRva);
    for (unsigned i = 0; i < source_count; ++i) destroy(source[i]);
    if (catalog_initialized) destroy(catalog_record);
    int post_error = poc_vector_count(vector) == source_count ? 0 : 17;
    if (!post_error && !owned_capture_after(base, item)) post_error = 18;
    if (request->kind == OWNED_EDIT_ADD_CATALOG_TEST) {
        if (!post_error) {
            const OwnedEditFingerprint* inserted = &g_owned_after[source_count - 1];
            PocRecordVector* confirmed_vector = NULL;
            BYTE* confirmed_records = NULL;
            unsigned confirmed_count = 0;
            OwnedAffixView confirmed = {};
            if (_stricmp(inserted->id, request->affix_id) || !inserted->stat_change ||
                !owned_vector(item, &confirmed_vector, &confirmed_records, &confirmed_count) ||
                confirmed_count != source_count ||
                !owned_read_affix(base, confirmed_records + (source_count - 1) * 0x20,
                                  &confirmed) ||
                (confirmed.raw * confirmed.factor - request->new_effective > 0.001f ||
                 request->new_effective - confirmed.raw * confirmed.factor > 0.001f))
                post_error = 28;
        }
        if (post_error) return owned_abort_transaction(base, vector, post_error);
    }
    if (post_error) return owned_abort_transaction(base, vector, post_error);
    return 0;
}
