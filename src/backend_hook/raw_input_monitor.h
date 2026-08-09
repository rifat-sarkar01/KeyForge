#pragma once

#include <windows.h>

// Passive Raw Input instrumentation for diagnosing special keyboard scan-code
// sequences. It never remaps or suppresses input; the low-level hook remains
// the remapping backend.
class RawInputMonitor {
public:
    bool Install(HWND hwnd);
    void Handle(HRAWINPUT input_handle);

private:
    void LogKeyboardEvent(const RAWKEYBOARD& keyboard);
};
