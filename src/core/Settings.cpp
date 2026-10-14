#include "Settings.h"
#include <shlobj.h>
#include <fstream>
#include <sstream>

namespace DisplayStabilizer {

static const wchar_t* REG_RUN_PATH = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const wchar_t* APP_RUN_NAME = L"DisplayStabilizer";

std::wstring SettingsManager::getAppDataDirectory() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
        std::wstring dir = std::wstring(path) + L"\\DisplayStabilizer";
        CreateDirectoryW(dir.c_str(), nullptr);
        return dir;
    }
    return L".";
}

std::wstring SettingsManager::getSettingsFilePath() {
    return getAppDataDirectory() + L"\\settings.json";
}

bool SettingsManager::loadSettings(AppSettings& settings, ProfileManager& profileManager, std::wstring& outErr) {
    std::wstring path = getSettingsFilePath();
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        outErr = L"No settings file found, using defaults.";
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();

    JsonValue root;
    if (!JsonValue::parseUtf8(content, root, outErr)) {
        return false;
    }

    if (!root.isObject()) {
        outErr = L"Settings file format is not a JSON object.";
        return false;
    }

    // Load custom profiles first
    if (root.hasKey(L"customProfiles")) {
        profileManager.loadCustomProfilesFromJson(root[L"customProfiles"]);
    }

    // Active profile
    if (root.hasKey(L"activeProfileId")) {
        settings.activeProfileId = root[L"activeProfileId"].asString(L"preset_neutral");
        profileManager.setActiveProfileId(settings.activeProfileId);
    }

    // Correction enabled state
    if (root.hasKey(L"correctionsEnabled")) {
        settings.correctionsEnabled = root[L"correctionsEnabled"].asBool(false);
    }

    // Window position
    if (root.hasKey(L"window")) {
        const auto& wObj = root[L"window"];
        if (wObj.isObject()) {
            settings.windowRect.left = wObj[L"left"].asInt(100);
            settings.windowRect.top = wObj[L"top"].asInt(100);
            settings.windowRect.right = wObj[L"right"].asInt(1050);
            settings.windowRect.bottom = wObj[L"bottom"].asInt(820);
        }
    }

    // Tab index
    if (root.hasKey(L"selectedTab")) {
        settings.selectedTabIndex = root[L"selectedTab"].asInt(0);
    }

    settings.startWithWindows = isAutoStartEnabled();

    return true;
}

bool SettingsManager::saveSettings(const AppSettings& settings, const ProfileManager& profileManager, std::wstring& outErr) {
    JsonValue root(JsonType::Object);
    root[L"formatVersion"] = JsonValue(1);
    root[L"appName"] = JsonValue(L"DisplayStabilizer");
    root[L"activeProfileId"] = JsonValue(settings.activeProfileId);
    root[L"correctionsEnabled"] = JsonValue(settings.correctionsEnabled);
    root[L"selectedTab"] = JsonValue(settings.selectedTabIndex);

    JsonValue wObj(JsonType::Object);
    wObj[L"left"] = JsonValue(static_cast<int>(settings.windowRect.left));
    wObj[L"top"] = JsonValue(static_cast<int>(settings.windowRect.top));
    wObj[L"right"] = JsonValue(static_cast<int>(settings.windowRect.right));
    wObj[L"bottom"] = JsonValue(static_cast<int>(settings.windowRect.bottom));
    root[L"window"] = wObj;

    root[L"customProfiles"] = profileManager.customProfilesToJson();

    std::string jsonUtf8 = root.serializeUtf8(true);
    std::wstring path = getSettingsFilePath();

    std::ofstream ofs(path, std::ios::binary);
    if (!ofs.is_open()) {
        outErr = L"Failed to write settings file: " + path;
        return false;
    }
    ofs.write(jsonUtf8.data(), jsonUtf8.size());
    ofs.close();

    return true;
}

bool SettingsManager::isAutoStartEnabled() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_RUN_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t buf[MAX_PATH];
        DWORD size = sizeof(buf);
        DWORD type = 0;
        LSTATUS status = RegQueryValueExW(hKey, APP_RUN_NAME, nullptr, &type, reinterpret_cast<LPBYTE>(buf), &size);
        RegCloseKey(hKey);
        return (status == ERROR_SUCCESS);
    }
    return false;
}

bool SettingsManager::setAutoStartEnabled(bool enable) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_RUN_PATH, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        if (enable) {
            wchar_t exePath[MAX_PATH];
            GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            std::wstring cmd = L"\"" + std::wstring(exePath) + L"\"";
            RegSetValueExW(hKey, APP_RUN_NAME, 0, REG_SZ, 
                           reinterpret_cast<const BYTE*>(cmd.c_str()), 
                           static_cast<DWORD>((cmd.size() + 1) * sizeof(wchar_t)));
        } else {
            RegDeleteValueW(hKey, APP_RUN_NAME);
        }
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

} // namespace DisplayStabilizer
