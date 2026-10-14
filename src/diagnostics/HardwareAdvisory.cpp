#include "HardwareAdvisory.h"

namespace DisplayStabilizer {

std::vector<DamageSymptom> HardwareAdvisory::getCommonDamageSymptoms() {
    return {
        {
            L"Visible crack in panel with black ink-like pools or spiderweb lines",
            L"Physical rupture of the liquid crystal substrate. Liquid crystal fluid has leaked between glass polarizers.",
            L"Observe panel when powered off under ambient light. Dark patches that do not change with PC power confirm physical glass fracture.",
            true
        },
        {
            L"Green speckles / colored dots present on the monitor's built-in OSD menu",
            L"Fault in the internal monitor timing controller (T-CON) chip, row/column driver ICs, or panel flex cables caused by drop impact.",
            L"Disconnect the VGA cable completely so monitor displays 'No Signal' or press OSD 'Menu'. If green speckles appear over the OSD box, PC software cannot repair it.",
            true
        },
        {
            L"Pink/magenta whites visible on PC desktop, but monitor OSD box is pure white",
            L"Analog VGA signal loss on Pin 2 (Green) in the VGA cable, connector, or PC DAC output.",
            L"Inspect and tighten the VGA cable thumbscrews; test with an alternate VGA cable. Software color calibration can also offset this.",
            false
        },
        {
            L"Shimmering / dancing grain across text edges and fine vertical lines",
            L"Analog VGA ADC Phase / Clock jitter or electrical noise pickup on analog coax lines.",
            L"Execute monitor 'Auto Adjustment' on full-screen vertical lines pattern; fine-tune OSD Phase.",
            false
        },
        {
            L"Persistent vertical or horizontal colored single-pixel lines across full height/width",
            L"Broken conductive anisotropic conductive film (ACF) bond or cracked row/column driver tab on the LCD glass due to shock.",
            L"Check if line exists across all resolutions and test patterns. If fixed in exact screen coordinate, it is a hardware bond fault.",
            true
        }
    };
}

std::wstring HardwareAdvisory::getDropDamageAnalysis() {
    return L"CRITICAL PHYSICAL HARDWARE ASSESSMENT — ACER V193HQL:\n\n"
           L"Because this monitor was previously dropped and has a visible crack, physical panel or board damage is a confirmed factor.\n\n"
           L"1. What software CAN do:\n"
           L"   - Compensate for analog VGA signal attenuation (e.g. slight gain loss on Pin 2 Green causing pink whites).\n"
           L"   - Reduce harsh contrast clipping to soften the appearance of noise.\n"
           L"   - Provide static test patterns to calibrate the monitor's internal ADC Clock & Phase.\n"
           L"   - Help you definitively isolate whether artifacts originate in the PC cable vs the physical monitor.\n\n"
           L"2. What software CANNOT do:\n"
           L"   - Repair fractured LCD substrate glass or ruptured liquid crystal fluid cells.\n"
           L"   - Fix broken TAB/COF flex cables or damaged timing controller (T-CON) traces.\n"
           L"   - Repair an aging or cracked CCFL backlight tube / high-voltage inverter.\n\n"
           L"3. The Golden Test (OSD Isolation):\n"
           L"   Unplug the VGA cable from the monitor. If green grain, flickering, or discoloration are still visible on the Acer floating 'No Signal' box or OSD Menu, the issue is 100% internal hardware damage from the drop.";
}

std::wstring HardwareAdvisory::getOsdTestInstruction() {
    return L"Step-by-step OSD Isolation Test:\n\n"
           L"1. Press the physical 'Menu' button on your Acer V193HQL bezel.\n"
           L"2. Look closely at the OSD menu window background and text:\n"
           L"   - If the OSD menu is clean, crisp, and shows pure white/gray without green speckles, your monitor panel is working, and the issue is VGA cable/signal related!\n"
           L"   - If the OSD menu itself has green grain, colored dots, or pink tint, the internal monitor hardware is damaged from the drop.";
}

std::wstring HardwareAdvisory::getSafetyWarning() {
    return L"SAFETY NOTICE — DO NOT ATTEMPT TO OPEN THE MONITOR:\n\n"
           L"The Acer V193HQL uses a CCFL (Cold Cathode Fluorescent Lamp) backlight with an internal high-voltage inverter transformer that generates 600V to 1500V AC. "
           L"Opening the monitor casing poses severe electrical shock hazard even when unplugged. "
           L"Only perform safe external checks: inspect cables, check thumbscrews, and tune external OSD buttons.";
}

} // namespace DisplayStabilizer
