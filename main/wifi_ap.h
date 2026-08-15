#pragma once

#include "esp_err.h"

#define WIFI_AP_SSID "ESP32S3-OV7670"
#define WIFI_AP_PASS "12345678"

esp_err_t wifi_ap_start(void);
