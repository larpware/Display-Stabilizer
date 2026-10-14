#pragma once

#include <windows.h>
#include <string>
#include <vector>

namespace DisplayStabilizer {

struct DisplayMode {
    int width = 0;
    int height = 0;
    int refreshRate = 0;
    int bitsPerPixel = 0;

    std::wstring toString() const;
    bool operator==(const DisplayMode& other) const {
        return width == other.width && height == other.height && 
               refreshRate == other.refreshRate && bitsPerPixel == other.bitsPerPixel;
    }
};

struct DisplayInfo {
    std::wstring deviceName;     // e.g. \\.\DISPLAY1
    std::wstring adapterString;   // e.g. Intel(R) HD Graphics
    std::wstring monitorString;   // e.g. Acer V193HQL
    bool isVgaOrAnalog = false;
    DisplayMode currentMode;
    int dpi = 96;
    int scalingPercent = 100;
    bool ddcCiSupported = false;
    DWORD monitorBrightness = 0;
    DWORD monitorContrast = 0;
};

class DisplayManager {
public:
    DisplayManager();

    bool refresh();

    const DisplayInfo& getPrimaryDisplayInfo() const { return m_info; }
    const std::vector<DisplayMode>& getSupportedModes() const { return m_supportedModes; }

    // Test a display mode with safety checks
    bool testMode(const DisplayMode& mode, std::wstring& outErr);
    bool applyModePermanent(const DisplayMode& mode, std::wstring& outErr);
    bool revertMode(std::wstring& outErr);

    const DisplayMode& getOriginalMode() const { return m_originalMode; }
    bool isModeChanged() const { return m_isModeChanged; }

private:
    void queryDdcCi();

    DisplayInfo m_info;
    std::vector<DisplayMode> m_supportedModes;
    DisplayMode m_originalMode;
    bool m_hasOriginalMode = false;
    bool m_isModeChanged = false;
};

} // namespace DisplayStabilizer
