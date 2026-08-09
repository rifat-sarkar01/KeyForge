#include "backend_hook/raw_input_monitor.h"

#include <fstream>
#include <string>
#include <vector>
#include <shlobj.h>

namespace {
std::wstring GetLogPath() {
    wchar_t app_data[MAX_PATH]{};
    if (SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, app_data) != S_OK) {
        return {};
    }
    const std::wstring directory = std::wstring(app_data) + L"\\KeyForge";
    CreateDirectoryW(directory.c_str(), nullptr);
    return directory + L"\\raw-input.log";
}

bool IsInteresting(const RAWKEYBOARD& keyboard) {
    return keyboard.MakeCode == 0x1D || keyboard.MakeCode == 0x45 ||
           keyboard.VKey == VK_PAUSE || keyboard.VKey == VK_CANCEL ||
           keyboard.VKey == VK_NUMLOCK;
}
}  // namespace

bool RawInputMonitor::Install(HWND hwnd) {
    RAWINPUTDEVICE device{};
    device.usUsagePage = 0x01;  // Generic Desktop Controls
    device.usUsage = 0x06;      // Keyboard
    device.dwFlags = RIDEV_INPUTSINK;
    device.hwndTarget = hwnd;
    return RegisterRawInputDevices(&device, 1, sizeof(device)) == TRUE;
}

void RawInputMonitor::Handle(HRAWINPUT input_handle) {
    UINT size = 0;
    if (GetRawInputData(input_handle, RID_INPUT, nullptr, &size,
                        sizeof(RAWINPUTHEADER)) != 0 || size == 0) {
        return;
    }

    std::vector<BYTE> data(size);
    if (GetRawInputData(input_handle, RID_INPUT, data.data(), &size,
                        sizeof(RAWINPUTHEADER)) != size) {
        return;
    }

    const auto* input = reinterpret_cast<const RAWINPUT*>(data.data());
    if (input->header.dwType == RIM_TYPEKEYBOARD && IsInteresting(input->data.keyboard)) {
        LogKeyboardEvent(input->data.keyboard);
    }
}

void RawInputMonitor::LogKeyboardEvent(const RAWKEYBOARD& keyboard) {
    const std::wstring path = GetLogPath();
    if (path.empty()) return;

    std::wofstream log(path, std::ios::app);
    if (!log.is_open()) return;
    log << L"MakeCode=0x" << std::hex << keyboard.MakeCode
        << L" Flags=0x" << keyboard.Flags
        << L" VKey=0x" << keyboard.VKey
        << L" Message=0x" << keyboard.Message << std::dec << L'\n';
}
