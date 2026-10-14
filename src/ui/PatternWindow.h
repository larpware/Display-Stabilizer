#pragma once

#include <windows.h>
#include "../diagnostics/TestPatterns.h"

namespace DisplayStabilizer {

class PatternWindow {
public:
    static void show(HWND hParent, PatternType initialPattern = PatternType::PureWhite);

private:
    PatternWindow(HWND hParent, PatternType initialPattern);
    ~PatternWindow();

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void onPaint(HDC hdc);
    void onKeyDown(WPARAM key);
    void nextPattern();
    void prevPattern();

    HWND m_hwnd = nullptr;
    HWND m_parent = nullptr;
    PatternType m_currentPattern;
    bool m_showHud = true;
};

} // namespace DisplayStabilizer
