#pragma once

#include <windows.h>
#include <shellapi.h>
#include <string>

namespace DisplayStabilizer {

static const UINT WM_TRAYICON = WM_USER + 100;
static const UINT TRAY_ICON_ID = 1;

class TrayIcon {
public:
    TrayIcon(HWND hwnd);
    ~TrayIcon();

    bool add();
    bool remove();
    bool updateTooltip(const std::wstring& tip);
    void showContextMenu();

private:
    HWND m_hwnd;
    bool m_isAdded = false;
    NOTIFYICONDATAW m_nid = {};
};

} // namespace DisplayStabilizer
