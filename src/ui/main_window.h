#pragma once
#include <windows.h>
#include <string>
#include "ui/renderer_d2d.h"
#include "ui/theme.h"
#include "layout/layout_types.h"
#include "engine/remap_table.h"
#include "backend_hook/keyboard_hook.h"
#include "backend_hook/raw_input_monitor.h"
#include "backend_registry/scancode_map_writer.h"
#include "profiles/profile_store.h"
#include "tray/tray_icon.h"

class MainWindow {
public:
    static MainWindow& Instance();
    bool Create(HINSTANCE hInstance);
    void RunMessageLoop();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg,
                                     WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void OnPaint();
    void OnResize(UINT w, UINT h);
    void OnLButtonDown(int x, int y);
    void OnMouseMove(int x, int y);
    void OnLButtonUp(int x, int y);
    void OnCommand(WPARAM wParam);
    void OnTrayMessage(LPARAM lParam);

    void RenderKeyboard();
    void RenderPendingChanges();
    void RenderHeader();
    void HitTestKeys(int x, int y);
    void GetKeyboardGeometry(float& scale, float& offset_x, float& offset_y) const;
    bool IsInButton(int x, int y, int button_index) const;
    bool IsInLayoutNav(int x, int y, int direction) const;
    void OpenKeyPicker(const std::string& key_id);
    void ApplyLive();
    void MakePermanent();
    void RestoreDefaults();
    void SwitchProfile(const std::string& name);
    void SwitchLayout(const std::string& layout_id);
    void SaveCurrentProfile();
    void LoadCurrentProfile();

    HWND m_hwnd = nullptr;
    HINSTANCE m_hInstance = nullptr;
    RendererD2D m_renderer;
    Theme m_theme;
    Layout m_current_layout;
    RemapTable m_remap_table;
    RawInputMonitor m_raw_input_monitor;
    ProfileStore m_profile_store;
    TrayIcon m_tray;
    std::string m_current_profile = "Default";
    std::string m_current_layout_id = "65_percent_ansi";
    std::vector<std::string> m_available_layouts;
    std::vector<std::string> m_available_profiles;
    int m_hovered_key = -1;
    bool m_paused = false;
    bool m_dragging = false;
    int m_drag_start_x = 0;
    int m_drag_start_y = 0;
    float m_scroll_offset = 0.0f;

    static constexpr int HEADER_HEIGHT = 50;
    static constexpr int KEYBOARD_PADDING = 20;
    static constexpr int PENDING_HEIGHT = 138;
    static constexpr int BUTTON_HEIGHT = 30;
    static constexpr int BUTTON_WIDTH = 120;
};
