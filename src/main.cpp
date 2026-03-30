#include <Arduino.h>
#include <TFT_eSPI.h>
#include <AnimatedGIF.h>

#include "display_test.h"
#include "eyes.h"

TFT_eSPI tft = TFT_eSPI();
AnimatedGIF gif;

static uint16_t g_line_buffer[240];
static int16_t gifX = 0;
static int16_t gifY = 0;
static int16_t gifW = 0;
static int16_t gifH = 0;
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
  tft.fillRect(gifX, gifY, gifW, gifH, TFT_BLACK);
}

static void gif_draw(GIFDRAW *p_draw) {
  int16_t x = p_draw->iX;
  int16_t y = p_draw->iY + p_draw->y;
  int16_t draw_x = x + gifX;
  int16_t draw_y = y + gifY;
  int16_t w = p_draw->iWidth;

  if (draw_y < 0 || draw_y >= tft.height() || draw_x >= tft.width()) {
    return;
  }
  if (draw_x < 0) {
    return;
  }
  if ((draw_x + w) > tft.width()) {
    w = tft.width() - draw_x;
  }
  if (w <= 0) {
    return;
  }

  uint8_t *src = p_draw->pPixels;
  uint16_t *palette = p_draw->pPalette;

  if (p_draw->ucHasTransparency) {
    const uint8_t transparent = p_draw->ucTransparent;
    // Build a full destination line to avoid flashing from line-by-line clears.
    for (int i = 0; i < w; i++) {
      g_line_buffer[i] = TFT_BLACK;
    }

    for (int i = 0; i < w; i++) {
      const uint8_t pix = src[i];
      if (pix != transparent) {
        g_line_buffer[i] = palette[pix];
      }
    }
    tft.pushImage(draw_x, draw_y, w, 1, g_line_buffer);
  } else {
    for (int i = 0; i < w; i++) {
      g_line_buffer[i] = palette[src[i]];
    }
    tft.pushImage(draw_x, draw_y, w, 1, g_line_buffer);
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

  gifX = (screen_w - canvas_w) / 2;
  gifY = (screen_h - canvas_h) / 2;
  if (gifX < 0) gifX = 0;
  if (gifY < 0) gifY = 0;

  gifW = canvas_w;
  gifH = canvas_h;
  if ((gifX + gifW) > screen_w) {
    gifW = screen_w - gifX;
  }
  if ((gifY + gifH) > screen_h) {
    gifH = screen_h - gifY;
  }

  clear_current_gif_canvas();
  Serial.printf("Playing %s at (%d,%d), canvas=%dx%d\n", item.name, gifX, gifY, canvas_w, canvas_h);
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
  gif.close();
  g_gif_open = false;
  delay(50);
  tft.fillScreen(TFT_BLACK);
  delay(50);
  drawDebugGrid();
  delay(50);

  gifX = 0;
  gifY = 0;
  gifW = 0;
  gifH = 0;

  reset_gif_state();
  g_current_gif_index = (g_current_gif_index + 1) % kGifCount;
}

static void init_display_hardware() {
  tft.init();
  tft.invertDisplay(true);
  tft.setRotation(2);
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
  tft.fillScreen(TFT_BLACK);
  drawDebugGrid();
  delay(2000);

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

  if (frame_delay_ms < 5) frame_delay_ms = 5;
  if (frame_delay_ms > 60) frame_delay_ms = 60;
  delay(frame_delay_ms);
}

#endif
