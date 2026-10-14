# Display Stabilizer

**A native Windows C++17 utility for diagnosing and mitigating analog VGA artifacts, color cast abnormalities, and image instability on LCD monitors.**

---

## 1. Hardware Target & Diagnostic Overview

### The Target Environment
- **Monitor:** Acer V193HQL (18.5" / 1366×768 native resolution @ 60 Hz).
- **Interface:** Analog VGA (DE-15 connector, 75Ω RGB coaxial signals).
- **Hardware History:** The monitor was previously dropped and has a visible physical crack in the panel.
- **Reported Symptoms:**
  - Green grain / colored speckles across gray and neutral fields.
  - Pink or magenta-tinted whites and highlights.
  - Pixelated, fuzzy, or shimmering text.
  - Image flickering and subtle movement.
- **Graphics Source:** Windows desktop using Intel Integrated Graphics (DAC outputting analog VGA).

### Physical Reality & Software Boundaries
> **IMPORTANT:** Software running on the PC graphics card cannot physically repair a cracked LCD glass substrate, ruptured liquid crystal cells, or severed driver tab bonds. 
> 
> Display Stabilizer is engineered with honest, rigorous diagnostic principles:
> 1. It provides safe, **100% reversible software adjustments** (channel gain, black-level offset, gamma, contrast, and color temperature) via documented Windows display APIs (`GetDeviceGammaRamp` / `SetDeviceGammaRamp`).
> 2. It differentiates between **analog VGA transmission defects** (which can be mitigated or calibrated) and **unrepairable monitor panel damage**.
> 3. It gives the user tools to isolate the exact fault using **15 static full-screen diagnostic patterns** and a **10-point guided VGA checklist**.

---

## 2. Technical Symptom & Physics Breakdown

| Visible Symptom | Technical Physical Cause | Can Software Fix It? | Recommended Action |
| :--- | :--- | :--- | :--- |
| **Pink / Magenta Whites** | Analog VGA **Pin 2 (Green video)** signal loss or oxidation. Pure white ($R+G+B$) minus Green yields Magenta ($R+B$). | **Partially.** Software can trim red/blue gain slightly to balance whites, but physical cable reseating is required. | Reseat cable, hand-tighten thumbscrews, inspect Pin 2 for bend/recession. Test **Preset D (Pink Cast Test)**. |
| **Green Grain / Speckles** | High-frequency RF ground noise on analog VGA coax line, broken cable shield, or ADC sampling jitter. | **Partially.** Reducing contrast or slight green gain trim softens noise. Static speckles in fixed pixels indicate damaged subpixels. | Route VGA cable away from AC power bricks. Test with shielded VGA cable. View **Pattern 15 (Medium Gray)**. |
| **Shimmering / Fuzzy Text** | Monitor ADC **Phase** or **Clock** frequency mismatch. ADC samples on the rising/falling analog voltage edge rather than the flat pixel center. | **No, monitor-side ADC setting.** Software cannot directly modify physical monitor ADC registers over standard VGA. | Launch **Pattern 12 (Fine Vertical Lines)**, then press the physical **AUTO** button on the Acer monitor bezel to lock ADC Phase. |
| **Fixed Vertical Lines / Inky Dark Spots** | Physical LCD glass fracture, broken ACF conductive traces, or leaked liquid crystal fluid from the drop. | **No.** Physical hardware damage. | Perform the **OSD Isolation Test**: if defects appear over the monitor's built-in OSD menu or "No Signal" screen, the panel is physically damaged. |
| **Full-Screen Flickering** | Aged CCFL backlight tube, failing high-voltage inverter transformer (from drop shock), or 50/60Hz ground hum. | **No.** Display Stabilizer intentionally avoids dangerous software flashing filters that worsen eyestrain. | Verify 60 Hz refresh rate in **Display Mode** tab. Plug PC and monitor into the same grounded surge protector. |

---

## 3. Architecture & Built-in Presets

### Preset System
All presets use neutral starting points. Built-in presets are immutable, but any preset can be duplicated into a custom profile and modified.

- **Preset A — Neutral (Default Baseline):** Pure identity transformation ($1.00\times$ gains, $0.00$ offsets, $1.00$ gammas, $1.00$ contrast, $1.00$ saturation, $0.00$ temp bias). Corrections disabled. Guarantees no unintended color transformation.
- **Preset B — Natural Whites:** Baseline profile prepared for subtle manual white-balance adjustments without aggressive filtering.
- **Preset C — Green Cast Test:** Subtle green-channel gain reduction ($-6\%$) to evaluate whether attenuating the green analog signal reduces green bias on neutral areas.
- **Preset D — Pink Cast Test:** Subtle red/blue trim ($-4\%$) to evaluate whether magenta whites on VGA can be compensated to achieve neutral gray.
- **Preset E — Low Contrast:** Reduced contrast ($-12\%$) and slight black-level lift ($+2\%$) to soften harsh clipping and diminish the visibility of edge grain artifacts.
- **Preset F — Diagnostic:** All color modifications disabled. Focuses on static test patterns, display mode verification, and VGA checklist diagnostics.
- **Preset G — Custom:** Fully editable profiles allowing custom configuration of all 12 parameters. Profiles can be duplicated, renamed, deleted, exported to JSON, and imported.

---

## 4. Key Application Features

### A. Modular Reversible Color Engine
- Uses documented Win32 display gamma ramp APIs (`GetDeviceGammaRamp`, `SetDeviceGammaRamp`).
- **Instant A/B Preview:** Single-click toggle compares active corrections against the baseline image.
- **Clean Exit Guarantee:** Destructor and shutdown hooks automatically restore the original display gamma ramp on exit or application close.
- **No Injections:** Zero DLL injection, zero driver hooking, zero fullscreen overlays that distort desktop rendering.

### B. Grain & RGB Artifact Diagnostics (15 Static Patterns)
Dedicated fullscreen test window with zero animation to prevent confusing artificial motion with monitor flicker:
1. Pure White (`#FFFFFF`)
2. Pure Black (`#000000`)
3. Neutral Gray stepped levels (25%, 50%, 75%)
4. Solid Red (`#FF0000`)
5. Solid Green (`#00FF00`)
6. Solid Blue (`#0000FF`)
7. Solid Cyan (`#00FFFF`)
8. Solid Magenta (`#FF00FF`)
9. Solid Yellow (`#FFFF00`)
10. Black-and-White Checkerboard ($1\times1$ pixel grid for ADC clock/phase sync test)
11. Fine Horizontal Lines ($1$ px alternating)
12. Fine Vertical Lines ($1$ px alternating — primary test for VGA pixel clock and ADC Phase jitter)
13. Smooth Gray Gradient ($0$ to $255$)
14. Full-Spectrum Color Gradient
15. Uniform Medium Gray (`#808080` — primary test for green grain)

*Hotkeys in fullscreen pattern window:*
- `ESC`, `Q`, or `Right-Click`: Close test pattern.
- `Left` / `Right` / `Space`: Cycle to next/previous pattern.
- `1` through `9`, `0`: Direct jump to patterns 1–10.
- `H`: Toggle diagnostic HUD overlay.

### C. Display Mode Switching with 15-Second Protected Rollback
- Enumerates all supported display modes via `EnumDisplaySettingsExW`.
- Highlights native $1366\times768$ @ 60 Hz and standard $1280\times1024$ / $1024\times768$ modes.
- **15-Second Safe Rollback Dialog:** When testing a mode, a modal countdown begins. If the screen goes black, loses sync, or the user does not click "Keep Changes", the system automatically reverts to the original mode without requiring a system restart.

### D. System Tray, Hotkeys & Persistence
- **Global Hotkeys:**
  - `Win + Alt + S` (or `Ctrl + Alt + D`): Toggle corrections on/off.
  - `Win + Alt + X` (or `Ctrl + Alt + X`): Emergency disable all corrections immediately.
- **System Tray Icon:** Minimizes unobtrusively; context menu offers quick toggle, disable, preset selection, and exit.
- **Persistence:** Settings and custom profiles saved to `%APPDATA%\DisplayStabilizer\settings.json` in clean, versioned JSON with schema validation.

---

## 5. Build & Compilation Instructions

### Prerequisites
- Windows 10 or Windows 11 (x64)
- Visual Studio 2022 / 2026 with C++ Desktop Development (MSVC `cl.exe` v19+)
- Windows 10/11 SDK (version 10.0.26100.0 or compatible)
- CMake 3.20 or newer

### Building from Developer Command Prompt
```cmd
cd c:\Projects\screenfix
cmake -B build -S . -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

The resulting binaries will be built at:
- `build\Release\DisplayStabilizer.exe` (Main Application)
- `build\Release\DisplayStabilizerTests.exe` (Automated Test Suite)
- Also copied to `bin\Release\` for distribution.

---

## 6. Automated Test Results

The test suite validates parsing, math, clamping, identity ramps, preset duplication, and serialization:

```
========================================
Display Stabilizer Automated Test Suite
========================================
Running testJsonHelperBasics... PASSED
Running testJsonHelperMalformed... PASSED
Running testProfileClamping... PASSED
Running testNeutralProfileIdentity... PASSED
Running testGreenCastRampCalculation... PASSED
Running testPresetManagementAndDuplication... PASSED
Running testProfileImportExport... PASSED
Running testDisplayModeProperties... PASSED
========================================
Test Results: 8 passed, 0 failed.
========================================
```

---

## 7. Known Technical Limitations

1. **Physical Monitor ADC Clock & Phase:**
   Clock and Phase are hardware parameters within the Acer V193HQL monitor scaler chip. They cannot be written directly over standard analog VGA cables because VGA carries only unidirectional video signals and basic DDC I2C communication. They must be calibrated using the physical **AUTO** button and OSD menu on the monitor bezel.
2. **Physical LCD Panel Damage:**
   Dropping an LCD panel can fracture the thin glass substrate or dislodge chip-on-glass (COG) driver circuits. Software color adjustments cannot seal glass cracks or revive permanently dead subpixel columns.
3. **Multi-Monitor Secondary Gamma Ramps:**
   Under standard Windows GDI display gamma ramp APIs, `SetDeviceGammaRamp` applies to the primary display device. In multi-monitor configurations, the Acer V193HQL must be configured as the primary display.

---

## 8. Safety Advisory

> [!CAUTION]
> **DO NOT OPEN OR DISASSEMBLE THE MONITOR.**
> The Acer V193HQL utilizes a Cold Cathode Fluorescent Lamp (CCFL) backlight powered by an internal step-up inverter circuit board that produces **600V to 1,500V AC**. Opening the monitor housing presents a severe electrical shock hazard even when the power cable is disconnected. Only perform external inspections of cables and OSD buttons.
