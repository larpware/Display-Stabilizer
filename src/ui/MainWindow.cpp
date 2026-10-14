#include "MainWindow.h"
#include "Controls.h"
#include "PatternWindow.h"
#include "RollbackDialog.h"
#include "../diagnostics/HardwareAdvisory.h"
#include <commdlg.h>
#include <windowsx.h>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace DisplayStabilizer {

static const wchar_t* MAIN_WND_CLASS = L"DisplayStabilizerMainWnd";

// Control IDs
enum ControlIds {
    IDC_CMB_PRESETS = 1001,
    IDC_BTN_ENABLE,
    IDC_BTN_DISABLE,
    IDC_BTN_PREVIEW,
    IDC_BTN_REVERT,
    IDC_BTN_RESET_NEUTRAL,
    IDC_BTN_SAVE_PRESET,
    IDC_BTN_DUP_PRESET,
    IDC_BTN_DEL_PRESET,
    IDC_BTN_EXPORT_PRESET,
    IDC_BTN_IMPORT_PRESET,
    IDC_TAB_MAIN,

    // Tab 0 Trackbars & Resets
    IDC_TB_RED_GAIN,
    IDC_TB_GREEN_GAIN,
    IDC_TB_BLUE_GAIN,
    IDC_BTN_RESET_RED_GAIN,
    IDC_BTN_RESET_GREEN_GAIN,
    IDC_BTN_RESET_BLUE_GAIN,

    IDC_TB_RED_OFFSET,
    IDC_TB_GREEN_OFFSET,
    IDC_TB_BLUE_OFFSET,
    IDC_BTN_RESET_RED_OFFSET,
    IDC_BTN_RESET_GREEN_OFFSET,
    IDC_BTN_RESET_BLUE_OFFSET,

    IDC_TB_RED_GAMMA,
    IDC_TB_GREEN_GAMMA,
    IDC_TB_BLUE_GAMMA,
    IDC_BTN_RESET_RED_GAMMA,
    IDC_BTN_RESET_GREEN_GAMMA,
    IDC_BTN_RESET_BLUE_GAMMA,

    IDC_TB_CONTRAST,
    IDC_TB_SATURATION,
    IDC_TB_COLOR_TEMP,
    IDC_BTN_RESET_BALANCE,

    // Tab 1
    IDC_CMB_PATTERNS,
    IDC_BTN_LAUNCH_PATTERN,

    // Tab 3
    IDC_CMB_MODES,
    IDC_BTN_TEST_MODE,

    // Tab 4
    IDC_LV_PROFILES,
    IDC_BTN_PROFILE_NEW,
    IDC_BTN_PROFILE_DUP,
    IDC_BTN_PROFILE_DEL,
    IDC_BTN_PROFILE_RESET_ALL,

    // Tab 5
    IDC_LV_CHECKLIST,
    IDC_BTN_OSD_GUIDE
};

int MainWindow::run(HINSTANCE hInstance, int nCmdShow) {
    // Single instance mutex
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"DisplayStabilizer_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND hExisting = FindWindowW(MAIN_WND_CLASS, nullptr);
        if (hExisting) {
            ShowWindow(hExisting, SW_RESTORE);
            SetForegroundWindow(hExisting);
        }
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX) };
    icex.dwICC = ICC_TAB_CLASSES | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    MainWindow app(hInstance);
    if (!app.create(nCmdShow)) {
        if (hMutex) CloseHandle(hMutex);
        return 1;
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    return static_cast<int>(msg.wParam);
}

MainWindow::MainWindow(HINSTANCE hInstance) : m_hInstance(hInstance) {
}

MainWindow::~MainWindow() {
    if (m_hFontTitle) DeleteObject(m_hFontTitle);
    if (m_hFontBold) DeleteObject(m_hFontBold);
    if (m_hFontNorm) DeleteObject(m_hFontNorm);
    if (m_hFontSmall) DeleteObject(m_hFontSmall);
}

bool MainWindow::create(int nCmdShow) {
    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = m_hInstance;
    wc.lpszClassName = MAIN_WND_CLASS;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    RegisterClassExW(&wc);

    // Initialize engines
    m_colorEngine.initialize(nullptr);
    m_displayManager.refresh();

    std::wstring err;
    SettingsManager::loadSettings(m_settings, m_profileManager, err);

    int clientW = UiHelpers::scale(900, 96);
    int clientH = UiHelpers::scale(670, 96);
    RECT adjRc = { 0, 0, clientW, clientH };
    AdjustWindowRectEx(&adjRc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE, WS_EX_WINDOWEDGE);
    int w = adjRc.right - adjRc.left;
    int h = adjRc.bottom - adjRc.top;

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - w) / 2;
    int posY = (screenH - h) / 2;
    if (posX < 0) posX = 20;
    if (posY < 0) posY = 20;

    m_hwnd = CreateWindowExW(
        WS_EX_WINDOWEDGE, MAIN_WND_CLASS,
        L"Display Stabilizer — VGA Artifact & Color Diagnostic",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
        posX, posY, w, h,
        nullptr, nullptr, m_hInstance, this
    );

    if (!m_hwnd) return false;

    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);

    // If settings had corrections enabled, apply safely
    if (m_settings.correctionsEnabled) {
        applyCorrections();
    } else {
        m_colorEngine.restoreOriginal();
    }
    updateStatusHeader();

    return true;
}

void MainWindow::onCreate() {
    m_dpi = 96;
    HDC hdc = GetDC(m_hwnd);
    if (hdc) {
        m_dpi = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(m_hwnd, hdc);
    }

    m_hFontTitle = UiHelpers::createDefaultFont(13, true, m_dpi);
    m_hFontBold = UiHelpers::createDefaultFont(9, true, m_dpi);
    m_hFontNorm = UiHelpers::createDefaultFont(9, false, m_dpi);
    m_hFontSmall = UiHelpers::createDefaultFont(8, false, m_dpi);

    // Tray Icon
    m_trayIcon = std::make_unique<TrayIcon>(m_hwnd);
    m_trayIcon->add();

    // Global Hotkeys: Win+Alt+S (toggle), Win+Alt+X (disable)
    if (!RegisterHotKey(m_hwnd, 1, MOD_WIN | MOD_ALT, 'S')) {
        RegisterHotKey(m_hwnd, 1, MOD_CONTROL | MOD_ALT, 'D'); // Fallback
    }
    if (!RegisterHotKey(m_hwnd, 2, MOD_WIN | MOD_ALT, 'X')) {
        RegisterHotKey(m_hwnd, 2, MOD_CONTROL | MOD_ALT, 'X'); // Fallback
    }

    createHeader(m_dpi);
    createTabControl(m_dpi);

    createTabColorCorrection(m_dpi);
    createTabGrainDiagnostics(m_dpi);
    createTabFlickerDiagnostics(m_dpi);
    createTabDisplayMode(m_dpi);
    createTabProfiles(m_dpi);
    createTabHardwareGuide(m_dpi);

    // Select initial tab
    TabCtrl_SetCurSel(m_hTabControl, m_settings.selectedTabIndex);
    onTabChanged();

    populatePresetCombo();
    populateModesCombo();
    populateProfilesList();
    syncUiFromActiveProfile();
}

void MainWindow::onDestroy() {
    // Unregister hotkeys
    UnregisterHotKey(m_hwnd, 1);
    UnregisterHotKey(m_hwnd, 2);

    // Always restore original gamma ramp safely on exit!
    m_colorEngine.restoreOriginal();

    // Save window rect and settings
    RECT rc;
    GetWindowRect(m_hwnd, &rc);
    m_settings.windowRect = rc;
    m_settings.selectedTabIndex = TabCtrl_GetCurSel(m_hTabControl);
    m_settings.activeProfileId = m_profileManager.getActiveProfile().id;
    m_settings.correctionsEnabled = m_profileManager.getActiveProfile().enabled && m_colorEngine.isApplied();

    std::wstring err;
    SettingsManager::saveSettings(m_settings, m_profileManager, err);

    if (m_trayIcon) {
        m_trayIcon->remove();
    }
    PostQuitMessage(0);
}

void MainWindow::createHeader(int dpi) {
    int pad = UiHelpers::scale(12, dpi);
    int top = pad;

    // App Title
    m_hLblTitle = UiHelpers::createLabel(m_hwnd, L"Display Stabilizer", pad, top, UiHelpers::scale(240, dpi), UiHelpers::scale(24, dpi));
    SendMessageW(m_hLblTitle, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontTitle), TRUE);

    // Active status badge
    m_hLblActiveBadge = UiHelpers::createLabel(m_hwnd, L"[CORRECTIONS DISABLED]", UiHelpers::scale(260, dpi), top + 2, UiHelpers::scale(220, dpi), UiHelpers::scale(22, dpi));
    SendMessageW(m_hLblActiveBadge, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    top += UiHelpers::scale(26, dpi);

    // Hardware Status Line
    std::wstring dispStatus = L"Display: " + m_displayManager.getPrimaryDisplayInfo().monitorString +
                             L" (VGA Analog) | Adapter: " + m_displayManager.getPrimaryDisplayInfo().adapterString;
    m_hLblDisplayStatus = UiHelpers::createLabel(m_hwnd, dispStatus.c_str(), pad, top, UiHelpers::scale(860, dpi), UiHelpers::scale(20, dpi));
    SendMessageW(m_hLblDisplayStatus, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);

    top += UiHelpers::scale(24, dpi);

    // Global Controls Bar
    HWND hLblPreset = UiHelpers::createLabel(m_hwnd, L"Preset:", pad, top + 4, UiHelpers::scale(50, dpi), UiHelpers::scale(20, dpi));
    SendMessageW(hLblPreset, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    m_hCmbPresets = UiHelpers::createComboBox(m_hwnd, pad + UiHelpers::scale(55, dpi), top, UiHelpers::scale(260, dpi), UiHelpers::scale(200, dpi), IDC_CMB_PRESETS);
    SendMessageW(m_hCmbPresets, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);

    int btnX = pad + UiHelpers::scale(325, dpi);
    int btnW = UiHelpers::scale(105, dpi);
    int btnH = UiHelpers::scale(26, dpi);
    int btnGap = UiHelpers::scale(6, dpi);

    m_hBtnEnable = UiHelpers::createButton(m_hwnd, L"Enable", btnX, top, btnW, btnH, IDC_BTN_ENABLE);
    btnX += btnW + btnGap;
    m_hBtnDisable = UiHelpers::createButton(m_hwnd, L"Disable All", btnX, top, btnW, btnH, IDC_BTN_DISABLE);
    btnX += btnW + btnGap;
    m_hBtnPreview = UiHelpers::createButton(m_hwnd, L"Preview A/B", btnX, top, btnW, btnH, IDC_BTN_PREVIEW);
    btnX += btnW + btnGap;
    m_hBtnRevert = UiHelpers::createButton(m_hwnd, L"Revert", btnX, top, UiHelpers::scale(85, dpi), btnH, IDC_BTN_REVERT);
    btnX += UiHelpers::scale(85, dpi) + btnGap;
    m_hBtnResetNeutral = UiHelpers::createButton(m_hwnd, L"Reset Neutral", btnX, top, UiHelpers::scale(105, dpi), btnH, IDC_BTN_RESET_NEUTRAL);

    SendMessageW(m_hBtnEnable, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
    SendMessageW(m_hBtnDisable, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
    SendMessageW(m_hBtnPreview, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
    SendMessageW(m_hBtnRevert, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
    SendMessageW(m_hBtnResetNeutral, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
}

void MainWindow::createTabControl(int dpi) {
    int pad = UiHelpers::scale(12, dpi);
    int top = UiHelpers::scale(88, dpi);
    int w = UiHelpers::scale(865, dpi);
    int h = UiHelpers::scale(565, dpi);

    m_hTabControl = CreateWindowExW(
        0, WC_TABCONTROLW, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        pad, top, w, h, m_hwnd, reinterpret_cast<HMENU>(IDC_TAB_MAIN),
        m_hInstance, nullptr
    );
    SendMessageW(m_hTabControl, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    const wchar_t* tabTitles[] = {
        L"Color Correction",
        L"Grain Diagnostics",
        L"Flicker Diagnostics",
        L"Display Mode",
        L"Profiles & Presets",
        L"Hardware & VGA Guide"
    };

    for (int i = 0; i < 6; ++i) {
        TCITEMW tie = { TCIF_TEXT };
        tie.pszText = const_cast<wchar_t*>(tabTitles[i]);
        TabCtrl_InsertItem(m_hTabControl, i, &tie);
    }
}

static const wchar_t* TAB_PAGE_CLASS = L"DisplayStabilizerTabPage";

static LRESULT CALLBACK TabPageWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND:
    case WM_HSCROLL:
    case WM_NOTIFY:
        return SendMessageW(GetParent(hwnd), msg, wParam, lParam);
    case WM_ERASEBKGND: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, GetSysColorBrush(COLOR_BTNFACE));
        return 1;
    }
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

static void registerTabPageClass() {
    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW tc = { sizeof(WNDCLASSEXW) };
        tc.lpfnWndProc = TabPageWndProc;
        tc.hInstance = GetModuleHandle(nullptr);
        tc.lpszClassName = TAB_PAGE_CLASS;
        tc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        tc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
        RegisterClassExW(&tc);
        registered = true;
    }
}

static HWND createTabPage(HWND parent, int dpi) {
    registerTabPageClass();
    int pad = UiHelpers::scale(16, dpi);
    int top = UiHelpers::scale(120, dpi);
    int w = UiHelpers::scale(855, dpi);
    int h = UiHelpers::scale(525, dpi);

    return CreateWindowExW(
        WS_EX_CONTROLPARENT, TAB_PAGE_CLASS, L"",
        WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        pad, top, w, h, parent, nullptr,
        GetModuleHandle(nullptr), nullptr
    );
}

void MainWindow::createTabColorCorrection(int dpi) {
    HWND page = createTabPage(m_hwnd, dpi);
    m_tabPages.push_back(page);

    int colW = UiHelpers::scale(260, dpi);
    int gap = UiHelpers::scale(16, dpi);
    int y = UiHelpers::scale(8, dpi);
    int gbH = UiHelpers::scale(190, dpi);

    // Column 1: Channel Gains (50% to 150%)
    HWND gbGain = UiHelpers::createGroupBox(page, L"RGB Channel Gain (Multiplier)", 0, y, colW, gbH);
    SendMessageW(gbGain, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    int rowY = UiHelpers::scale(24, dpi);
    m_hLblRedGain = UiHelpers::createLabel(page, L"Red Gain: 100%", UiHelpers::scale(10, dpi), y + rowY, UiHelpers::scale(160, dpi), UiHelpers::scale(18, dpi));
    UiHelpers::createButton(page, L"Reset", UiHelpers::scale(190, dpi), y + rowY - 2, UiHelpers::scale(55, dpi), UiHelpers::scale(22, dpi), IDC_BTN_RESET_RED_GAIN);
    m_hTbRedGain = UiHelpers::createTrackbar(page, UiHelpers::scale(10, dpi), y + rowY + 20, UiHelpers::scale(235, dpi), UiHelpers::scale(28, dpi), IDC_TB_RED_GAIN, 50, 150, 100);

    rowY += UiHelpers::scale(52, dpi);
    m_hLblGreenGain = UiHelpers::createLabel(page, L"Green Gain: 100%", UiHelpers::scale(10, dpi), y + rowY, UiHelpers::scale(160, dpi), UiHelpers::scale(18, dpi));
    UiHelpers::createButton(page, L"Reset", UiHelpers::scale(190, dpi), y + rowY - 2, UiHelpers::scale(55, dpi), UiHelpers::scale(22, dpi), IDC_BTN_RESET_GREEN_GAIN);
    m_hTbGreenGain = UiHelpers::createTrackbar(page, UiHelpers::scale(10, dpi), y + rowY + 20, UiHelpers::scale(235, dpi), UiHelpers::scale(28, dpi), IDC_TB_GREEN_GAIN, 50, 150, 100);

    rowY += UiHelpers::scale(52, dpi);
    m_hLblBlueGain = UiHelpers::createLabel(page, L"Blue Gain: 100%", UiHelpers::scale(10, dpi), y + rowY, UiHelpers::scale(160, dpi), UiHelpers::scale(18, dpi));
    UiHelpers::createButton(page, L"Reset", UiHelpers::scale(190, dpi), y + rowY - 2, UiHelpers::scale(55, dpi), UiHelpers::scale(22, dpi), IDC_BTN_RESET_BLUE_GAIN);
    m_hTbBlueGain = UiHelpers::createTrackbar(page, UiHelpers::scale(10, dpi), y + rowY + 20, UiHelpers::scale(235, dpi), UiHelpers::scale(28, dpi), IDC_TB_BLUE_GAIN, 50, 150, 100);

    // Column 2: Channel Offsets (-20% to +20%)
    int col2X = colW + gap;
    HWND gbOffset = UiHelpers::createGroupBox(page, L"RGB Channel Offset (Black Level)", col2X, y, colW, gbH);
    SendMessageW(gbOffset, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    rowY = UiHelpers::scale(24, dpi);
    m_hLblRedOffset = UiHelpers::createLabel(page, L"Red Offset: 0.0%", col2X + UiHelpers::scale(10, dpi), y + rowY, UiHelpers::scale(160, dpi), UiHelpers::scale(18, dpi));
    UiHelpers::createButton(page, L"Reset", col2X + UiHelpers::scale(190, dpi), y + rowY - 2, UiHelpers::scale(55, dpi), UiHelpers::scale(22, dpi), IDC_BTN_RESET_RED_OFFSET);
    m_hTbRedOffset = UiHelpers::createTrackbar(page, col2X + UiHelpers::scale(10, dpi), y + rowY + 20, UiHelpers::scale(235, dpi), UiHelpers::scale(28, dpi), IDC_TB_RED_OFFSET, -20, 20, 0);

    rowY += UiHelpers::scale(52, dpi);
    m_hLblGreenOffset = UiHelpers::createLabel(page, L"Green Offset: 0.0%", col2X + UiHelpers::scale(10, dpi), y + rowY, UiHelpers::scale(160, dpi), UiHelpers::scale(18, dpi));
    UiHelpers::createButton(page, L"Reset", col2X + UiHelpers::scale(190, dpi), y + rowY - 2, UiHelpers::scale(55, dpi), UiHelpers::scale(22, dpi), IDC_BTN_RESET_GREEN_OFFSET);
    m_hTbGreenOffset = UiHelpers::createTrackbar(page, col2X + UiHelpers::scale(10, dpi), y + rowY + 20, UiHelpers::scale(235, dpi), UiHelpers::scale(28, dpi), IDC_TB_GREEN_OFFSET, -20, 20, 0);

    rowY += UiHelpers::scale(52, dpi);
    m_hLblBlueOffset = UiHelpers::createLabel(page, L"Blue Offset: 0.0%", col2X + UiHelpers::scale(10, dpi), y + rowY, UiHelpers::scale(160, dpi), UiHelpers::scale(18, dpi));
    UiHelpers::createButton(page, L"Reset", col2X + UiHelpers::scale(190, dpi), y + rowY - 2, UiHelpers::scale(55, dpi), UiHelpers::scale(22, dpi), IDC_BTN_RESET_BLUE_OFFSET);
    m_hTbBlueOffset = UiHelpers::createTrackbar(page, col2X + UiHelpers::scale(10, dpi), y + rowY + 20, UiHelpers::scale(235, dpi), UiHelpers::scale(28, dpi), IDC_TB_BLUE_OFFSET, -20, 20, 0);

    // Column 3: Channel Gamma (0.50 to 2.50)
    int col3X = col2X + colW + gap;
    HWND gbGamma = UiHelpers::createGroupBox(page, L"RGB Channel Gamma (Non-linear)", col3X, y, colW, gbH);
    SendMessageW(gbGamma, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    rowY = UiHelpers::scale(24, dpi);
    m_hLblRedGamma = UiHelpers::createLabel(page, L"Red Gamma: 1.00", col3X + UiHelpers::scale(10, dpi), y + rowY, UiHelpers::scale(160, dpi), UiHelpers::scale(18, dpi));
    UiHelpers::createButton(page, L"Reset", col3X + UiHelpers::scale(190, dpi), y + rowY - 2, UiHelpers::scale(55, dpi), UiHelpers::scale(22, dpi), IDC_BTN_RESET_RED_GAMMA);
    m_hTbRedGamma = UiHelpers::createTrackbar(page, col3X + UiHelpers::scale(10, dpi), y + rowY + 20, UiHelpers::scale(235, dpi), UiHelpers::scale(28, dpi), IDC_TB_RED_GAMMA, 50, 250, 100);

    rowY += UiHelpers::scale(52, dpi);
    m_hLblGreenGamma = UiHelpers::createLabel(page, L"Green Gamma: 1.00", col3X + UiHelpers::scale(10, dpi), y + rowY, UiHelpers::scale(160, dpi), UiHelpers::scale(18, dpi));
    UiHelpers::createButton(page, L"Reset", col3X + UiHelpers::scale(190, dpi), y + rowY - 2, UiHelpers::scale(55, dpi), UiHelpers::scale(22, dpi), IDC_BTN_RESET_GREEN_GAMMA);
    m_hTbGreenGamma = UiHelpers::createTrackbar(page, col3X + UiHelpers::scale(10, dpi), y + rowY + 20, UiHelpers::scale(235, dpi), UiHelpers::scale(28, dpi), IDC_TB_GREEN_GAMMA, 50, 250, 100);

    rowY += UiHelpers::scale(52, dpi);
    m_hLblBlueGamma = UiHelpers::createLabel(page, L"Blue Gamma: 1.00", col3X + UiHelpers::scale(10, dpi), y + rowY, UiHelpers::scale(160, dpi), UiHelpers::scale(18, dpi));
    UiHelpers::createButton(page, L"Reset", col3X + UiHelpers::scale(190, dpi), y + rowY - 2, UiHelpers::scale(55, dpi), UiHelpers::scale(22, dpi), IDC_BTN_RESET_BLUE_GAMMA);
    m_hTbBlueGamma = UiHelpers::createTrackbar(page, col3X + UiHelpers::scale(10, dpi), y + rowY + 20, UiHelpers::scale(235, dpi), UiHelpers::scale(28, dpi), IDC_TB_BLUE_GAMMA, 50, 250, 100);

    // Row 2: Contrast, Saturation, Color Temp, and Notes
    int y2 = y + gbH + UiHelpers::scale(12, dpi);
    int gb2W = UiHelpers::scale(536, dpi);
    int gb2H = UiHelpers::scale(150, dpi);

    HWND gbBalance = UiHelpers::createGroupBox(page, L"Overall Image Balance & Temperature", 0, y2, gb2W, gb2H);
    SendMessageW(gbBalance, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    rowY = UiHelpers::scale(24, dpi);
    m_hLblContrast = UiHelpers::createLabel(page, L"Contrast: 100%", UiHelpers::scale(10, dpi), y2 + rowY, UiHelpers::scale(180, dpi), UiHelpers::scale(18, dpi));
    m_hTbContrast = UiHelpers::createTrackbar(page, UiHelpers::scale(10, dpi), y2 + rowY + 20, UiHelpers::scale(235, dpi), UiHelpers::scale(28, dpi), IDC_TB_CONTRAST, 50, 150, 100);

    m_hLblSaturation = UiHelpers::createLabel(page, L"Saturation: 100%", UiHelpers::scale(275, dpi), y2 + rowY, UiHelpers::scale(180, dpi), UiHelpers::scale(18, dpi));
    m_hTbSaturation = UiHelpers::createTrackbar(page, UiHelpers::scale(275, dpi), y2 + rowY + 20, UiHelpers::scale(235, dpi), UiHelpers::scale(28, dpi), IDC_TB_SATURATION, 0, 200, 100);

    rowY += UiHelpers::scale(56, dpi);
    m_hLblColorTemp = UiHelpers::createLabel(page, L"Color Temperature Bias: Neutral (0.00)", UiHelpers::scale(10, dpi), y2 + rowY, UiHelpers::scale(300, dpi), UiHelpers::scale(18, dpi));
    m_hTbColorTemp = UiHelpers::createTrackbar(page, UiHelpers::scale(10, dpi), y2 + rowY + 20, UiHelpers::scale(380, dpi), UiHelpers::scale(28, dpi), IDC_TB_COLOR_TEMP, -100, 100, 0);

    HWND btnResBal = UiHelpers::createButton(page, L"Reset Balance", UiHelpers::scale(410, dpi), y2 + rowY + 20, UiHelpers::scale(105, dpi), UiHelpers::scale(26, dpi), IDC_BTN_RESET_BALANCE);
    SendMessageW(btnResBal, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);

    // Right Box: Info & Rules
    HWND gbNotes = UiHelpers::createGroupBox(page, L"Important Calibration Guidelines", gb2W + gap, y2, colW, gb2H);
    SendMessageW(gbNotes, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    const wchar_t* rulesTxt =
        L"• Keep adjustments small (1% to 8%).\n"
        L"• Do not assume pink whites require boosting green globally.\n"
        L"• Reversible instantly: use 'Preview A/B' or 'Disable All'.\n"
        L"• Hardware gamma ramps apply system-wide without DLL injection.";
    HWND hRules = UiHelpers::createLabel(page, rulesTxt, gb2W + gap + UiHelpers::scale(10, dpi), y2 + UiHelpers::scale(24, dpi), colW - UiHelpers::scale(20, dpi), gb2H - UiHelpers::scale(32, dpi));
    SendMessageW(hRules, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontSmall), TRUE);

    // Apply font to all labels
    SendMessageW(m_hLblRedGain, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
    SendMessageW(m_hLblGreenGain, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
    SendMessageW(m_hLblBlueGain, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
    SendMessageW(m_hLblRedOffset, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
    SendMessageW(m_hLblGreenOffset, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
    SendMessageW(m_hLblBlueOffset, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
    SendMessageW(m_hLblRedGamma, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
    SendMessageW(m_hLblGreenGamma, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
    SendMessageW(m_hLblBlueGamma, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
    SendMessageW(m_hLblContrast, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
    SendMessageW(m_hLblSaturation, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
    SendMessageW(m_hLblColorTemp, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
}

void MainWindow::createTabGrainDiagnostics(int dpi) {
    HWND page = createTabPage(m_hwnd, dpi);
    m_tabPages.push_back(page);

    int y = UiHelpers::scale(10, dpi);
    HWND hTitle = UiHelpers::createLabel(page, L"Grain and RGB Artifact Test Patterns", 0, y, UiHelpers::scale(500, dpi), UiHelpers::scale(22, dpi));
    SendMessageW(hTitle, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    y += UiHelpers::scale(26, dpi);
    const wchar_t* intro =
        L"Display static full-screen reference patterns to differentiate analog ADC Phase jitter, cable interference, "
        L"and LCD panel fracture. All test patterns are 100% static with no animations to prevent false flicker.";
    HWND hIntro = UiHelpers::createLabel(page, intro, 0, y, UiHelpers::scale(820, dpi), UiHelpers::scale(36, dpi));
    SendMessageW(hIntro, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);

    y += UiHelpers::scale(42, dpi);
    HWND gbPatterns = UiHelpers::createGroupBox(page, L"Test Pattern Selector", 0, y, UiHelpers::scale(830, dpi), UiHelpers::scale(180, dpi));
    SendMessageW(gbPatterns, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    m_hCmbPatterns = UiHelpers::createComboBox(page, UiHelpers::scale(20, dpi), y + UiHelpers::scale(30, dpi), UiHelpers::scale(400, dpi), UiHelpers::scale(250, dpi), IDC_CMB_PATTERNS);
    SendMessageW(m_hCmbPatterns, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);

    for (const auto& pi : TestPatterns::getPatterns()) {
        SendMessageW(m_hCmbPatterns, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(pi.name.c_str()));
    }
    SendMessageW(m_hCmbPatterns, CB_SETCURSEL, 0, 0);

    HWND btnLaunch = UiHelpers::createButton(page, L"Launch Fullscreen Test Pattern", UiHelpers::scale(440, dpi), y + UiHelpers::scale(28, dpi), UiHelpers::scale(240, dpi), UiHelpers::scale(32, dpi), IDC_BTN_LAUNCH_PATTERN, BS_DEFPUSHBUTTON);
    SendMessageW(btnLaunch, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    m_hLblPatternInfo = UiHelpers::createLabel(page, L"", UiHelpers::scale(20, dpi), y + UiHelpers::scale(75, dpi), UiHelpers::scale(790, dpi), UiHelpers::scale(30, dpi));
    SendMessageW(m_hLblPatternInfo, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);

    m_hLblPatternPurpose = UiHelpers::createLabel(page, L"", UiHelpers::scale(20, dpi), y + UiHelpers::scale(110, dpi), UiHelpers::scale(790, dpi), UiHelpers::scale(50, dpi));
    SendMessageW(m_hLblPatternPurpose, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);

    y += UiHelpers::scale(195, dpi);
    HWND gbDiagGuide = UiHelpers::createGroupBox(page, L"Diagnostic Symptom Mapping Guide", 0, y, UiHelpers::scale(830, dpi), UiHelpers::scale(145, dpi));
    SendMessageW(gbDiagGuide, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    const wchar_t* guide =
        L"1. Fine Vertical Lines & Checkerboard:\n"
        L"   If lines shimmer or look blurry, the monitor ADC Phase is out of sync. Press 'AUTO' on your Acer monitor.\n"
        L"2. Uniform Medium Gray:\n"
        L"   Reveals colored noise and green grain. If speckles stay fixed to physical spots on the glass, subpixels are damaged.\n"
        L"3. Pure White & Grays:\n"
        L"   Tests for pink/magenta cast. If white is magenta but black is pure black, Pin 2 (Green) gain is attenuated.";
    HWND hGuide = UiHelpers::createLabel(page, guide, UiHelpers::scale(20, dpi), y + UiHelpers::scale(25, dpi), UiHelpers::scale(790, dpi), UiHelpers::scale(110, dpi));
    SendMessageW(hGuide, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);

    // Update initial pattern description
    const PatternInfo& pi = TestPatterns::getPatternInfo(PatternType::PureWhite);
    SetWindowTextW(m_hLblPatternInfo, (L"Description: " + pi.description).c_str());
    SetWindowTextW(m_hLblPatternPurpose, (L"Diagnostic Objective: " + pi.diagnosticPurpose).c_str());
}

void MainWindow::createTabFlickerDiagnostics(int dpi) {
    HWND page = createTabPage(m_hwnd, dpi);
    m_tabPages.push_back(page);

    int y = UiHelpers::scale(10, dpi);
    HWND hTitle = UiHelpers::createLabel(page, L"Flicker Diagnostics and Refresh Stability", 0, y, UiHelpers::scale(500, dpi), UiHelpers::scale(22, dpi));
    SendMessageW(hTitle, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    y += UiHelpers::scale(30, dpi);
    HWND gbStatus = UiHelpers::createGroupBox(page, L"Current Refresh Rate and Signal Timing", 0, y, UiHelpers::scale(830, dpi), UiHelpers::scale(100, dpi));
    SendMessageW(gbStatus, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    std::wstring refStr = L"Active Display Frequency: " + std::to_wstring(m_displayManager.getPrimaryDisplayInfo().currentMode.refreshRate) + L" Hz";
    m_hLblRefreshRate = UiHelpers::createLabel(page, refStr.c_str(), UiHelpers::scale(20, dpi), y + UiHelpers::scale(28, dpi), UiHelpers::scale(400, dpi), UiHelpers::scale(24, dpi));
    SendMessageW(m_hLblRefreshRate, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    const wchar_t* timingNotes =
        L"Status: Stable VSync reported by graphics driver. Note: software cannot measure physical panel luminance flicker.";
    HWND hTimingNotes = UiHelpers::createLabel(page, timingNotes, UiHelpers::scale(20, dpi), y + UiHelpers::scale(58, dpi), UiHelpers::scale(780, dpi), UiHelpers::scale(30, dpi));
    SendMessageW(hTimingNotes, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);

    y += UiHelpers::scale(115, dpi);
    HWND gbAnalysis = UiHelpers::createGroupBox(page, L"Physical Flicker Root Cause Analysis (Acer V193HQL)", 0, y, UiHelpers::scale(830, dpi), UiHelpers::scale(240, dpi));
    SendMessageW(gbAnalysis, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    const wchar_t* analysis =
        L"The Acer V193HQL is an analog VGA CCFL (fluorescent tube) backlit monitor. Common causes of flicker:\n\n"
        L"1. CCFL Inverter Aging or Drop Impact:\n"
        L"   Dropping the monitor can fracture the CCFL tube or loosen high-voltage inverter transformer solder joints.\n"
        L"   Inverter instability produces low-frequency (100–120 Hz) full-screen brightness pulsation that software cannot fix.\n\n"
        L"2. Analog VGA Pixel Jitter (Moiré Flicker):\n"
        L"   When the monitor's ADC Clock and Phase are not locked to the analog pixel rate, high-contrast text flickers\n"
        L"   rapidly. Fix this by pressing the physical 'AUTO' button on the monitor while displaying a static pattern.\n\n"
        L"3. Ground Loop / 50-60 Hz AC Ripple:\n"
        L"   If the PC and monitor are on different ungrounded outlets, or the VGA cable shield is broken, 50/60 Hz hum\n"
        L"   rolls vertically across the screen. Connect both to the same surge protector and tighten VGA thumbscrews.\n\n"
        L"Notice: Display Stabilizer intentionally DOES NOT apply rapidly oscillating software filters, as software brightness\n"
        L"pumps worsen eyestrain and cannot stabilize a physically damaged CCFL inverter.";
    m_hLblFlickerAnalysis = UiHelpers::createLabel(page, analysis, UiHelpers::scale(20, dpi), y + UiHelpers::scale(24, dpi), UiHelpers::scale(790, dpi), UiHelpers::scale(205, dpi));
    SendMessageW(m_hLblFlickerAnalysis, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
}

void MainWindow::createTabDisplayMode(int dpi) {
    HWND page = createTabPage(m_hwnd, dpi);
    m_tabPages.push_back(page);

    int y = UiHelpers::scale(10, dpi);
    HWND hTitle = UiHelpers::createLabel(page, L"Display Mode Configuration and Protected Rollback", 0, y, UiHelpers::scale(600, dpi), UiHelpers::scale(22, dpi));
    SendMessageW(hTitle, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    y += UiHelpers::scale(30, dpi);
    HWND gbCurrent = UiHelpers::createGroupBox(page, L"Current Active Display Mode", 0, y, UiHelpers::scale(830, dpi), UiHelpers::scale(80, dpi));
    SendMessageW(gbCurrent, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    std::wstring curModeStr = L"Active Resolution: " + m_displayManager.getPrimaryDisplayInfo().currentMode.toString() +
                             L" | Recommended Native: 1366 x 768 @ 60 Hz";
    m_hLblCurrentMode = UiHelpers::createLabel(page, curModeStr.c_str(), UiHelpers::scale(20, dpi), y + UiHelpers::scale(30, dpi), UiHelpers::scale(780, dpi), UiHelpers::scale(30, dpi));
    SendMessageW(m_hLblCurrentMode, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    y += UiHelpers::scale(95, dpi);
    HWND gbSwitch = UiHelpers::createGroupBox(page, L"Supported Display Modes (Hardware Safe Rollback)", 0, y, UiHelpers::scale(830, dpi), UiHelpers::scale(140, dpi));
    SendMessageW(gbSwitch, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    HWND hLblSelect = UiHelpers::createLabel(page, L"Select Supported Mode:", UiHelpers::scale(20, dpi), y + UiHelpers::scale(32, dpi), UiHelpers::scale(180, dpi), UiHelpers::scale(20, dpi));
    SendMessageW(hLblSelect, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    m_hCmbSupportedModes = UiHelpers::createComboBox(page, UiHelpers::scale(200, dpi), y + UiHelpers::scale(28, dpi), UiHelpers::scale(320, dpi), UiHelpers::scale(250, dpi), IDC_CMB_MODES);
    SendMessageW(m_hCmbSupportedModes, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);

    m_hBtnTestMode = UiHelpers::createButton(page, L"Test Mode (15s Rollback)", UiHelpers::scale(535, dpi), y + UiHelpers::scale(26, dpi), UiHelpers::scale(200, dpi), UiHelpers::scale(30, dpi), IDC_BTN_TEST_MODE, BS_DEFPUSHBUTTON);
    SendMessageW(m_hBtnTestMode, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    const wchar_t* rollbackNote =
        L"Safety Guarantee: Testing a display mode starts a 15-second countdown dialog. If the monitor cannot display the signal\n"
        L"or you do not click 'Keep Changes', Display Stabilizer automatically reverts to the original mode without requiring a reboot.";
    HWND hRollbackNote = UiHelpers::createLabel(page, rollbackNote, UiHelpers::scale(20, dpi), y + UiHelpers::scale(75, dpi), UiHelpers::scale(780, dpi), UiHelpers::scale(45, dpi));
    SendMessageW(hRollbackNote, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);

    y += UiHelpers::scale(155, dpi);
    HWND gbAdvice = UiHelpers::createGroupBox(page, L"Analog VGA Mode Advice for Acer V193HQL", 0, y, UiHelpers::scale(830, dpi), UiHelpers::scale(120, dpi));
    SendMessageW(gbAdvice, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    const wchar_t* advice =
        L"• The Acer V193HQL has a physical 1366x768 60Hz LCD matrix. Running 1366x768 provides 1:1 pixel mapping with zero blur.\n"
        L"• If your graphics driver does not list 1366x768, test standard conservative modes like 1280x1024 @ 60Hz or 1024x768 @ 60Hz.\n"
        L"• Always run 60 Hz over analog VGA. Higher refresh rates (75 Hz) exceed analog cable bandwidth and worsen pixel ghosting.";
    HWND hAdvice = UiHelpers::createLabel(page, advice, UiHelpers::scale(20, dpi), y + UiHelpers::scale(25, dpi), UiHelpers::scale(780, dpi), UiHelpers::scale(80, dpi));
    SendMessageW(hAdvice, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
}

void MainWindow::createTabProfiles(int dpi) {
    HWND page = createTabPage(m_hwnd, dpi);
    m_tabPages.push_back(page);

    int y = UiHelpers::scale(10, dpi);
    HWND hTitle = UiHelpers::createLabel(page, L"Profile and Preset Management", 0, y, UiHelpers::scale(500, dpi), UiHelpers::scale(22, dpi));
    SendMessageW(hTitle, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    y += UiHelpers::scale(30, dpi);
    m_hLvProfiles = UiHelpers::createListView(page, 0, y, UiHelpers::scale(830, dpi), UiHelpers::scale(240, dpi), IDC_LV_PROFILES);
    SendMessageW(m_hLvProfiles, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
    ListView_SetExtendedListViewStyle(m_hLvProfiles, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    LVCOLUMNW lvc = { LVCF_TEXT | LVCF_WIDTH };
    lvc.pszText = const_cast<wchar_t*>(L"Preset Name");
    lvc.cx = UiHelpers::scale(220, dpi);
    ListView_InsertColumn(m_hLvProfiles, 0, &lvc);

    lvc.pszText = const_cast<wchar_t*>(L"Type");
    lvc.cx = UiHelpers::scale(90, dpi);
    ListView_InsertColumn(m_hLvProfiles, 1, &lvc);

    lvc.pszText = const_cast<wchar_t*>(L"RGB Gain (R/G/B)");
    lvc.cx = UiHelpers::scale(150, dpi);
    ListView_InsertColumn(m_hLvProfiles, 2, &lvc);

    lvc.pszText = const_cast<wchar_t*>(L"Contrast / Temp");
    lvc.cx = UiHelpers::scale(120, dpi);
    ListView_InsertColumn(m_hLvProfiles, 3, &lvc);

    lvc.pszText = const_cast<wchar_t*>(L"Description");
    lvc.cx = UiHelpers::scale(230, dpi);
    ListView_InsertColumn(m_hLvProfiles, 4, &lvc);

    y += UiHelpers::scale(250, dpi);
    int btnW = UiHelpers::scale(145, dpi);
    int btnH = UiHelpers::scale(28, dpi);
    int btnGap = UiHelpers::scale(12, dpi);
    int bx = 0;

    HWND btnNew = UiHelpers::createButton(page, L"New Custom Preset", bx, y, btnW, btnH, IDC_BTN_PROFILE_NEW);
    bx += btnW + btnGap;
    HWND btnDup = UiHelpers::createButton(page, L"Duplicate Selected", bx, y, btnW, btnH, IDC_BTN_PROFILE_DUP);
    bx += btnW + btnGap;
    HWND btnDel = UiHelpers::createButton(page, L"Delete Custom", bx, y, btnW, btnH, IDC_BTN_PROFILE_DEL);
    bx += btnW + btnGap;
    HWND btnExp = UiHelpers::createButton(page, L"Export JSON", bx, y, UiHelpers::scale(110, dpi), btnH, IDC_BTN_EXPORT_PRESET);
    bx += UiHelpers::scale(110, dpi) + btnGap;
    HWND btnImp = UiHelpers::createButton(page, L"Import JSON", bx, y, UiHelpers::scale(110, dpi), btnH, IDC_BTN_IMPORT_PRESET);

    SendMessageW(btnNew, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
    SendMessageW(btnDup, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
    SendMessageW(btnDel, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
    SendMessageW(btnExp, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
    SendMessageW(btnImp, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);

    y += UiHelpers::scale(40, dpi);
    HWND gbStorage = UiHelpers::createGroupBox(page, L"Profile Safety and Storage Details", 0, y, UiHelpers::scale(830, dpi), UiHelpers::scale(85, dpi));
    SendMessageW(gbStorage, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    std::wstring storageInfo = L"Profiles are validated and stored in: " + SettingsManager::getAppDataDirectory() +
                              L"\nBuilt-in presets (A-F) are immutable and cannot be deleted or overwritten. Imported JSON profiles are sanitised.";
    HWND hStorageInfo = UiHelpers::createLabel(page, storageInfo.c_str(), UiHelpers::scale(20, dpi), y + UiHelpers::scale(25, dpi), UiHelpers::scale(780, dpi), UiHelpers::scale(45, dpi));
    SendMessageW(hStorageInfo, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
}

void MainWindow::createTabHardwareGuide(int dpi) {
    HWND page = createTabPage(m_hwnd, dpi);
    m_tabPages.push_back(page);

    int y = UiHelpers::scale(10, dpi);
    HWND hTitle = UiHelpers::createLabel(page, L"Guided VGA Checklist and Hardware Damage Assessment", 0, y, UiHelpers::scale(600, dpi), UiHelpers::scale(22, dpi));
    SendMessageW(hTitle, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    y += UiHelpers::scale(28, dpi);
    m_hLvChecklist = UiHelpers::createListView(page, 0, y, UiHelpers::scale(830, dpi), UiHelpers::scale(180, dpi), IDC_LV_CHECKLIST);
    SendMessageW(m_hLvChecklist, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);
    ListView_SetExtendedListViewStyle(m_hLvChecklist, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);

    LVCOLUMNW lvc = { LVCF_TEXT | LVCF_WIDTH };
    lvc.pszText = const_cast<wchar_t*>(L"#");
    lvc.cx = UiHelpers::scale(35, dpi);
    ListView_InsertColumn(m_hLvChecklist, 0, &lvc);

    lvc.pszText = const_cast<wchar_t*>(L"Diagnostic Checkpoint");
    lvc.cx = UiHelpers::scale(250, dpi);
    ListView_InsertColumn(m_hLvChecklist, 1, &lvc);

    lvc.pszText = const_cast<wchar_t*>(L"Physical Action Required");
    lvc.cx = UiHelpers::scale(320, dpi);
    ListView_InsertColumn(m_hLvChecklist, 2, &lvc);

    lvc.pszText = const_cast<wchar_t*>(L"Technical Diagnostic Insight");
    lvc.cx = UiHelpers::scale(350, dpi);
    ListView_InsertColumn(m_hLvChecklist, 3, &lvc);

    for (const auto& item : m_vgaWorkflow.getItems()) {
        LVITEMW lvi = { LVIF_TEXT };
        lvi.iItem = item.id - 1;
        lvi.iSubItem = 0;
        std::wstring idStr = std::to_wstring(item.id);
        lvi.pszText = const_cast<wchar_t*>(idStr.c_str());
        ListView_InsertItem(m_hLvChecklist, &lvi);

        ListView_SetItemText(m_hLvChecklist, item.id - 1, 1, const_cast<wchar_t*>(item.title.c_str()));
        ListView_SetItemText(m_hLvChecklist, item.id - 1, 2, const_cast<wchar_t*>(item.physicalAction.c_str()));
        ListView_SetItemText(m_hLvChecklist, item.id - 1, 3, const_cast<wchar_t*>(item.diagnosticInsight.c_str()));
    }

    y += UiHelpers::scale(190, dpi);
    HWND gbAdvisory = UiHelpers::createGroupBox(page, L"Hardware Advisory — Dropped Monitor and Panel Crack", 0, y, UiHelpers::scale(830, dpi), UiHelpers::scale(175, dpi));
    SendMessageW(gbAdvisory, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);

    std::wstring advText = HardwareAdvisory::getDropDamageAnalysis();
    m_hLblHardwareAdvisory = UiHelpers::createLabel(page, advText.c_str(), UiHelpers::scale(20, dpi), y + UiHelpers::scale(22, dpi), UiHelpers::scale(790, dpi), UiHelpers::scale(140, dpi));
    SendMessageW(m_hLblHardwareAdvisory, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontNorm), TRUE);

    y += UiHelpers::scale(180, dpi);
    HWND btnOsd = UiHelpers::createButton(page, L"View Clock & Phase ADC Technical Details", 0, y, UiHelpers::scale(320, dpi), UiHelpers::scale(28, dpi), IDC_BTN_OSD_GUIDE);
    SendMessageW(btnOsd, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), TRUE);
}

void MainWindow::onTabChanged() {
    int curSel = TabCtrl_GetCurSel(m_hTabControl);
    for (size_t i = 0; i < m_tabPages.size(); ++i) {
        if (static_cast<int>(i) == curSel) {
            ShowWindow(m_tabPages[i], SW_SHOW);
            BringWindowToTop(m_tabPages[i]);
            InvalidateRect(m_tabPages[i], nullptr, TRUE);
            UpdateWindow(m_tabPages[i]);
        } else {
            ShowWindow(m_tabPages[i], SW_HIDE);
        }
    }
}

void MainWindow::populatePresetCombo() {
    SendMessageW(m_hCmbPresets, CB_RESETCONTENT, 0, 0);
    const auto& profiles = m_profileManager.getAllProfiles();
    int activeIdx = 0;
    const std::wstring& activeId = m_profileManager.getActiveProfile().id;

    for (size_t i = 0; i < profiles.size(); ++i) {
        SendMessageW(m_hCmbPresets, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(profiles[i].name.c_str()));
        if (profiles[i].id == activeId) {
            activeIdx = static_cast<int>(i);
        }
    }
    SendMessageW(m_hCmbPresets, CB_SETCURSEL, activeIdx, 0);
}

void MainWindow::populateModesCombo() {
    SendMessageW(m_hCmbSupportedModes, CB_RESETCONTENT, 0, 0);
    const auto& modes = m_displayManager.getSupportedModes();
    const auto& curMode = m_displayManager.getPrimaryDisplayInfo().currentMode;
    int curIdx = 0;

    for (size_t i = 0; i < modes.size(); ++i) {
        std::wstring s = modes[i].toString();
        if (modes[i].width == 1366 && modes[i].height == 768) {
            s += L" [Native Panel Mode]";
        } else if (modes[i].width == 1280 && modes[i].height == 1024) {
            s += L" [Standard 5:4 Mode]";
        }
        SendMessageW(m_hCmbSupportedModes, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s.c_str()));
        if (modes[i] == curMode) {
            curIdx = static_cast<int>(i);
        }
    }
    SendMessageW(m_hCmbSupportedModes, CB_SETCURSEL, curIdx, 0);
}

void MainWindow::populateProfilesList() {
    ListView_DeleteAllItems(m_hLvProfiles);
    const auto& profiles = m_profileManager.getAllProfiles();

    for (size_t i = 0; i < profiles.size(); ++i) {
        const auto& p = profiles[i];
        LVITEMW lvi = { LVIF_TEXT };
        lvi.iItem = static_cast<int>(i);
        lvi.iSubItem = 0;
        lvi.pszText = const_cast<wchar_t*>(p.name.c_str());
        ListView_InsertItem(m_hLvProfiles, &lvi);

        std::wstring typeStr = p.isBuiltIn ? L"Built-in" : L"Custom";
        ListView_SetItemText(m_hLvProfiles, static_cast<int>(i), 1, const_cast<wchar_t*>(typeStr.c_str()));

        wchar_t gainBuf[64];
        swprintf_s(gainBuf, L"%.2f / %.2f / %.2f", p.redGain, p.greenGain, p.blueGain);
        ListView_SetItemText(m_hLvProfiles, static_cast<int>(i), 2, gainBuf);

        wchar_t balBuf[64];
        swprintf_s(balBuf, L"%.2fx / %+.2f", p.contrast, p.colorTempBias);
        ListView_SetItemText(m_hLvProfiles, static_cast<int>(i), 3, balBuf);

        ListView_SetItemText(m_hLvProfiles, static_cast<int>(i), 4, const_cast<wchar_t*>(p.description.c_str()));
    }
}

void MainWindow::updateStatusHeader() {
    const Profile& active = m_profileManager.getActiveProfile();
    bool isApplied = m_colorEngine.isApplied() && active.enabled;

    if (isApplied) {
        SetWindowTextW(m_hLblActiveBadge, L"[● CORRECTIONS ACTIVE]");
    } else {
        SetWindowTextW(m_hLblActiveBadge, L"[● CORRECTIONS DISABLED]");
    }

    std::wstring tip = L"Display Stabilizer — " + active.name + (isApplied ? L" (Active)" : L" (Disabled)");
    if (m_trayIcon) {
        m_trayIcon->updateTooltip(tip);
    }
}

void MainWindow::syncUiFromActiveProfile() {
    const Profile& p = m_profileManager.getActiveProfile();

    // Gains (50 to 150)
    int rg = static_cast<int>(std::round(p.redGain * 100.0));
    int gg = static_cast<int>(std::round(p.greenGain * 100.0));
    int bg = static_cast<int>(std::round(p.blueGain * 100.0));
    SendMessageW(m_hTbRedGain, TBM_SETPOS, TRUE, rg);
    SendMessageW(m_hTbGreenGain, TBM_SETPOS, TRUE, gg);
    SendMessageW(m_hTbBlueGain, TBM_SETPOS, TRUE, bg);
    SetWindowTextW(m_hLblRedGain, (L"Red Gain: " + std::to_wstring(rg) + L"%").c_str());
    SetWindowTextW(m_hLblGreenGain, (L"Green Gain: " + std::to_wstring(gg) + L"%").c_str());
    SetWindowTextW(m_hLblBlueGain, (L"Blue Gain: " + std::to_wstring(bg) + L"%").c_str());

    // Offsets (-20 to +20)
    int ro = static_cast<int>(std::round(p.redOffset * 100.0));
    int go = static_cast<int>(std::round(p.greenOffset * 100.0));
    int bo = static_cast<int>(std::round(p.blueOffset * 100.0));
    SendMessageW(m_hTbRedOffset, TBM_SETPOS, TRUE, ro);
    SendMessageW(m_hTbGreenOffset, TBM_SETPOS, TRUE, go);
    SendMessageW(m_hTbBlueOffset, TBM_SETPOS, TRUE, bo);
    SetWindowTextW(m_hLblRedOffset, (L"Red Offset: " + std::to_wstring(ro) + L"%").c_str());
    SetWindowTextW(m_hLblGreenOffset, (L"Green Offset: " + std::to_wstring(go) + L"%").c_str());
    SetWindowTextW(m_hLblBlueOffset, (L"Blue Offset: " + std::to_wstring(bo) + L"%").c_str());

    // Gammas (50 to 250)
    int rgm = static_cast<int>(std::round(p.redGamma * 100.0));
    int ggm = static_cast<int>(std::round(p.greenGamma * 100.0));
    int bgm = static_cast<int>(std::round(p.blueGamma * 100.0));
    SendMessageW(m_hTbRedGamma, TBM_SETPOS, TRUE, rgm);
    SendMessageW(m_hTbGreenGamma, TBM_SETPOS, TRUE, ggm);
    SendMessageW(m_hTbBlueGamma, TBM_SETPOS, TRUE, bgm);
    wchar_t buf[64];
    swprintf_s(buf, L"Red Gamma: %.2f", p.redGamma); SetWindowTextW(m_hLblRedGamma, buf);
    swprintf_s(buf, L"Green Gamma: %.2f", p.greenGamma); SetWindowTextW(m_hLblGreenGamma, buf);
    swprintf_s(buf, L"Blue Gamma: %.2f", p.blueGamma); SetWindowTextW(m_hLblBlueGamma, buf);

    // Contrast & Saturation
    int ct = static_cast<int>(std::round(p.contrast * 100.0));
    int st = static_cast<int>(std::round(p.saturation * 100.0));
    SendMessageW(m_hTbContrast, TBM_SETPOS, TRUE, ct);
    SendMessageW(m_hTbSaturation, TBM_SETPOS, TRUE, st);
    SetWindowTextW(m_hLblContrast, (L"Contrast: " + std::to_wstring(ct) + L"%").c_str());
    SetWindowTextW(m_hLblSaturation, (L"Saturation: " + std::to_wstring(st) + L"%").c_str());

    // Color Temp (-100 to 100)
    int tp = static_cast<int>(std::round(p.colorTempBias * 100.0));
    SendMessageW(m_hTbColorTemp, TBM_SETPOS, TRUE, tp);
    std::wstring tempStr = L"Color Temp: Neutral (0.00)";
    if (tp > 0) tempStr = L"Color Temp: Warmer (+" + std::to_wstring(tp) + L"%)";
    else if (tp < 0) tempStr = L"Color Temp: Cooler (" + std::to_wstring(tp) + L"%)";
    SetWindowTextW(m_hLblColorTemp, tempStr.c_str());

    updateStatusHeader();
}

void MainWindow::syncActiveProfileFromUi() {
    Profile p = m_profileManager.getActiveProfile();

    int rg = static_cast<int>(SendMessageW(m_hTbRedGain, TBM_GETPOS, 0, 0));
    int gg = static_cast<int>(SendMessageW(m_hTbGreenGain, TBM_GETPOS, 0, 0));
    int bg = static_cast<int>(SendMessageW(m_hTbBlueGain, TBM_GETPOS, 0, 0));
    p.redGain = rg / 100.0;
    p.greenGain = gg / 100.0;
    p.blueGain = bg / 100.0;

    int ro = static_cast<int>(SendMessageW(m_hTbRedOffset, TBM_GETPOS, 0, 0));
    int go = static_cast<int>(SendMessageW(m_hTbGreenOffset, TBM_GETPOS, 0, 0));
    int bo = static_cast<int>(SendMessageW(m_hTbBlueOffset, TBM_GETPOS, 0, 0));
    p.redOffset = ro / 100.0;
    p.greenOffset = go / 100.0;
    p.blueOffset = bo / 100.0;

    int rgm = static_cast<int>(SendMessageW(m_hTbRedGamma, TBM_GETPOS, 0, 0));
    int ggm = static_cast<int>(SendMessageW(m_hTbGreenGamma, TBM_GETPOS, 0, 0));
    int bgm = static_cast<int>(SendMessageW(m_hTbBlueGamma, TBM_GETPOS, 0, 0));
    p.redGamma = rgm / 100.0;
    p.greenGamma = ggm / 100.0;
    p.blueGamma = bgm / 100.0;

    int ct = static_cast<int>(SendMessageW(m_hTbContrast, TBM_GETPOS, 0, 0));
    int st = static_cast<int>(SendMessageW(m_hTbSaturation, TBM_GETPOS, 0, 0));
    p.contrast = ct / 100.0;
    p.saturation = st / 100.0;

    int tp = static_cast<int>(SendMessageW(m_hTbColorTemp, TBM_GETPOS, 0, 0));
    p.colorTempBias = tp / 100.0;

    p.clamp();
    m_profileManager.updateProfile(p);

    // If corrections currently enabled, re-apply live
    if (p.enabled && m_colorEngine.isApplied()) {
        m_colorEngine.applyProfile(p);
    }
}

void MainWindow::onHScroll(HWND hTrackbar) {
    if (!hTrackbar) return;

    if (hTrackbar == m_hTbRedGain) {
        int v = static_cast<int>(SendMessageW(hTrackbar, TBM_GETPOS, 0, 0));
        SetWindowTextW(m_hLblRedGain, (L"Red Gain: " + std::to_wstring(v) + L"%").c_str());
    } else if (hTrackbar == m_hTbGreenGain) {
        int v = static_cast<int>(SendMessageW(hTrackbar, TBM_GETPOS, 0, 0));
        SetWindowTextW(m_hLblGreenGain, (L"Green Gain: " + std::to_wstring(v) + L"%").c_str());
    } else if (hTrackbar == m_hTbBlueGain) {
        int v = static_cast<int>(SendMessageW(hTrackbar, TBM_GETPOS, 0, 0));
        SetWindowTextW(m_hLblBlueGain, (L"Blue Gain: " + std::to_wstring(v) + L"%").c_str());
    } else if (hTrackbar == m_hTbRedOffset) {
        int v = static_cast<int>(SendMessageW(hTrackbar, TBM_GETPOS, 0, 0));
        SetWindowTextW(m_hLblRedOffset, (L"Red Offset: " + std::to_wstring(v) + L"%").c_str());
    } else if (hTrackbar == m_hTbGreenOffset) {
        int v = static_cast<int>(SendMessageW(hTrackbar, TBM_GETPOS, 0, 0));
        SetWindowTextW(m_hLblGreenOffset, (L"Green Offset: " + std::to_wstring(v) + L"%").c_str());
    } else if (hTrackbar == m_hTbBlueOffset) {
        int v = static_cast<int>(SendMessageW(hTrackbar, TBM_GETPOS, 0, 0));
        SetWindowTextW(m_hLblBlueOffset, (L"Blue Offset: " + std::to_wstring(v) + L"%").c_str());
    } else if (hTrackbar == m_hTbRedGamma) {
        double v = static_cast<int>(SendMessageW(hTrackbar, TBM_GETPOS, 0, 0)) / 100.0;
        wchar_t b[64]; swprintf_s(b, L"Red Gamma: %.2f", v); SetWindowTextW(m_hLblRedGamma, b);
    } else if (hTrackbar == m_hTbGreenGamma) {
        double v = static_cast<int>(SendMessageW(hTrackbar, TBM_GETPOS, 0, 0)) / 100.0;
        wchar_t b[64]; swprintf_s(b, L"Green Gamma: %.2f", v); SetWindowTextW(m_hLblGreenGamma, b);
    } else if (hTrackbar == m_hTbBlueGamma) {
        double v = static_cast<int>(SendMessageW(hTrackbar, TBM_GETPOS, 0, 0)) / 100.0;
        wchar_t b[64]; swprintf_s(b, L"Blue Gamma: %.2f", v); SetWindowTextW(m_hLblBlueGamma, b);
    } else if (hTrackbar == m_hTbContrast) {
        int v = static_cast<int>(SendMessageW(hTrackbar, TBM_GETPOS, 0, 0));
        SetWindowTextW(m_hLblContrast, (L"Contrast: " + std::to_wstring(v) + L"%").c_str());
    } else if (hTrackbar == m_hTbSaturation) {
        int v = static_cast<int>(SendMessageW(hTrackbar, TBM_GETPOS, 0, 0));
        SetWindowTextW(m_hLblSaturation, (L"Saturation: " + std::to_wstring(v) + L"%").c_str());
    } else if (hTrackbar == m_hTbColorTemp) {
        int v = static_cast<int>(SendMessageW(hTrackbar, TBM_GETPOS, 0, 0));
        std::wstring s = L"Color Temp: Neutral (0.00)";
        if (v > 0) s = L"Color Temp: Warmer (+" + std::to_wstring(v) + L"%)";
        else if (v < 0) s = L"Color Temp: Cooler (" + std::to_wstring(v) + L"%)";
        SetWindowTextW(m_hLblColorTemp, s.c_str());
    }

    syncActiveProfileFromUi();
}

void MainWindow::applyCorrections() {
    Profile p = m_profileManager.getActiveProfile();
    p.enabled = true;
    m_profileManager.updateProfile(p);
    m_colorEngine.applyProfile(p);
    updateStatusHeader();
}

void MainWindow::disableAllCorrections() {
    Profile p = m_profileManager.getActiveProfile();
    p.enabled = false;
    m_profileManager.updateProfile(p);
    m_colorEngine.disableAll();
    updateStatusHeader();
}

void MainWindow::togglePreviewAB() {
    if (!m_previewToggled) {
        // Switch to original / neutral for comparison
        m_prePreviewProfile = m_profileManager.getActiveProfile();
        m_colorEngine.restoreOriginal();
        m_previewToggled = true;
        SetWindowTextW(m_hBtnPreview, L"Preview A/B [Original]");
    } else {
        // Re-apply modified active profile
        m_colorEngine.applyProfile(m_prePreviewProfile);
        m_previewToggled = false;
        SetWindowTextW(m_hBtnPreview, L"Preview A/B");
    }
    updateStatusHeader();
}

void MainWindow::revertToPrevious() {
    m_colorEngine.restoreOriginal();
    m_previewToggled = false;
    SetWindowTextW(m_hBtnPreview, L"Preview A/B");
    updateStatusHeader();
}

void MainWindow::resetToNeutral() {
    Profile p = m_profileManager.getActiveProfile();
    p.resetToNeutral();
    p.enabled = false;
    m_profileManager.updateProfile(p);
    m_colorEngine.restoreOriginal();
    syncUiFromActiveProfile();
    updateStatusHeader();
}

void MainWindow::duplicateCurrentPreset() {
    const Profile& cur = m_profileManager.getActiveProfile();
    std::wstring newId;
    std::wstring newName = cur.name + L" (Copy)";
    if (m_profileManager.duplicateProfile(cur.id, newName, newId)) {
        m_profileManager.setActiveProfileId(newId);
        populatePresetCombo();
        populateProfilesList();
        syncUiFromActiveProfile();
        MessageBoxW(m_hwnd, (L"Preset duplicated successfully as:\n" + newName).c_str(), L"Preset Duplicated", MB_OK | MB_ICONINFORMATION);
    }
}

void MainWindow::deleteCurrentPreset() {
    const Profile& cur = m_profileManager.getActiveProfile();
    if (cur.isBuiltIn) {
        MessageBoxW(m_hwnd, L"Built-in presets (Preset A - F) cannot be deleted.", L"Cannot Delete", MB_OK | MB_ICONWARNING);
        return;
    }

    if (MessageBoxW(m_hwnd, (L"Are you sure you want to delete profile:\n" + cur.name).c_str(), L"Confirm Delete", MB_YESNO | MB_ICONQUESTION) == IDYES) {
        m_profileManager.deleteProfile(cur.id);
        populatePresetCombo();
        populateProfilesList();
        syncUiFromActiveProfile();
    }
}

void MainWindow::exportCurrentPreset() {
    const Profile& cur = m_profileManager.getActiveProfile();
    wchar_t fileName[MAX_PATH] = L"preset_export.json";

    OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = L"JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"json";

    if (GetSaveFileNameW(&ofn)) {
        std::wstring err;
        if (m_profileManager.exportProfileToJsonFile(cur.id, fileName, err)) {
            MessageBoxW(m_hwnd, L"Preset successfully exported to JSON file.", L"Export Succeeded", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxW(m_hwnd, err.c_str(), L"Export Failed", MB_OK | MB_ICONERROR);
        }
    }
}

void MainWindow::importPreset() {
    wchar_t fileName[MAX_PATH] = L"";

    OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = L"JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        std::wstring newId, err;
        if (m_profileManager.importProfileFromJsonFile(fileName, newId, err)) {
            m_profileManager.setActiveProfileId(newId);
            populatePresetCombo();
            populateProfilesList();
            syncUiFromActiveProfile();
            MessageBoxW(m_hwnd, L"Profile successfully imported and activated.", L"Import Succeeded", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxW(m_hwnd, err.c_str(), L"Import Failed", MB_OK | MB_ICONERROR);
        }
    }
}

void MainWindow::testSelectedDisplayMode() {
    int idx = static_cast<int>(SendMessageW(m_hCmbSupportedModes, CB_GETCURSEL, 0, 0));
    const auto& modes = m_displayManager.getSupportedModes();
    if (idx < 0 || idx >= static_cast<int>(modes.size())) return;

    const DisplayMode& targetMode = modes[idx];
    std::wstring err;

    if (!m_displayManager.testMode(targetMode, err)) {
        MessageBoxW(m_hwnd, err.c_str(), L"Mode Switch Failed", MB_OK | MB_ICONERROR);
        return;
    }

    // Launch safe countdown dialog with 15s automatic rollback
    bool keep = RollbackDialog::show(m_hwnd, targetMode, 15);
    if (keep) {
        m_displayManager.applyModePermanent(targetMode, err);
        MessageBoxW(m_hwnd, L"Display mode change confirmed and saved.", L"Mode Confirmed", MB_OK | MB_ICONINFORMATION);
    } else {
        m_displayManager.revertMode(err);
    }

    m_displayManager.refresh();
    populateModesCombo();
    std::wstring curModeStr = L"Active Resolution: " + m_displayManager.getPrimaryDisplayInfo().currentMode.toString() +
                             L" | Recommended Native: 1366 x 768 @ 60 Hz";
    SetWindowTextW(m_hLblCurrentMode, curModeStr.c_str());
}

void MainWindow::launchPattern(PatternType type) {
    PatternWindow::show(m_hwnd, type);
}

void MainWindow::onCommand(WORD id, WORD notifyCode, HWND hCtrl) {
    (void)hCtrl;
    switch (id) {
    case IDC_CMB_PRESETS: {
        if (notifyCode == CBN_SELCHANGE) {
            int sel = static_cast<int>(SendMessageW(m_hCmbPresets, CB_GETCURSEL, 0, 0));
            const auto& profiles = m_profileManager.getAllProfiles();
            if (sel >= 0 && sel < static_cast<int>(profiles.size())) {
                m_profileManager.setActiveProfileId(profiles[sel].id);
                syncUiFromActiveProfile();
                if (profiles[sel].enabled) {
                    m_colorEngine.applyProfile(profiles[sel]);
                } else {
                    m_colorEngine.restoreOriginal();
                }
                updateStatusHeader();
            }
        }
        break;
    }
    case IDC_BTN_ENABLE:
        applyCorrections();
        break;
    case IDC_BTN_DISABLE:
        disableAllCorrections();
        break;
    case IDC_BTN_PREVIEW:
        togglePreviewAB();
        break;
    case IDC_BTN_REVERT:
        revertToPrevious();
        break;
    case IDC_BTN_RESET_NEUTRAL:
        resetToNeutral();
        break;
    case IDC_BTN_RESET_RED_GAIN: {
        SendMessageW(m_hTbRedGain, TBM_SETPOS, TRUE, 100);
        SetWindowTextW(m_hLblRedGain, L"Red Gain: 100%");
        syncActiveProfileFromUi();
        break;
    }
    case IDC_BTN_RESET_GREEN_GAIN: {
        SendMessageW(m_hTbGreenGain, TBM_SETPOS, TRUE, 100);
        SetWindowTextW(m_hLblGreenGain, L"Green Gain: 100%");
        syncActiveProfileFromUi();
        break;
    }
    case IDC_BTN_RESET_BLUE_GAIN: {
        SendMessageW(m_hTbBlueGain, TBM_SETPOS, TRUE, 100);
        SetWindowTextW(m_hLblBlueGain, L"Blue Gain: 100%");
        syncActiveProfileFromUi();
        break;
    }
    case IDC_BTN_RESET_RED_OFFSET: {
        SendMessageW(m_hTbRedOffset, TBM_SETPOS, TRUE, 0);
        SetWindowTextW(m_hLblRedOffset, L"Red Offset: 0.0%");
        syncActiveProfileFromUi();
        break;
    }
    case IDC_BTN_RESET_GREEN_OFFSET: {
        SendMessageW(m_hTbGreenOffset, TBM_SETPOS, TRUE, 0);
        SetWindowTextW(m_hLblGreenOffset, L"Green Offset: 0.0%");
        syncActiveProfileFromUi();
        break;
    }
    case IDC_BTN_RESET_BLUE_OFFSET: {
        SendMessageW(m_hTbBlueOffset, TBM_SETPOS, TRUE, 0);
        SetWindowTextW(m_hLblBlueOffset, L"Blue Offset: 0.0%");
        syncActiveProfileFromUi();
        break;
    }
    case IDC_BTN_RESET_RED_GAMMA: {
        SendMessageW(m_hTbRedGamma, TBM_SETPOS, TRUE, 100);
        SetWindowTextW(m_hLblRedGamma, L"Red Gamma: 1.00");
        syncActiveProfileFromUi();
        break;
    }
    case IDC_BTN_RESET_GREEN_GAMMA: {
        SendMessageW(m_hTbGreenGamma, TBM_SETPOS, TRUE, 100);
        SetWindowTextW(m_hLblGreenGamma, L"Green Gamma: 1.00");
        syncActiveProfileFromUi();
        break;
    }
    case IDC_BTN_RESET_BLUE_GAMMA: {
        SendMessageW(m_hTbBlueGamma, TBM_SETPOS, TRUE, 100);
        SetWindowTextW(m_hLblBlueGamma, L"Blue Gamma: 1.00");
        syncActiveProfileFromUi();
        break;
    }
    case IDC_BTN_RESET_BALANCE: {
        SendMessageW(m_hTbContrast, TBM_SETPOS, TRUE, 100);
        SendMessageW(m_hTbSaturation, TBM_SETPOS, TRUE, 100);
        SendMessageW(m_hTbColorTemp, TBM_SETPOS, TRUE, 0);
        SetWindowTextW(m_hLblContrast, L"Contrast: 100%");
        SetWindowTextW(m_hLblSaturation, L"Saturation: 100%");
        SetWindowTextW(m_hLblColorTemp, L"Color Temp: Neutral (0.00)");
        syncActiveProfileFromUi();
        break;
    }
    case IDC_CMB_PATTERNS: {
        if (notifyCode == CBN_SELCHANGE) {
            int sel = static_cast<int>(SendMessageW(m_hCmbPatterns, CB_GETCURSEL, 0, 0));
            if (sel >= 0 && sel < static_cast<int>(PatternType::Count)) {
                const PatternInfo& pi = TestPatterns::getPatternInfo(static_cast<PatternType>(sel));
                SetWindowTextW(m_hLblPatternInfo, (L"Description: " + pi.description).c_str());
                SetWindowTextW(m_hLblPatternPurpose, (L"Diagnostic Objective: " + pi.diagnosticPurpose).c_str());
            }
        }
        break;
    }
    case IDC_BTN_LAUNCH_PATTERN: {
        int sel = static_cast<int>(SendMessageW(m_hCmbPatterns, CB_GETCURSEL, 0, 0));
        if (sel >= 0 && sel < static_cast<int>(PatternType::Count)) {
            launchPattern(static_cast<PatternType>(sel));
        }
        break;
    }
    case IDC_BTN_TEST_MODE:
        testSelectedDisplayMode();
        break;
    case IDC_BTN_PROFILE_NEW: {
        std::wstring newId;
        m_profileManager.duplicateProfile(L"preset_neutral", L"Custom Profile", newId);
        m_profileManager.setActiveProfileId(newId);
        populatePresetCombo();
        populateProfilesList();
        syncUiFromActiveProfile();
        break;
    }
    case IDC_BTN_PROFILE_DUP:
        duplicateCurrentPreset();
        break;
    case IDC_BTN_PROFILE_DEL:
        deleteCurrentPreset();
        break;
    case IDC_BTN_EXPORT_PRESET:
        exportCurrentPreset();
        break;
    case IDC_BTN_IMPORT_PRESET:
        importPreset();
        break;
    case IDC_BTN_OSD_GUIDE: {
        std::wstring guide = VgaWorkflow::getClockPhaseExplanation() + L"\n\n" + VgaWorkflow::getVgaColorPinoutExplanation();
        MessageBoxW(m_hwnd, guide.c_str(), L"Analog VGA Clock, Phase & Pinout Guide", MB_OK | MB_ICONINFORMATION);
        break;
    }
    // Tray Menu
    case 2001: // Open
        ShowWindow(m_hwnd, SW_RESTORE);
        SetForegroundWindow(m_hwnd);
        break;
    case 2002: // Toggle
        if (m_colorEngine.isApplied()) disableAllCorrections();
        else applyCorrections();
        break;
    case 2003: // Disable All
        disableAllCorrections();
        break;
    case 2004: // Reset Neutral
        resetToNeutral();
        break;
    case 2005: // Exit
        DestroyWindow(m_hwnd);
        break;
    }
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_NCCREATE: {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        MainWindow* pApp = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
        if (pApp) {
            pApp->m_hwnd = hwnd;
        }
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pApp));
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    case WM_CREATE: {
        if (!self) {
            self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }
        if (self) {
            self->m_hwnd = hwnd;
            self->onCreate();
        }
        return 0;
    }
    case WM_COMMAND:
        if (self) self->onCommand(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<HWND>(lParam));
        return 0;
    case WM_HSCROLL:
        if (self) self->onHScroll(reinterpret_cast<HWND>(lParam));
        return 0;
    case WM_NOTIFY: {
        NMHDR* pnm = reinterpret_cast<NMHDR*>(lParam);
        if (pnm->idFrom == IDC_TAB_MAIN && pnm->code == TCN_SELCHANGE) {
            if (self) self->onTabChanged();
        }
        return 0;
    }
    case WM_HOTKEY: {
        if (self) {
            if (wParam == 1) { // Toggle corrections
                if (self->m_colorEngine.isApplied()) {
                    self->disableAllCorrections();
                } else {
                    self->applyCorrections();
                }
            } else if (wParam == 2) { // Emergency disable all
                self->disableAllCorrections();
            }
        }
        return 0;
    }
    case WM_TRAYICON: {
        if (lParam == WM_RBUTTONUP) {
            if (self && self->m_trayIcon) self->m_trayIcon->showContextMenu();
        } else if (lParam == WM_LBUTTONDBLCLK) {
            ShowWindow(hwnd, SW_RESTORE);
            SetForegroundWindow(hwnd);
        }
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        if (self) self->onDestroy();
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

} // namespace DisplayStabilizer
