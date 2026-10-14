#include "TestPatterns.h"
#include <cmath>

namespace DisplayStabilizer {

static const std::vector<PatternInfo> s_patterns = {
    {
        PatternType::PureWhite,
        L"1. Pure White (#FFFFFF)",
        L"Solid 100% white screen.",
        L"Tests for pink/magenta color cast on highlights, uneven backlight, and edge vignetting."
    },
    {
        PatternType::PureBlack,
        L"2. Pure Black (#000000)",
        L"Solid 0% black screen.",
        L"Tests for stuck bright subpixels, backlight bleed, and green grain in shadow/black areas."
    },
    {
        PatternType::NeutralGrayLevels,
        L"3. Neutral Gray (25%, 50%, 75%)",
        L"Stepped bars of 25%, 50%, and 75% neutral gray.",
        L"Identifies whether green grain or pink tint occurs across all tones or specifically in midtones."
    },
    {
        PatternType::SolidRed,
        L"4. Solid Red (#FF0000)",
        L"Solid pure red primary.",
        L"Evaluates red channel health, pin 1 VGA signal integrity, and subpixel dropouts."
    },
    {
        PatternType::SolidGreen,
        L"5. Solid Green (#00FF00)",
        L"Solid pure green primary.",
        L"Evaluates green channel health, pin 2 VGA line integrity, and green speckle baseline."
    },
    {
        PatternType::SolidBlue,
        L"6. Solid Blue (#0000FF)",
        L"Solid pure blue primary.",
        L"Evaluates blue channel health, pin 3 VGA line integrity, and blue subpixel responsiveness."
    },
    {
        PatternType::SolidCyan,
        L"7. Solid Cyan (#00FFFF)",
        L"Solid cyan (Green + Blue).",
        L"Tests color balance when red is absent; reveals red channel leakage or green dominance."
    },
    {
        PatternType::SolidMagenta,
        L"8. Solid Magenta (#FF00FF)",
        L"Solid magenta (Red + Blue).",
        L"Tests color balance when green is absent; reveals if pink whites share the same hue."
    },
    {
        PatternType::SolidYellow,
        L"9. Solid Yellow (#FFFF00)",
        L"Solid yellow (Red + Green).",
        L"Tests color balance when blue is absent; highlights green vs red channel gain imbalances."
    },
    {
        PatternType::Checkerboard1px,
        L"10. B&W Checkerboard (1x1 px)",
        L"Single-pixel alternating black and white checkerboard grid.",
        L"High-frequency ADC test: reveals clock/phase sync jitter, shimmering pixels, and blur."
    },
    {
        PatternType::FineHorizontalLines,
        L"11. Fine Horizontal Lines (1px)",
        L"Alternating 1-pixel black and white horizontal lines.",
        L"Tests vertical sync, vertical line bleeding, and scanline stability."
    },
    {
        PatternType::FineVerticalLines,
        L"12. Fine Vertical Lines (1px)",
        L"Alternating 1-pixel black and white vertical lines.",
        L"Critical VGA test: reveals ADC Phase jitter, pixel clock aliasing, and analog ghosting."
    },
    {
        PatternType::GrayGradient,
        L"13. Gray Gradient (0 to 255)",
        L"Smooth horizontal ramp from black to white.",
        L"Detects color banding, DAC quantization errors, and tonal threshold where green speckles begin."
    },
    {
        PatternType::ColorGradient,
        L"14. Full Color Gradient (Spectrum)",
        L"Full spectrum horizontal gradient across all hues.",
        L"Evaluates non-linear RGB channel tracking and transition boundaries."
    },
    {
        PatternType::UniformMediumGray,
        L"15. Uniform Medium Gray (#808080)",
        L"Solid 50% neutral gray field.",
        L"The primary test for analog green grain: reveals subtle RGB speckling across a neutral canvas."
    }
};

const std::vector<PatternInfo>& TestPatterns::getPatterns() {
    return s_patterns;
}

const PatternInfo& TestPatterns::getPatternInfo(PatternType type) {
    size_t idx = static_cast<size_t>(type);
    if (idx < s_patterns.size()) {
        return s_patterns[idx];
    }
    return s_patterns[0];
}

void TestPatterns::render(HDC hdc, int width, int height, PatternType type) {
    if (width <= 0 || height <= 0) return;

    RECT rc = { 0, 0, width, height };

    switch (type) {
    case PatternType::PureWhite: {
        HBRUSH hBr = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &rc, hBr);
        DeleteObject(hBr);
        break;
    }
    case PatternType::PureBlack: {
        HBRUSH hBr = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdc, &rc, hBr);
        DeleteObject(hBr);
        break;
    }
    case PatternType::NeutralGrayLevels: {
        int w3 = width / 3;
        RECT r1 = { 0, 0, w3, height };
        RECT r2 = { w3, 0, w3 * 2, height };
        RECT r3 = { w3 * 2, 0, width, height };

        HBRUSH b1 = CreateSolidBrush(RGB(64, 64, 64));     // 25%
        HBRUSH b2 = CreateSolidBrush(RGB(128, 128, 128));   // 50%
        HBRUSH b3 = CreateSolidBrush(RGB(192, 192, 192));   // 75%

        FillRect(hdc, &r1, b1);
        FillRect(hdc, &r2, b2);
        FillRect(hdc, &r3, b3);

        DeleteObject(b1);
        DeleteObject(b2);
        DeleteObject(b3);
        break;
    }
    case PatternType::SolidRed: {
        HBRUSH hBr = CreateSolidBrush(RGB(255, 0, 0));
        FillRect(hdc, &rc, hBr);
        DeleteObject(hBr);
        break;
    }
    case PatternType::SolidGreen: {
        HBRUSH hBr = CreateSolidBrush(RGB(0, 255, 0));
        FillRect(hdc, &rc, hBr);
        DeleteObject(hBr);
        break;
    }
    case PatternType::SolidBlue: {
        HBRUSH hBr = CreateSolidBrush(RGB(0, 0, 255));
        FillRect(hdc, &rc, hBr);
        DeleteObject(hBr);
        break;
    }
    case PatternType::SolidCyan: {
        HBRUSH hBr = CreateSolidBrush(RGB(0, 255, 255));
        FillRect(hdc, &rc, hBr);
        DeleteObject(hBr);
        break;
    }
    case PatternType::SolidMagenta: {
        HBRUSH hBr = CreateSolidBrush(RGB(255, 0, 255));
        FillRect(hdc, &rc, hBr);
        DeleteObject(hBr);
        break;
    }
    case PatternType::SolidYellow: {
        HBRUSH hBr = CreateSolidBrush(RGB(255, 255, 0));
        FillRect(hdc, &rc, hBr);
        DeleteObject(hBr);
        break;
    }
    case PatternType::Checkerboard1px: {
        // Create 2x2 monochrome pattern bitmap for fast tiling
        WORD pattern[4] = { 0x5555, 0xAAAA, 0x5555, 0xAAAA };
        HBITMAP hBm = CreateBitmap(16, 4, 1, 1, pattern);
        if (hBm) {
            HBRUSH hBr = CreatePatternBrush(hBm);
            if (hBr) {
                SetTextColor(hdc, RGB(255, 255, 255));
                SetBkColor(hdc, RGB(0, 0, 0));
                FillRect(hdc, &rc, hBr);
                DeleteObject(hBr);
            }
            DeleteObject(hBm);
        }
        break;
    }
    case PatternType::FineHorizontalLines: {
        // Alternating lines
        WORD pattern[4] = { 0xFFFF, 0x0000, 0xFFFF, 0x0000 };
        HBITMAP hBm = CreateBitmap(16, 4, 1, 1, pattern);
        if (hBm) {
            HBRUSH hBr = CreatePatternBrush(hBm);
            if (hBr) {
                SetTextColor(hdc, RGB(255, 255, 255));
                SetBkColor(hdc, RGB(0, 0, 0));
                FillRect(hdc, &rc, hBr);
                DeleteObject(hBr);
            }
            DeleteObject(hBm);
        }
        break;
    }
    case PatternType::FineVerticalLines: {
        // Alternating 1px vertical stripes
        WORD pattern[4] = { 0x5555, 0x5555, 0x5555, 0x5555 };
        HBITMAP hBm = CreateBitmap(16, 4, 1, 1, pattern);
        if (hBm) {
            HBRUSH hBr = CreatePatternBrush(hBm);
            if (hBr) {
                SetTextColor(hdc, RGB(255, 255, 255));
                SetBkColor(hdc, RGB(0, 0, 0));
                FillRect(hdc, &rc, hBr);
                DeleteObject(hBr);
            }
            DeleteObject(hBm);
        }
        break;
    }
    case PatternType::GrayGradient: {
        // Smooth gradient from x=0 to width
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = 1;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        std::vector<DWORD> pixels(width);
        for (int x = 0; x < width; ++x) {
            BYTE val = static_cast<BYTE>(std::round(static_cast<double>(x) * 255.0 / (width - 1)));
            pixels[x] = RGB(val, val, val);
        }

        StretchDIBits(hdc, 0, 0, width, height, 0, 0, width, 1, pixels.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
        break;
    }
    case PatternType::ColorGradient: {
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = 1;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        std::vector<DWORD> pixels(width);
        for (int x = 0; x < width; ++x) {
            double t = static_cast<double>(x) / (width - 1) * 6.0;
            int segment = static_cast<int>(t);
            double frac = t - segment;
            BYTE v1 = static_cast<BYTE>(255.0 * (1.0 - frac));
            BYTE v2 = static_cast<BYTE>(255.0 * frac);

            BYTE r = 0, g = 0, b = 0;
            switch (segment % 6) {
            case 0: r = 255; g = v2; b = 0; break;     // Red to Yellow
            case 1: r = v1; g = 255; b = 0; break;     // Yellow to Green
            case 2: r = 0; g = 255; b = v2; break;     // Green to Cyan
            case 3: r = 0; g = v1; b = 255; break;     // Cyan to Blue
            case 4: r = v2; g = 0; b = 255; break;     // Blue to Magenta
            case 5: r = 255; g = 0; b = v1; break;     // Magenta to Red
            }
            pixels[x] = RGB(b, g, r); // Windows DIB DWORD is 0x00RRGGBB or BGRA
        }

        StretchDIBits(hdc, 0, 0, width, height, 0, 0, width, 1, pixels.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
        break;
    }
    case PatternType::UniformMediumGray: {
        HBRUSH hBr = CreateSolidBrush(RGB(128, 128, 128));
        FillRect(hdc, &rc, hBr);
        DeleteObject(hBr);
        break;
    }
    default:
        break;
    }
}

} // namespace DisplayStabilizer
