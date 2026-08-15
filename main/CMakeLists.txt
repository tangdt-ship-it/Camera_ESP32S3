idf_component_register(
    SRCS
        "main.c"
        "camera_driver.c"
        "vision.c"
        "wifi_ap.c"
        "web_server.c"
    INCLUDE_DIRS "."
    REQUIRES
        esp_wifi
        esp_event
        esp_netif
        nvs_flash
        esp_http_server
        espressif__esp32-camera
)
