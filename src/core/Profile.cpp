#include "Profile.h"
#include <windows.h>
#include <algorithm>
#include <cmath>

namespace DisplayStabilizer {

static double clampVal(double val, double minVal, double maxVal) {
    if (std::isnan(val)) return minVal;
    return (val < minVal) ? minVal : ((val > maxVal) ? maxVal : val);
}

void Profile::clamp() {
    redGain = clampVal(redGain, 0.50, 1.50);
    greenGain = clampVal(greenGain, 0.50, 1.50);
    blueGain = clampVal(blueGain, 0.50, 1.50);

    redOffset = clampVal(redOffset, -0.20, 0.20);
    greenOffset = clampVal(greenOffset, -0.20, 0.20);
    blueOffset = clampVal(blueOffset, -0.20, 0.20);

    redGamma = clampVal(redGamma, 0.50, 2.50);
    greenGamma = clampVal(greenGamma, 0.50, 2.50);
    blueGamma = clampVal(blueGamma, 0.50, 2.50);

    contrast = clampVal(contrast, 0.50, 1.50);
    saturation = clampVal(saturation, 0.00, 2.00);
    colorTempBias = clampVal(colorTempBias, -1.00, 1.00);
}

bool Profile::isNeutral() const {
    const double eps = 0.001;
    return std::abs(redGain - 1.0) < eps &&
           std::abs(greenGain - 1.0) < eps &&
           std::abs(blueGain - 1.0) < eps &&
           std::abs(redOffset - 0.0) < eps &&
           std::abs(greenOffset - 0.0) < eps &&
           std::abs(blueOffset - 0.0) < eps &&
           std::abs(redGamma - 1.0) < eps &&
           std::abs(greenGamma - 1.0) < eps &&
           std::abs(blueGamma - 1.0) < eps &&
           std::abs(contrast - 1.0) < eps &&
           std::abs(saturation - 1.0) < eps &&
           std::abs(colorTempBias - 0.0) < eps;
}

void Profile::resetToNeutral() {
    redGain = 1.0;
    greenGain = 1.0;
    blueGain = 1.0;

    redOffset = 0.0;
    greenOffset = 0.0;
    blueOffset = 0.0;

    redGamma = 1.0;
    greenGamma = 1.0;
    blueGamma = 1.0;

    contrast = 1.0;
    saturation = 1.0;
    colorTempBias = 0.0;
}

JsonValue Profile::toJson() const {
    JsonValue jv(JsonType::Object);
    jv[L"id"] = JsonValue(id);
    jv[L"name"] = JsonValue(name);
    jv[L"description"] = JsonValue(description);
    jv[L"isBuiltIn"] = JsonValue(isBuiltIn);
    jv[L"enabled"] = JsonValue(enabled);

    jv[L"redGain"] = JsonValue(redGain);
    jv[L"greenGain"] = JsonValue(greenGain);
    jv[L"blueGain"] = JsonValue(blueGain);

    jv[L"redOffset"] = JsonValue(redOffset);
    jv[L"greenOffset"] = JsonValue(greenOffset);
    jv[L"blueOffset"] = JsonValue(blueOffset);

    jv[L"redGamma"] = JsonValue(redGamma);
    jv[L"greenGamma"] = JsonValue(greenGamma);
    jv[L"blueGamma"] = JsonValue(blueGamma);

    jv[L"contrast"] = JsonValue(contrast);
    jv[L"saturation"] = JsonValue(saturation);
    jv[L"colorTempBias"] = JsonValue(colorTempBias);

    return jv;
}

Profile Profile::fromJson(const JsonValue& jv) {
    Profile p;
    if (!jv.isObject()) return p;

    p.id = jv[L"id"].asString(L"");
    p.name = jv[L"name"].asString(L"Custom Profile");
    p.description = jv[L"description"].asString(L"");
    p.isBuiltIn = jv[L"isBuiltIn"].asBool(false);
    p.enabled = jv[L"enabled"].asBool(false);

    p.redGain = jv[L"redGain"].asDouble(1.0);
    p.greenGain = jv[L"greenGain"].asDouble(1.0);
    p.blueGain = jv[L"blueGain"].asDouble(1.0);

    p.redOffset = jv[L"redOffset"].asDouble(0.0);
    p.greenOffset = jv[L"greenOffset"].asDouble(0.0);
    p.blueOffset = jv[L"blueOffset"].asDouble(0.0);

    p.redGamma = jv[L"redGamma"].asDouble(1.0);
    p.greenGamma = jv[L"greenGamma"].asDouble(1.0);
    p.blueGamma = jv[L"blueGamma"].asDouble(1.0);

    p.contrast = jv[L"contrast"].asDouble(1.0);
    p.saturation = jv[L"saturation"].asDouble(1.0);
    p.colorTempBias = jv[L"colorTempBias"].asDouble(0.0);

    p.clamp();
    return p;
}

Profile Profile::createNeutral() {
    Profile p;
    p.id = L"preset_neutral";
    p.name = L"Preset A — Neutral (Default Baseline)";
    p.description = L"Pure identity baseline with zero adjustments. Corrections disabled. Standard reference.";
    p.isBuiltIn = true;
    p.enabled = false;
    p.resetToNeutral();
    return p;
}

Profile Profile::createNaturalWhites() {
    Profile p;
    p.id = L"preset_natural_whites";
    p.name = L"Preset B — Natural Whites";
    p.description = L"Neutral starting point for gentle white-balance fine tuning. Minimal subtle adjustments.";
    p.isBuiltIn = true;
    p.enabled = true;
    p.resetToNeutral();
    // Neutral starting point ready for manual fine tuning without heavy filtering
    return p;
}

Profile Profile::createGreenCastTest() {
    Profile p;
    p.id = L"preset_green_cast_test";
    p.name = L"Preset C — Green Cast Test";
    p.description = L"Tests if reducing green channel gain (-6%) balances excess green analog bias or tint.";
    p.isBuiltIn = true;
    p.enabled = true;
    p.resetToNeutral();
    p.greenGain = 0.94; // Subtle 6% trim on green channel
    return p;
}

Profile Profile::createPinkCastTest() {
    Profile p;
    p.id = L"preset_pink_cast_test";
    p.name = L"Preset D — Pink Cast Test";
    p.description = L"Tests if subtle red/blue trim (-4%) mitigates pink/magenta white tint on analog VGA.";
    p.isBuiltIn = true;
    p.enabled = true;
    p.resetToNeutral();
    p.redGain = 0.96;
    p.blueGain = 0.96;
    return p;
}

Profile Profile::createLowContrast() {
    Profile p;
    p.id = L"preset_low_contrast";
    p.name = L"Preset E — Low Contrast";
    p.description = L"Reduces contrast (-12%) and slightly elevates black level to soften harsh edge artifacts.";
    p.isBuiltIn = true;
    p.enabled = true;
    p.resetToNeutral();
    p.contrast = 0.88;
    p.redGain = 0.95;
    p.greenGain = 0.95;
    p.blueGain = 0.95;
    p.redOffset = 0.02;
    p.greenOffset = 0.02;
    p.blueOffset = 0.02;
    return p;
}

Profile Profile::createDiagnostic() {
    Profile p;
    p.id = L"preset_diagnostic";
    p.name = L"Preset F — Diagnostic";
    p.description = L"All color corrections disabled. Used for viewing static test patterns and analyzing hardware.";
    p.isBuiltIn = true;
    p.enabled = false;
    p.resetToNeutral();
    return p;
}

Profile Profile::createCustomTemplate(const std::wstring& customName) {
    Profile p;
    p.id = L"custom_" + std::to_wstring(GetTickCount64());
    p.name = customName.empty() ? L"Preset G — Custom" : customName;
    p.description = L"User-configured profile. Fully editable gains, offsets, gammas, contrast, and balance.";
    p.isBuiltIn = false;
    p.enabled = true;
    p.resetToNeutral();
    return p;
}

} // namespace DisplayStabilizer
