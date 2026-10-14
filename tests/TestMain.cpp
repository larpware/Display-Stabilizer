#include <iostream>
#include <cassert>
#include <cmath>
#include <windows.h>
#include "../src/core/JsonHelper.h"
#include "../src/core/Profile.h"
#include "../src/core/ProfileManager.h"
#include "../src/core/ColorEngine.h"
#include "../src/core/DisplayManager.h"

using namespace DisplayStabilizer;

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::wcerr << L"[FAIL] " << msg << L" (" << __FILE__ << L":" << __LINE__ << L")" << std::endl; \
            g_testsFailed++; \
            return; \
        } \
    } while (0)

#define RUN_TEST(fn) \
    do { \
        std::wcout << L"Running " << L#fn << L"... "; \
        int prevFail = g_testsFailed; \
        fn(); \
        if (g_testsFailed == prevFail) { \
            std::wcout << L"PASSED" << std::endl; \
            g_testsPassed++; \
        } \
    } while (0)

void testJsonHelperBasics() {
    JsonValue root(JsonType::Object);
    root[L"name"] = JsonValue(L"TestProfile");
    root[L"gain"] = JsonValue(1.25);
    root[L"enabled"] = JsonValue(true);

    JsonValue arr(JsonType::Array);
    arr.arrayValue.push_back(JsonValue(10));
    arr.arrayValue.push_back(JsonValue(20));
    root[L"items"] = arr;

    std::wstring serialized = root.serialize(false);
    TEST_ASSERT(!serialized.empty(), L"Serialized string is not empty");

    JsonValue parsed;
    std::wstring err;
    bool ok = JsonValue::parse(serialized, parsed, err);
    TEST_ASSERT(ok, L"Json parse succeeded");
    TEST_ASSERT(parsed[L"name"].asString() == L"TestProfile", L"Name matches");
    TEST_ASSERT(std::abs(parsed[L"gain"].asDouble() - 1.25) < 0.0001, L"Gain matches");
    TEST_ASSERT(parsed[L"enabled"].asBool() == true, L"Enabled matches");
    TEST_ASSERT(parsed[L"items"].arrayValue.size() == 2, L"Array size matches");
    TEST_ASSERT(parsed[L"items"][1].asInt() == 20, L"Array element matches");
}

void testJsonHelperMalformed() {
    JsonValue parsed;
    std::wstring err;
    bool ok = JsonValue::parse(L"{\"unclosed\": 123", parsed, err);
    TEST_ASSERT(!ok, L"Malformed json returns false");
    TEST_ASSERT(!err.empty(), L"Error message populated");
}

void testProfileClamping() {
    Profile p;
    p.redGain = 5.0; // Out of range [0.5, 1.5]
    p.greenGain = -2.0;
    p.redOffset = 0.99; // Out of range [-0.2, +0.2]
    p.redGamma = 10.0; // Out of range [0.5, 2.5]
    p.contrast = -5.0; // Out of range [0.5, 1.5]

    p.clamp();

    TEST_ASSERT(p.redGain == 1.50, L"Red gain clamped to 1.50");
    TEST_ASSERT(p.greenGain == 0.50, L"Green gain clamped to 0.50");
    TEST_ASSERT(p.redOffset == 0.20, L"Red offset clamped to 0.20");
    TEST_ASSERT(p.redGamma == 2.50, L"Red gamma clamped to 2.50");
    TEST_ASSERT(p.contrast == 0.50, L"Contrast clamped to 0.50");
}

void testNeutralProfileIdentity() {
    Profile neutral = Profile::createNeutral();
    TEST_ASSERT(neutral.isNeutral(), L"Profile::createNeutral() reports isNeutral() == true");

    WORD ramp[3][256] = {};
    ColorEngine::computeRamp(neutral, ramp);

    for (int i = 0; i < 256; ++i) {
        WORD expected = static_cast<WORD>(std::round(static_cast<double>(i) * 65535.0 / 255.0));
        TEST_ASSERT(ramp[0][i] == expected, L"Red channel ramp is pure identity");
        TEST_ASSERT(ramp[1][i] == expected, L"Green channel ramp is pure identity");
        TEST_ASSERT(ramp[2][i] == expected, L"Blue channel ramp is pure identity");
    }
}

void testGreenCastRampCalculation() {
    Profile p = Profile::createGreenCastTest();
    TEST_ASSERT(!p.isNeutral(), L"Green cast profile is not neutral");
    TEST_ASSERT(p.greenGain == 0.94, L"Green gain is 0.94");

    WORD ramp[3][256] = {};
    ColorEngine::computeRamp(p, ramp);

    // At midtone i = 128, green channel output should be strictly lower than red and blue
    int mid = 128;
    TEST_ASSERT(ramp[1][mid] < ramp[0][mid], L"Green midtone is attenuated relative to Red");
    TEST_ASSERT(ramp[1][mid] < ramp[2][mid], L"Green midtone is attenuated relative to Blue");
    TEST_ASSERT(ramp[0][mid] == ramp[2][mid], L"Red and Blue channels remain balanced");
}

void testPresetManagementAndDuplication() {
    ProfileManager pm;
    const auto& presets = pm.getAllProfiles();
    TEST_ASSERT(presets.size() >= 6, L"At least 6 initial presets loaded");

    // Check Preset A to F exist
    TEST_ASSERT(pm.getProfileById(L"preset_neutral") != nullptr, L"Preset Neutral exists");
    TEST_ASSERT(pm.getProfileById(L"preset_natural_whites") != nullptr, L"Preset Natural Whites exists");
    TEST_ASSERT(pm.getProfileById(L"preset_green_cast_test") != nullptr, L"Preset Green Cast exists");
    TEST_ASSERT(pm.getProfileById(L"preset_pink_cast_test") != nullptr, L"Preset Pink Cast exists");
    TEST_ASSERT(pm.getProfileById(L"preset_low_contrast") != nullptr, L"Preset Low Contrast exists");
    TEST_ASSERT(pm.getProfileById(L"preset_diagnostic") != nullptr, L"Preset Diagnostic exists");

    // Attempting to delete built-in preset must fail
    bool delBuiltIn = pm.deleteProfile(L"preset_neutral");
    TEST_ASSERT(!delBuiltIn, L"Deleting built-in preset is rejected");

    // Duplicating preset creates a custom preset
    std::wstring newId;
    bool dupOk = pm.duplicateProfile(L"preset_green_cast_test", L"My Custom Green Trim", newId);
    TEST_ASSERT(dupOk, L"Duplicate profile succeeds");
    TEST_ASSERT(!newId.empty(), L"New ID generated");

    const Profile* custom = pm.getProfileById(newId);
    TEST_ASSERT(custom != nullptr, L"Custom profile found by ID");
    TEST_ASSERT(!custom->isBuiltIn, L"Custom profile isBuiltIn is false");
    TEST_ASSERT(custom->name == L"My Custom Green Trim", L"Custom profile name matches");
    TEST_ASSERT(custom->greenGain == 0.94, L"Custom profile inherited green gain");

    // Deleting custom preset must succeed
    bool delCustom = pm.deleteProfile(newId);
    TEST_ASSERT(delCustom, L"Deleting custom preset succeeds");
    TEST_ASSERT(pm.getProfileById(newId) == nullptr, L"Deleted custom preset no longer in manager");
}

void testProfileImportExport() {
    ProfileManager pm;
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    std::wstring filePath = std::wstring(tempPath) + L"test_profile_export.json";

    std::wstring err;
    bool expOk = pm.exportProfileToJsonFile(L"preset_pink_cast_test", filePath, err);
    TEST_ASSERT(expOk, L"Exporting profile to temp JSON succeeds");

    std::wstring importedId;
    bool impOk = pm.importProfileFromJsonFile(filePath, importedId, err);
    TEST_ASSERT(impOk, L"Importing profile succeeds");

    const Profile* impProfile = pm.getProfileById(importedId);
    TEST_ASSERT(impProfile != nullptr, L"Imported profile exists in manager");
    TEST_ASSERT(!impProfile->isBuiltIn, L"Imported profile marked as custom");
    TEST_ASSERT(impProfile->redGain == 0.96, L"Imported red gain matches exported");
    TEST_ASSERT(impProfile->blueGain == 0.96, L"Imported blue gain matches exported");

    DeleteFileW(filePath.c_str());
}

void testDisplayModeProperties() {
    DisplayMode m1;
    m1.width = 1366;
    m1.height = 768;
    m1.refreshRate = 60;
    m1.bitsPerPixel = 32;

    DisplayMode m2 = m1;
    TEST_ASSERT(m1 == m2, L"DisplayMode equality matches");

    std::wstring str = m1.toString();
    TEST_ASSERT(str.find(L"1366 x 768") != std::wstring::npos, L"Mode string contains 1366 x 768");
    TEST_ASSERT(str.find(L"60 Hz") != std::wstring::npos, L"Mode string contains 60 Hz");
}

int main() {
    std::wcout << L"========================================" << std::endl;
    std::wcout << L"Display Stabilizer Automated Test Suite" << std::endl;
    std::wcout << L"========================================" << std::endl;

    RUN_TEST(testJsonHelperBasics);
    RUN_TEST(testJsonHelperMalformed);
    RUN_TEST(testProfileClamping);
    RUN_TEST(testNeutralProfileIdentity);
    RUN_TEST(testGreenCastRampCalculation);
    RUN_TEST(testPresetManagementAndDuplication);
    RUN_TEST(testProfileImportExport);
    RUN_TEST(testDisplayModeProperties);

    std::wcout << L"========================================" << std::endl;
    std::wcout << L"Test Results: " << g_testsPassed << L" passed, " << g_testsFailed << L" failed." << std::endl;
    std::wcout << L"========================================" << std::endl;

    return (g_testsFailed == 0) ? 0 : 1;
}
