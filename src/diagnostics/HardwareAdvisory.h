#pragma once

#include <string>
#include <vector>

namespace DisplayStabilizer {

struct DamageSymptom {
    std::wstring symptom;
    std::wstring physicalMechanism;
    std::wstring testMethod;
    bool isLikelyHardwareUnrepairable;
};

class HardwareAdvisory {
public:
    static std::vector<DamageSymptom> getCommonDamageSymptoms();
    static std::wstring getDropDamageAnalysis();
    static std::wstring getOsdTestInstruction();
    static std::wstring getSafetyWarning();
};

} // namespace DisplayStabilizer
