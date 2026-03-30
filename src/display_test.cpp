#include "display_test.h"

#include <Arduino.h>

namespace {

void drawDebugGrid(TFT_eSPI &tft) {
  for (int x = 0; x < 240; x += 20) {
    tft.drawFastVLine(x, 0, 320, TFT_DARKGREY);
  }
  for (int y = 0; y < 320; y += 20) {
    tft.drawFastHLine(0, y, 240, TFT_DARKGREY);
  }
}

}  // namespace

namespace DisplayTest {

void begin(TFT_eSPI &tft) {
  tft.fillScreen(TFT_BLACK);
  drawDebugGrid(tft);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("DISPLAY SMOKE TEST", tft.width() / 2, tft.height() / 2, 2);
  Serial.println("Display smoke test firmware running");
}

void tick(TFT_eSPI &tft) {
  static uint8_t step = 0;
  static uint32_t lastMs = 0;

  const uint32_t now = millis();
  if ((now - lastMs) < 1000) {
    return;
  }
  lastMs = now;

  const uint16_t colors[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_WHITE, TFT_BLACK};
  const uint16_t bg = colors[step % 5];

  tft.fillScreen(bg);
  drawDebugGrid(tft);
  tft.setTextColor((bg == TFT_WHITE) ? TFT_BLACK : TFT_YELLOW, bg);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("DISPLAY TEST", tft.width() / 2, tft.height() / 2 - 20, 4);
  tft.drawString("STEP " + String(step), tft.width() / 2, tft.height() / 2 + 20, 4);

  Serial.printf("Display smoke test step %u\n", step);
  step++;
}

}  // namespace DisplayTest
