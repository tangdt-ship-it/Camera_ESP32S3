#include "web_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "img_converters.h"

#include "camera_driver.h"
#include "vision.h"

static const char *TAG = "web_server";

static httpd_handle_t s_httpd = NULL;
static httpd_handle_t s_stream_httpd = NULL;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *STREAM_CONTENT_TYPE =
    "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *STREAM_BOUNDARY =
    "\r\n--" PART_BOUNDARY "\r\n";
static const char *STREAM_PART =
    "Content-Type: image/jpeg\r\nContent-Length: %zu\r\n\r\n";

static const char INDEX_HTML[] =
"<!doctype html>"
"<html lang='vi'>"
"<head>"
"<meta charset='utf-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>ESP32-S3 OV7670 AI Vision</title>"
"<style>"
"body{font-family:Arial,sans-serif;background:#111827;color:#e5e7eb;margin:0;padding:20px}"
".wrap{max-width:900px;margin:auto}"
".card{background:#1f2937;border-radius:14px;padding:16px;margin-bottom:16px;box-shadow:0 8px 24px #0004}"
"h1{font-size:24px;margin:0 0 6px}.sub{color:#9ca3af;margin-bottom:16px}"
"img{width:100%;max-width:640px;image-rendering:auto;border-radius:10px;background:#000}"
".grid{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;margin-top:14px}"
".box{background:#111827;padding:12px;border-radius:10px;text-align:center}"
".v{font-size:24px;font-weight:700}.k{font-size:12px;color:#9ca3af}"
"a{color:#93c5fd}.ok{color:#86efac}"
"@media(max-width:600px){.grid{grid-template-columns:repeat(2,1fr)}}"
"</style>"
"</head>"
"<body><div class='wrap'>"
"<div class='card'>"
"<h1>ESP32-S3 N16R8 + OV7670</h1>"
"<div class='sub'>AI Vision Starter · QVGA RGB565 · software JPEG stream</div>"
"<img id='cam' src='http://192.168.4.1:81/stream' alt='camera stream'>"
"<p><a href='/capture.jpg' target='_blank'>Chụp ảnh JPEG</a></p>"
"</div>"
"<div class='card'>"
"<div class='grid'>"
"<div class='box'><div id='br' class='v'>--</div><div class='k'>BRIGHTNESS</div></div>"
"<div class='box'><div id='mo' class='v'>--</div><div class='k'>MOTION</div></div>"
"<div class='box'><div id='fr' class='v'>--</div><div class='k'>FRAMES</div></div>"
"<div class='box'><div id='rr' class='v'>--</div><div class='k'>MEAN R</div></div>"
"<div class='box'><div id='gg' class='v'>--</div><div class='k'>MEAN G</div></div>"
"<div class='box'><div id='bb' class='v'>--</div><div class='k'>MEAN B</div></div>"
"</div>"
"<p class='sub'>Motion là độ chênh mức sáng trung bình của lưới 16×12 giữa hai khung hình liên tiếp.</p>"
"</div>"
"</div>"
"<script>"
"async function tick(){"
"try{let r=await fetch('/api/stats',{cache:'no-store'});let s=await r.json();"
"document.getElementById('br').textContent=s.brightness.toFixed(1);"
"document.getElementById('mo').textContent=s.motion.toFixed(1);"
"document.getElementById('fr').textContent=s.frames;"
"document.getElementById('rr').textContent=s.r.toFixed(0);"
"document.getElementById('gg').textContent=s.g.toFixed(0);"
"document.getElementById('bb').textContent=s.b.toFixed(0);"
"}catch(e){}setTimeout(tick,700)}tick();"
"</script></body></html>";

static esp_err_t index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t stats_handler(httpd_req_t *req)
{
    vision_stats_t s = vision_get_stats();

    char json[256];
    int n = snprintf(json, sizeof(json),
        "{\"frames\":%lu,\"brightness\":%.2f,\"r\":%.2f,\"g\":%.2f,\"b\":%.2f,\"motion\":%.2f}",
        (unsigned long)s.frames,
        s.brightness, s.mean_r, s.mean_g, s.mean_b, s.motion);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    return httpd_resp_send(req, json, n);
}

static esp_err_t capture_handler(httpd_req_t *req)
{
    camera_fb_t *fb = camera_acquire();
    if (!fb) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    vision_process(fb);

    uint8_t *jpg_buf = NULL;
    size_t jpg_len = 0;

    bool ok = frame2jpg(fb, 80, &jpg_buf, &jpg_len);
    camera_release(fb);

    if (!ok || !jpg_buf) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=ov7670.jpg");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");

    esp_err_t res = httpd_resp_send(req, (const char *)jpg_buf, jpg_len);
    free(jpg_buf);
    return res;
}

static esp_err_t stream_handler(httpd_req_t *req)
{
    esp_err_t res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
    if (res != ESP_OK) {
        return res;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");

    while (true) {
        camera_fb_t *fb = camera_acquire();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            return ESP_FAIL;
        }

        vision_process(fb);

        uint8_t *jpg_buf = NULL;
        size_t jpg_len = 0;

        bool ok = frame2jpg(fb, 70, &jpg_buf, &jpg_len);
        camera_release(fb);

        if (!ok || !jpg_buf) {
            ESP_LOGE(TAG, "Software JPEG conversion failed");
            free(jpg_buf);
            return ESP_FAIL;
        }

        char part_buf[96];
        int hlen = snprintf(part_buf, sizeof(part_buf), STREAM_PART, jpg_len);

        if (hlen <= 0 || hlen >= (int)sizeof(part_buf)) {
            free(jpg_buf);
            return ESP_FAIL;
        }

        res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, part_buf, hlen);
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char *)jpg_buf, jpg_len);
        }

        free(jpg_buf);

        if (res != ESP_OK) {
            break;
        }
    }

    return res;
}

esp_err_t web_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.ctrl_port = 32768;
    config.max_uri_handlers = 8;
    config.stack_size = 8192;

    esp_err_t err = httpd_start(&s_httpd, &config);
    if (err != ESP_OK) {
        return err;
    }

    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_handler,
        .user_ctx = NULL
    };

    httpd_uri_t capture_uri = {
        .uri = "/capture.jpg",
        .method = HTTP_GET,
        .handler = capture_handler,
        .user_ctx = NULL
    };

    httpd_uri_t stats_uri = {
        .uri = "/api/stats",
        .method = HTTP_GET,
        .handler = stats_handler,
        .user_ctx = NULL
    };

    ESP_ERROR_CHECK(httpd_register_uri_handler(s_httpd, &index_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_httpd, &capture_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_httpd, &stats_uri));

    /*
     * Stream dùng server riêng để vòng lặp MJPEG không chặn giao diện/API.
     */
    httpd_config_t stream_config = HTTPD_DEFAULT_CONFIG();
    stream_config.server_port = 81;
    stream_config.ctrl_port = 32769;
    stream_config.stack_size = 10240;
    stream_config.max_open_sockets = 2;

    err = httpd_start(&s_stream_httpd, &stream_config);
    if (err != ESP_OK) {
        return err;
    }

    httpd_uri_t stream_uri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = NULL
    };

    ESP_ERROR_CHECK(httpd_register_uri_handler(s_stream_httpd, &stream_uri));

    ESP_LOGI(TAG, "Web UI : http://192.168.4.1");
    ESP_LOGI(TAG, "Stream : http://192.168.4.1:81/stream");

    return ESP_OK;
}
