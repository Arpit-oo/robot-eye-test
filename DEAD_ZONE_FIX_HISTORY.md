# Dead-Zone Fix History and Playbook

This file captures every major dead-zone/display-mapping fix attempted and the resulting stable strategy.

## 1) Problem Definition

- Symptom: A region of the TFT showed noise, unmapped pixels, or incomplete rendering while other regions worked.
- Side symptoms observed during troubleshooting: white screen, color inversion anomalies, and orientation-dependent artifacts.
- Context: ILI9341 clone behavior can diverge from reference panel assumptions.

## 2) Constraints Used During Fixing

- Keep hardware wiring fixed (do not repin during software iteration).
- Limit risky code changes during diagnosis.
- Prefer only changing:
  - `include/User_Setup.h`
  - `tft.setRotation(...)` in `src/main.cpp`
  - inversion line in `setup()` when needed

## 3) Fixes Applied (Chronological)

## A. Baseline display configuration enforcement

What was done:

- Forced project setup inclusion through build flags in `platformio.ini`.
- Standardized panel driver setup in `User_Setup.h`.

Why:

- Ensure project settings win over library defaults and avoid mismatch drift.

Effect:

- Reduced random config behavior across rebuilds.

## B. Driver selection alignment for clone panel

What was done:

- Settled on `ILI9341_2_DRIVER` in `User_Setup.h`.

Why:

- Clone panel behavior matched this driver variant better than generic alternatives.

Effect:

- Improved panel bring-up reliability.

## C. Inversion handling cleanup

What was done:

- Used startup inversion define and runtime inversion intentionally.
- Current runtime includes `tft.invertDisplay(true)` after `tft.init()`.

Why:

- Wrong inversion state can look like dead-zone/noise in some scenarios.

Effect:

- Corrected visible color polarity in current state.

## D. MADCTL tuning for panel memory mapping

What was done:

- Added/kept `#define TFT_DISPLAY_MADCTL 0x68` in `User_Setup.h`.

Why:

- Clone controllers frequently need non-default memory access settings.
- Dead-zone artifacts often map to incorrect address-direction settings.

Effect:

- Improved full-frame mapping consistency in tested orientation combinations.

## E. Rotation sweep strategy

What was done:

- Tested runtime `tft.setRotation(...)` values to find orientation/mapping combo that minimizes dead-zone artifacts.
- Used sequence approach: try 0, 1, 2, 3 and visually inspect full-screen behavior.

Why:

- Some clone panels behave correctly only for specific MADCTL + rotation pairs.

Effect:

- Confirmed working landscape and portrait variants; currently set to portrait 180 via rotation `2`.

## F. SPI speed strategy during troubleshooting

What was done:

- Earlier troubleshooting included reducing SPI speed to suppress noise, then switching to a verified high-speed config request.
- Current `User_Setup.h` value: `SPI_FREQUENCY 40000000` and `SPI_READ_FREQUENCY 16000000`.

Why:

- SPI speed can influence signal quality and apparent display artifacts.

Effect:

- Current configuration is running and was explicitly requested for the tested setup.

## G. GIF playlist simplification during stabilization

What was done:

- Removed `doggif1.gif` from active playlist in `src/main.cpp`.

Why:

- Asset-specific open failures created noise during display debugging.

Effect:

- Cleaner validation loop while testing display stability.

## 4) Current Known Working Configuration Snapshot

## `include/User_Setup.h`

- `ILI9341_2_DRIVER`
- `TFT_MISO 19`
- `TFT_MOSI 23`
- `TFT_SCLK 18`
- `TFT_CS 5`
- `TFT_DC 27`
- `TFT_RST 33`
- `SPI_FREQUENCY 40000000`
- `SPI_READ_FREQUENCY 16000000`
- `TFT_INVERT_ON_START`
- `TFT_DISPLAY_MADCTL 0x68`

## `src/main.cpp` runtime init

- `tft.init();`
- `tft.invertDisplay(true);`
- `tft.setRotation(2);` (portrait 180)

## 5) Repeatable Dead-Zone Recovery Procedure

When dead-zone reappears, follow this exact order:

1. Confirm wiring and no loose connections.
2. Keep `ILI9341_2_DRIVER` and `TFT_DISPLAY_MADCTL 0x68` in `User_Setup.h`.
3. Keep inversion line in setup (`tft.invertDisplay(true)`) while testing.
4. Run rotation sweep in this order:
   - `setRotation(0)`
   - `setRotation(1)`
   - `setRotation(2)`
   - `setRotation(3)`
5. For each rotation, build/upload and inspect:
   - full screen fill
   - grid continuity
   - absence of noisy/dead strip
6. If all fail, temporarily test lower SPI frequency to isolate signal integrity impact.
7. Lock best combination and document it in this file.

## 6) Validation Checklist

- Boot logs appear on serial.
- Startup grid draws fully without clipped areas.
- GIF frames render in expected area without persistent blank strip.
- No recurring noise band during playlist transitions.
- Orientation matches requested user mode.

## 7) What Not to Change During Dead-Zone Diagnosis

- Do not simultaneously refactor GIF rendering logic and display setup.
- Do not change multiple unrelated subsystems in one attempt.
- Do not treat generic ILI9341 defaults as ground truth for clone panels.

## 8) Naming Convention for Future Fix Entries

When adding new attempts, use this template:

- `DZ-YYYYMMDD-<short-tag>`
  - Change made
  - Reason
  - Visual result
  - Keep/revert decision

Example:

- `DZ-20260330-rot2-madctl68`
  - Set rotation to 2 with MADCTL 0x68 and runtime inversion true
  - Targeted portrait 180 without dead-zone
  - Result: stable playback in portrait upside-down
  - Decision: keep
