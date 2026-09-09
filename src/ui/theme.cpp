#include "ui/theme.h"
#include <windows.h>

Theme::Theme() {
    SetMode(ThemeMode::Dark);
}

bool Theme::IsDark() const {
    if (m_mode == ThemeMode::System) {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD value = 0, size = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr,
                reinterpret_cast<LPBYTE>(&value), &size) == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                return value == 0;
            }
            RegCloseKey(hKey);
        }
        return true;
    }
    return m_mode == ThemeMode::Dark;
}

void Theme::SetMode(ThemeMode mode) {
    m_mode = mode;
    ApplyColors();
}

void Theme::ApplyColors() {
    if (IsDark()) {
        m_colors.background = D2D1::ColorF(0.12f, 0.12f, 0.12f);
        m_colors.surface = D2D1::ColorF(0.18f, 0.18f, 0.18f);
        m_colors.keyDefault = D2D1::ColorF(0.28f, 0.28f, 0.28f);
        m_colors.keyHover = D2D1::ColorF(0.35f, 0.35f, 0.35f);
        m_colors.keyRemapped = D2D1::ColorF(0.15f, 0.30f, 0.55f);
        m_colors.keyDisabled = D2D1::ColorF(0.18f, 0.18f, 0.18f);
        m_colors.text = D2D1::ColorF(0.92f, 0.92f, 0.92f);
        m_colors.textMuted = D2D1::ColorF(0.55f, 0.55f, 0.55f);
        m_colors.accent = D2D1::ColorF(0.25f, 0.55f, 0.95f);
        m_colors.border = D2D1::ColorF(0.25f, 0.25f, 0.25f);
        m_colors.pendingBg = D2D1::ColorF(0.15f, 0.20f, 0.30f);
    } else {
        m_colors.background = D2D1::ColorF(0.96f, 0.96f, 0.96f);
        m_colors.surface = D2D1::ColorF(1.0f, 1.0f, 1.0f);
        m_colors.keyDefault = D2D1::ColorF(0.88f, 0.88f, 0.88f);
        m_colors.keyHover = D2D1::ColorF(0.78f, 0.78f, 0.78f);
        m_colors.keyRemapped = D2D1::ColorF(0.20f, 0.45f, 0.85f);
        m_colors.keyDisabled = D2D1::ColorF(0.85f, 0.85f, 0.85f);
        m_colors.text = D2D1::ColorF(0.12f, 0.12f, 0.12f);
        m_colors.textMuted = D2D1::ColorF(0.50f, 0.50f, 0.50f);
        m_colors.accent = D2D1::ColorF(0.15f, 0.45f, 0.90f);
        m_colors.border = D2D1::ColorF(0.80f, 0.80f, 0.80f);
        m_colors.pendingBg = D2D1::ColorF(0.90f, 0.93f, 0.97f);
    }
}
