#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "layout/layout_types.h"

struct PickResult {
    std::string target_key_id;
    std::string target_label;
    uint16_t scan_code = 0;
    bool extended = false;
    bool disabled = false;
    bool cancelled = true;
};

class KeyPickerDialog {
public:
    PickResult ShowModal(HWND parent, const Layout& layout,
                        const std::string& source_key_id);

private:
    static LRESULT CALLBACK DialogProc(HWND hwnd, UINT msg,
                                        WPARAM wParam, LPARAM lParam);
    void OnInitDialog(HWND hwnd);
    void OnCommand(WPARAM wParam);
    void PopulateKeys();

    HWND m_hwnd = nullptr;
    HWND m_parent = nullptr;
    Layout m_layout;
    std::string m_source_key_id;
    PickResult m_result;
    HWND m_key_list = nullptr;
    HWND m_disabled_check = nullptr;
    HWND m_ok_btn = nullptr;
    HWND m_cancel_btn = nullptr;

    // Virtual (non-physical) targets - Volume Up/Down, media keys, etc.
    // Populated once in ShowModal so the addresses stay stable for the
    // dialog's lifetime, same as m_layout.keys.
    std::vector<KeyCell> m_function_key_cells;
};
