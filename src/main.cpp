#include <Arduino.h>
#include <TFT_eSPI.h>
#include <AnimatedGIF.h>

#include "display_test.h"
#include "eyes.h"

TFT_eSPI tft = TFT_eSPI();
AnimatedGIF gif;
static constexpr uint16_t kBackgroundColor = TFT_WHITE;

static uint16_t g_line_buffer[320];
static int16_t gifX = 0;
static int16_t gifY = 0;
static int16_t gifW = 0;
static int16_t gifH = 0;
static int16_t gifSrcW = 0;
static int16_t gifSrcH = 0;
static bool g_gif_open = false;
static size_t g_current_gif_index = 0;
static bool g_runtime_display_test = false;

struct EmbeddedGif {
  const char *name;
  const uint8_t *data;
  size_t size;
};

static constexpr EmbeddedGif kGifPlaylist[] = {
  {"eyes.gif", eyes_gif, eyes_gif_len},
};
static constexpr size_t kGifCount = sizeof(kGifPlaylist) / sizeof(kGifPlaylist[0]);

static void drawDebugGrid() {
  for (int x = 0; x < 240; x += 20) {
    tft.drawFastVLine(x, 0, 320, TFT_DARKGREY);
  }
  for (int y = 0; y < 320; y += 20) {
    tft.drawFastHLine(0, y, 240, TFT_DARKGREY);
  }
}

static void reset_gif_state() {
  gifX = 0;
  gifY = 0;
  gifW = 0;
  gifH = 0;
}

static void clear_current_gif_canvas() {
  if (gifW <= 0 || gifH <= 0) {
    return;
  }
  tft.fillRect(gifX, gifY, gifW, gifH, kBackgroundColor);
}

static void gif_draw(GIFDRAW *p_draw) {
  if (gifSrcW <= 0 || gifSrcH <= 0 || gifW <= 0 || gifH <= 0) {
    return;
  }

  const int16_t srcY = p_draw->iY + p_draw->y;
  const int16_t srcX0 = p_draw->iX;
  const int16_t srcX1 = p_draw->iX + p_draw->iWidth;
  if (srcY < 0 || srcY >= gifSrcH || srcX0 >= srcX1) {
    return;
  }

  int16_t dstY0 = (srcY * gifH) / gifSrcH;
  int16_t dstY1 = ((srcY + 1) * gifH) / gifSrcH;
  if (dstY1 <= dstY0) dstY1 = dstY0 + 1;

  int16_t dstX0 = (srcX0 * gifW) / gifSrcW;
  int16_t dstX1 = (srcX1 * gifW) / gifSrcW;
  if (dstX1 <= dstX0) dstX1 = dstX0 + 1;

  int16_t drawX = gifX + dstX0;
  int16_t drawW = dstX1 - dstX0;
  int16_t drawY0 = gifY + dstY0;
  int16_t drawY1 = gifY + dstY1;

  if (drawW <= 0 || drawY1 <= 0 || drawY0 >= tft.height()) {
    return;
  }

  // Clip horizontally to avoid writing outside display bounds.
  int16_t clipLeft = 0;
  if (drawX < 0) {
    clipLeft = -drawX;
    drawW -= clipLeft;
    drawX = 0;
  }
  if ((drawX + drawW) > tft.width()) {
    drawW = tft.width() - drawX;
  }
  if (drawW <= 0) {
    return;
  }

  uint8_t *src = p_draw->pPixels;
  uint16_t *palette = p_draw->pPalette;

  for (int16_t xo = 0; xo < drawW; xo++) {
    const int32_t numer = static_cast<int32_t>(xo + clipLeft) * p_draw->iWidth;
    int16_t srcIndex = static_cast<int16_t>(numer / (dstX1 - dstX0));
    if (srcIndex < 0) srcIndex = 0;
    if (srcIndex >= p_draw->iWidth) srcIndex = p_draw->iWidth - 1;

    const uint8_t pix = src[srcIndex];
    g_line_buffer[xo] = kBackgroundColor;
    if (!p_draw->ucHasTransparency || pix != p_draw->ucTransparent) {
      g_line_buffer[xo] = palette[pix];
    }
  }

  // Replace GIF background color with TFT_WHITE for seamless blending
  // Only replaces exact background palette index, never touches animation colors
  uint8_t bgIndex = p_draw->ucBackground;
  for (int x = 0; x < drawW; x++) {
    const int32_t numer = static_cast<int32_t>(x + clipLeft) * p_draw->iWidth;
    int16_t srcIndex = static_cast<int16_t>(numer / (dstX1 - dstX0));
    if (srcIndex < 0) srcIndex = 0;
    if (srcIndex >= p_draw->iWidth) srcIndex = p_draw->iWidth - 1;
    if (p_draw->pPixels[srcIndex] == bgIndex) {
      g_line_buffer[x] = TFT_WHITE;
    }
  }

  for (int16_t yOut = drawY0; yOut < drawY1; yOut++) {
    if (yOut >= 0 && yOut < tft.height()) {
      tft.pushImage(drawX, yOut, drawW, 1, g_line_buffer);
    }
  }
}

static bool open_current_gif() {
  const EmbeddedGif &item = kGifPlaylist[g_current_gif_index];
  Serial.printf("Free heap before GIF %d: %d\n", static_cast<int>(g_current_gif_index), ESP.getFreeHeap());

  reset_gif_state();

  if (!gif.open((uint8_t *)item.data, static_cast<int32_t>(item.size), gif_draw)) {
    Serial.printf("GIF open failed: %s\n", item.name);
    g_gif_open = false;
    return false;
  }

  const int16_t screen_w = tft.width();
  const int16_t screen_h = tft.height();
  const int16_t canvas_w = gif.getCanvasWidth();
  const int16_t canvas_h = gif.getCanvasHeight();

  gifSrcW = canvas_w;
  gifSrcH = canvas_h;

  const float scale_x = static_cast<float>(screen_w) / static_cast<float>(canvas_w);
  const float scale_y = static_cast<float>(screen_h) / static_cast<float>(canvas_h);
  const float scale = (scale_x < scale_y) ? scale_x : scale_y;
  const int16_t scaled_w = static_cast<int16_t>(canvas_w * scale + 0.5f);
  const int16_t scaled_h = static_cast<int16_t>(canvas_h * scale + 0.5f);

  gifX = (screen_w - scaled_w) / 2;
  gifY = (screen_h - scaled_h) / 2;
  if (gifX < 0) gifX = 0;
  if (gifY < 0) gifY = 0;

  gifW = scaled_w;
  gifH = scaled_h;
  if ((gifX + gifW) > screen_w) {
    gifW = screen_w - gifX;
  }
  if ((gifY + gifH) > screen_h) {
    gifH = screen_h - gifY;
  }

  clear_current_gif_canvas();
  Serial.printf("Playing %s at (%d,%d), canvas=%dx%d scaled=%dx%d\n", item.name, gifX, gifY, canvas_w, canvas_h, gifW, gifH);
  g_gif_open = true;
  return true;
}

static bool open_next_available_gif() {
  for (size_t attempt = 0; attempt < kGifCount; ++attempt) {
    if (open_current_gif()) {
      return true;
    }
    g_current_gif_index = (g_current_gif_index + 1) % kGifCount;
  }
  return false;
}

static void transition_to_next_gif() {
  // Reset decoder to frame 0 for seamless replay of the same open GIF.
  gif.reset();
}

static void init_display_hardware() {
  tft.init();
  tft.invertDisplay(true);
  tft.setRotation(1);
}

static void startDisplaySmokeTest() {
  gif.close();
  g_gif_open = false;
  g_runtime_display_test = true;
  DisplayTest::begin(tft);
}

#ifdef DISPLAY_SMOKE_TEST

void setup() {
  Serial.begin(115200);
  delay(250);

  init_display_hardware();
  DisplayTest::begin(tft);
}

void loop() {
  DisplayTest::tick(tft);
}

#else

void setup() {
  Serial.begin(115200);
  delay(250);

  init_display_hardware();
  tft.fillScreen(kBackgroundColor);

  gif.begin(LITTLE_ENDIAN_PIXELS);

  if (!open_next_available_gif()) {
    Serial.println("Initial GIF open failed; loop() will retry");
  }

  Serial.println("Eyes GIF playback started");
  Serial.println("Send 't' in Serial Monitor to start display smoke test mode");
}

void loop() {
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == 't' || c == 'T') {
      startDisplaySmokeTest();
    }
  }

  if (g_runtime_display_test) {
    DisplayTest::tick(tft);
    return;
  }

  if (!g_gif_open) {
    if (!open_next_available_gif()) {
      delay(250);
      return;
    }
  }

  int frame_delay_ms = 0;
  if (!gif.playFrame(false, &frame_delay_ms)) {
    transition_to_next_gif();
    return;
  }

  if (frame_delay_ms < 1) frame_delay_ms = 1;
  delay(frame_delay_ms);
}

#endif
