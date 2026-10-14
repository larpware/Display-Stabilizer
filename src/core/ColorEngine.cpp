#include "ColorEngine.h"
#include <cmath>
#include <algorithm>

namespace DisplayStabilizer {

ColorEngine::ColorEngine() {
    computeIdentityRamp(m_originalRamp);
    computeIdentityRamp(m_currentRamp);
}

ColorEngine::~ColorEngine() {
    if (m_isApplied) {
        restoreOriginal();
    }
}

HDC ColorEngine::getDisplayDC() {
    HDC hdc = CreateDCW(L"DISPLAY", nullptr, nullptr, nullptr);
    if (!hdc) {
        hdc = GetDC(nullptr);
    }
    return hdc;
}

void ColorEngine::releaseDisplayDC(HDC hdc) {
    if (hdc) {
        DeleteDC(hdc);
    }
}

bool ColorEngine::initialize(HWND /*hwnd*/) {
    HDC hdc = getDisplayDC();
    if (!hdc) {
        m_isSupported = false;
        return false;
    }

    // Try retrieving current hardware gamma ramp
    BOOL res = GetDeviceGammaRamp(hdc, m_originalRamp);
    if (res) {
        m_isSupported = true;
        m_hasOriginalBackup = true;
    } else {
        // Fall back to synthetic identity ramp if driver doesn't allow reading
        computeIdentityRamp(m_originalRamp);
        m_hasOriginalBackup = true;
        // Test if SetDeviceGammaRamp is supported
        BOOL testSet = SetDeviceGammaRamp(hdc, m_originalRamp);
        m_isSupported = (testSet != FALSE);
    }

    releaseDisplayDC(hdc);
    return m_isSupported;
}

void ColorEngine::computeIdentityRamp(WORD ramp[3][256]) {
    for (int i = 0; i < 256; ++i) {
        WORD val = static_cast<WORD>(std::round(static_cast<double>(i) * 65535.0 / 255.0));
        ramp[0][i] = val; // Red
        ramp[1][i] = val; // Green
        ramp[2][i] = val; // Blue
    }
}

static inline double clamp01(double v) {
    if (std::isnan(v)) return 0.0;
    return (v < 0.0) ? 0.0 : ((v > 1.0) ? 1.0 : v);
}

void ColorEngine::computeRamp(const Profile& profile, WORD ramp[3][256]) {
    if (profile.isNeutral() || !profile.enabled) {
        computeIdentityRamp(ramp);
        return;
    }

    for (int i = 0; i < 256; ++i) {
        double x = static_cast<double>(i) / 255.0;

        // 1. Contrast adjustment around midpoint (0.5)
        double xc = 0.5 + (x - 0.5) * profile.contrast;

        // 2. Channel Gain and Offset
        double r1 = xc * profile.redGain + profile.redOffset;
        double g1 = xc * profile.greenGain + profile.greenOffset;
        double b1 = xc * profile.blueGain + profile.blueOffset;

        // 3. Color temperature bias (-1.0 Cool to +1.0 Warm)
        if (profile.colorTempBias > 0.0) {
            // Warmer: add red, reduce blue
            r1 += 0.08 * profile.colorTempBias;
            b1 -= 0.08 * profile.colorTempBias;
        } else if (profile.colorTempBias < 0.0) {
            // Cooler: reduce red, add blue
            double bias = -profile.colorTempBias;
            r1 -= 0.08 * bias;
            b1 += 0.08 * bias;
        }

        // Clamp intermediate channels to [0.0, 1.0] before saturation
        r1 = clamp01(r1);
        g1 = clamp01(g1);
        b1 = clamp01(b1);

        // 4. Saturation adjustment
        double luma = 0.2126 * r1 + 0.7152 * g1 + 0.0722 * b1;
        double r2 = luma + (r1 - luma) * profile.saturation;
        double g2 = luma + (g1 - luma) * profile.saturation;
        double b2 = luma + (b1 - luma) * profile.saturation;

        r2 = clamp01(r2);
        g2 = clamp01(g2);
        b2 = clamp01(b2);

        // 5. Channel Gamma correction
        double rGamma = (profile.redGamma > 0.01) ? std::pow(r2, 1.0 / profile.redGamma) : r2;
        double gGamma = (profile.greenGamma > 0.01) ? std::pow(g2, 1.0 / profile.greenGamma) : g2;
        double bGamma = (profile.blueGamma > 0.01) ? std::pow(b2, 1.0 / profile.blueGamma) : b2;

        rGamma = clamp01(rGamma);
        gGamma = clamp01(gGamma);
        bGamma = clamp01(bGamma);

        // 6. Scale to 16-bit WORD [0, 65535]
        ramp[0][i] = static_cast<WORD>(std::round(rGamma * 65535.0));
        ramp[1][i] = static_cast<WORD>(std::round(gGamma * 65535.0));
        ramp[2][i] = static_cast<WORD>(std::round(bGamma * 65535.0));
    }
}

bool ColorEngine::applyProfile(const Profile& profile) {
    if (!profile.enabled) {
        return restoreOriginal();
    }

    HDC hdc = getDisplayDC();
    if (!hdc) return false;

    computeRamp(profile, m_currentRamp);

    BOOL res = SetDeviceGammaRamp(hdc, m_currentRamp);
    releaseDisplayDC(hdc);

    if (res) {
        m_isApplied = true;
        return true;
    }
    return false;
}

bool ColorEngine::restoreOriginal() {
    HDC hdc = getDisplayDC();
    if (!hdc) return false;

    BOOL res = FALSE;
    if (m_hasOriginalBackup) {
        res = SetDeviceGammaRamp(hdc, m_originalRamp);
    } else {
        WORD identity[3][256];
        computeIdentityRamp(identity);
        res = SetDeviceGammaRamp(hdc, identity);
    }

    releaseDisplayDC(hdc);
    m_isApplied = false;
    return (res != FALSE);
}

bool ColorEngine::disableAll() {
    return restoreOriginal();
}

} // namespace DisplayStabilizer
