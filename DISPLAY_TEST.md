# Display Test Runbook

Use this file whenever you want a quick display validation on the ESP32 + ILI9341 setup.

## Method 1: Agent-Run Smoke Test Firmware

Ask the agent:

- Build `esp32dev_display_test`
- Upload `esp32dev_display_test`

Equivalent commands:

```powershell
C:/Users/arpit/.platformio/penv/Scripts/platformio.exe run -e esp32dev_display_test
C:/Users/arpit/.platformio/penv/Scripts/platformio.exe run -e esp32dev_display_test -t upload
```

Expected result on TFT:

- Screen cycles: red -> green -> blue -> white -> black
- Grid is visible
- Text shows `DISPLAY TEST` and `STEP N`

If upload is unstable, keep BOOT pressed until flashing starts and use the test env (already configured with slower upload speed).

## Method 2: Direct In-Code Trigger (No Reflash to test env)

Normal firmware includes a direct trigger method in code:

- Method name: `startDisplaySmokeTest()`
- File: `src/main.cpp`

How to trigger now (already wired):

1. Upload normal firmware (`env:esp32dev`)
2. Open serial monitor at 115200
3. Send character `t`

This calls `startDisplaySmokeTest()` and switches to smoke-test visuals immediately.

## Return to Eye GIF Firmware

To run eye GIF playback again, upload normal env:

```powershell
C:/Users/arpit/.platformio/penv/Scripts/platformio.exe run -e esp32dev -t upload
```

## Notes

- Smoke test logic is separated into `include/display_test.h` and `src/display_test.cpp`.
- Main firmware can call that logic directly, and test env can run it as dedicated firmware.
