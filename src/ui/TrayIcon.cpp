#include "TrayIcon.h"

namespace DisplayStabilizer {

TrayIcon::TrayIcon(HWND hwnd) : m_hwnd(hwnd) {
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = hwnd;
    m_nid.uID = TRAY_ICON_ID;
    m_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcscpy_s(m_nid.szTip, L"Display Stabilizer");
}

TrayIcon::~TrayIcon() {
    remove();
}

bool TrayIcon::add() {
    if (!m_isAdded) {
        m_isAdded = (Shell_NotifyIconW(NIM_ADD, &m_nid) != FALSE);
    }
    return m_isAdded;
}

bool TrayIcon::remove() {
    if (m_isAdded) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_isAdded = false;
        return true;
    }
    return false;
}

bool TrayIcon::updateTooltip(const std::wstring& tip) {
    wcsncpy_s(m_nid.szTip, tip.c_str(), sizeof(m_nid.szTip) / sizeof(wchar_t) - 1);
    if (m_isAdded) {
        return (Shell_NotifyIconW(NIM_MODIFY, &m_nid) != FALSE);
    }
    return false;
}

void TrayIcon::showContextMenu() {
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    AppendMenuW(hMenu, MF_STRING, 2001, L"&Open Display Stabilizer");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, 2002, L"Toggle &Correction");
    AppendMenuW(hMenu, MF_STRING, 2003, L"&Disable All Corrections");
    AppendMenuW(hMenu, MF_STRING, 2004, L"&Reset to Neutral");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, 2005, L"E&xit");

    SetMenuDefaultItem(hMenu, 2001, FALSE);

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(m_hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, m_hwnd, nullptr);
    PostMessageW(m_hwnd, WM_NULL, 0, 0);

    DestroyMenu(hMenu);
}

} // namespace DisplayStabilizer
