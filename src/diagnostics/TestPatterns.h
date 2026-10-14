#pragma once

#include <windows.h>
#include <string>
#include <vector>

namespace DisplayStabilizer {

enum class PatternType {
    PureWhite = 0,
    PureBlack,
    NeutralGrayLevels,
    SolidRed,
    SolidGreen,
    SolidBlue,
    SolidCyan,
    SolidMagenta,
    SolidYellow,
    Checkerboard1px,
    FineHorizontalLines,
    FineVerticalLines,
    GrayGradient,
    ColorGradient,
    UniformMediumGray,
    Count
};

struct PatternInfo {
    PatternType type;
    std::wstring name;
    std::wstring description;
    std::wstring diagnosticPurpose;
};

class TestPatterns {
public:
    static const std::vector<PatternInfo>& getPatterns();
    static const PatternInfo& getPatternInfo(PatternType type);

    // Completely static rendering onto device context
    static void render(HDC hdc, int width, int height, PatternType type);
};

} // namespace DisplayStabilizer
