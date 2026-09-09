#pragma once
#include <windows.h>
#include <shellapi.h>

#define WM_TRAYICON (WM_USER + 1)
#define IDM_TRAY_OPEN 1001
#define IDM_TRAY_PAUSE 1002
#define IDM_TRAY_QUIT 1003

class TrayIcon {
public:
    bool Install(HWND hwnd);
    void Uninstall();
    void ShowBalloon(const wchar_t* title, const wchar_t* msg);
    void SetPaused(bool paused);

private:
    NOTIFYICONDATAA m_nid{};
};
