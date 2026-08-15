#pragma once

#include "esp_camera.h"

/* ESP32-S3 N16R8 <-> OV7670 (no FIFO) */
#define CAM_PIN_D0       4
#define CAM_PIN_D1       5
#define CAM_PIN_D2       6
#define CAM_PIN_D3       7
#define CAM_PIN_D4       8
#define CAM_PIN_D5       9
#define CAM_PIN_D6      10
#define CAM_PIN_D7      11

#define CAM_PIN_PCLK    12
#define CAM_PIN_VSYNC   13
#define CAM_PIN_HREF    14
#define CAM_PIN_XCLK    15
#define CAM_PIN_SIOD    16
#define CAM_PIN_SIOC    17

#define CAM_PIN_RESET   -1
#define CAM_PIN_PWDN    -1

/* Keep the known-good timing from the original working project. */
#define CAM_XCLK_HZ     10000000
#define CAM_PIXEL_FORMAT PIXFORMAT_RGB565
#define CAM_FRAME_SIZE   FRAMESIZE_QVGA

/* Two frame buffers + GRAB_LATEST reduce visible latency on the web stream. */
#define CAM_FB_COUNT     2

/* Software JPEG settings. OV7670 has no hardware JPEG encoder. */
#define CAM_STREAM_JPEG_QUALITY 55
#define CAM_CAPTURE_JPEG_QUALITY 85
