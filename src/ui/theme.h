#pragma once
#include <d2d1.h>
#include <d2d1helper.h>
#include <string>

enum class ThemeMode { Light, Dark, System };

struct ThemeColors {
    D2D1_COLOR_F background;
    D2D_COLOR_F surface;
    D2D_COLOR_F keyDefault;
    D2D_COLOR_F keyHover;
    D2D_COLOR_F keyRemapped;
    D2D_COLOR_F keyDisabled;
    D2D_COLOR_F text;
    D2D_COLOR_F textMuted;
    D2D_COLOR_F accent;
    D2D_COLOR_F border;
    D2D_COLOR_F pendingBg;
};

class Theme {
public:
    Theme();
    void SetMode(ThemeMode mode);
    ThemeMode GetMode() const { return m_mode; }
    const ThemeColors& GetColors() const { return m_colors; }
    bool IsDark() const;

private:
    void ApplyColors();
    ThemeMode m_mode = ThemeMode::System;
    ThemeColors m_colors{};
};
