#include "PatternWindow.h"
#include <windowsx.h>

namespace DisplayStabilizer {

static const wchar_t* PATTERN_WND_CLASS = L"DisplayStabilizerPatternWnd";

void PatternWindow::show(HWND hParent, PatternType initialPattern) {
    PatternWindow* pw = new PatternWindow(hParent, initialPattern);
    MSG msg;
    while (IsWindow(pw->m_hwnd) && GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    delete pw;
}

PatternWindow::PatternWindow(HWND hParent, PatternType initialPattern)
    : m_parent(hParent), m_currentPattern(initialPattern) {
    
    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = PATTERN_WND_CLASS;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr; // Double-buffering / custom paint
    RegisterClassExW(&wc);

    HMONITOR hMon = MonitorFromWindow(hParent ? hParent : GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi = { sizeof(MONITORINFO) };
    GetMonitorInfoW(hMon, &mi);

    int x = mi.rcMonitor.left;
    int y = mi.rcMonitor.top;
    int w = mi.rcMonitor.right - mi.rcMonitor.left;
    int h = mi.rcMonitor.bottom - mi.rcMonitor.top;

    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST, PATTERN_WND_CLASS, L"Display Stabilizer - Fullscreen Test Pattern",
        WS_POPUP | WS_VISIBLE,
        x, y, w, h,
        hParent, nullptr, GetModuleHandle(nullptr), this
    );

    SetFocus(m_hwnd);
}

PatternWindow::~PatternWindow() {
    if (m_hwnd && IsWindow(m_hwnd)) {
        DestroyWindow(m_hwnd);
    }
}

void PatternWindow::nextPattern() {
    int idx = static_cast<int>(m_currentPattern) + 1;
    if (idx >= static_cast<int>(PatternType::Count)) {
        idx = 0;
    }
    m_currentPattern = static_cast<PatternType>(idx);
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void PatternWindow::prevPattern() {
    int idx = static_cast<int>(m_currentPattern) - 1;
    if (idx < 0) {
        idx = static_cast<int>(PatternType::Count) - 1;
    }
    m_currentPattern = static_cast<PatternType>(idx);
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void PatternWindow::onKeyDown(WPARAM key) {
    switch (key) {
    case VK_ESCAPE:
    case 'Q':
        DestroyWindow(m_hwnd);
        break;
    case VK_RIGHT:
    case VK_DOWN:
    case VK_SPACE:
    case VK_NEXT:
        nextPattern();
        break;
    case VK_LEFT:
    case VK_UP:
    case VK_PRIOR:
        prevPattern();
        break;
    case 'H':
        m_showHud = !m_showHud;
        InvalidateRect(m_hwnd, nullptr, FALSE);
        break;
    default:
        if (key >= '1' && key <= '9') {
            int target = static_cast<int>(key - '1');
            if (target < static_cast<int>(PatternType::Count)) {
                m_currentPattern = static_cast<PatternType>(target);
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
        } else if (key == '0') {
            m_currentPattern = PatternType::Checkerboard1px; // 10th
            InvalidateRect(m_hwnd, nullptr, FALSE);
        } else if (key == VK_OEM_MINUS || key == VK_SUBTRACT) {
            m_currentPattern = PatternType::FineHorizontalLines; // 11th
            InvalidateRect(m_hwnd, nullptr, FALSE);
        } else if (key == VK_OEM_PLUS || key == VK_ADD) {
            m_currentPattern = PatternType::FineVerticalLines; // 12th
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
        break;
    }
}

void PatternWindow::onPaint(HDC hdc) {
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    // Create double-buffer DC to avoid any flicker
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBm = CreateCompatibleBitmap(hdc, w, h);
    HGDIOBJ oldBm = SelectObject(memDC, memBm);

    // Render diagnostic pattern
    TestPatterns::render(memDC, w, h, m_currentPattern);

    // Optional HUD overlay
    if (m_showHud) {
        const PatternInfo& pi = TestPatterns::getPatternInfo(m_currentPattern);

        int hudH = 75;
        RECT hudRc = { 20, h - hudH - 20, w - 20, h - 20 };

        // Dark box with border
        HBRUSH hudBg = CreateSolidBrush(RGB(20, 20, 24));
        HPEN hudPen = CreatePen(PS_SOLID, 1, RGB(80, 80, 90));
        HGDIOBJ oldPen = SelectObject(memDC, hudPen);
        HGDIOBJ oldBrush = SelectObject(memDC, hudBg);

        RoundRect(memDC, hudRc.left, hudRc.top, hudRc.right, hudRc.bottom, 10, 10);

        SelectObject(memDC, oldPen);
        SelectObject(memDC, oldBrush);
        DeleteObject(hudPen);
        DeleteObject(hudBg);

        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, RGB(255, 255, 255));

        HFONT hFontBold = CreateFontW(-16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        HFONT hFontNorm = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");

        // Line 1: Pattern Name
        HGDIOBJ oldFont = SelectObject(memDC, hFontBold);
        RECT txtRc = { hudRc.left + 15, hudRc.top + 8, hudRc.right - 15, hudRc.top + 30 };
        DrawTextW(memDC, pi.name.c_str(), -1, &txtRc, DT_LEFT | DT_SINGLELINE);

        // Line 2: Diagnostic insight
        SelectObject(memDC, hFontNorm);
        SetTextColor(memDC, RGB(200, 210, 225));
        RECT purpRc = { hudRc.left + 15, hudRc.top + 30, hudRc.right - 15, hudRc.top + 48 };
        DrawTextW(memDC, pi.diagnosticPurpose.c_str(), -1, &purpRc, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);

        // Line 3: Controls hint
        SetTextColor(memDC, RGB(150, 160, 175));
        RECT ctrlRc = { hudRc.left + 15, hudRc.top + 50, hudRc.right - 15, hudRc.bottom - 6 };
        std::wstring hint = L"Navigation: [Left / Right / Space] Cycle Patterns | [1..9, 0] Jump | [H] Toggle HUD | [ESC / Right Click] Exit";
        DrawTextW(memDC, hint.c_str(), -1, &ctrlRc, DT_LEFT | DT_SINGLELINE);

        SelectObject(memDC, oldFont);
        DeleteObject(hFontBold);
        DeleteObject(hFontNorm);
    }

    BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBm);
    DeleteObject(memBm);
    DeleteDC(memDC);
}

LRESULT CALLBACK PatternWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PatternWindow* self = reinterpret_cast<PatternWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_NCCREATE: {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (self) self->onPaint(hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1; // Prevent flicker
    case WM_KEYDOWN:
        if (self) self->onKeyDown(wParam);
        return 0;
    case WM_RBUTTONUP:
        DestroyWindow(hwnd);
        return 0;
    case WM_LBUTTONUP:
        if (self) self->nextPattern();
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

} // namespace DisplayStabilizer
