// Native full-instance copy, deliberately separate from ordinary inventory.give.
// EA3250 copies the 0x160-byte state using D34890/D7C670, constructs a fresh
// InventoryItem, checks native capacity, inserts it and notifies observers.
// Descriptor copy 9D7F90/9F8DD0 deep-copies overrides and affix effects and
// copies attachment definitions into an independent vector. Never memcpy an
// InventoryItem: its runtime identity, ownership and callbacks must be native.

struct OwnedDuplicateState {
    BYTE scalars[0x160];
    uintptr_t attachments[32];
    char attachment_ids[32][160];
    BYTE attachment_state[32][0x8C];
    unsigned attachment_count;
    uintptr_t attachment_array;
    uintptr_t override_object;
    OwnedAffixView affixes[kOwnedAffixTestLimit];
    uintptr_t effects[kOwnedAffixTestLimit];
    uintptr_t affix_array;
    unsigned affix_count;
};

static void* owned_duplicate_bucket(BYTE* base, void* player) {
    if (!is_readable_range((BYTE*)player + 0x450, 8)) return NULL;
    BYTE* carried = *(BYTE**)((BYTE*)player + 0x450);
    if (!is_readable_range(carried, 0x60)) return NULL;
    BYTE* inventory = *(BYTE**)(carried + 0x58);
    if (!is_readable_range(inventory, 0x58)) return NULL;
    BYTE* bucket = *(BYTE**)(inventory + 0x50);
    if (!is_readable_range(bucket, 0x48) ||
        *(void**)bucket != base + 0x29AE670 ||
        *(void**)(base + 0x29AE670 + 0x38) != base + 0xEA3250 ||
        *(void**)(base + 0x29AE670 + 0x18) != base + 0xF17C00) return NULL;
    return bucket;
}

static bool owned_duplicate_members(void* bucket, void** items, unsigned* count) {
    const PocRecordVector* vector = (const PocRecordVector*)((BYTE*)bucket + 0x38);
    const unsigned n = poc_vector_count(vector);
    const void* data = (const void*)(vector->packed & 0xFFFFFFFFFFFFull);
    if (n > 256 || (n && !is_readable_range(data, n * sizeof(void*)))) return false;
    if (n) memcpy(items, data, n * sizeof(void*));
    *count = n;
    return true;
}

static bool owned_duplicate_capture(BYTE* base, void* item, OwnedDuplicateState* out) {
    if (!is_readable_range(item, 0x178) ||
        *(void**)item != base + kPocItemVtableRva) return false;
    memset(out, 0, sizeof(*out));
    const BYTE* state = (const BYTE*)item + 0x18;
    // Only fields explicitly copied by the native state/descriptor assignment.
    // Pointer vectors, callbacks and new runtime identity are not compared as bytes.
    struct Range { unsigned offset, size; };
    static const Range ranges[] = {
        {0, 0x10}, {0x34, 4}, {0x40, 0x30}, {0x88, 0x64},
        {0x100, 8}, {0x110, 0x1C}, {0x144, 0x10}
    };
    for (const Range& range : ranges)
        memcpy(out->scalars + range.offset, state + range.offset, range.size);
    const PocRecordVector* attachments = (const PocRecordVector*)((BYTE*)item + 0x88);
    out->attachment_count = poc_vector_count(attachments);
    out->attachment_array = attachments->packed & 0xFFFFFFFFFFFFull;
    if (out->attachment_count > _countof(out->attachments) ||
        (out->attachment_count && !is_readable_range(
            (const void*)out->attachment_array, out->attachment_count * 8))) return false;
    if (out->attachment_count) memcpy(out->attachments,
        (const void*)out->attachment_array, out->attachment_count * 8);
    for (unsigned i = 0; i < out->attachment_count; ++i) {
        const BYTE* context = (const BYTE*)out->attachments[i];
        if (!context) continue;
        // Entries are ItemDescWithContext subobjects, NOT static definition
        // pointers. The native override clone rebinds them to its own contexts.
        if (!is_readable_range(context, 0xD8) ||
            *(void* const*)context != base + 0x289AAF0) return false;
        const BYTE* definition = *(BYTE* const*)(context + 8);
        if (!is_readable_range(definition, 0x10) || !owned_read_ascii(
            (const char*)(*(uintptr_t const*)(definition + 8) & 0xFFFFFFFFFFFFull),
            out->attachment_ids[i], _countof(out->attachment_ids[i]))) return false;
        memcpy(out->attachment_state[i], context + 0x10, 0x28);
        memcpy(out->attachment_state[i] + 0x28, context + 0x50, 0x64);
    }
    out->override_object = *(uintptr_t*)((BYTE*)item + 0x98);
    if (out->override_object && !is_readable_range((const void*)out->override_object, 0x568))
        return false;
    PocRecordVector* vector = NULL; BYTE* records = NULL; unsigned count = 0;
    if (!owned_vector(item, &vector, &records, &count)) return false;
    out->affix_count = count;
    out->affix_array = vector->packed & 0xFFFFFFFFFFFFull;
    for (unsigned i = 0; i < count; ++i) {
        // This preview's reader decodes one effect per affix. Refuse an
        // unverified multi-effect representation instead of claiming equality.
        const PocRecordVector* effects = (const PocRecordVector*)(records + i * 0x20 + 8);
        if (poc_vector_count(effects) != 1 ||
            !owned_read_affix(base, records + i * 0x20, &out->affixes[i])) return false;
        out->effects[i] = *(uintptr_t*)(records + i * 0x20 + 0x10) & 0xFFFFFFFFFFFFull;
    }
    return true;
}

static bool owned_duplicate_equal(const OwnedDuplicateState* a,
                                  const OwnedDuplicateState* b, bool independent) {
    if (memcmp(a->scalars, b->scalars, sizeof(a->scalars)) ||
        a->attachment_count != b->attachment_count || a->affix_count != b->affix_count ||
        (!!a->override_object != !!b->override_object)) return false;
    for (unsigned i = 0; i < a->attachment_count; ++i) {
        if (!!a->attachments[i] != !!b->attachments[i] ||
            strcmp(a->attachment_ids[i], b->attachment_ids[i]) ||
            memcmp(a->attachment_state[i], b->attachment_state[i], 0x8C) ||
            (!independent && a->attachments[i] != b->attachments[i])) return false;
    }
    if (independent && ((a->affix_count && a->affix_array == b->affix_array) ||
        (a->attachment_count && a->attachment_array == b->attachment_array) ||
        (a->override_object && a->override_object == b->override_object))) return false;
    if (!independent && (a->affix_array != b->affix_array ||
        a->attachment_array != b->attachment_array ||
        a->override_object != b->override_object)) return false;
    for (unsigned i = 0; i < a->affix_count; ++i) {
        const OwnedAffixView* x = &a->affixes[i];
        const OwnedAffixView* y = &b->affixes[i];
        if (strcmp(x->id, y->id) || x->effect_vtable != y->effect_vtable ||
            x->stat_change != y->stat_change ||
            (x->stat_change && (x->raw != y->raw || x->factor != y->factor)) ||
            (independent ? a->effects[i] == b->effects[i] : a->effects[i] != b->effects[i]))
            return false;
    }
    return true;
}

static int owned_duplicate_weapon(BYTE* base, void* player,
                                   const OwnedEditRequest* request) {
    if (g_owned_duplicate_faulted) return 57;
    static const BYTE insert_bytes[] = {0x48,0x89,0x5C,0x24,0x10,0x48,0x89,0x6C,0x24,0x18};
    static const BYTE assign_bytes[] = {0x48,0x3B,0xCA,0x0F,0x84};
    static const BYTE descriptor_bytes[] = {0x40,0x55,0x56,0x48,0x83,0xEC,0x28};
    static const BYTE stack_bytes[] = {0x48,0x83,0xEC,0x28,0x48,0x8B,0x41,0x38};
    static const BYTE capacity_bytes[] = {0x8B,0x41,0x48,0x85,0xC0,0x79,0x03};
    if (!validate_function_bytes(base, 0xEA3250, insert_bytes, sizeof(insert_bytes), L"weapon clone insertion") ||
        !validate_function_bytes(base, 0xD34890, insert_bytes, sizeof(insert_bytes), L"item state copy") ||
        !validate_function_bytes(base, 0xD82D40, insert_bytes, 5, L"item instance constructor") ||
        !validate_function_bytes(base, 0xD7C670, assign_bytes, sizeof(assign_bytes), L"item state assignment") ||
        !validate_function_bytes(base, 0x9D7F90, insert_bytes, 5, L"descriptor override copy") ||
        !validate_function_bytes(base, 0x9F8DD0, descriptor_bytes, sizeof(descriptor_bytes), L"descriptor assignment") ||
        !validate_function_bytes(base, 0xDD3DE0, stack_bytes, sizeof(stack_bytes), L"native stack check") ||
        !validate_function_bytes(base, 0xF17C00, capacity_bytes, sizeof(capacity_bytes), L"weapon capacity")) return 50;
    void* source = owned_resolve_request_target(base, player, request);
    if (!source) return 33;
    void* bucket = owned_duplicate_bucket(base, player);
    void* before_items[256] = {}; unsigned before_count = 0;
    if (!bucket || !owned_duplicate_members(bucket, before_items, &before_count) ||
        before_count >= 256) return 51;
    typedef bool (__fastcall *StackFn)(const void*);
    typedef unsigned (__fastcall *CapacityFn)(void*);
    typedef void* (__fastcall *InsertFn)(void*, const void*, int, bool);
    if (((StackFn)(base + 0xDD3DE0))((BYTE*)source + 0x18)) return 52;
    const unsigned capacity = ((CapacityFn)(base + 0xF17C00))(bucket);
    if (!capacity || before_count >= capacity) return 53;
    OwnedDuplicateState before = {}, after = {}, copy = {};
    if (!owned_duplicate_capture(base, source, &before)) return 54;
    log_line(L"[OWNED_DUPLICATE] item=%S source=%p requested=1 dispatch_count=1 affixes=%u attachments=%u",
             request->item_id, source, before.affix_count, before.attachment_count);
    // ONE authoritative native dispatch. Reason 3 preserves native capacity
    // checks here. Do not use player wrapper 1260700: it overwrites item+68
    // with the player level. This path copies the original's actual level.
    void* clone = ((InsertFn)(base + 0xEA3250))(bucket, (BYTE*)source + 0x18, 3, false);
    void* after_items[256] = {}; unsigned after_count = 0;
    bool verified = owned_duplicate_bucket(base, player) == bucket &&
        owned_duplicate_members(bucket, after_items, &after_count);
    if (!clone && verified && after_count == before_count &&
        !memcmp(before_items, after_items, before_count * sizeof(void*)) &&
        owned_duplicate_capture(base, source, &after) &&
        owned_duplicate_equal(&before, &after, false)) return 55;
    verified = verified && clone && clone != source && after_count == before_count + 1;
    unsigned fresh_count = 0;
    if (verified) {
        for (unsigned i = 0; i < after_count; ++i) {
            bool existed = false;
            for (unsigned j = 0; j < before_count; ++j) existed |= after_items[i] == before_items[j];
            if (!existed) { ++fresh_count; verified &= after_items[i] == clone; }
        }
        for (unsigned j = 0; j < before_count; ++j) {
            bool retained = false;
            for (unsigned i = 0; i < after_count; ++i) retained |= before_items[j] == after_items[i];
            verified &= retained;
        }
    }
    verified = verified && fresh_count == 1 &&
        owned_locate_carried(base, player, source, request->item_id) &&
        owned_locate_carried(base, player, clone, request->item_id) &&
        owned_duplicate_capture(base, source, &after) &&
        owned_duplicate_capture(base, clone, &copy) &&
        owned_duplicate_equal(&before, &after, false) &&
        owned_duplicate_equal(&before, &copy, true);
    if (!verified) {
        // Insertion may already have happened. Never repeat or guess at a
        // destructor/removal call. Leave the source alone and stop copies.
        // A verification mismatch is not an exception in the affix editor.
        // Block further copies, but let independently validated affix actions
        // run. Shared mutable effects WOULD compromise edit isolation.
        g_owned_duplicate_faulted = true;
        PocRecordVector* clone_vector = NULL; BYTE* clone_records = NULL; unsigned clone_count = 0;
        // Independence must still be readable even if an unrelated scalar or
        // attachment mismatch stopped the full verifier early.
        if (!clone || !owned_vector(clone, &clone_vector, &clone_records, &clone_count) ||
            (clone_count && (clone_vector->packed & 0xFFFFFFFFFFFFull) == before.affix_array) ||
            !is_readable_range((BYTE*)clone + 0x98, 8) ||
            (before.override_object && *(uintptr_t*)((BYTE*)clone + 0x98) == before.override_object)) {
            g_owned_edit_faulted = true;
        } else {
            for (unsigned i = 0; i < before.affix_count; ++i)
                for (unsigned j = 0; j < clone_count; ++j)
                    if (before.effects[i] == (*(uintptr_t*)(clone_records + j * 0x20 + 0x10) & 0xFFFFFFFFFFFFull))
                        g_owned_edit_faulted = true;
        }
        for (unsigned i = 0; i < sizeof(before.scalars); ++i)
            if (before.scalars[i] != copy.scalars[i])
                log_line(L"[OWNED_DUPLICATE] scalar mismatch item_offset=0x%X source=%u copy=%u",
                         i + 0x18, before.scalars[i], copy.scalars[i]);
        log_line(L"[OWNED_DUPLICATE] verification FAILED source=%p copy=%p before=%u after=%u; no automatic retry/removal",
                 source, clone, before_count, after_count);
        return 56;
    }
    g_owned_duplicate_result = clone;
    log_line(L"[OWNED_DUPLICATE] verified source=%p copy=%p affixes=%u attachments=%u actual_level=%u source_unchanged=1 independent_effects=1",
             source, clone, copy.affix_count, copy.attachment_count,
             *(unsigned*)((BYTE*)clone + 0x68));
    return 0;
}
