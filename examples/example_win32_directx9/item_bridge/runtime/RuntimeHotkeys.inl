// Portable hotkey settings stay beside the runtime, separate from favorites.
static const UINT kDefaultMenuToggleVk = VK_INSERT;

static bool parse_menu_toggle_vk(const wchar_t* text, UINT* result) {
    if (!text || !result) return false;
    while (iswspace(*text)) ++text;
    if (*text == L'-' || *text == L'+' || !*text) return false;
    const int base = text[0] == L'0' && (text[1] == L'x' || text[1] == L'X') ? 16 : 10;
    errno = 0;
    wchar_t* end = NULL;
    const unsigned long value = wcstoul(text,&end,base);
    if (end == text || errno == ERANGE) return false;
    while (iswspace(*end)) ++end;
    // Escape remains the fixed close key. 0/0xFF aren't valid toggle bindings.
    if (*end || !value || value > 0xFE || value == VK_ESCAPE) return false;
    *result = (UINT)value;
    return true;
}

static bool menu_hotkey_edge(SHORT state, bool* was_down) {
    const bool down = (state & 0x8000) != 0;
    const bool pressed = down && !*was_down;
    *was_down = down;
    return pressed;
}

static UINT load_menu_toggle_vk(const wchar_t* path) {
    if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES &&
        (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND)) {
        if (!WritePrivateProfileStringW(L"Hotkeys",L"ToggleVK",L"0x2D",path))
            log_line(L"[CONFIG] Could not create hotkey config; using Insert. error=%lu",GetLastError());
    }
    wchar_t value[64] = {};
    const DWORD length = GetPrivateProfileStringW(L"Hotkeys",L"ToggleVK",L"0x2D",value,_countof(value),path);
    UINT vk = kDefaultMenuToggleVk;
    if (length >= _countof(value)-1 || !parse_menu_toggle_vk(value,&vk)) {
        log_line(L"[CONFIG] Invalid ToggleVK=%s; using Insert (0x2D). Escape is close-only.",value);
        vk = kDefaultMenuToggleVk;
    }
    log_line(L"[CONFIG] Menu toggle VK=0x%02X; config=%s; reload requires game restart",vk,path);
    return vk;
}
