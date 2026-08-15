#include <stdio.h>

#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "camera_driver.h"
#include "vision.h"
#include "web_server.h"
#include "wifi_ap.h"

static const char *TAG = "main";

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(err);
    }

    vision_init();

    ESP_LOGI(TAG, "======================================");
    ESP_LOGI(TAG, " ESP32-S3 N16R8 + OV7670 V1.2");
    ESP_LOGI(TAG, " RGB565 color correction profile");
    ESP_LOGI(TAG, "======================================");

    ESP_ERROR_CHECK(wifi_ap_start());

    err = camera_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed: %s", esp_err_to_name(err));
        ESP_LOGW(TAG, "Wi-Fi remains available at 192.168.4.1");
    }

    ESP_ERROR_CHECK(web_server_start());

    ESP_LOGI(TAG, "READY");
    ESP_LOGI(TAG, "Connect Wi-Fi: %s", WIFI_AP_SSID);
    ESP_LOGI(TAG, "Password     : %s", WIFI_AP_PASS);
    ESP_LOGI(TAG, "Open browser : http://192.168.4.1");
}
