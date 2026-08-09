#pragma once
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wrl/client.h>
#include "ui/theme.h"
#include "layout/layout_types.h"

using Microsoft::WRL::ComPtr;

struct KeyRenderInfo {
    const KeyCell* cell = nullptr;
    bool is_hovered = false;
    bool is_remapped = false;
    bool is_disabled = false;
    std::string remap_label;
};

class RendererD2D {
public:
    bool Initialize(HWND hwnd);
    void Uninitialize();
    void BeginDraw();
    void EndDraw();
    void Clear(D2D1_COLOR_F color);
    void Resize(UINT width, UINT height);

    void DrawKey(const KeyRenderInfo& info, const ThemeColors& theme,
                 float x, float y, float w, float h);
    void DrawText(const wchar_t* text, float x, float y,
                  const ThemeColors& theme, float font_size = 12.0f);
    void DrawTextMuted(const wchar_t* text, float x, float y,
                       const ThemeColors& theme, float font_size = 10.0f);
    void DrawRect(float x, float y, float w, float h, D2D1_COLOR_F color);
    void DrawRoundedRect(float x, float y, float w, float h,
                         D2D1_COLOR_F fill, D2D1_COLOR_F stroke,
                         float stroke_width = 1.0f);
    void DrawPendingItem(const wchar_t* text, float x, float y,
                         float w, const ThemeColors& theme);

    D2D1_SIZE_U GetSize() const { return m_size; }

private:
    HWND m_hwnd = nullptr;
    D2D1_SIZE_U m_size{};
    ComPtr<ID2D1Factory> m_factory;
    ComPtr<ID2D1HwndRenderTarget> m_rt;
    ComPtr<IDWriteFactory> m_dwrite;
    ComPtr<IDWriteTextFormat> m_text_fmt;
    ComPtr<IDWriteTextFormat> m_text_fmt_small;
};
