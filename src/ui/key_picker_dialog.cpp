#include "ui/key_picker_dialog.h"
#include "engine/function_keys.h"
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

LRESULT CALLBACK KeyPickerDialog::DialogProc(HWND hwnd, UINT msg,
                                              WPARAM wParam, LPARAM lParam) {
    KeyPickerDialog* dlg = nullptr;
    if (msg == WM_INITDIALOG) {
        dlg = reinterpret_cast<KeyPickerDialog*>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dlg));
        dlg->m_hwnd = hwnd;
        dlg->OnInitDialog(hwnd);
    } else {
        dlg = reinterpret_cast<KeyPickerDialog*>(
            GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    if (dlg) {
        switch (msg) {
        case WM_COMMAND:
            dlg->OnCommand(wParam);
            return TRUE;
        case WM_CLOSE:
            EndDialog(hwnd, 0);
            return TRUE;
        }
    }
    return FALSE;
}

void KeyPickerDialog::OnInitDialog(HWND hwnd) {
    m_key_list = GetDlgItem(hwnd, 1001);
    m_disabled_check = GetDlgItem(hwnd, 1002);
    m_ok_btn = GetDlgItem(hwnd, 1);
    m_cancel_btn = GetDlgItem(hwnd, 2);
    PopulateKeys();
}

namespace {
void AddListEntry(HWND list, const std::string& text, const KeyCell* item_data) {
    wchar_t wentry[128];
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wentry, ARRAYSIZE(wentry));
    int idx = static_cast<int>(SendMessage(list, LB_ADDSTRING, 0,
                                           reinterpret_cast<LPARAM>(wentry)));
    if (idx >= 0 && item_data) {
        SendMessage(list, LB_SETITEMDATA, idx,
                    reinterpret_cast<LPARAM>(item_data));
    }
}

std::string FormatKeyEntry(const std::string& label, uint16_t scan_code,
                            bool extended) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%02X", scan_code);
    std::string entry = label + " (0x" + buf;
    if (extended) entry += " E0";
    entry += ")";
    return entry;
}
}  // namespace

void KeyPickerDialog::PopulateKeys() {
    SendMessage(m_key_list, LB_RESETCONTENT, 0, 0);

    for (const auto& key : m_layout.keys) {
        if (key.id == m_source_key_id) continue;
        if (key.hardware_only) continue;
        AddListEntry(m_key_list, FormatKeyEntry(key.label, key.scan_code,
                                                  key.extended), &key);
    }

    // Virtual targets - Volume Up/Down and other media/system keys that
    // aren't a physical key on this layout, but are still perfectly valid
    // remap targets since the engine only ever deals in scan codes.
    m_function_key_cells.clear();
    for (const auto& fk : GetFunctionKeys()) {
        KeyCell cell;
        cell.id = fk.id;
        cell.label = fk.label;
        cell.scan_code = fk.scan_code;
        cell.extended = fk.extended;
        m_function_key_cells.push_back(cell);
    }

    if (!m_function_key_cells.empty()) {
        AddListEntry(m_key_list, "\xE2\x94\x80\xE2\x94\x80 Function / Media Keys \xE2\x94\x80\xE2\x94\x80",
                     nullptr);
        for (const auto& cell : m_function_key_cells) {
            AddListEntry(m_key_list,
                         FormatKeyEntry(cell.label, cell.scan_code, cell.extended),
                         &cell);
        }
    }
}

void KeyPickerDialog::OnCommand(WPARAM wParam) {
    switch (LOWORD(wParam)) {
    case 1: {
        const bool disabled = m_disabled_check &&
            SendMessage(m_disabled_check, BM_GETCHECK, 0, 0) == BST_CHECKED;
        if (disabled) {
            m_result.target_label = "Disabled";
            m_result.disabled = true;
            m_result.cancelled = false;
            EndDialog(m_hwnd, 1);
            break;
        }
        int sel = static_cast<int>(SendMessage(m_key_list, LB_GETCURSEL, 0, 0));
        if (sel == LB_ERR) break;
        auto* cell = reinterpret_cast<KeyCell*>(
            SendMessage(m_key_list, LB_GETITEMDATA, sel, 0));
        if (cell) {
            m_result.target_key_id = cell->id;
            m_result.target_label = cell->label;
            m_result.scan_code = cell->scan_code;
            m_result.extended = cell->extended;
            m_result.disabled = false;
            m_result.cancelled = false;
        }
        // Clicking the separator (no item data) is a no-op: cell is null,
        // so we just fall through without closing the dialog.
        if (cell) {
            EndDialog(m_hwnd, 1);
        }
        break;
    }
    case 2:
        m_result.cancelled = true;
        EndDialog(m_hwnd, 0);
        break;
    }
}

PickResult KeyPickerDialog::ShowModal(HWND parent, const Layout& layout,
                                       const std::string& source_key_id) {
    m_parent = parent;
    m_layout = layout;
    m_source_key_id = source_key_id;
    m_result = PickResult();

    DialogBoxParam(GetModuleHandle(nullptr), MAKEINTRESOURCE(100),
                   parent, DialogProc, reinterpret_cast<LPARAM>(this));
    return m_result;
}
