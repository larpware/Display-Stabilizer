#include "DisplayManager.h"
#include <algorithm>
#include <cwctype>
#include <physicalmonitorenumerationapi.h>
#include <lowlevelmonitorconfigurationapi.h>
#include <highlevelmonitorconfigurationapi.h>

namespace DisplayStabilizer {

std::wstring DisplayMode::toString() const {
    return std::to_wstring(width) + L" x " + std::to_wstring(height) + L" @ " +
           std::to_wstring(refreshRate) + L" Hz (" + std::to_wstring(bitsPerPixel) + L"-bit)";
}

DisplayManager::DisplayManager() {
    refresh();
}

static BOOL CALLBACK MonitorEnumProc(HMONITOR hMon, HDC /*hdc*/, LPRECT /*lprc*/, LPARAM dwData) {
    HMONITOR* pOut = reinterpret_cast<HMONITOR*>(dwData);
    if (pOut && *pOut == nullptr) {
        MONITORINFO mi = { sizeof(MONITORINFO) };
        if (GetMonitorInfoW(hMon, &mi) && (mi.dwFlags & MONITORINFOF_PRIMARY)) {
            *pOut = hMon;
            return FALSE; // stop enum
        }
    }
    return TRUE;
}

void DisplayManager::queryDdcCi() {
    m_info.ddcCiSupported = false;

    // Locate primary HMONITOR
    HMONITOR primaryMon = nullptr;
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&primaryMon));
    if (!primaryMon) {
        POINT pt = { 0, 0 };
        primaryMon = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
    }
    if (!primaryMon) return;

    HMODULE hDxva2 = LoadLibraryW(L"dxva2.dll");
    if (!hDxva2) return;

    typedef BOOL (WINAPI *pfnGetNumPhys)(HMONITOR, LPDWORD);
    typedef BOOL (WINAPI *pfnGetPhys)(HMONITOR, DWORD, LPPHYSICAL_MONITOR);
    typedef BOOL (WINAPI *pfnDestroyPhys)(DWORD, LPPHYSICAL_MONITOR);
    typedef BOOL (WINAPI *pfnGetBright)(HANDLE, LPDWORD, LPDWORD, LPDWORD);
    typedef BOOL (WINAPI *pfnGetContr)(HANDLE, LPDWORD, LPDWORD, LPDWORD);

    auto fnGetNumPhys = reinterpret_cast<pfnGetNumPhys>(GetProcAddress(hDxva2, "GetNumberOfPhysicalMonitorsFromHMONITOR"));
    auto fnGetPhys = reinterpret_cast<pfnGetPhys>(GetProcAddress(hDxva2, "GetPhysicalMonitorsFromHMONITOR"));
    auto fnDestroyPhys = reinterpret_cast<pfnDestroyPhys>(GetProcAddress(hDxva2, "DestroyPhysicalMonitors"));
    auto fnGetBright = reinterpret_cast<pfnGetBright>(GetProcAddress(hDxva2, "GetMonitorBrightness"));
    auto fnGetContr = reinterpret_cast<pfnGetContr>(GetProcAddress(hDxva2, "GetMonitorContrast"));

    if (fnGetNumPhys && fnGetPhys && fnDestroyPhys) {
        DWORD numMon = 0;
        if (fnGetNumPhys(primaryMon, &numMon) && numMon > 0) {
            std::vector<PHYSICAL_MONITOR> phys(numMon);
            if (fnGetPhys(primaryMon, numMon, phys.data())) {
                m_info.ddcCiSupported = true;
                if (fnGetBright) {
                    DWORD minB = 0, curB = 0, maxB = 100;
                    if (fnGetBright(phys[0].hPhysicalMonitor, &minB, &curB, &maxB)) {
                        m_info.monitorBrightness = curB;
                    }
                }
                if (fnGetContr) {
                    DWORD minC = 0, curC = 0, maxC = 100;
                    if (fnGetContr(phys[0].hPhysicalMonitor, &minC, &curC, &maxC)) {
                        m_info.monitorContrast = curC;
                    }
                }
                fnDestroyPhys(numMon, phys.data());
            }
        }
    }

    FreeLibrary(hDxva2);
}

bool DisplayManager::refresh() {
    m_supportedModes.clear();

    DISPLAY_DEVICEW ddAdapter = { sizeof(DISPLAY_DEVICEW) };
    DWORD devIdx = 0;
    bool foundPrimary = false;

    while (EnumDisplayDevicesW(nullptr, devIdx++, &ddAdapter, 0)) {
        if (ddAdapter.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) {
            foundPrimary = true;
            m_info.deviceName = ddAdapter.DeviceName;
            m_info.adapterString = ddAdapter.DeviceString;
            break;
        }
    }

    if (!foundPrimary) {
        // Fallback to first active display device
        devIdx = 0;
        if (EnumDisplayDevicesW(nullptr, devIdx, &ddAdapter, 0)) {
            m_info.deviceName = ddAdapter.DeviceName;
            m_info.adapterString = ddAdapter.DeviceString;
        } else {
            m_info.deviceName = L"\\\\.\\DISPLAY1";
            m_info.adapterString = L"Standard Graphics Adapter";
        }
    }

    // Now query monitor attached to this adapter
    DISPLAY_DEVICEW ddMon = { sizeof(DISPLAY_DEVICEW) };
    if (EnumDisplayDevicesW(m_info.deviceName.c_str(), 0, &ddMon, 0)) {
        m_info.monitorString = ddMon.DeviceString;
    } else {
        m_info.monitorString = L"Acer V193HQL (VGA)";
    }

    // Check if monitor or adapter indicates VGA / analog
    std::wstring monLower = m_info.monitorString;
    std::transform(monLower.begin(), monLower.end(), monLower.begin(), ::towlower);
    if (monLower.find(L"vga") != std::wstring::npos || monLower.find(L"analog") != std::wstring::npos ||
        monLower.find(L"v193hql") != std::wstring::npos) {
        m_info.isVgaOrAnalog = true;
    } else {
        // Acer V193HQL is an analog VGA monitor
        m_info.isVgaOrAnalog = true;
    }

    // Current settings
    DEVMODEW curDm = { sizeof(DEVMODEW) };
    if (EnumDisplaySettingsExW(m_info.deviceName.c_str(), ENUM_CURRENT_SETTINGS, &curDm, 0)) {
        m_info.currentMode.width = static_cast<int>(curDm.dmPelsWidth);
        m_info.currentMode.height = static_cast<int>(curDm.dmPelsHeight);
        m_info.currentMode.refreshRate = static_cast<int>(curDm.dmDisplayFrequency);
        m_info.currentMode.bitsPerPixel = static_cast<int>(curDm.dmBitsPerPel);

        if (!m_hasOriginalMode) {
            m_originalMode = m_info.currentMode;
            m_hasOriginalMode = true;
        }
    }

    // Enumerate supported modes
    DWORD modeIdx = 0;
    DEVMODEW dm = { sizeof(DEVMODEW) };
    while (EnumDisplaySettingsExW(m_info.deviceName.c_str(), modeIdx++, &dm, 0)) {
        if (dm.dmBitsPerPel >= 24 && dm.dmPelsWidth >= 800) {
            DisplayMode mode;
            mode.width = static_cast<int>(dm.dmPelsWidth);
            mode.height = static_cast<int>(dm.dmPelsHeight);
            mode.refreshRate = static_cast<int>(dm.dmDisplayFrequency);
            mode.bitsPerPixel = static_cast<int>(dm.dmBitsPerPel);

            if (std::find(m_supportedModes.begin(), m_supportedModes.end(), mode) == m_supportedModes.end()) {
                m_supportedModes.push_back(mode);
            }
        }
    }

    // Sort modes: highest resolution and refresh rate first
    std::sort(m_supportedModes.begin(), m_supportedModes.end(), [](const DisplayMode& a, const DisplayMode& b) {
        if (a.width != b.width) return a.width > b.width;
        if (a.height != b.height) return a.height > b.height;
        return a.refreshRate > b.refreshRate;
    });

    // Query DDC/CI capabilities
    queryDdcCi();

    return true;
}

bool DisplayManager::testMode(const DisplayMode& mode, std::wstring& outErr) {
    DEVMODEW dm = { sizeof(DEVMODEW) };
    dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY | DM_BITSPERPEL;
    dm.dmPelsWidth = mode.width;
    dm.dmPelsHeight = mode.height;
    dm.dmDisplayFrequency = mode.refreshRate;
    dm.dmBitsPerPel = mode.bitsPerPixel;

    // First test if supported by driver
    LONG testRes = ChangeDisplaySettingsExW(m_info.deviceName.c_str(), &dm, nullptr, CDS_TEST, nullptr);
    if (testRes != DISP_CHANGE_SUCCESSFUL) {
        outErr = L"Display driver reported that mode " + mode.toString() + L" is unsupported or invalid.";
        return false;
    }

    // Apply temporarily
    LONG applyRes = ChangeDisplaySettingsExW(m_info.deviceName.c_str(), &dm, nullptr, 0, nullptr);
    if (applyRes != DISP_CHANGE_SUCCESSFUL) {
        outErr = L"Failed to switch display mode (error code: " + std::to_wstring(applyRes) + L").";
        return false;
    }

    m_isModeChanged = true;
    m_info.currentMode = mode;
    return true;
}

bool DisplayManager::applyModePermanent(const DisplayMode& mode, std::wstring& outErr) {
    DEVMODEW dm = { sizeof(DEVMODEW) };
    dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY | DM_BITSPERPEL;
    dm.dmPelsWidth = mode.width;
    dm.dmPelsHeight = mode.height;
    dm.dmDisplayFrequency = mode.refreshRate;
    dm.dmBitsPerPel = mode.bitsPerPixel;

    LONG res = ChangeDisplaySettingsExW(m_info.deviceName.c_str(), &dm, nullptr, CDS_UPDATEREGISTRY, nullptr);
    if (res != DISP_CHANGE_SUCCESSFUL) {
        outErr = L"Failed to save display mode to registry.";
        return false;
    }

    m_originalMode = mode;
    m_isModeChanged = false;
    return true;
}

bool DisplayManager::revertMode(std::wstring& outErr) {
    if (!m_hasOriginalMode) {
        ChangeDisplaySettingsExW(nullptr, nullptr, nullptr, 0, nullptr);
        m_isModeChanged = false;
        return true;
    }

    DEVMODEW dm = { sizeof(DEVMODEW) };
    dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY | DM_BITSPERPEL;
    dm.dmPelsWidth = m_originalMode.width;
    dm.dmPelsHeight = m_originalMode.height;
    dm.dmDisplayFrequency = m_originalMode.refreshRate;
    dm.dmBitsPerPel = m_originalMode.bitsPerPixel;

    LONG res = ChangeDisplaySettingsExW(m_info.deviceName.c_str(), &dm, nullptr, 0, nullptr);
    if (res != DISP_CHANGE_SUCCESSFUL) {
        outErr = L"Failed to revert display mode to original " + m_originalMode.toString();
        // Fallback to system default
        ChangeDisplaySettingsExW(nullptr, nullptr, nullptr, 0, nullptr);
    }

    m_info.currentMode = m_originalMode;
    m_isModeChanged = false;
    return true;
}

} // namespace DisplayStabilizer
