#pragma once
#include <windows.h>
#include <cstdint>
#include <array>
#include <atomic>

class KeyboardHook {
public:
    static KeyboardHook& Instance();
    bool Install();
    void Uninstall();
    void UpdateTable(const std::array<uint16_t, 512>& table);
    bool IsInstalled() const { return m_hook != nullptr; }

private:
    KeyboardHook() = default;
    static LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam);
    HHOOK m_hook = nullptr;
    static std::atomic<uint16_t*> s_table_ptr;
    static std::array<uint16_t, 512> s_table;
};
