#include "vision.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#define GRID_W 16
#define GRID_H 12
#define GRID_N (GRID_W * GRID_H)

static uint8_t s_prev_grid[GRID_N];
static bool s_have_prev = false;
static vision_stats_t s_stats;
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

static inline uint8_t expand5(uint8_t v)
{
    return (uint8_t)((v << 3) | (v >> 2));
}

static inline uint8_t expand6(uint8_t v)
{
    return (uint8_t)((v << 2) | (v >> 4));
}

void vision_init(void)
{
    memset(s_prev_grid, 0, sizeof(s_prev_grid));
    memset(&s_stats, 0, sizeof(s_stats));
    s_have_prev = false;
}

void vision_process(const camera_fb_t *fb)
{
    if (!fb || fb->format != PIXFORMAT_RGB565 || fb->width == 0 || fb->height == 0) {
        return;
    }

    const uint8_t *buf = fb->buf;
    uint8_t now_grid[GRID_N];

    uint32_t sum_r = 0;
    uint32_t sum_g = 0;
    uint32_t sum_b = 0;
    uint32_t sum_y = 0;
    uint32_t diff_sum = 0;
    uint32_t idx = 0;

    /*
     * Lấy 16x12 điểm mẫu. OV7670 phát byte cao trước byte thấp đối với RGB565.
     * Việc lấy mẫu nhẹ giúp còn tài nguyên cho Wi-Fi và AI/robot về sau.
     */
    for (uint32_t gy = 0; gy < GRID_H; ++gy) {
        uint32_t y = ((gy * 2 + 1) * fb->height) / (GRID_H * 2);

        for (uint32_t gx = 0; gx < GRID_W; ++gx) {
            uint32_t x = ((gx * 2 + 1) * fb->width) / (GRID_W * 2);
            size_t off = ((size_t)y * fb->width + x) * 2;

            uint16_t p = ((uint16_t)buf[off] << 8) | buf[off + 1];

            uint8_t r = expand5((p >> 11) & 0x1F);
            uint8_t g = expand6((p >> 5) & 0x3F);
            uint8_t b = expand5(p & 0x1F);

            uint8_t y8 = (uint8_t)((77u * r + 150u * g + 29u * b) >> 8);

            now_grid[idx] = y8;
            sum_r += r;
            sum_g += g;
            sum_b += b;
            sum_y += y8;

            if (s_have_prev) {
                int d = (int)y8 - (int)s_prev_grid[idx];
                diff_sum += (uint32_t)(d < 0 ? -d : d);
            }
            idx++;
        }
    }

    vision_stats_t next;
    next.frames = s_stats.frames + 1;
    next.mean_r = (float)sum_r / GRID_N;
    next.mean_g = (float)sum_g / GRID_N;
    next.mean_b = (float)sum_b / GRID_N;
    next.brightness = (float)sum_y / GRID_N;
    next.motion = s_have_prev ? (float)diff_sum / GRID_N : 0.0f;

    memcpy(s_prev_grid, now_grid, sizeof(now_grid));
    s_have_prev = true;

    portENTER_CRITICAL(&s_mux);
    s_stats = next;
    portEXIT_CRITICAL(&s_mux);
}

vision_stats_t vision_get_stats(void)
{
    vision_stats_t copy;
    portENTER_CRITICAL(&s_mux);
    copy = s_stats;
    portEXIT_CRITICAL(&s_mux);
    return copy;
}
