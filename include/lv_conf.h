#ifndef LV_CONF_H
#define LV_CONF_H

/*
 * Minimal LVGL config for ESP32 (internal RAM only, no PSRAM).
 * Keep buffers and feature set small to leave heap for Lottie parsing/rendering.
 */

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

#define LV_MEM_SIZE (32U * 1024U)

#define LV_DEF_REFR_PERIOD 20
#define LV_DPI_DEF 130

#define LV_USE_OS LV_OS_NONE

#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

#define LV_USE_DRAW_SW 1
#define LV_DRAW_SW_COMPLEX 1

#define LV_USE_CANVAS 1
#define LV_USE_LOTTIE 0
#define LV_USE_THORVG 0
#define LV_USE_THORVG_INTERNAL 0
#define LV_USE_VECTOR_GRAPHIC 0
#define LV_USE_MATRIX 0
#define LV_USE_FLOAT 0
#define LV_USE_GIF 1

/* App-level render chunk size: 240 * 24 px for RAM-safe partial refresh. */
#define APP_DRAW_BUFFER_LINES 24

/* Keep common widgets available */
#define LV_USE_LABEL 1
#define LV_USE_IMG 1

/* Disable heavy optional features for RAM safety */
#define LV_USE_FS_STDIO 0
#define LV_USE_FS_POSIX 0
#define LV_USE_FS_WIN32 0
#define LV_USE_GPU_STM32_DMA2D 0
#define LV_USE_GPU_NXP_PXP 0
#define LV_USE_GPU_NXP_VG_LITE 0

#endif
