#include "RollbackDialog.h"
#include "Controls.h"

namespace DisplayStabilizer {

static const wchar_t* ROLLBACK_WND_CLASS = L"DisplayStabilizerRollbackWnd";
static const int IDC_BTN_KEEP = 101;
static const int IDC_BTN_REVERT = 102;
static const UINT_PTR TIMER_ID_ROLLBACK = 1;

bool RollbackDialog::show(HWND hParent, const DisplayMode& newMode, int countdownSeconds) {
    RollbackDialog dlg(hParent, newMode, countdownSeconds);
    if (hParent) EnableWindow(hParent, FALSE);

    MSG msg;
    while (IsWindow(dlg.m_hwnd) && GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (hParent) {
        EnableWindow(hParent, TRUE);
        SetForegroundWindow(hParent);
    }
    return dlg.m_confirmed;
}

RollbackDialog::RollbackDialog(HWND hParent, const DisplayMode& newMode, int countdownSeconds)
    : m_parent(hParent), m_mode(newMode), m_remainingSeconds(countdownSeconds) {

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = ROLLBACK_WND_CLASS;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    RegisterClassExW(&wc);

    int w = 480;
    int h = 230;
    int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;

    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_DLGMODALFRAME, ROLLBACK_WND_CLASS,
        L"Display Stabilizer — Mode Switch Confirmation",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        x, y, w, h,
        hParent, nullptr, GetModuleHandle(nullptr), this
    );

    SetTimer(m_hwnd, TIMER_ID_ROLLBACK, 1000, nullptr);
}

RollbackDialog::~RollbackDialog() {
    if (m_hwnd && IsWindow(m_hwnd)) {
        KillTimer(m_hwnd, TIMER_ID_ROLLBACK);
        DestroyWindow(m_hwnd);
    }
}

LRESULT CALLBACK RollbackDialog::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    RollbackDialog* self = reinterpret_cast<RollbackDialog*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_NCCREATE: {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    case WM_CREATE: {
        HFONT hFontBold = UiHelpers::createDefaultFont(11, true);
        HFONT hFontNorm = UiHelpers::createDefaultFont(10, false);

        HWND hTitle = UiHelpers::createLabel(hwnd, L"Testing Selected Display Mode", 20, 20, 440, 25);
        SendMessageW(hTitle, WM_SETFONT, reinterpret_cast<WPARAM>(hFontBold), TRUE);

        std::wstring modeStr = L"New Resolution: " + self->m_mode.toString();
        HWND hModeLbl = UiHelpers::createLabel(hwnd, modeStr.c_str(), 20, 48, 440, 22);
        SendMessageW(hModeLbl, WM_SETFONT, reinterpret_cast<WPARAM>(hFontNorm), TRUE);

        HWND hDesc = UiHelpers::createLabel(
            hwnd,
            L"Does the screen look stable, sharp, and correctly centered?\n"
            L"If the display remains blank or distorted, wait for the timer to restore settings automatically.",
            20, 72, 440, 40
        );
        SendMessageW(hDesc, WM_SETFONT, reinterpret_cast<WPARAM>(hFontNorm), TRUE);

        std::wstring timerStr = L"Automatically reverting in " + std::to_wstring(self->m_remainingSeconds) + L" seconds...";
        self->m_hTimerLabel = UiHelpers::createLabel(hwnd, timerStr.c_str(), 20, 120, 440, 25);
        SendMessageW(self->m_hTimerLabel, WM_SETFONT, reinterpret_cast<WPARAM>(hFontBold), TRUE);

        HWND hBtnKeep = UiHelpers::createButton(hwnd, L"Keep Changes (Confirm)", 110, 155, 175, 32, IDC_BTN_KEEP, BS_DEFPUSHBUTTON);
        HWND hBtnRevert = UiHelpers::createButton(hwnd, L"Revert Now (Cancel)", 295, 155, 160, 32, IDC_BTN_REVERT);

        SendMessageW(hBtnKeep, WM_SETFONT, reinterpret_cast<WPARAM>(hFontNorm), TRUE);
        SendMessageW(hBtnRevert, WM_SETFONT, reinterpret_cast<WPARAM>(hFontNorm), TRUE);
        SetFocus(hBtnKeep);
        return 0;
    }
    case WM_TIMER: {
        if (wParam == TIMER_ID_ROLLBACK && self) {
            self->m_remainingSeconds--;
            if (self->m_remainingSeconds <= 0) {
                KillTimer(hwnd, TIMER_ID_ROLLBACK);
                self->m_confirmed = false;
                DestroyWindow(hwnd);
                return 0;
            }
            std::wstring timerStr = L"Automatically reverting in " + std::to_wstring(self->m_remainingSeconds) + L" seconds...";
            SetWindowTextW(self->m_hTimerLabel, timerStr.c_str());
        }
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDC_BTN_KEEP) {
            if (self) self->m_confirmed = true;
            DestroyWindow(hwnd);
            return 0;
        } else if (id == IDC_BTN_REVERT || id == IDCANCEL) {
            if (self) self->m_confirmed = false;
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }
    case WM_KEYDOWN: {
        if (wParam == VK_ESCAPE) {
            if (self) self->m_confirmed = false;
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }
    case WM_CLOSE: {
        if (self) self->m_confirmed = false;
        DestroyWindow(hwnd);
        return 0;
    }
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

} // namespace DisplayStabilizer
