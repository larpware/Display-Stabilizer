#pragma once

#include <windows.h>
#include <string>
#include "../core/DisplayManager.h"

namespace DisplayStabilizer {

class RollbackDialog {
public:
    static bool show(HWND hParent, const DisplayMode& newMode, int countdownSeconds = 15);

private:
    RollbackDialog(HWND hParent, const DisplayMode& newMode, int countdownSeconds);
    ~RollbackDialog();

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND m_hwnd = nullptr;
    HWND m_parent = nullptr;
    DisplayMode m_mode;
    int m_remainingSeconds;
    bool m_confirmed = false;
    HWND m_hStatusLabel = nullptr;
    HWND m_hTimerLabel = nullptr;
};

} // namespace DisplayStabilizer
