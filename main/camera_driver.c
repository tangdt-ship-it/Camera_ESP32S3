#include "camera_driver.h"
#include "camera_board.h"

#include "esp_log.h"

static const char *TAG = "camera_driver";

esp_err_t camera_start(void)
{
    camera_config_t config = {
        .pin_pwdn = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SIOD,
        .pin_sccb_scl = CAM_PIN_SIOC,

        .pin_d7 = CAM_PIN_D7,
        .pin_d6 = CAM_PIN_D6,
        .pin_d5 = CAM_PIN_D5,
        .pin_d4 = CAM_PIN_D4,
        .pin_d3 = CAM_PIN_D3,
        .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1,
        .pin_d0 = CAM_PIN_D0,

        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,

        .xclk_freq_hz = CAM_XCLK_HZ,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = CAM_PIXEL_FORMAT,
        .frame_size = CAM_FRAME_SIZE,
        .jpeg_quality = 12,
        .fb_count = CAM_FB_COUNT,
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    };

    ESP_LOGI(TAG, "Khoi tao OV7670...");
    ESP_LOGI(TAG, "XCLK=%d Hz, RGB565, QVGA", CAM_XCLK_HZ);

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_camera_init failed: 0x%x (%s)", err, esp_err_to_name(err));
        return err;
    }

    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor) {
        ESP_LOGI(TAG, "Camera PID=0x%02x VER=0x%02x MIDH=0x%02x MIDL=0x%02x",
                 sensor->id.PID, sensor->id.VER, sensor->id.MIDH, sensor->id.MIDL);

        /* Các thiết lập này được driver map theo khả năng của sensor. */
        if (sensor->set_brightness) sensor->set_brightness(sensor, 0);
        if (sensor->set_contrast)   sensor->set_contrast(sensor, 0);
        if (sensor->set_saturation) sensor->set_saturation(sensor, 0);
    }

    /* ESP32-S3 hỗ trợ DMA thẳng tới PSRAM. */
    err = esp_camera_set_psram_mode(true);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "PSRAM DMA enabled");
    } else {
        ESP_LOGW(TAG, "Khong bat duoc PSRAM DMA: %s", esp_err_to_name(err));
    }

    return ESP_OK;
}

camera_fb_t *camera_acquire(void)
{
    return esp_camera_fb_get();
}

void camera_release(camera_fb_t *fb)
{
    if (fb) {
        esp_camera_fb_return(fb);
    }
}
