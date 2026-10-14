#include "VgaWorkflow.h"

namespace DisplayStabilizer {

VgaWorkflow::VgaWorkflow() {
    m_items = {
        {
            1,
            L"Cable Seating and Connector Pins",
            L"Physical Connection",
            L"Inspect both ends of the VGA cable (PC and Acer monitor). Look for bent, recessed, or missing pins in the DE-15 connector.",
            L"Power down the PC and monitor. Disconnect the VGA cable. Inspect pins 1 (Red), 2 (Green), and 3 (Blue). Re-insert firmly and hand-tighten both thumbscrews securely.",
            L"A loose Pin 2 (Green signal) causes whites to appear magenta/pink. A poor ground connection (Pins 6, 7, 8) introduces high-frequency ground loop noise seen as moving speckles.",
            false
        },
        {
            2,
            L"Cable Shielding and Interference",
            L"Signal Integrity",
            L"Check if the VGA cable runs alongside AC power cords, surge protectors, unshielded power supplies, or phone chargers.",
            L"Reroute the VGA cable so it does not touch AC power cables. Ensure ferrite chokes (the plastic cylinders near cable ends) are present on the cable.",
            L"Analog VGA is vulnerable to electromagnetic induction. AC power 50/60 Hz ripple causes rolling bars, while high-frequency switching noise manifests as shimmering green/colored grain.",
            false
        },
        {
            3,
            L"Native Resolution and Refresh Rate",
            L"Display Timing",
            L"Confirm the desktop is set to the Acer V193HQL's exact native resolution (1366x768 @ 60 Hz) or standard 60 Hz mode.",
            L"Use the 'Display Mode' tab in this app to verify resolution is 1366x768 or supported 60 Hz. Avoid non-native aspect ratios that force analog interpolation scaling.",
            L"Running LCD panels at non-native resolutions over VGA causes severe pixel aliasing, text distortion, and fuzzy shimmer because analog pixels must be rescaled across fixed LCD subpixels.",
            false
        },
        {
            4,
            L"Monitor 'Auto Adjustment' Execution",
            L"Monitor Hardware ADC",
            L"Trigger the hardware Auto Adjust feature built into the Acer V193HQL front bezel.",
            L"Open the 'Grain Diagnostics' tab in this app, select '12. Fine Vertical Lines' or '10. Checkerboard', press 'Launch Fullscreen Pattern', and then press the physical 'AUTO' button on the monitor bezel.",
            L"Auto-Adjustment requires a high-contrast pattern across the full screen to accurately detect pixel edges. Triggering it on a plain desktop often miscalibrates the ADC.",
            false
        },
        {
            5,
            L"Monitor OSD Clock Fine Tuning",
            L"Monitor Hardware ADC",
            L"Manually tune the 'Clock' setting in the Acer V193HQL on-screen display (OSD) menu.",
            L"Press the monitor's 'Menu' button, navigate to Picture / Geometry settings, find 'Clock'. Adjust left/right while viewing vertical test lines until vertical bands disappear.",
            L"Clock sets the total number of ADC samples per horizontal line. If Clock is slightly off, wide vertical shimmering bands (moiré bands) appear across the screen.",
            false
        },
        {
            6,
            L"Monitor OSD Phase Fine Tuning",
            L"Monitor Hardware ADC",
            L"Manually tune the 'Phase' setting in the Acer V193HQL on-screen display (OSD) menu.",
            L"In the monitor's OSD menu, locate 'Phase'. Adjust by single increments until small text and fine checkerboard patterns stop shimmering and become sharp.",
            L"Phase adjusts the exact timing instant within each pixel period when the ADC samples the analog voltage. If Phase is off, samples occur on voltage transition edges, creating colored noise and grain.",
            false
        },
        {
            7,
            L"Horizontal and Vertical Image Position",
            L"Monitor Geometry",
            L"Verify the active video image fills the display area without black borders or edge clipping.",
            L"In monitor OSD, adjust H-Position and V-Position so the desktop image aligns precisely with the physical borders of the LCD panel.",
            L"Misaligned geometry indicates analog sync porch errors, which can distort analog black-level clamping and cause tinted backgrounds.",
            false
        },
        {
            8,
            L"Monitor Color Temperature and sRGB Reset",
            L"Monitor Color Space",
            L"Check whether internal monitor color presets (Warm, Cool, User) have become skewed.",
            L"In the Acer OSD Color menu, select 'Reset' or switch to 'User' and ensure Red, Green, and Blue gains are all set to neutral 50/50/50.",
            L"An internal monitor preset configured to 'Warm' or with reduced Green gain will tilt all whites toward pink/magenta.",
            false
        },
        {
            9,
            L"Test with Known-Good VGA Cable",
            L"Hardware Isolation",
            L"Test the monitor using an alternate, shielded VGA cable with intact ferrite beads.",
            L"Borrow a known-working shielded VGA cable to compare whether the green grain and pink cast disappear or stay identical.",
            L"Internal coaxial shield breaks or micro-fractures in cheap cables frequently cause high-frequency impedance mismatch, producing colored speckles and ghosting.",
            false
        },
        {
            10,
            L"Cross-System Verification",
            L"Root Cause Isolation",
            L"Connect the Acer monitor to another computer, or connect a different monitor to this PC.",
            L"Connect a laptop or second PC to the Acer V193HQL VGA input. Observe if the exact same green grain and cracked-area artifacts appear.",
            L"If artifacts remain identical across different computers, the fault is 100% inside the monitor panel/electronics, confirming that software calibration cannot physically repair it.",
            false
        }
    };
}

const VgaCheckItem* VgaWorkflow::getItem(int id) const {
    for (const auto& item : m_items) {
        if (item.id == id) return &item;
    }
    return nullptr;
}

void VgaWorkflow::setCompleted(int id, bool completed) {
    for (auto& item : m_items) {
        if (item.id == id) {
            item.isCompleted = completed;
            return;
        }
    }
}

int VgaWorkflow::getCompletedCount() const {
    int count = 0;
    for (const auto& item : m_items) {
        if (item.isCompleted) ++count;
    }
    return count;
}

std::wstring VgaWorkflow::getClockPhaseExplanation() {
    return L"Why Clock and Phase cannot be adjusted via standard Windows software:\n\n"
           L"Analog VGA sends continuous analog voltages (0.0V to 0.7V) for Red, Green, and Blue. "
           L"An LCD monitor must convert these continuous analog waves back into discrete digital pixel coordinates "
           L"using its internal Analog-to-Digital Converter (ADC) chip.\n\n"
           L"1. 'Clock' controls the total number of ADC clock cycles per scanline. If mismatched, horizontal pixel width varies across the screen.\n"
           L"2. 'Phase' delays the ADC sampling point by fractions of a nanosecond to sample the analog pulse right in the middle rather than on rising/falling edges.\n\n"
           L"Because the ADC chip resides strictly inside the monitor housing, these controls are hardware parameters of the monitor scaler. "
           L"They cannot be manipulated through standard Windows desktop display APIs. They must be tuned using the physical buttons on the Acer monitor bezel.";
}

std::wstring VgaWorkflow::getVgaColorPinoutExplanation() {
    return L"Analog VGA Pinout & Color Artifact Mapping:\n\n"
           L"Pin 1: Red Video (75 ohm analog)\n"
           L"Pin 2: Green Video (75 ohm analog) -> If open/poor: Pink/Magenta Whites! If noisy: Green Grain!\n"
           L"Pin 3: Blue Video (75 ohm analog)\n"
           L"Pin 6, 7, 8: Red, Green, Blue Ground returns\n"
           L"Pin 13: Horizontal Sync\n"
           L"Pin 14: Vertical Sync\n\n"
           L"Notice that pure white is Red + Green + Blue. If Pin 2 (Green) is degraded or oxidized, only Red + Blue reach the screen, creating Magenta/Pink whites. "
           L"Similarly, if the green coax shield is broken, high-frequency RF noise injects into the green line, causing bright green grain.";
}

} // namespace DisplayStabilizer
