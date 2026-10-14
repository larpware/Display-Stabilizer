#pragma once

#include <string>
#include "JsonHelper.h"

namespace DisplayStabilizer {

struct Profile {
    std::wstring id;
    std::wstring name;
    std::wstring description;
    bool isBuiltIn = false;
    bool enabled = false;

    // Color adjustments
    double redGain = 1.0;       // Range: 0.50 to 1.50, Neutral: 1.00
    double greenGain = 1.0;     // Range: 0.50 to 1.50, Neutral: 1.00
    double blueGain = 1.0;      // Range: 0.50 to 1.50, Neutral: 1.00

    double redOffset = 0.0;     // Range: -0.20 to +0.20, Neutral: 0.00
    double greenOffset = 0.0;   // Range: -0.20 to +0.20, Neutral: 0.00
    double blueOffset = 0.0;    // Range: -0.20 to +0.20, Neutral: 0.00

    double redGamma = 1.0;      // Range: 0.50 to 2.50, Neutral: 1.00
    double greenGamma = 1.0;    // Range: 0.50 to 2.50, Neutral: 1.00
    double blueGamma = 1.0;     // Range: 0.50 to 2.50, Neutral: 1.00

    double contrast = 1.0;      // Range: 0.50 to 1.50, Neutral: 1.00
    double saturation = 1.0;    // Range: 0.00 to 2.00, Neutral: 1.00
    double colorTempBias = 0.0; // Range: -1.00 to +1.00, Neutral: 0.00 (-1.0 Cool, +1.0 Warm)

    Profile() = default;

    void clamp();
    bool isNeutral() const;
    void resetToNeutral();

    JsonValue toJson() const;
    static Profile fromJson(const JsonValue& val);

    // Built-in presets
    static Profile createNeutral();
    static Profile createNaturalWhites();
    static Profile createGreenCastTest();
    static Profile createPinkCastTest();
    static Profile createLowContrast();
    static Profile createDiagnostic();
    static Profile createCustomTemplate(const std::wstring& customName);
};

} // namespace DisplayStabilizer
