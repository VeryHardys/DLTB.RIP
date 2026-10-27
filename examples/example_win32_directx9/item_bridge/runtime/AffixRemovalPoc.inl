// Development-only affix removal experiment. Never compile into a public build.
// Runs on the game's window thread; only targets the verified disposable
// Peace Keeper copy with Damage vs Humans = 30. Ctrl+Shift+F10 removes its
// first affix, Ctrl+Shift+F11 restores all three from a native deep snapshot.

static const SIZE_T kPocItemVtableRva = 0x295FC18;
static const SIZE_T kPocEffectVtableRva = 0x299F070;
static const SIZE_T kPocRecordCopyRva = 0xD33270;
static const SIZE_T kPocRecordDestroyRva = 0xD43090;
static const SIZE_T kPocVectorAssignRva = 0x9B2940;
static const BYTE kPocRecordCopyBytes[] = {
    0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18, 0x57
};
static const BYTE kPocRecordDestroyBytes[] = {
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x6C, 0x24, 0x10, 0x48
};
static const BYTE kPocVectorAssignBytes[] = {
    0x48, 0x8B, 0xC4, 0x56, 0x41, 0x56, 0x48, 0x83, 0xEC, 0x58
};

struct PocRecordVector {
    uint64_t packed;
    uint32_t count;
    uint32_t capacity;
};
static_assert(sizeof(PocRecordVector) == 16, "Unexpected affix-vector header");
typedef void* (__fastcall *PocRecordCopyFn)(void*, const void*);
typedef void (__fastcall *PocRecordDestroyFn)(void*);
typedef void (__fastcall *PocVectorAssignFn)(void*, const void*);

alignas(16) static BYTE g_poc_saved_records[3][0x20] = {};
static unsigned g_poc_saved_count = 0;
static void* g_poc_saved_item = NULL;
static uint64_t g_poc_packed_flags = 0;
static unsigned g_poc_mode = 0; // 0: idle, 1: removed, 2: donor affix added

static unsigned poc_vector_count(const PocRecordVector* vector) {
    const unsigned inline_count = (unsigned)(vector->packed >> 56);
    return inline_count ? inline_count - 1 : vector->count;
}

static bool poc_affix_id(const BYTE* record, const char* expected) {
    const uintptr_t mask = 0xFFFFFFFFFFFFull;
    const uintptr_t reference = *(const uintptr_t*)record & mask;
    if (!is_readable_range((const void*)reference, 8)) return false;
    const uintptr_t header = *(const uintptr_t*)reference & mask;
    if (!is_readable_range((const void*)header, 8)) return false;
    const uintptr_t string = *(const uintptr_t*)header & mask;
    const SIZE_T length = strlen(expected) + 1;
    return length < 128 && is_readable_range((const void*)string, length) &&
           !memcmp((const void*)string, expected, length);
}

static bool poc_target_record(BYTE* base, void* item) {
    if (!is_readable_range(item, 0x118) ||
        *(void**)item != base + kPocItemVtableRva) return false;
    void* descriptor = *(void**)((BYTE*)item + 0x58);
    if (!is_readable_range(descriptor, 0x10)) return false;
    const uintptr_t name = *(uintptr_t*)((BYTE*)descriptor + 8) & 0xFFFFFFFFFFFFull;
    static const char target_id[] = "dlc_ft_WPN_15HB_04_graffiti_r5";
    if (!is_readable_range((const void*)name, sizeof(target_id)) ||
        memcmp((const void*)name, target_id, sizeof(target_id))) return false;
    PocRecordVector* vector = (PocRecordVector*)((BYTE*)item + 0x108);
    if (poc_vector_count(vector) != 3 || vector->capacity < 3 ||
        vector->capacity > 12) return false;
    const uintptr_t data = vector->packed & 0xFFFFFFFFFFFFull;
    if (!is_readable_range((const void*)data, 3 * 0x20)) return false;
    const BYTE* records = (const BYTE*)data;
    if (!poc_affix_id(records, "Weapon_Damage_vs_Humans") ||
        !poc_affix_id(records + 0x20, "Affix_Outfit_RandomHealer_HealingBoost") ||
        !poc_affix_id(records + 0x40, "PowerAttack_Stamina_Cost_Reduction")) return false;
    const uintptr_t effect = *(const uintptr_t*)(records + 0x10) & 0xFFFFFFFFFFFFull;
    return is_readable_range((const void*)effect, 0x14) &&
           *(void**)effect == base + kPocEffectVtableRva &&
           *(float*)(effect + 0x10) == 30.0f;
}

static bool poc_donor_record(BYTE* base, void* item) {
    if (!is_readable_range(item, 0x118) ||
        *(void**)item != base + kPocItemVtableRva) return false;
    void* descriptor = *(void**)((BYTE*)item + 0x58);
    if (!is_readable_range(descriptor, 0x10)) return false;
    const uintptr_t name = *(uintptr_t*)((BYTE*)descriptor + 8) & 0xFFFFFFFFFFFFull;
    static const char target_id[] = "dlc_ft_WPN_15HB_04_graffiti_r5";
    if (!is_readable_range((const void*)name, sizeof(target_id)) ||
        memcmp((const void*)name, target_id, sizeof(target_id))) return false;
    const PocRecordVector* vector = (const PocRecordVector*)((BYTE*)item + 0x108);
    if (poc_vector_count(vector) != 3 || vector->capacity < 3 ||
        vector->capacity > 12) return false;
    const BYTE* records = (const BYTE*)(vector->packed & 0xFFFFFFFFFFFFull);
    if (!is_readable_range(records, 3 * 0x20) ||
        !poc_affix_id(records, "Weapon_Parkour_Damage_Multiplier") ||
        !poc_affix_id(records + 0x20, "Affix_Outfit_RandomHealer_HealingBoost") ||
        !poc_affix_id(records + 0x40, "PowerAttack_Stamina_Cost_Reduction")) return false;
    const uintptr_t effect = *(const uintptr_t*)(records + 0x10) & 0xFFFFFFFFFFFFull;
    return is_readable_range((const void*)effect, 0x14) &&
           *(void**)effect == base + kPocEffectVtableRva &&
           *(float*)(effect + 0x10) == 75.0f;
}

static void* poc_find_disposable(BYTE* base, void* player) {
    if (!is_readable_range((BYTE*)player + 0x450, 8)) return NULL;
    void* container = *(void**)((BYTE*)player + 0x450);
    if (!is_readable_range((BYTE*)container + 0x58, 8)) return NULL;
    void* main_inventory = *(void**)((BYTE*)container + 0x58);
    if (!is_readable_range((BYTE*)main_inventory + 0x50, 8)) return NULL;
    void* weapons = *(void**)((BYTE*)main_inventory + 0x50);
    if (!is_readable_range((BYTE*)weapons + 0x48, 8)) return NULL;
    const uint64_t packed = *(uint64_t*)((BYTE*)weapons + 0x38);
    const unsigned inline_count = (unsigned)(packed >> 56);
    const unsigned count = inline_count ? inline_count - 1 :
        *(unsigned*)((BYTE*)weapons + 0x40);
    const uintptr_t data = packed & 0xFFFFFFFFFFFFull;
    if (!count || count > 128 || !is_readable_range((const void*)data, count * 8))
        return NULL;
    void* found = NULL;
    for (unsigned i = 0; i < count; ++i) {
        void* candidate = ((void**)data)[i];
        if (!poc_target_record(base, candidate)) continue;
        if (found) return NULL; // Ambiguous matching copies: fail closed.
        found = candidate;
    }
    return found;
}

static void* poc_find_donor(BYTE* base, void* player) {
    if (!is_readable_range((BYTE*)player + 0x450, 8)) return NULL;
    void* container = *(void**)((BYTE*)player + 0x450);
    if (!is_readable_range((BYTE*)container + 0x58, 8)) return NULL;
    void* main_inventory = *(void**)((BYTE*)container + 0x58);
    if (!is_readable_range((BYTE*)main_inventory + 0x50, 8)) return NULL;
    void* weapons = *(void**)((BYTE*)main_inventory + 0x50);
    if (!is_readable_range((BYTE*)weapons + 0x48, 8)) return NULL;
    const uint64_t packed = *(uint64_t*)((BYTE*)weapons + 0x38);
    const unsigned count = (packed >> 56) ? (unsigned)(packed >> 56) - 1 :
        *(unsigned*)((BYTE*)weapons + 0x40);
    const void* data = (const void*)(packed & 0xFFFFFFFFFFFFull);
    if (!count || count > 128 || !is_readable_range(data, count * 8)) return NULL;
    void* found = NULL;
    for (unsigned i = 0; i < count; ++i) {
        void* candidate = ((void* const*)data)[i];
        if (!poc_donor_record(base, candidate)) continue;
        if (found) return NULL; // Never guess between matching originals.
        found = candidate;
    }
    return found;
}

static PocRecordVector poc_make_vector(uint64_t flags, void* records, unsigned count) {
    PocRecordVector vector = {};
    vector.packed = (flags & 0xFFFF000000000000ull) |
                    ((uintptr_t)records & 0xFFFFFFFFFFFFull);
    vector.count = count;
    vector.capacity = count;
    return vector;
}

static int poc_change_affix(BYTE* base, void* player, bool restore) {
    if (!validate_give_build((HMODULE)base) ||
        !validate_function_bytes(base, kPocRecordCopyRva, kPocRecordCopyBytes,
                                 sizeof(kPocRecordCopyBytes), L"affix record copy") ||
        !validate_function_bytes(base, kPocRecordDestroyRva, kPocRecordDestroyBytes,
                                 sizeof(kPocRecordDestroyBytes), L"affix record destroy") ||
        !validate_function_bytes(base, kPocVectorAssignRva, kPocVectorAssignBytes,
                                 sizeof(kPocVectorAssignBytes), L"affix vector assign")) return 1;
    PocRecordCopyFn copy = (PocRecordCopyFn)(base + kPocRecordCopyRva);
    PocRecordDestroyFn destroy = (PocRecordDestroyFn)(base + kPocRecordDestroyRva);
    PocVectorAssignFn assign = (PocVectorAssignFn)(base + kPocVectorAssignRva);
    if (restore) {
        if (!g_poc_saved_item || g_poc_saved_count != 3 ||
            (g_poc_mode != 1 && g_poc_mode != 2) ||
            !is_readable_range(g_poc_saved_item, 0x118)) return 2;
        // The item must still be in carried inventory, with only the two
        // expected entries. Never restore into a stale or moved item pointer.
        void* item = g_poc_saved_item;
        void* container = *(void**)((BYTE*)player + 0x450);
        void* main_inventory = is_readable_range((BYTE*)container + 0x58, 8)
            ? *(void**)((BYTE*)container + 0x58) : NULL;
        void* weapons = is_readable_range((BYTE*)main_inventory + 0x50, 8)
            ? *(void**)((BYTE*)main_inventory + 0x50) : NULL;
        if (!is_readable_range((BYTE*)weapons + 0x48, 8)) return 3;
        uint64_t packed = *(uint64_t*)((BYTE*)weapons + 0x38);
        unsigned count = (packed >> 56) ? (unsigned)(packed >> 56) - 1 :
            *(unsigned*)((BYTE*)weapons + 0x40);
        void** data = (void**)(packed & 0xFFFFFFFFFFFFull);
        bool present = false;
        if (count > 128 || !is_readable_range(data, count * 8)) return 3;
        for (unsigned i = 0; i < count; ++i) present |= data[i] == item;
        if (!present || *(void**)item != base + kPocItemVtableRva) return 3;
        PocRecordVector* vector = (PocRecordVector*)((BYTE*)item + 0x108);
        if (poc_vector_count(vector) != (g_poc_mode == 1 ? 2u : 4u)) return 4;
        const BYTE* current = (const BYTE*)(vector->packed & 0xFFFFFFFFFFFFull);
        if (g_poc_mode == 1) {
            if (!is_readable_range(current, 0x40) ||
                !poc_affix_id(current, "Affix_Outfit_RandomHealer_HealingBoost") ||
                !poc_affix_id(current + 0x20, "PowerAttack_Stamina_Cost_Reduction")) return 4;
        } else {
            if (!is_readable_range(current, 0x80) ||
                !poc_affix_id(current, "Weapon_Damage_vs_Humans") ||
                !poc_affix_id(current + 0x20, "Affix_Outfit_RandomHealer_HealingBoost") ||
                !poc_affix_id(current + 0x40, "PowerAttack_Stamina_Cost_Reduction") ||
                !poc_affix_id(current + 0x60, "Weapon_Parkour_Damage_Multiplier")) return 4;
        }
        PocRecordVector snapshot = poc_make_vector(g_poc_packed_flags,
                                                  g_poc_saved_records, 3);
        assign(vector, &snapshot);
        if (poc_vector_count(vector) != 3 || !poc_target_record(base, item)) return 5;
        for (unsigned i = 0; i < 3; ++i) destroy(g_poc_saved_records[i]);
        g_poc_saved_count = 0;
        g_poc_saved_item = NULL;
        g_poc_mode = 0;
        return 0;
    }

    if (g_poc_saved_count) return 6;
    void* item = poc_find_disposable(base, player);
    if (!item) return 7;
    PocRecordVector* vector = (PocRecordVector*)((BYTE*)item + 0x108);
    BYTE* original = (BYTE*)(vector->packed & 0xFFFFFFFFFFFFull);
    g_poc_packed_flags = vector->packed;
    for (unsigned i = 0; i < 3; ++i) {
        copy(g_poc_saved_records[i], original + i * 0x20);
        g_poc_saved_count = i + 1;
    }
    g_poc_saved_item = item;
    g_poc_mode = 1;
    // Source aliases the independent native clones, not the soon-to-be
    // destroyed original records. The engine's assignment deep-copies again.
    BYTE kept[2][0x20] = {};
    memcpy(kept[0], g_poc_saved_records[1], 0x20);
    memcpy(kept[1], g_poc_saved_records[2], 0x20);
    PocRecordVector subset = poc_make_vector(g_poc_packed_flags, kept, 2);
    assign(vector, &subset);
    if (poc_vector_count(vector) != 2) return 8;
    const BYTE* after = (const BYTE*)(vector->packed & 0xFFFFFFFFFFFFull);
    if (!is_readable_range(after, 0x40) ||
        !poc_affix_id(after, "Affix_Outfit_RandomHealer_HealingBoost") ||
        !poc_affix_id(after + 0x20, "PowerAttack_Stamina_Cost_Reduction")) return 8;
    return 0;
}

static int poc_add_donor_affix(BYTE* base, void* player) {
    if (!validate_give_build((HMODULE)base) ||
        !validate_function_bytes(base, kPocRecordCopyRva, kPocRecordCopyBytes,
                                 sizeof(kPocRecordCopyBytes), L"affix record copy") ||
        !validate_function_bytes(base, kPocRecordDestroyRva, kPocRecordDestroyBytes,
                                 sizeof(kPocRecordDestroyBytes), L"affix record destroy") ||
        !validate_function_bytes(base, kPocVectorAssignRva, kPocVectorAssignBytes,
                                 sizeof(kPocVectorAssignBytes), L"affix vector assign")) return 1;
    if (g_poc_saved_count || g_poc_mode) return 6;
    void* item = poc_find_disposable(base, player);
    void* donor = poc_find_donor(base, player);
    if (!item || !donor || item == donor) return 7;
    PocRecordVector* target = (PocRecordVector*)((BYTE*)item + 0x108);
    const PocRecordVector* source = (const PocRecordVector*)((BYTE*)donor + 0x108);
    const BYTE* target_records = (const BYTE*)(target->packed & 0xFFFFFFFFFFFFull);
    const BYTE* donor_records = (const BYTE*)(source->packed & 0xFFFFFFFFFFFFull);
    PocRecordCopyFn copy = (PocRecordCopyFn)(base + kPocRecordCopyRva);
    PocRecordDestroyFn destroy = (PocRecordDestroyFn)(base + kPocRecordDestroyRva);
    PocVectorAssignFn assign = (PocVectorAssignFn)(base + kPocVectorAssignRva);
    g_poc_packed_flags = target->packed;
    for (unsigned i = 0; i < 3; ++i) {
        copy(g_poc_saved_records[i], target_records + i * 0x20);
        g_poc_saved_count = i + 1;
    }
    g_poc_saved_item = item;
    g_poc_mode = 2;
    alignas(16) BYTE donor_copy[0x20] = {};
    copy(donor_copy, donor_records);
    alignas(16) BYTE combined[4][0x20] = {};
    for (unsigned i = 0; i < 3; ++i)
        memcpy(combined[i], g_poc_saved_records[i], 0x20);
    memcpy(combined[3], donor_copy, 0x20);
    PocRecordVector expanded = poc_make_vector(g_poc_packed_flags, combined, 4);
    assign(target, &expanded);
    destroy(donor_copy);
    if (poc_vector_count(target) != 4) return 8;
    const BYTE* after = (const BYTE*)(target->packed & 0xFFFFFFFFFFFFull);
    if (!is_readable_range(after, 0x80) ||
        !poc_affix_id(after, "Weapon_Damage_vs_Humans") ||
        !poc_affix_id(after + 0x20, "Affix_Outfit_RandomHealer_HealingBoost") ||
        !poc_affix_id(after + 0x40, "PowerAttack_Stamina_Cost_Reduction") ||
        !poc_affix_id(after + 0x60, "Weapon_Parkour_Damage_Multiplier")) return 8;
    return 0;
}
