#include "tray/tray_icon.h"
#include <shellapi.h>

bool TrayIcon::Install(HWND hwnd) {
    m_nid.cbSize = sizeof(NOTIFYICONDATAA);
    m_nid.hWnd = hwnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    lstrcpyA(m_nid.szTip, "KeyForge - Keyboard Remapper");
    return Shell_NotifyIconA(NIM_ADD, &m_nid) != 0;
}

void TrayIcon::Uninstall() {
    Shell_NotifyIconA(NIM_DELETE, &m_nid);
}

void TrayIcon::ShowBalloon(const wchar_t* title, const wchar_t* msg) {
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = m_nid.hWnd;
    nid.uID = m_nid.uID;
    nid.uFlags = NIF_INFO;
    lstrcpynW(nid.szInfoTitle, title, ARRAYSIZE(nid.szInfoTitle));
    lstrcpynW(nid.szInfo, msg, ARRAYSIZE(nid.szInfo));
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

void TrayIcon::SetPaused(bool paused) {
    lstrcpyA(m_nid.szTip, paused
        ? "KeyForge - Paused"
        : "KeyForge - Keyboard Remapper");
    Shell_NotifyIconA(NIM_MODIFY, &m_nid);
}
