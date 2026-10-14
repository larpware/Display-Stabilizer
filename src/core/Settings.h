#pragma once

#include <windows.h>
#include <string>
#include "ProfileManager.h"

namespace DisplayStabilizer {

struct AppSettings {
    std::wstring activeProfileId = L"preset_neutral";
    bool correctionsEnabled = false;
    bool startWithWindows = false;
    RECT windowRect = { 100, 100, 950, 720 };
    int selectedTabIndex = 0;
};

class SettingsManager {
public:
    static std::wstring getAppDataDirectory();
    static std::wstring getSettingsFilePath();

    static bool loadSettings(AppSettings& settings, ProfileManager& profileManager, std::wstring& outErr);
    static bool saveSettings(const AppSettings& settings, const ProfileManager& profileManager, std::wstring& outErr);

    static bool isAutoStartEnabled();
    static bool setAutoStartEnabled(bool enable);
};

} // namespace DisplayStabilizer
