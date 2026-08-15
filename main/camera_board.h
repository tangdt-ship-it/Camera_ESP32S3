#pragma once

#include "esp_camera.h"

/*
 * ESP32-S3 N16R8 <-> OV7670 (no FIFO)
 *
 * Chọn GPIO4..17 liên tục để dễ đi dây và tránh các chân đang thường dùng
 * cho USB/UART hoặc PSRAM/Flash trên nhiều board ESP32-S3.
 */

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

/* RESET nối 3V3, PWDN nối GND nên không cần GPIO điều khiển. */
#define CAM_PIN_RESET   -1
#define CAM_PIN_PWDN    -1

/*
 * Datasheet OV7670 cho phép XCLK 10..48 MHz.
 * 10 MHz ưu tiên độ ổn định khi thử bằng dây Dupont.
 */
#define CAM_XCLK_HZ     10000000

/*
 * OV7670 không có JPEG encoder.
 * RGB565 thuận tiện cho xử lý ảnh trực tiếp.
 */
#define CAM_PIXEL_FORMAT PIXFORMAT_RGB565
#define CAM_FRAME_SIZE   FRAMESIZE_QVGA
#define CAM_FB_COUNT     1
