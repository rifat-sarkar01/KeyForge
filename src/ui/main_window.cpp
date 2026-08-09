#include "ui/main_window.h"
#include "ui/key_picker_dialog.h"
#include "layout/layout_loader.h"
#include <commctrl.h>
#include <shellapi.h>
#include <vector>
#include <algorithm>

#pragma comment(lib, "comctl32.lib")

MainWindow& MainWindow::Instance() {
    static MainWindow inst;
    return inst;
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg,
                                      WPARAM wParam, LPARAM lParam) {
    MainWindow* self = nullptr;
    if (msg == WM_CREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hwnd = hwnd;
    } else {
        self = reinterpret_cast<MainWindow*>(
            GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    if (self) {
        return self->HandleMessage(hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool MainWindow::Create(HINSTANCE hInstance) {
    m_hInstance = hInstance;
    INITCOMMONCONTROLSEX icc{sizeof(icc), ICC_LISTVIEW_CLASSES};
    InitCommonControlsEx(&icc);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"KeyForgeClass";
    RegisterClassExW(&wc);

    m_available_layouts = {"65_percent_ansi", "sixty_percent",
                           "forty_percent", "seventy_five_percent",
                           "tkl", "layout_1800_compact", "full_size_ansi"};
    m_available_profiles = m_profile_store.ListProfiles();
    if (m_available_profiles.empty()) {
        m_available_profiles.push_back("Default");
    }

    for (const auto& name : m_available_layouts) {
        Layout layout;
        if (LayoutLoader::LoadBuiltIn(name, layout)) {
            if (name == m_current_layout_id) {
                m_current_layout = layout;
            }
        }
    }

    if (m_current_layout.keys.empty()) {
        LayoutLoader::LoadBuiltIn("65_percent_ansi", m_current_layout);
    }

    m_hwnd = CreateWindowExW(
        0, L"KeyForgeClass", L"KeyForge",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1100, 700,
        nullptr, nullptr, hInstance, this);

    if (!m_hwnd) return false;

    if (!m_renderer.Initialize(m_hwnd)) return false;

    m_theme.SetMode(ThemeMode::Dark);

    m_tray.Install(m_hwnd);
    m_raw_input_monitor.Install(m_hwnd);

    LoadCurrentProfile();
    ApplyLive();

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
    return true;
}

void MainWindow::RunMessageLoop() {
    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT MainWindow::HandleMessage(HWND hwnd, UINT msg,
                                   WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT:
        OnPaint();
        return 0;
    case WM_SIZE:
        OnResize(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_GETMINMAXINFO: {
        auto* minmax = reinterpret_cast<MINMAXINFO*>(lParam);
        minmax->ptMinTrackSize.x = 800;
        minmax->ptMinTrackSize.y = 560;
        return 0;
    }
    case WM_LBUTTONDOWN:
        SetFocus(hwnd);
        OnLButtonDown(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_MOUSEMOVE:
        OnMouseMove(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_LBUTTONUP:
        OnLButtonUp(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_COMMAND:
        OnCommand(wParam);
        return 0;
    case WM_INPUT:
        m_raw_input_monitor.Handle(reinterpret_cast<HRAWINPUT>(lParam));
        return 0;
    case WM_TRAYICON:
        OnTrayMessage(lParam);
        return 0;
    case WM_CLOSE:
        ShowWindow(m_hwnd, SW_HIDE);
        return 0;
    case WM_DESTROY:
        SaveCurrentProfile();
        KeyboardHook::Instance().Uninstall();
        m_tray.Uninstall();
        m_renderer.Uninitialize();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void MainWindow::OnPaint() {
    PAINTSTRUCT ps;
    BeginPaint(m_hwnd, &ps);
    m_renderer.BeginDraw();
    m_renderer.Clear(m_theme.GetColors().background);

    RenderHeader();
    RenderKeyboard();
    RenderPendingChanges();

    m_renderer.EndDraw();
    EndPaint(m_hwnd, &ps);
}

void MainWindow::OnResize(UINT w, UINT h) {
    if (w > 0 && h > 0) {
        m_renderer.Resize(w, h);
    }
}

void MainWindow::OnLButtonDown(int x, int y) {
    for (int direction : {-1, 1}) {
        if (!IsInLayoutNav(x, y, direction)) continue;
        const auto current = std::find(m_available_layouts.begin(),
                                       m_available_layouts.end(), m_current_layout_id);
        if (current == m_available_layouts.end() || m_available_layouts.empty()) return;
        if (m_remap_table.HasChanges() && MessageBoxW(m_hwnd,
            L"Switching layouts clears the current remaps. Continue?",
            L"Change Layout", MB_YESNO | MB_ICONWARNING) != IDYES) {
            return;
        }
        const size_t count = m_available_layouts.size();
        const size_t index = static_cast<size_t>(std::distance(m_available_layouts.begin(), current));
        const size_t next = direction < 0 ? (index + count - 1) % count :
                                            (index + 1) % count;
        SwitchLayout(m_available_layouts[next]);
        return;
    }
    if (IsInButton(x, y, 0)) {
        SaveCurrentProfile();
        ApplyLive();
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return;
    }
    if (IsInButton(x, y, 1)) {
        MakePermanent();
        return;
    }
    if (IsInButton(x, y, 2)) {
        RestoreDefaults();
        return;
    }

    HitTestKeys(x, y);
    if (m_hovered_key >= 0 &&
        m_hovered_key < static_cast<int>(m_current_layout.keys.size())) {
        const auto& key = m_current_layout.keys[m_hovered_key];
        if (!key.hardware_only) {
            OpenKeyPicker(key.id);
        }
    }
}

void MainWindow::OnMouseMove(int x, int y) {
    int prev = m_hovered_key;
    HitTestKeys(x, y);
    if (m_hovered_key != prev) {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void MainWindow::OnLButtonUp(int, int) {
    m_dragging = false;
}

void MainWindow::HitTestKeys(int mx, int my) {
    m_hovered_key = -1;
    float scale = 1.0f, offset_x = 0.0f, offset_y = 0.0f;
    GetKeyboardGeometry(scale, offset_x, offset_y);

    for (size_t i = 0; i < m_current_layout.keys.size(); ++i) {
        const auto& key = m_current_layout.keys[i];
        float kx = offset_x + key.x * m_current_layout.unit_px * scale;
        float ky = offset_y + key.y * m_current_layout.unit_px * scale;
        float kw = key.w * m_current_layout.unit_px * scale;
        float kh = key.h * m_current_layout.unit_px * scale;

        if (mx >= kx && mx <= kx + kw && my >= ky && my <= ky + kh) {
            m_hovered_key = static_cast<int>(i);
            return;
        }
    }
}

void MainWindow::GetKeyboardGeometry(float& scale, float& offset_x,
                                     float& offset_y) const {
    RECT rc{};
    GetClientRect(m_hwnd, &rc);
    const float client_width = static_cast<float>(rc.right - rc.left);
    const float client_height = static_cast<float>(rc.bottom - rc.top);
    float logical_width = 1.0f;
    float logical_height = 1.0f;
    for (const auto& key : m_current_layout.keys) {
        logical_width = std::max(logical_width,
            (key.x + key.w) * static_cast<float>(m_current_layout.unit_px));
        logical_height = std::max(logical_height,
            (static_cast<float>(key.y) + key.h) * static_cast<float>(m_current_layout.unit_px));
    }
    const float available_width = std::max(1.0f, client_width - 2.0f * KEYBOARD_PADDING);
    const float available_height = std::max(1.0f, client_height - HEADER_HEIGHT -
                                             PENDING_HEIGHT - 2.0f * KEYBOARD_PADDING);
    scale = std::max(0.25f, std::min({1.0f, available_width / logical_width,
                                      available_height / logical_height}));
    const float keyboard_width = logical_width * scale;
    const float keyboard_height = logical_height * scale;
    offset_x = std::max(0.0f, (client_width - keyboard_width) / 2.0f);
    offset_y = HEADER_HEIGHT + KEYBOARD_PADDING +
        std::max(0.0f, (available_height - keyboard_height) / 2.0f);
}

bool MainWindow::IsInButton(int x, int y, int button_index) const {
    RECT rc{};
    GetClientRect(m_hwnd, &rc);
    const int button_x = rc.right - BUTTON_WIDTH - 20;
    const int button_y = rc.bottom - PENDING_HEIGHT + 8 +
                         button_index * (BUTTON_HEIGHT + 8);
    return x >= button_x && x <= button_x + BUTTON_WIDTH &&
           y >= button_y && y <= button_y + BUTTON_HEIGHT;
}

bool MainWindow::IsInLayoutNav(int x, int y, int direction) const {
    const int button_x = direction < 0 ? 470 : 506;
    return x >= button_x && x <= button_x + 28 && y >= 11 && y <= 39;
}

void MainWindow::RenderHeader() {
    const auto& theme = m_theme.GetColors();
    const auto size = m_renderer.GetSize();
    m_renderer.DrawRect(0, 0, static_cast<float>(size.width), HEADER_HEIGHT, theme.surface);

    m_renderer.DrawText(L"KeyForge", 12, 12, theme, 18.0f);

    const std::string layout = "Layout: " + m_current_layout.display_name;
    const std::string profile = "Profile: " + m_current_profile;
    wchar_t layout_text[256]{};
    wchar_t profile_text[128]{};
    MultiByteToWideChar(CP_UTF8, 0, layout.c_str(), -1, layout_text, ARRAYSIZE(layout_text));
    MultiByteToWideChar(CP_UTF8, 0, profile.c_str(), -1, profile_text, ARRAYSIZE(profile_text));
    m_renderer.DrawText(layout_text, 120, 18, theme, 12.0f);
    m_renderer.DrawRoundedRect(470, 11, 28, 28, theme.keyDefault, theme.border);
    m_renderer.DrawText(L"<", 480, 13, theme, 16.0f);
    m_renderer.DrawRoundedRect(506, 11, 28, 28, theme.keyDefault, theme.border);
    m_renderer.DrawText(L">", 516, 13, theme, 16.0f);
    m_renderer.DrawText(profile_text, 560, 18, theme, 12.0f);
}

void MainWindow::RenderKeyboard() {
    const auto& theme = m_theme.GetColors();
    float scale = 1.0f, offset_x = 0.0f, offset_y = 0.0f;
    GetKeyboardGeometry(scale, offset_x, offset_y);

    for (size_t i = 0; i < m_current_layout.keys.size(); ++i) {
        const auto& key = m_current_layout.keys[i];
        float kx = offset_x + key.x * m_current_layout.unit_px * scale;
        float ky = offset_y + key.y * m_current_layout.unit_px * scale;
        float kw = key.w * m_current_layout.unit_px * scale;
        float kh = key.h * m_current_layout.unit_px * scale;

        KeyRenderInfo info;
        info.cell = &key;
        info.is_hovered = (static_cast<int>(i) == m_hovered_key);
        info.is_disabled = key.hardware_only;

        auto it = std::find_if(m_remap_table.GetPending().begin(),
                               m_remap_table.GetPending().end(),
                               [&](const PendingChange& p) {
                                   return p.key_id == key.id;
                               });
        if (it != m_remap_table.GetPending().end()) {
            info.is_remapped = true;
            info.remap_label = it->disabled ? "-> Disabled" : ("-> " + it->to_label);
        }

        m_renderer.DrawKey(info, theme, kx, ky, kw, kh);
    }
}

void MainWindow::RenderPendingChanges() {
    const auto& theme = m_theme.GetColors();
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    int w = rc.right;
    int h = rc.bottom;

    int pending_y = h - PENDING_HEIGHT;
    m_renderer.DrawRect(0, static_cast<float>(pending_y), static_cast<float>(w),
                        static_cast<float>(PENDING_HEIGHT), theme.surface);

    m_renderer.DrawText(L"Pending changes:", 20,
                        static_cast<float>(pending_y + 8), theme, 12.0f);

    const auto& pending = m_remap_table.GetPending();
    constexpr size_t max_visible_changes = 3;
    for (size_t i = 0; i < std::min(pending.size(), max_visible_changes); ++i) {
        const auto& p = pending[i];
        std::string text = p.key_id + " -> " +
                           (p.disabled ? std::string("Disabled") : p.to_label);
        wchar_t wtext[128];
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1,
                            wtext, ARRAYSIZE(wtext));
        m_renderer.DrawPendingItem(wtext, 20.0f,
                                   static_cast<float>(pending_y + 30 + i * 28),
                                   300.0f, theme);
    }
    if (pending.size() > max_visible_changes) {
        const std::wstring more = L"+ " + std::to_wstring(pending.size() - max_visible_changes) +
                                  L" more changes";
        m_renderer.DrawTextMuted(more.c_str(), 20.0f,
            static_cast<float>(pending_y + 30 + max_visible_changes * 28), theme, 10.0f);
    }

    int btn_x = w - BUTTON_WIDTH - 20;
    int btn_y = pending_y + 8;

    m_renderer.DrawRoundedRect(
        static_cast<float>(btn_x), static_cast<float>(btn_y),
        BUTTON_WIDTH, BUTTON_HEIGHT,
        theme.accent, theme.accent, 0.0f);
    m_renderer.DrawText(L"Save && Apply", static_cast<float>(btn_x + 10),
                        static_cast<float>(btn_y + 6), theme, 11.0f);

    btn_y += BUTTON_HEIGHT + 8;
    m_renderer.DrawRoundedRect(
        static_cast<float>(btn_x), static_cast<float>(btn_y),
        BUTTON_WIDTH, BUTTON_HEIGHT,
        D2D1::ColorF(0.2f, 0.5f, 0.2f), D2D1::ColorF(0.2f, 0.5f, 0.2f),
        0.0f);
    m_renderer.DrawText(L"Make Permanent", static_cast<float>(btn_x + 10),
                        static_cast<float>(btn_y + 6), theme, 11.0f);

    btn_y += BUTTON_HEIGHT + 8;
    m_renderer.DrawRoundedRect(
        static_cast<float>(btn_x), static_cast<float>(btn_y),
        BUTTON_WIDTH, BUTTON_HEIGHT,
        D2D1::ColorF(0.5f, 0.2f, 0.2f), D2D1::ColorF(0.5f, 0.2f, 0.2f),
        0.0f);
    m_renderer.DrawText(L"Restore Defaults",
                        static_cast<float>(btn_x + 10),
                        static_cast<float>(btn_y + 6), theme, 11.0f);
}

void MainWindow::OpenKeyPicker(const std::string& key_id) {
    KeyPickerDialog picker;
    PickResult result = picker.ShowModal(m_hwnd, m_current_layout, key_id);
    if (!result.cancelled) {
        std::string label = result.disabled ? "Disabled" : result.target_label;
        m_remap_table.AddRemap(key_id, label, result.scan_code,
                               result.extended, result.disabled);
        ApplyLive();
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void MainWindow::ApplyLive() {
    if (m_paused) return;
    auto entries = m_remap_table.BuildEntries(m_current_layout);
    auto table = BuildLookupTable(entries);
    KeyboardHook::Instance().UpdateTable(table);
    if (!KeyboardHook::Instance().IsInstalled()) {
        KeyboardHook::Instance().Install();
    }
}

void MainWindow::MakePermanent() {
    const bool includes_pause = std::any_of(
        m_remap_table.GetPending().begin(), m_remap_table.GetPending().end(),
        [](const PendingChange& change) { return change.key_id == "PAUSE"; });
    if (includes_pause) {
        MessageBoxW(m_hwnd,
            L"Pause/Break uses a special Windows E1 scan-code sequence and cannot be saved "
            L"safely as a permanent registry remap.\n\n"
            L"Use Save && Apply for Pause/Break remaps while KeyForge is running. "
            L"If you previously made this mapping permanent, choose Restore Defaults, "
            L"approve the administrator prompt, and restart Windows once.",
            L"Pause/Break Remap", MB_OK | MB_ICONINFORMATION);
        return;
    }
    auto entries = m_remap_table.BuildEntries(m_current_layout);
    auto blob = ScancodeMapWriter::BuildBlob(entries);
    SaveCurrentProfile();
    if (ScancodeMapWriter::NeedsElevation()) {
        int result = MessageBoxW(m_hwnd,
            L"Writing to the registry requires administrator privileges.\n"
            L"Restart as administrator?",
            L"KeyForge", MB_YESNO | MB_ICONWARNING);
        if (result == IDYES) {
            wchar_t exe_path[MAX_PATH]{};
            GetModuleFileNameW(nullptr, exe_path, ARRAYSIZE(exe_path));
            // Pass the finished binary map to the elevated helper rather than
            // reloading a profile. Elevation can use a different Windows
            // account, whose AppData folder does not contain this profile.
            constexpr wchar_t kHexDigits[] = L"0123456789ABCDEF";
            std::wstring encoded_blob;
            encoded_blob.reserve(blob.size() * 2);
            for (const uint8_t byte : blob) {
                encoded_blob.push_back(kHexDigits[byte >> 4]);
                encoded_blob.push_back(kHexDigits[byte & 0x0F]);
            }
            const std::wstring arguments =
                L"--apply-permanent-blob " + encoded_blob;
            const auto launched = ShellExecuteW(nullptr, L"runas", exe_path,
                                                arguments.c_str(), nullptr, SW_SHOWNORMAL);
            if (reinterpret_cast<INT_PTR>(launched) <= 32) {
                MessageBoxW(m_hwnd, L"The elevated helper could not be started.",
                            L"KeyForge", MB_OK | MB_ICONERROR);
            }
        }
        return;
    }
    if (ScancodeMapWriter::WriteToRegistry(blob)) {
        int result = MessageBoxW(m_hwnd,
            L"Registry updated successfully.\n"
            L"A restart is required for permanent changes to take effect.\n"
            L"Restart now?",
            L"KeyForge", MB_YESNO | MB_ICONINFORMATION);
        if (result == IDYES) {
            ExitWindowsEx(EWX_RESTARTAPPS, 0);
        }
    } else {
        MessageBoxW(m_hwnd, L"Failed to write to registry.",
                    L"KeyForge", MB_OK | MB_ICONERROR);
    }
}

void MainWindow::RestoreDefaults() {
    const int answer = MessageBoxW(m_hwnd,
        L"Clear all remaps in this profile and remove the permanent registry map?",
        L"Restore Defaults", MB_YESNO | MB_ICONWARNING);
    if (answer != IDYES) return;
    m_remap_table.Clear();
    KeyboardHook::Instance().UpdateTable({});
    SaveCurrentProfile();
    if (ScancodeMapWriter::NeedsElevation()) {
        wchar_t exe_path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, exe_path, ARRAYSIZE(exe_path));
        const auto launched = ShellExecuteW(nullptr, L"runas", exe_path,
                                            L"--clear-permanent", nullptr, SW_SHOWNORMAL);
        if (reinterpret_cast<INT_PTR>(launched) <= 32) {
            MessageBoxW(m_hwnd, L"The elevated helper could not be started.",
                        L"KeyForge", MB_OK | MB_ICONERROR);
        }
    } else if (!ScancodeMapWriter::ClearRegistry()) {
        MessageBoxW(m_hwnd, L"The live remaps were cleared, but the permanent registry map could not be removed.",
                    L"KeyForge", MB_OK | MB_ICONERROR);
    }
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void MainWindow::SwitchProfile(const std::string& name) {
    m_current_profile = name;
    LoadCurrentProfile();
    ApplyLive();
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void MainWindow::SwitchLayout(const std::string& layout_id) {
    Layout layout;
    if (LayoutLoader::LoadBuiltIn(layout_id, layout)) {
        m_current_layout_id = layout_id;
        m_current_layout = layout;
        m_remap_table.Clear();
        ApplyLive();
        SaveCurrentProfile();
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void MainWindow::SaveCurrentProfile() {
    Profile profile;
    profile.name = m_current_profile;
    profile.layout_id = m_current_layout_id;
    profile.remaps = m_remap_table.GetPending();
    m_profile_store.Save(profile);
}

void MainWindow::LoadCurrentProfile() {
    Profile profile;
    if (m_profile_store.Load(m_current_profile, profile)) {
        m_remap_table.Clear();
        Layout layout;
        if (LayoutLoader::LoadBuiltIn(profile.layout_id, layout)) {
            m_current_layout_id = profile.layout_id;
            m_current_layout = layout;
        }
        for (const auto& remap : profile.remaps) {
            bool source_exists = std::any_of(
                m_current_layout.keys.begin(), m_current_layout.keys.end(),
                [&](const KeyCell& k) { return k.id == remap.key_id; });
            if (source_exists) {
                m_remap_table.AddRemap(remap.key_id, remap.to_label,
                                       remap.to_scan_code,
                                       remap.to_extended, remap.disabled);
            }
        }
    }
}

void MainWindow::OnCommand(WPARAM wParam) {
    switch (LOWORD(wParam)) {
    case IDM_TRAY_OPEN:
        ShowWindow(m_hwnd, SW_RESTORE);
        SetForegroundWindow(m_hwnd);
        break;
    case IDM_TRAY_PAUSE:
        m_paused = !m_paused;
        m_tray.SetPaused(m_paused);
        if (m_paused) {
            KeyboardHook::Instance().Uninstall();
        } else {
            ApplyLive();
        }
        break;
    case IDM_TRAY_QUIT:
        SaveCurrentProfile();
        DestroyWindow(m_hwnd);
        break;
    }
}

void MainWindow::OnTrayMessage(LPARAM lParam) {
    if (lParam == WM_RBUTTONUP) {
        HMENU hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING, IDM_TRAY_OPEN, L"Open");
        AppendMenuW(hMenu, MF_STRING, IDM_TRAY_PAUSE,
                    m_paused ? L"Resume" : L"Pause");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, IDM_TRAY_QUIT, L"Quit");

        POINT pt;
        GetCursorPos(&pt);
        SetForegroundWindow(m_hwnd);
        TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y,
                       0, m_hwnd, nullptr);
        DestroyMenu(hMenu);
    } else if (lParam == WM_LBUTTONDBLCLK) {
        ShowWindow(m_hwnd, SW_RESTORE);
        SetForegroundWindow(m_hwnd);
    }
}
