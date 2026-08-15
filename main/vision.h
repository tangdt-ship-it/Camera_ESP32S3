#pragma once

#include <stdint.h>
#include "esp_camera.h"

typedef struct {
    uint32_t frames;
    float brightness;
    float mean_r;
    float mean_g;
    float mean_b;
    float motion;
} vision_stats_t;

void vision_init(void);
void vision_process(const camera_fb_t *fb);
vision_stats_t vision_get_stats(void);
