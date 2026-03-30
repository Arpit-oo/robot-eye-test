#pragma once

#include <TFT_eSPI.h>

namespace DisplayTest {

void begin(TFT_eSPI &tft);
void tick(TFT_eSPI &tft);

}  // namespace DisplayTest
