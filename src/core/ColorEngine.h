#pragma once

#include <windows.h>
#include "Profile.h"

namespace DisplayStabilizer {

class ColorEngine {
public:
    ColorEngine();
    ~ColorEngine();

    // Initialize and query display gamma ramp support
    bool initialize(HWND hwnd = nullptr);

    // Apply or revert profile
    bool applyProfile(const Profile& profile);
    bool restoreOriginal();
    bool disableAll();

    // Compute gamma ramp table directly (pure function, testable)
    static void computeRamp(const Profile& profile, WORD ramp[3][256]);
    static void computeIdentityRamp(WORD ramp[3][256]);

    bool isSupported() const { return m_isSupported; }
    bool isApplied() const { return m_isApplied; }
    bool hasOriginalBackup() const { return m_hasOriginalBackup; }

private:
    HDC getDisplayDC();
    void releaseDisplayDC(HDC hdc);

    bool m_isSupported = false;
    bool m_isApplied = false;
    bool m_hasOriginalBackup = false;
    WORD m_originalRamp[3][256] = {};
    WORD m_currentRamp[3][256] = {};
};

} // namespace DisplayStabilizer
