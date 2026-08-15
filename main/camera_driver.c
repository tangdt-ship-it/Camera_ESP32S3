#include "camera_driver.h"
#include "camera_board.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "img_converters.h"

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
        .grab_mode = CAMERA_GRAB_LATEST,
    };

    ESP_LOGI(TAG, "OV7670 V1.2 color-correction profile");
    ESP_LOGI(TAG, "XCLK=%d Hz RGB565 QVGA fb_count=%d grab=LATEST",
             CAM_XCLK_HZ, CAM_FB_COUNT);

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_camera_init failed: 0x%x (%s)", err, esp_err_to_name(err));
        return err;
    }

    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor) {
        ESP_LOGI(TAG, "Camera PID=0x%02x VER=0x%02x MIDH=0x%02x MIDL=0x%02x",
                 sensor->id.PID, sensor->id.VER, sensor->id.MIDH, sensor->id.MIDL);

        /* Keep automatic controls enabled and apply only mild image tuning. */
        if (sensor->set_whitebal)      sensor->set_whitebal(sensor, 1);
        if (sensor->set_awb_gain)      sensor->set_awb_gain(sensor, 1);
        if (sensor->set_exposure_ctrl) sensor->set_exposure_ctrl(sensor, 1);
        if (sensor->set_gain_ctrl)     sensor->set_gain_ctrl(sensor, 1);
        if (sensor->set_brightness)    sensor->set_brightness(sensor, 0);
        if (sensor->set_contrast)      sensor->set_contrast(sensor, 1);
        if (sensor->set_saturation)    sensor->set_saturation(sensor, -1);
    }

    /*
     * Critical OV7670 RGB565 fix:
     * the framebuffer produced by this ESP32-S3/OV7670 combination is
     * interpreted as little-endian RGB565 by the ESP32 camera path. The JPEG
     * converter defaults to big-endian input, which produces a strong green
     * cast and unrealistic colours. Match the converter to the actual buffer.
     */
    jpgSetRgb565BE(false);
    ESP_LOGI(TAG, "RGB565 JPEG byte order: LITTLE-ENDIAN");

    /* CONFIG_CAMERA_PSRAM_DMA is already enabled in sdkconfig.defaults.
       Do not call esp_camera_set_psram_mode(true) here because that function
       reconfigures the whole camera a second time. */
    ESP_LOGI(TAG, "PSRAM DMA configured at build time; no second camera init");

    /* Let auto exposure / gain / white balance settle before streaming. */
    for (int i = 0; i < 3; ++i) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb) {
            esp_camera_fb_return(fb);
        }
    }
    vTaskDelay(pdMS_TO_TICKS(120));

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
