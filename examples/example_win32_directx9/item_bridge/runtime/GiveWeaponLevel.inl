// Opt-in preview: correct only newly given carried weapons AFTER normal Give.
// Verified current build: player wrapper 126088E stores player level at +68;
// ItemDescWithContext::GetLevel (A12C00 / slot1158) reads context+18 (=item+68).
// Save BaseItem constructor 16581E8 calls that getter and saves its result.
// No code patch, global level override, repeated Give, or per-frame enforcement.
struct RankGiveSnapshot {
    void* bucket;
    void* items[256];
    DWORD levels[256];
    unsigned count;
};
static bool g_rank_give_faulted;

static bool rank_give_level_valid(DWORD level) { return level >= 1 && level <= 15; }

static bool rank_give_item_matches(BYTE* base, void* item, const char* id, DWORD rank) {
    if (!is_readable_range(item, 0x1A0) ||
        *(void**)item != base + kPocItemVtableRva ||
        *(void**)((BYTE*)item + 0x50) != base + 0x289AAF0 ||
        *(DWORD*)((BYTE*)item + 0x140) != rank) return false;
    const BYTE* definition = *(BYTE**)((BYTE*)item + 0x58);
    if (!is_readable_range(definition, 16) ||
        *(void* const*)definition == base + 0x28963D0) return false;
    char found[160] = {};
    return owned_read_ascii((const char*)(*(uintptr_t const*)(definition + 8) &
        0xFFFFFFFFFFFFull), found, _countof(found)) && !_stricmp(found, id);
}

static bool rank_give_prepare(BYTE* base, void* player, int quantity,
                              RankGiveSnapshot* snapshot) {
    if (g_rank_give_faulted || quantity < 1 || quantity > 256) return false;
    static const BYTE normalize[] = {0x41,0x89,0x47,0x68};
    static const BYTE getter[] = {0x8B,0x41,0x18,0xC3};
    static const BYTE save_getter[] = {0x48,0x8B,0x46,0x38,0xFF,0x90,0x58,0x11,0,0};
    if (!validate_function_bytes(base, 0x126088E, normalize, sizeof(normalize), L"give level normalization") ||
        !validate_function_bytes(base, 0xA12C00, getter, sizeof(getter), L"actual item level getter") ||
        !validate_function_bytes(base, 0x16581E8, save_getter, sizeof(save_getter), L"saved item level getter") ||
        !is_readable_range(base + 0x289AAF0 + 0x1158, 8) ||
        *(void**)(base + 0x289AAF0 + 0x1158) != base + 0xA12C00) return false;
    snapshot->bucket = owned_duplicate_bucket(base, player);
    if (!snapshot->bucket || !owned_duplicate_members(snapshot->bucket,
        snapshot->items, &snapshot->count) ||
        (unsigned)quantity > 256 - snapshot->count) return false;
    for (unsigned i = 0; i < snapshot->count; ++i) {
        if (!is_readable_range(snapshot->items[i], 0x6C) ||
            *(void**)snapshot->items[i] != base + kPocItemVtableRva) return false;
        for (unsigned j = 0; j < i; ++j)
            if (snapshot->items[j] == snapshot->items[i]) return false;
        snapshot->levels[i] = *(DWORD*)((BYTE*)snapshot->items[i] + 0x68);
    }
    return true;
}

static bool rank_give_apply(BYTE* base, void* player, const RankGiveSnapshot* before,
                            const char* id, DWORD rank, int quantity) {
    if (!rank_give_level_valid(rank) || quantity < 1 || quantity > 256 ||
        before->count > 256 || (unsigned)quantity > 256 - before->count) return false;
    void* after[256] = {}; unsigned count = 0;
    if (owned_duplicate_bucket(base, player) != before->bucket ||
        !owned_duplicate_members(before->bucket, after, &count) ||
        count != before->count + (unsigned)quantity) return false;
    // Validate the complete change set before writing any level. Existing
    // items must all remain and no existing instance may be selected as new.
    for (unsigned i = 0; i < count; ++i)
        for (unsigned j = 0; j < i; ++j) if (after[i] == after[j]) return false;
    for (unsigned i = 0; i < before->count; ++i) {
        bool retained = false;
        for (unsigned j = 0; j < count; ++j) retained |= after[j] == before->items[i];
        if (!retained || !is_readable_range(before->items[i], 0x6C) ||
            *(DWORD*)((BYTE*)before->items[i] + 0x68) != before->levels[i]) return false;
    }
    void* targets[256] = {}; DWORD old_levels[256] = {}; unsigned fresh = 0;
    for (unsigned i = 0; i < count; ++i) {
        bool existed = false;
        for (unsigned j = 0; j < before->count; ++j) existed |= after[i] == before->items[j];
        if (existed) continue;
        if (!rank_give_item_matches(base, after[i], id, rank)) return false;
        targets[fresh] = after[i];
        old_levels[fresh++] = *(DWORD*)((BYTE*)after[i] + 0x68);
    }
    if (fresh != (unsigned)quantity) return false;
    unsigned written = 0;
    bool success = false;
    __try {
        for (; written < fresh; ++written) {
            // This is the exact scalar assignment used by the native wrapper,
            // scoped to proven new objects, not a guessed descriptor offset.
            *(volatile DWORD*)((BYTE*)targets[written] + 0x68) = rank;
        }
        success = true;
        for (unsigned i = 0; i < fresh; ++i)
            success &= rank_give_item_matches(base, targets[i], id, rank) &&
                *(volatile DWORD*)((BYTE*)targets[i] + 0x68) == rank;
    } __except (EXCEPTION_EXECUTE_HANDLER) { success = false; }
    if (!success) {
        g_rank_give_faulted = true;
        // No engine calls/yields between the verified snapshot and scalar writes.
        // Best-effort scalar rollback; never destroy/re-give a created weapon.
        for (unsigned i = 0; i < written; ++i) {
            __try {
                if (rank_give_item_matches(base, targets[i], id, rank))
                    *(volatile DWORD*)((BYTE*)targets[i] + 0x68) = old_levels[i];
            } __except (EXCEPTION_EXECUTE_HANDLER) {}
        }
        return false;
    }
    for (unsigned i = 0; i < fresh; ++i)
        log_line(L"[GIVE_LEVEL] item=%S instance=%p before=%lu actual_level=%lu requested_rank=%lu",
                 id, targets[i], old_levels[i], rank, rank);
    log_line(L"[GIVE_LEVEL] verified requested=%d corrected=%u dispatch_count=1 existing_untouched=1; refresh/save-reload test required",
             quantity, fresh);
    return true;
}

static bool rank_give_unchanged(BYTE* base, void* player, const RankGiveSnapshot* before) {
    void* items[256] = {}; unsigned count = 0;
    if (owned_duplicate_bucket(base, player) != before->bucket ||
        !owned_duplicate_members(before->bucket, items, &count) || count != before->count ||
        memcmp(items, before->items, count * sizeof(void*))) return false;
    for (unsigned i = 0; i < count; ++i)
        if (!is_readable_range(items[i], 0x6C) ||
            *(DWORD*)((BYTE*)items[i] + 0x68) != before->levels[i]) return false;
    return true;
}
