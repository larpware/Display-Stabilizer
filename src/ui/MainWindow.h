#pragma once

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <memory>
#include "../core/ColorEngine.h"
#include "../core/ProfileManager.h"
#include "../core/DisplayManager.h"
#include "../core/Settings.h"
#include "../diagnostics/VgaWorkflow.h"
#include "../diagnostics/TestPatterns.h"
#include "TrayIcon.h"

namespace DisplayStabilizer {

class MainWindow {
public:
    static int run(HINSTANCE hInstance, int nCmdShow);

private:
    MainWindow(HINSTANCE hInstance);
    ~MainWindow();

    bool create(int nCmdShow);
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void onCreate();
    void onDestroy();
    void onCommand(WORD id, WORD notifyCode, HWND hCtrl);
    void onHScroll(HWND hTrackbar);
    void onTabChanged();
    void onPaintRampPreview(HDC hdc, const RECT& rc);

    // UI construction helpers
    void createHeader(int dpi);
    void createTabControl(int dpi);
    void createTabColorCorrection(int dpi);
    void createTabGrainDiagnostics(int dpi);
    void createTabFlickerDiagnostics(int dpi);
    void createTabDisplayMode(int dpi);
    void createTabProfiles(int dpi);
    void createTabHardwareGuide(int dpi);

    // Profile & UI synchronization
    void syncUiFromActiveProfile();
    void syncActiveProfileFromUi();
    void populatePresetCombo();
    void populateModesCombo();
    void populateProfilesList();
    void updateStatusHeader();

    // Actions
    void applyCorrections();
    void disableAllCorrections();
    void togglePreviewAB();
    void revertToPrevious();
    void resetToNeutral();
    void saveCurrentPreset();
    void duplicateCurrentPreset();
    void deleteCurrentPreset();
    void exportCurrentPreset();
    void importPreset();
    void testSelectedDisplayMode();
    void launchPattern(PatternType type);

    HINSTANCE m_hInstance;
    HWND m_hwnd = nullptr;
    int m_dpi = 96;
    HFONT m_hFontTitle = nullptr;
    HFONT m_hFontBold = nullptr;
    HFONT m_hFontNorm = nullptr;
    HFONT m_hFontSmall = nullptr;

    // Core engines
    ColorEngine m_colorEngine;
    ProfileManager m_profileManager;
    DisplayManager m_displayManager;
    AppSettings m_settings;
    VgaWorkflow m_vgaWorkflow;
    std::unique_ptr<TrayIcon> m_trayIcon;

    // A/B preview state
    bool m_previewToggled = false;
    Profile m_prePreviewProfile;

    // Header & Global Controls
    HWND m_hLblTitle = nullptr;
    HWND m_hLblDisplayStatus = nullptr;
    HWND m_hLblProfileStatus = nullptr;
    HWND m_hLblActiveBadge = nullptr;
    HWND m_hCmbPresets = nullptr;
    HWND m_hBtnEnable = nullptr;
    HWND m_hBtnDisable = nullptr;
    HWND m_hBtnPreview = nullptr;
    HWND m_hBtnRevert = nullptr;
    HWND m_hBtnResetNeutral = nullptr;

    // Tabs
    HWND m_hTabControl = nullptr;
    std::vector<HWND> m_tabPages;

    // Tab 0: Color Correction Controls
    HWND m_hTbRedGain = nullptr, m_hLblRedGain = nullptr;
    HWND m_hTbGreenGain = nullptr, m_hLblGreenGain = nullptr;
    HWND m_hTbBlueGain = nullptr, m_hLblBlueGain = nullptr;

    HWND m_hTbRedOffset = nullptr, m_hLblRedOffset = nullptr;
    HWND m_hTbGreenOffset = nullptr, m_hLblGreenOffset = nullptr;
    HWND m_hTbBlueOffset = nullptr, m_hLblBlueOffset = nullptr;

    HWND m_hTbRedGamma = nullptr, m_hLblRedGamma = nullptr;
    HWND m_hTbGreenGamma = nullptr, m_hLblGreenGamma = nullptr;
    HWND m_hTbBlueGamma = nullptr, m_hLblBlueGamma = nullptr;

    HWND m_hTbContrast = nullptr, m_hLblContrast = nullptr;
    HWND m_hTbSaturation = nullptr, m_hLblSaturation = nullptr;
    HWND m_hTbColorTemp = nullptr, m_hLblColorTemp = nullptr;
    HWND m_hPreviewSwatch = nullptr;

    // Tab 1: Grain Diagnostics Controls
    HWND m_hCmbPatterns = nullptr;
    HWND m_hLblPatternInfo = nullptr;
    HWND m_hLblPatternPurpose = nullptr;

    // Tab 2: Flicker Diagnostics Controls
    HWND m_hLblRefreshRate = nullptr;
    HWND m_hLblFlickerAnalysis = nullptr;

    // Tab 3: Display Mode Controls
    HWND m_hLblCurrentMode = nullptr;
    HWND m_hCmbSupportedModes = nullptr;
    HWND m_hBtnTestMode = nullptr;

    // Tab 4: Profiles Controls
    HWND m_hLvProfiles = nullptr;

    // Tab 5: Hardware Guide Controls
    HWND m_hLvChecklist = nullptr;
    HWND m_hLblHardwareAdvisory = nullptr;
};

} // namespace DisplayStabilizer
