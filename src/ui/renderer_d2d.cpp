#include "ui/renderer_d2d.h"
#include <d2d1.h>
#include <dwrite.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

bool RendererD2D::Initialize(HWND hwnd) {
    m_hwnd = hwnd;
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
        m_factory.GetAddressOf());
    if (FAILED(hr)) return false;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(m_dwrite.GetAddressOf()));
    if (FAILED(hr)) return false;

    hr = m_dwrite->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 14.0f, L"",
        m_text_fmt.GetAddressOf());
    if (FAILED(hr)) return false;
    m_text_fmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_text_fmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    hr = m_dwrite->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"",
        m_text_fmt_small.GetAddressOf());
    if (FAILED(hr)) return false;
    m_text_fmt_small->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_text_fmt_small->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    RECT rc;
    GetClientRect(hwnd, &rc);
    m_size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties();
    D2D1_HWND_RENDER_TARGET_PROPERTIES hprops =
        D2D1::HwndRenderTargetProperties(hwnd, m_size);

    hr = m_factory->CreateHwndRenderTarget(props, hprops, m_rt.GetAddressOf());
    return SUCCEEDED(hr);
}

void RendererD2D::Uninitialize() {
    m_rt.Reset();
    m_text_fmt.Reset();
    m_text_fmt_small.Reset();
    m_dwrite.Reset();
    m_factory.Reset();
}

void RendererD2D::BeginDraw() {
    m_rt->BeginDraw();
}

void RendererD2D::EndDraw() {
    m_rt->EndDraw();
}

void RendererD2D::Clear(D2D1_COLOR_F color) {
    m_rt->Clear(color);
}

void RendererD2D::Resize(UINT width, UINT height) {
    m_size = D2D1::SizeU(width, height);
    m_rt->Resize(m_size);
}

void RendererD2D::DrawKey(const KeyRenderInfo& info, const ThemeColors& theme,
                           float x, float y, float w, float h) {
    D2D1_COLOR_F fill = info.is_disabled ? theme.keyDisabled :
                        info.is_remapped ? theme.keyRemapped :
                        info.is_hovered ? theme.keyHover :
                        theme.keyDefault;

    D2D1_COLOR_F stroke = info.is_hovered ? theme.accent : theme.border;
    float sw = info.is_hovered ? 1.5f : 0.5f;

    ComPtr<ID2D1RoundedRectangleGeometry> rr;
    D2D1_ROUNDED_RECT rrc = D2D1::RoundedRect(
        D2D1::RectF(x + 1, y + 1, x + w - 1, y + h - 1), 4.0f, 4.0f);
    m_factory->CreateRoundedRectangleGeometry(&rrc, rr.GetAddressOf());

    ComPtr<ID2D1SolidColorBrush> fill_brush;
    m_rt->CreateSolidColorBrush(fill, fill_brush.GetAddressOf());
    m_rt->FillGeometry(rr.Get(), fill_brush.Get());

    ComPtr<ID2D1SolidColorBrush> stroke_brush;
    m_rt->CreateSolidColorBrush(stroke, stroke_brush.GetAddressOf());
    m_rt->DrawGeometry(rr.Get(), stroke_brush.Get(), sw);

    if (info.cell && !info.cell->hardware_only) {
        ComPtr<ID2D1SolidColorBrush> text_brush;
        m_rt->CreateSolidColorBrush(theme.text, text_brush.GetAddressOf());

        wchar_t label[64];
        MultiByteToWideChar(CP_UTF8, 0, info.cell->label.c_str(), -1,
                            label, ARRAYSIZE(label));
        m_rt->DrawText(label, static_cast<UINT32>(wcslen(label)),
                       m_text_fmt.Get(),
                       D2D1::RectF(x + 2, y + 2, x + w - 2, y + h - 14),
                       text_brush.Get());

        if (info.is_remapped && !info.remap_label.empty()) {
            ComPtr<ID2D1SolidColorBrush> muted_brush;
            m_rt->CreateSolidColorBrush(theme.textMuted,
                                        muted_brush.GetAddressOf());
            wchar_t remap[64];
            MultiByteToWideChar(CP_UTF8, 0, info.remap_label.c_str(), -1,
                                remap, ARRAYSIZE(remap));
            m_rt->DrawText(remap, static_cast<UINT32>(wcslen(remap)),
                           m_text_fmt_small.Get(),
                           D2D1::RectF(x + 2, y + h - 16, x + w - 2, y + h - 2),
                           muted_brush.Get());
        }
    } else if (info.cell && info.cell->hardware_only) {
        ComPtr<ID2D1SolidColorBrush> muted_brush;
        m_rt->CreateSolidColorBrush(theme.textMuted, muted_brush.GetAddressOf());
        wchar_t label[64];
        MultiByteToWideChar(CP_UTF8, 0, info.cell->label.c_str(), -1,
                            label, ARRAYSIZE(label));
        m_rt->DrawText(label, static_cast<UINT32>(wcslen(label)),
                       m_text_fmt.Get(),
                       D2D1::RectF(x + 2, y + 2, x + w - 2, y + h - 2),
                       muted_brush.Get());
    }
}

void RendererD2D::DrawText(const wchar_t* text, float x, float y,
                            const ThemeColors& theme, float font_size) {
    ComPtr<ID2D1SolidColorBrush> brush;
    m_rt->CreateSolidColorBrush(theme.text, brush.GetAddressOf());
    ComPtr<IDWriteTextFormat> format;
    if (FAILED(m_dwrite->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, font_size, L"", format.GetAddressOf()))) {
        return;
    }
    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    m_rt->DrawText(text, static_cast<UINT32>(wcslen(text)),
                   format.Get(),
                   D2D1::RectF(x, y, x + 300, y + 30),
                   brush.Get());
}

void RendererD2D::DrawTextMuted(const wchar_t* text, float x, float y,
                                 const ThemeColors& theme, float font_size) {
    ComPtr<ID2D1SolidColorBrush> brush;
    m_rt->CreateSolidColorBrush(theme.textMuted, brush.GetAddressOf());
    ComPtr<IDWriteTextFormat> format;
    if (FAILED(m_dwrite->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, font_size, L"", format.GetAddressOf()))) {
        return;
    }
    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    m_rt->DrawText(text, static_cast<UINT32>(wcslen(text)),
                   format.Get(),
                   D2D1::RectF(x, y, x + 300, y + 20),
                   brush.Get());
}

void RendererD2D::DrawRect(float x, float y, float w, float h,
                            D2D1_COLOR_F color) {
    ComPtr<ID2D1SolidColorBrush> brush;
    m_rt->CreateSolidColorBrush(color, brush.GetAddressOf());
    m_rt->FillRectangle(D2D1::RectF(x, y, x + w, y + h), brush.Get());
}

void RendererD2D::DrawRoundedRect(float x, float y, float w, float h,
                                   D2D1_COLOR_F fill, D2D1_COLOR_F stroke,
                                   float stroke_width) {
    ComPtr<ID2D1RoundedRectangleGeometry> rr;
    D2D1_ROUNDED_RECT rrc = D2D1::RoundedRect(
        D2D1::RectF(x, y, x + w, y + h), 4.0f, 4.0f);
    m_factory->CreateRoundedRectangleGeometry(&rrc, rr.GetAddressOf());

    ComPtr<ID2D1SolidColorBrush> fill_brush;
    m_rt->CreateSolidColorBrush(fill, fill_brush.GetAddressOf());
    m_rt->FillGeometry(rr.Get(), fill_brush.Get());

    ComPtr<ID2D1SolidColorBrush> stroke_brush;
    m_rt->CreateSolidColorBrush(stroke, stroke_brush.GetAddressOf());
    m_rt->DrawGeometry(rr.Get(), stroke_brush.Get(), stroke_width);
}

void RendererD2D::DrawPendingItem(const wchar_t* text, float x, float y,
                                   float w, const ThemeColors& theme) {
    DrawRoundedRect(x, y, w, 24.0f, theme.pendingBg, theme.border, 0.5f);
    ComPtr<ID2D1SolidColorBrush> text_brush;
    m_rt->CreateSolidColorBrush(theme.text, text_brush.GetAddressOf());
    m_rt->DrawText(text, static_cast<UINT32>(wcslen(text)),
                   m_text_fmt_small.Get(),
                   D2D1::RectF(x + 8, y + 2, x + w - 8, y + 22),
                   text_brush.Get());
}
