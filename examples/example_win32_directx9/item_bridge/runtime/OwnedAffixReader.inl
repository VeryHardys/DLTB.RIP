// Research-only owned weapon/gear snapshot. This reads exact game-owned item
// instances; no addresses survive a process restart and no game memory is
// written. Keep snapshot refresh on the game's window thread.

static bool owned_read_ascii(const char* source, char* output, SIZE_T capacity) {
    if (!source || !output || capacity < 2) return false;
    for (SIZE_T i = 0; i < capacity; ++i) {
        if (!is_readable_range(source + i, 1)) return false;
        const char c = source[i];
        if (!c) { output[i] = 0; return i != 0; }
        if ((unsigned char)c < 0x20) return false;
        output[i] = c;
    }
    output[capacity - 1] = 0;
    return false; // Truncated IDs cannot identify a safe target.
}

static bool owned_read_affix(BYTE* base, const BYTE* record, OwnedAffixView* out) {
    const uintptr_t mask = 0xFFFFFFFFFFFFull;
    if (!record || !out) return false;
    const uintptr_t reference = *(const uintptr_t*)record & mask;
    if (!is_readable_range((const void*)reference, 8)) return false;
    const uintptr_t header = *(const uintptr_t*)reference & mask;
    if (!is_readable_range((const void*)header, 8)) return false;
    const uintptr_t name = *(const uintptr_t*)header & mask;
    if (!owned_read_ascii((const char*)name, out->id, _countof(out->id))) return false;
    const uintptr_t effect = *(const uintptr_t*)(record + 0x10) & mask;
    if (!is_readable_range((const void*)effect, 0x18)) return false;
    out->effect_vtable = *(const uintptr_t*)effect;
    out->stat_change = out->effect_vtable == (uintptr_t)(base + 0x299F070);
    if (!out->stat_change) return true; // ID is still known; value type is not.
    const uintptr_t definition = *(const uintptr_t*)(effect + 8) & mask;
    if (!is_readable_range((const void*)(definition + 0x1C), 4)) return false;
    out->raw = *(const float*)(effect + 0x10);
    out->factor = *(const float*)(definition + 0x1C);
    return _finite(out->raw) && _finite(out->factor);
}

// InventoryContainerDI's native EA3530 routes through +0x38 + index*8.
// Index 4 (+58) is InventoryMain; index 5 (+60) is InventoryItems. The
// latter also owns crafting materials: only RTTI-proven ItemDescOutfitPart
// definitions may enter the gear editor. Never identify gear by ID spelling.
static int owned_collect_instances(BYTE* base, void* player, void** output,
                                   bool* gear_flags, unsigned limit, unsigned* total) {
    *total = 0;
    if (!validate_give_build((HMODULE)base) ||
        !is_readable_range((BYTE*)player + 0x450, 8)) return 1;
    BYTE* container = *(BYTE**)((BYTE*)player + 0x450);
    if (!is_readable_range(container, 0x68) || *(void**)container != base + 0x29AE8C0) return 2;
    BYTE* main_inventory = *(BYTE**)(container + 0x58);
    if (!is_readable_range(main_inventory, 0x58) || *(void**)main_inventory != base + 0x29AEA00) return 2;
    BYTE* buckets[] = { *(BYTE**)(main_inventory + 0x50), *(BYTE**)(container + 0x60) };
    for (unsigned b = 0; b < _countof(buckets); ++b) {
        BYTE* bucket = buckets[b];
        if (!is_readable_range(bucket, 0x48) ||
            *(void**)bucket != base + (b ? 0x29AECC0 : 0x29AE670)) return 2;
        const uint64_t packed = *(const uint64_t*)(bucket + 0x38);
        const unsigned inline_count = (unsigned)(packed >> 56);
        const unsigned count = inline_count ? inline_count - 1 : *(const unsigned*)(bucket + 0x40);
        const unsigned capacity = *(const unsigned*)(bucket + 0x44);
        const void* const* instances = (const void* const*)(packed & 0xFFFFFFFFFFFFull);
        if (count > 512 || (!inline_count && (count > capacity || capacity > 1024)) ||
            (count && !is_readable_range(instances, count * sizeof(void*)))) return 3;
        for (unsigned i = 0; i < count; ++i) {
            const BYTE* item = (const BYTE*)instances[i];
            if (!is_readable_range(item, 0x118) || *(void* const*)item != base + 0x295FC18) return 5;
            const BYTE* descriptor = *(BYTE* const*)(item + 0x58);
            if (!is_readable_range(descriptor, 0x10)) return 5;
            const bool gear = *(void* const*)descriptor == base + 0x28963D0;
            if (b && !gear) continue; // Material/charm/etc is not editable gear.
            if (!b && gear) return 5; // Unexpected cross-container ownership.
            bool duplicate = false;
            for (unsigned j = 0; j < *total; ++j) duplicate |= output[j] == item;
            if (duplicate) continue;
            if (*total >= limit) return 3;
            output[*total] = (void*)item;
            gear_flags[(*total)++] = gear;
        }
    }
    return 0;
}

// SetPlayerOutfitPart resolves component index 3410850 through 1EF3970.
// FEBD20 stores the exact owned InventoryItem* in six slots at +20. This
// reads identity only: no equip calls, descriptor guesses or item flag writes.
static bool owned_read_worn_slots(BYTE* base, void* player, void* const* carried,
                                  const bool* gear_flags, unsigned count,
                                  unsigned* worn_slots) {
    for (unsigned i = 0; i < count; ++i) worn_slots[i] = 0;
    static const BYTE getter[] = { 0x48,0x8B,0x81,0xC8,0x01,0x00,0x00 };
    static const BYTE setter[] = { 0x4C,0x8B,0x6C,0xD9,0x20,0x4D,0x3B,0xE8 };
    if (!validate_function_bytes(base, 0x1EF3970, getter, sizeof(getter), L"outfit component getter") ||
        !validate_function_bytes(base, 0xFEBD4A, setter, sizeof(setter), L"outfit slot setter") ||
        !is_readable_range(base + 0x3410850, 4) ||
        !is_readable_range((BYTE*)player + 0x1C8, 16)) return false;
    const int index = *(const int*)(base + 0x3410850);
    const uint64_t packed = *(const uint64_t*)((BYTE*)player + 0x1C8);
    const unsigned inlined = (unsigned)(packed >> 56);
    const unsigned n = inlined ? inlined - 1 : *(const unsigned*)((BYTE*)player + 0x1D0);
    const unsigned capacity = *(const unsigned*)((BYTE*)player + 0x1D4);
    const BYTE* components = (const BYTE*)(packed & 0xFFFFFFFFFFFFull);
    if (index < 0 || (unsigned)index >= n || n > 256 ||
        (!inlined && (capacity > 512 || n > capacity)) ||
        !is_readable_range(components, n * sizeof(void*))) return false;
    const BYTE* outfit = *(BYTE* const*)(components + index * sizeof(void*));
    if (!is_readable_range(outfit, 0x50) || *(void* const*)outfit != base + 0x29B9A00 ||
        *(void* const*)(outfit + 8) != player)
        return false;
    unsigned verified[512] = {};
    if (count > _countof(verified)) return false;
    for (unsigned slot = 0; slot < 6; ++slot) {
        const void* item = *(void* const*)(outfit + 0x20 + slot * sizeof(void*));
        if (!item) continue;
        unsigned row = 0;
        while (row < count && carried[row] != item) ++row;
        // Unknown, non-carried or duplicated slot identities invalidate the
        // whole badge snapshot, never pretend that a guessed item is equipped.
        if (row == count || !gear_flags[row] || verified[row]) return false;
        verified[row] = slot + 1;
    }
    memcpy(worn_slots, verified, count * sizeof(unsigned));
    return true;
}

static int owned_scan_weapons(BYTE* base, void* player) {
    void* instances[_countof(g_owned_weapons)] = {};
    bool gear_flags[_countof(g_owned_weapons)] = {};
    unsigned count = 0;
    const int collected = owned_collect_instances(base, player, instances, gear_flags,
                                                  _countof(instances), &count);
    if (collected) return collected;
    unsigned worn_slots[_countof(g_owned_weapons)] = {};
    const bool equipment_known = owned_read_worn_slots(base, player, instances,
        gear_flags, count, worn_slots);
    OwnedWeaponView* snapshot = (OwnedWeaponView*)HeapAlloc(
        GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(g_owned_weapons));
    if (!snapshot) return 4;
    int result = 0;
    for (unsigned i = 0; i < count; ++i) {
        const BYTE* item = (const BYTE*)instances[i];
        OwnedWeaponView* row = &snapshot[i];
        if (!is_readable_range(item, 0x118) ||
            *(void* const*)item != base + 0x295FC18) { result = 5; break; }
        row->instance = (void*)item;
        row->is_gear = gear_flags[i];
        row->worn_slot = worn_slots[i];
        // Actual game-owned level; catalogue Rank is a separate generation tier.
        row->weapon_level = row->is_gear ? 0 : *(const DWORD*)(item + 0x68);
        void* descriptor = *(void* const*)(item + 0x58);
        if (!is_readable_range((BYTE*)descriptor + 8, 8)) { result = 5; break; }
        const uintptr_t id = *(const uintptr_t*)((BYTE*)descriptor + 8) & 0xFFFFFFFFFFFFull;
        if (!owned_read_ascii((const char*)id, row->id, _countof(row->id))) {
            result = 5; break;
        }
        strcpy_s(row->name, row->id);
        const uint64_t affix_packed = *(const uint64_t*)(item + 0x108);
        const unsigned affix_inline = (unsigned)(affix_packed >> 56);
        row->affix_count = affix_inline ? affix_inline - 1 :
            *(const unsigned*)(item + 0x110);
        const unsigned capacity = *(const unsigned*)(item + 0x114);
        if (row->affix_count > _countof(row->affixes) ||
            (!affix_inline && (capacity > 64 || row->affix_count > capacity))) { result = 6; break; }
        const BYTE* records = (const BYTE*)(affix_packed & 0xFFFFFFFFFFFFull);
        if (row->affix_count &&
            !is_readable_range(records, row->affix_count * 0x20)) { result = 6; break; }
        for (unsigned j = 0; j < row->affix_count; ++j) {
            if (!owned_read_affix(base, records + j * 0x20, &row->affixes[j])) {
                result = 7; break;
            }
        }
        if (result) break;
    }
    if (!result) {
        EnterCriticalSection(&g_data_lock);
        for (unsigned i = 0; i < count; ++i) {
            for (DWORD j = 0; j < g_item_count; ++j) {
                if (_stricmp(snapshot[i].id, g_items[j].id)) continue;
                strcpy_s(snapshot[i].name, g_items[j].name);
                strcpy_s(snapshot[i].category, g_items[j].category);
                strcpy_s(snapshot[i].rarity, g_items[j].rarity);
                snapshot[i].favorite = g_items[j].favorite;
                snapshot[i].recent_order = g_items[j].recent_order;
                break;
            }
            snapshot[i].same_id_ordinal = 1;
            for (unsigned earlier = 0; earlier < i; ++earlier)
                if (!_stricmp(snapshot[earlier].id, snapshot[i].id))
                    ++snapshot[i].same_id_ordinal;
        }
        memcpy(g_owned_weapons, snapshot, sizeof(g_owned_weapons));
        g_owned_count = count;
        g_owned_equipment_known = equipment_known;
        LeaveCriticalSection(&g_data_lock);
    }
    HeapFree(GetProcessHeap(), 0, snapshot);
    return result;
}
