#include "backend_hook/keyboard_hook.h"
#include "engine/scancode_utils.h"
#include <windows.h>

namespace {
bool IsKeyUpMessage(WPARAM message) {
    return message == WM_KEYUP || message == WM_SYSKEYUP;
}

void SetKeyboardInput(INPUT& input, uint16_t target, bool key_up) {
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = 0;
    input.ki.wScan = static_cast<WORD>(target & 0xFF);
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    if (IsExtended(target)) input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    if (key_up) input.ki.dwFlags |= KEYEVENTF_KEYUP;
}
}  // namespace

std::atomic<uint16_t*> KeyboardHook::s_table_ptr{nullptr};
std::array<uint16_t, 512> KeyboardHook::s_table{};

KeyboardHook& KeyboardHook::Instance() {
    static KeyboardHook inst;
    return inst;
}

LRESULT CALLBACK KeyboardHook::HookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        auto* pkb = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

        if (pkb->flags & LLKHF_INJECTED) {
            return CallNextHookEx(nullptr, nCode, wParam, lParam);
        }

        uint16_t* table = s_table_ptr.load(std::memory_order_acquire);
        if (table) {
            const bool extended = (pkb->flags & LLKHF_EXTENDED) != 0;
            const bool control_down = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
            const HookSourceKind source_kind = ClassifyHookSource(
                pkb->vkCode, control_down);

            // Ctrl+Pause is the distinct Break operation. Do not silently
            // turn it into Pause or Num Lock; leave it for Windows to handle.
            if (source_kind == HookSourceKind::Break) {
                return CallNextHookEx(nullptr, nCode, wParam, lParam);
            }
            const uint16_t full_code = SourceScanCodeForHook(
                pkb->scanCode, extended, source_kind);

            const size_t index = LookupIndex(full_code);
            uint16_t target = table[index];
            if (target == DisabledScanCode) {
                return 1;
            }
            if (target != 0) {
                // Pause has no normal key-up event. Emit a complete target
                // press here so media keys (for example Volume Up) are not
                // left down or ignored by Windows.
                if (source_kind == HookSourceKind::Pause) {
                    if (IsKeyUpMessage(wParam)) return 1;
                    INPUT inputs[2]{};
                    SetKeyboardInput(inputs[0], target, false);
                    SetKeyboardInput(inputs[1], target, true);
                    if (SendInput(2, inputs, sizeof(INPUT)) != 2) {
                        return CallNextHookEx(nullptr, nCode, wParam, lParam);
                    }
                    return 1;
                }

                INPUT input{};
                SetKeyboardInput(input, target, IsKeyUpMessage(wParam));
                if (SendInput(1, &input, sizeof(INPUT)) == 0) {
                    return CallNextHookEx(nullptr, nCode, wParam, lParam);
                }
                return 1;
            }
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

bool KeyboardHook::Install() {
    if (m_hook) return true;
    m_hook = SetWindowsHookExW(WH_KEYBOARD_LL, HookProc, GetModuleHandleW(nullptr), 0);
    if (m_hook) {
        s_table_ptr.store(s_table.data(), std::memory_order_release);
    }
    return m_hook != nullptr;
}

void KeyboardHook::Uninstall() {
    if (m_hook) {
        UnhookWindowsHookEx(m_hook);
        m_hook = nullptr;
        s_table_ptr.store(nullptr, std::memory_order_release);
    }
}

void KeyboardHook::UpdateTable(const std::array<uint16_t, 512>& table) {
    s_table = table;
    s_table_ptr.store(s_table.data(), std::memory_order_release);
}
