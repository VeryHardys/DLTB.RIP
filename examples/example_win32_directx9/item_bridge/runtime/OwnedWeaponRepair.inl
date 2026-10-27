// From GuiInventoryItemData::PerformRepair -> 12B89E0 -> 12B8780 ->
// E2FC40(item, repairFraction, consumeRepairSlot, optionalDescriptor).
// Use the native item repair stage only; no currency/material removal.
static bool owned_repair_values_valid(float current, float maximum, int done, int allowed) {
    return _finite(current) && _finite(maximum) && maximum > 0 && current >= 0 &&
        current <= maximum && done >= 0 && done <= 100000 && allowed >= -1;
}

static bool owned_repair_readback_valid(float current, float maximum, float original_maximum,
                                        int done, int original_done) {
    return _finite(current) && _finite(maximum) && current == maximum &&
        maximum == original_maximum && done == original_done + 1;
}

static int owned_repair_weapon(BYTE* base, void* player, const OwnedEditRequest* request) {
    static const BYTE repair_bytes[] = {0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x6C,0x24,0x10};
    static const BYTE setter_bytes[] = {0x40,0x57,0x48,0x83,0xEC,0x40};
    static const BYTE current_bytes[] = {0xF3,0x0F,0x10,0x81,0xB0,0x00,0x00,0x00,0xC3};
    static const BYTE maximum_bytes[] = {0x40,0x56,0x48,0x83,0xEC,0x40};
    if (!validate_function_bytes(base, 0xE2FC40, repair_bytes, sizeof(repair_bytes), L"native weapon repair") ||
        !validate_function_bytes(base, 0xA6E500, setter_bytes, sizeof(setter_bytes), L"native durability setter") ||
        !validate_function_bytes(base, 0xA12BF0, current_bytes, sizeof(current_bytes), L"current durability") ||
        !validate_function_bytes(base, 0xA266D0, maximum_bytes, sizeof(maximum_bytes), L"maximum durability")) return 70;
    BYTE* item = (BYTE*)owned_resolve_request_target(base, player, request);
    if (!item) return 33;
    BYTE* context = item + 0x50;
    if (!is_readable_range(item, 0x118) || *(void**)context != base + 0x289AAF0 ||
        *(void**)(base + 0x289AAF0 + 0x1140) != base + 0xA12BF0 ||
        *(void**)(base + 0x289AAF0 + 0x1128) != base + 0xA266D0 ||
        *(void**)(base + 0x289AAF0 + 0xEE8) != base + 0xA30980 ||
        *(void**)(base + 0x289AAF0 + 0xA38) != base + 0xA0D2F0) return 71;
    typedef float (__fastcall *DurabilityFn)(void*);
    typedef bool (__fastcall *RepairableFn)(void*);
    typedef int (__fastcall *SlotsFn)(void*);
    typedef void (__fastcall *RepairFn)(void*, float, bool, void*);
    void** vt = *(void***)context;
    const float before = ((DurabilityFn)(base + 0xA12BF0))(context);
    const float maximum = ((DurabilityFn)(base + 0xA266D0))(context);
    const int repairs_done = *(int*)(item + 0x1C);
    const int allowed = ((SlotsFn)vt[0xA38 / 8])(context);
    if (!owned_repair_values_valid(before, maximum, repairs_done, allowed)) return 72;
    if (before == maximum) return 73;
    if (!((RepairableFn)vt[0xEE8 / 8])(context) ||
        (allowed != -1 && repairs_done >= allowed)) return 74;
    log_line(L"[OWNED_REPAIR] item=%S target=%p durability=%g/%g repairs_done=%d requested=1 dispatch_count=1",
             request->item_id, item, before, maximum, repairs_done);
    // Native code clamps to max, consumes one repair slot, and rebuilds the
    // descriptor through A6E500. Affix records/attachments are not written.
    ((RepairFn)(base + 0xE2FC40))(item, 1.0f, true, NULL);
    const float after = ((DurabilityFn)(base + 0xA12BF0))(context);
    const float after_maximum = ((DurabilityFn)(base + 0xA266D0))(context);
    if (!owned_locate_carried(base, player, item, request->item_id) ||
        !owned_request_item_matches(base, item, request) ||
        !owned_repair_readback_valid(after, after_maximum, maximum,
                                    *(int*)(item + 0x1C), repairs_done)) {
        g_owned_edit_faulted = true;
        log_line(L"[OWNED_REPAIR] verification FAILED target=%p before=%g after=%g maximum=%g; no retry",
                 item, before, after, after_maximum);
        return 75;
    }
    log_line(L"[OWNED_REPAIR] verified target=%p durability=%g/%g affixes_unchanged=1 repairs_done=%d",
             item, after, after_maximum, repairs_done + 1);
    return 0;
}
