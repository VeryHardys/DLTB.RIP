// All scope/visibility state is owned by the UI thread. The hotkey thread
// posts requests; automatic suspension NEVER activates or focuses a window.
static bool g_menu_open_requested;
static bool g_menu_scope_transition;

static bool catalogue_process_foreground() {
    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(),&pid);
    return pid == GetCurrentProcessId();
}

static HWND catalogue_game_owner() {
    HWND owner = GetWindow(g_window,GW_OWNER);
    if (!owner || !IsWindow(owner)) {
        owner = find_game_window();
        if (owner && owner != g_window && IsWindow(owner)) {
            SetLastError(ERROR_SUCCESS);
            SetWindowLongPtrW(g_window,GWLP_HWNDPARENT,(LONG_PTR)owner);
            if (GetWindow(g_window,GW_OWNER) != owner) return NULL;
        }
    }
    return owner;
}

static void dismiss_catalogue_popups() {
    if (g_tooltip) SendMessageW(g_tooltip,TTM_POP,0,0);
    EnumChildWindows(g_window,[](HWND child,LPARAM)->BOOL {
        wchar_t name[32] = {}; GetClassNameW(child,name,_countof(name));
        if (!_wcsicmp(name,WC_COMBOBOXW)) SendMessageW(child,CB_SHOWDROPDOWN,FALSE,0);
        return TRUE;
    },0);
}

static void synchronize_catalogue_scope(bool foreground) {
    if (!g_window || !IsWindow(g_window)) return;
    HWND owner = g_menu_open_requested ? catalogue_game_owner() : NULL;
    const bool show = g_menu_open_requested && foreground && owner && !IsIconic(owner);
    if (show == (IsWindowVisible(g_window) != FALSE)) return;
    g_menu_scope_transition = true;
    if (!show) dismiss_catalogue_popups();
    ShowWindow(g_window,show ? SW_SHOWNOACTIVATE : SW_HIDE);
    g_menu_scope_transition = false;
}

static void close_catalogue(bool return_to_game) {
    g_menu_open_requested = false;
    synchronize_catalogue_scope(false);
    if (return_to_game && catalogue_process_foreground()) focus_game_after_catalogue();
}

static void handle_catalogue_scope_request(unsigned request) {
    const bool foreground = catalogue_process_foreground();
    if (request == 1 && foreground) {
        if (g_menu_open_requested) close_catalogue(true);
        else {
            g_menu_open_requested = true;
            position_catalogue_for_first_show();
            synchronize_catalogue_scope(true);
            if (IsWindowVisible(g_window) && catalogue_process_foreground()) {
                // Focus is acquired only for the user's explicit toggle.
                BringWindowToTop(g_window);
                SetForegroundWindow(g_window);
                SetFocus(g_search);
            }
        }
    } else if (request == 2 && foreground) close_catalogue(true);
    synchronize_catalogue_scope(catalogue_process_foreground());
}
