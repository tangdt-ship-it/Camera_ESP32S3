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

    ESP_LOGI(TAG, "OV7670 V1.3 natural-color profile");
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

        /* OV7670 in esp32-camera 2.1.7 only implements these image controls.
           Brightness/contrast/saturation/sharpness/AWB-gain/WB-mode are dummy
           functions for OV7670, so do not call them and pretend they changed
           the image. Keep the three real automatic controls enabled. */
        if (sensor->set_whitebal)      sensor->set_whitebal(sensor, 1);
        if (sensor->set_exposure_ctrl) sensor->set_exposure_ctrl(sensor, 1);
        if (sensor->set_gain_ctrl)     sensor->set_gain_ctrl(sensor, 1);
    }

    /* esp32-camera's JPEG encoder defaults to big-endian RGB565, and the
       official documentation says this matches most camera frame buffers.
       V1.2 forced little-endian and produced the psychedelic false colours
       seen on the user's screen. Restore the correct/default interpretation. */
    jpgSetRgb565BE(true);
    ESP_LOGI(TAG, "RGB565 JPEG byte order: BIG-ENDIAN (driver default)");

    /* CONFIG_CAMERA_PSRAM_DMA is enabled at build time. Do not call
       esp_camera_set_psram_mode(true), because it reconfigures the camera. */
    ESP_LOGI(TAG, "PSRAM DMA configured at build time; no second camera init");

    /* Give AWB/AEC/AGC several frames to settle before the web stream starts. */
    for (int i = 0; i < 5; ++i) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb) {
            esp_camera_fb_return(fb);
        }
    }
    vTaskDelay(pdMS_TO_TICKS(150));

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
