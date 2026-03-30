# ESP32 TFT GIF Player - Full Agent Context

This document is intended to be copy-pasted to a coding agent so work can resume without losing prior context.

## 1) Project Identity and Goal

- Project type: PlatformIO Arduino firmware for ESP32.
- Main goal: Play embedded GIF animations on a 2.8" ILI9341-compatible SPI TFT panel.
- Secondary goal: Keep display stable on clone hardware that has shown white-screen, inversion, and dead-zone/noise behavior.

## 2) Workspace Layout

- Main firmware project: `robot-test/`
- Asset source project (external path used by scripts): `C:\code\gettingshitdone\robot\animation\`

Within `robot-test/`:

- `platformio.ini`: Build, upload, and library config.
- `src/main.cpp`: Firmware logic, GIF playlist, drawing pipeline, orientation/inversion runtime setup.
- `include/User_Setup.h`: TFT_eSPI board/display wiring and display driver settings.
- `include/catsitt.h`, `include/collarcatgif.h`, `include/doggif1.h`: Embedded GIF byte arrays.
- `tools/*.py`: Helper scripts for GIF/header generation, JSON sync, and LVGL patching.
- `data/`: Runtime data files used by some helper flows.

## 3) Hardware Mapping (Known Working Wiring)

- Board: ESP32 Dev Module
- Display driver family: ILI9341 clone panel
- Pins:
  - `TFT_MISO` = 19
  - `TFT_MOSI` = 23
  - `TFT_SCLK` = 18
  - `TFT_CS` = 5
  - `TFT_DC` = 27
  - `TFT_RST` = 33

## 4) Build System and Runtime Stack

- Framework: Arduino (Espressif32 platform)
- Libraries:
  - `TFT_eSPI` (display driver abstraction)
  - `AnimatedGIF` (GIF decode/playback)
  - `lvgl` (present as dependency)
- Build flags include forced include of `include/User_Setup.h`, so display config is globally injected during compile.

## 5) Current Runtime Behavior (Latest State)

- GIF playlist currently includes 2 animations:
  - `catsitt.gif`
  - `collarcatgif.gif`
- `doggif1.gif` was removed from playback list because prior runs showed open failures.
- Playback loop:
  - Open GIF
  - Center canvas on screen
  - Render frame-by-frame with transparency-aware scanline pushes
  - Transition to next GIF when complete
- Debug behavior:
  - Draws a grid on startup
  - Prints heap and GIF open/play logs over serial

## 6) Key Files and Responsibilities

## `platformio.ini`

- Defines environment `esp32dev`
- Uses `upload_port = COM10`
- Includes pre-build hook: `tools/patch_thorvg.py`
- Declares `lib_deps`
- Uses `build_flags` to include project setup (`-include include/User_Setup.h`)

## `include/User_Setup.h`

Purpose: Single source of truth for TFT_eSPI setup for this board/panel combo.

Current important defines:

- `ILI9341_2_DRIVER`
- `SPI_FREQUENCY 40000000`
- `SPI_READ_FREQUENCY 16000000`
- `TFT_INVERT_ON_START`
- `TFT_DISPLAY_MADCTL 0x68`

Notes:

- `TFT_DISPLAY_MADCTL 0x68` is intentionally set to handle clone-panel memory mapping behavior.
- Runtime orientation is still controlled in `src/main.cpp` via `tft.setRotation(...)`.

## `src/main.cpp`

Core objects:

- `TFT_eSPI tft`
- `AnimatedGIF gif`

Core functions and roles:

- `gif_draw(...)`: low-level per-line draw callback from decoder, supports transparency and uses `tft.pushImage`.
- `open_current_gif()`: opens current playlist entry, computes centered draw rectangle, logs status.
- `open_next_available_gif()`: retries across playlist if an item fails.
- `transition_to_next_gif()`: closes current GIF and rotates index.
- `setup()`: initializes serial/TFT/inversion/rotation, draws startup grid, starts GIF system.
- `loop()`: plays frames and advances GIFs.

Current orientation at the time of writing:

- `tft.setRotation(2)`
- Meaning: portrait orientation, rotated 180 degrees (upside-down portrait)

## 7) Tooling Scripts in `tools/`

- `convert_gifs_to_headers.py`
  - Converts GIF files from external animation folder into C header arrays in `include/`.
- `generate_resized_cat_header.py`
  - Reads a `.lottie` file, rescales animation payload, emits `include/cat_animation.h`.
- `sync_gif_to_data.py`
  - Copies source GIFs into `data/` for filesystem-based workflows.
- `sync_temp_json_to_data.py`
  - Reads JSON or hex-dump JSON and writes normalized `data/temp.json`.
- `patch_thorvg.py`
  - Applies compatibility patch in LVGL thorvg header when needed.

## 8) Known Historical Issues and What Was Learned

- White screen while firmware seemed alive:
  - Usually caused by wrong driver/setup mismatch or setup override behavior.
- Incorrect/inconsistent colors:
  - Usually inversion mismatch (`TFT_INVERT_ON_START` + runtime inversion combinations).
- Noise/dead-zone artifacts:
  - Sensitive to panel-specific MADCTL mapping and orientation pairing.
- GIF open failures:
  - Individual assets can fail; playlist fallback logic prevents complete halt.

## 9) Orientation Quick Switch Guide

Adjust only this line in `src/main.cpp`:

- `tft.setRotation(0)` = portrait
- `tft.setRotation(1)` = landscape
- `tft.setRotation(2)` = portrait (180)
- `tft.setRotation(3)` = landscape (270)

Recommended procedure:

1. Change rotation value.
2. Build and upload.
3. Validate full-screen render and dead-zone behavior.

## 10) Build/Flash/Monitor Commands

- Build: `platformio run`
- Upload: `platformio run --target upload`
- Serial monitor: `platformio device monitor --port COM10 --baud 115200`

Equivalent VS Code tasks exist:

- `PIO: Build`
- `PIO: Upload Firmware`

## 11) Current Constraints / Practical Notes

- Project depends on absolute external asset source paths in helper scripts.
- Display behavior is hardware-clone sensitive; values that are "correct" in generic docs may still fail on this unit.
- Because `User_Setup.h` is forcibly included by build flags, local TFT_eSPI defaults may show redefinition warnings; this has been tolerated in current workflow.

## 12) Recommended Next Improvements (Optional)

- Move external hard-coded asset paths into project-local config/env variables.
- Add compile-time constants for rotation/inversion to avoid manual edits.
- Add a tiny startup test pattern mode (solid fills + corner markers) toggled by a macro for fast panel diagnostics.
- Add simple unit-style checks for playlist metadata validity.

## 13) Prompt Snippet for Future AI Handoff

Use this when handing off to another agent:

"Read CODEBASE_AGENT_CONTEXT.md and DEAD_ZONE_FIX_HISTORY.md first. This is an ESP32 + ILI9341 clone TFT GIF player in PlatformIO. Keep current wiring and driver family, preserve GIF playback behavior, and do not remove existing debug serial logs unless asked. Prioritize avoiding regressions in display mapping/inversion/dead-zone behavior."
