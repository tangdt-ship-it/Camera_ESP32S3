# ESP32-S3 N16R8 + OV7670 — AI Vision Starter

Project ESP-IDF hoàn chỉnh để thử camera OV7670 không FIFO với ESP32-S3 N16R8.

## Chức năng

- Nhận ảnh song song DVP 8-bit từ OV7670 bằng driver chính thức `espressif/esp32-camera`.
- Dùng PSRAM 8 MB.
- Camera chạy mặc định ở QVGA 320x240, RGB565.
- ESP32-S3 tự tạo Wi-Fi Access Point, không cần router.
- Web UI tại `http://192.168.4.1`.
- MJPEG stream tại cổng 81.
- Chụp ảnh JPEG tại `/capture.jpg`.
- Xử lý ảnh cơ bản trực tiếp trên ESP32:
  - độ sáng trung bình;
  - trung bình R/G/B;
  - chỉ số thay đổi hình ảnh (motion score).
- Kiến trúc tách module để sau này thêm nhận dạng vật thể, tracking, gửi ảnh lên AI server hoặc tích hợp robot.

## 1. Kết nối OV7670

> Dùng module OV7670 loại **không FIFO** với các chân D0..D7, PCLK, VSYNC, HREF, XCLK, SIOD, SIOC.

| OV7670 | ESP32-S3 |
|---|---:|
| VCC / 3.3V | 3V3 |
| GND | GND |
| D0 | GPIO4 |
| D1 | GPIO5 |
| D2 | GPIO6 |
| D3 | GPIO7 |
| D4 | GPIO8 |
| D5 | GPIO9 |
| D6 | GPIO10 |
| D7 | GPIO11 |
| PCLK | GPIO12 |
| VSYNC | GPIO13 |
| HREF | GPIO14 |
| XCLK | GPIO15 |
| SIOD / SDA | GPIO16 |
| SIOC / SCL | GPIO17 |
| RESET | 3V3 |
| PWDN | GND |

### Rất quan trọng

- **Không cấp 5V trực tiếp vào chân nguồn cảm biến OV7670 trần.**
- Project này giả định module dùng mức logic 3.3V và được cấp từ chân 3V3.
- Nối GND camera và ESP32-S3 thật chắc.
- Dây D0..D7, PCLK, HREF, VSYNC và XCLK nên ngắn, tốt nhất dưới khoảng 10–15 cm.
- Nếu ảnh nhiễu hoặc camera không nhận, giảm chiều dài dây trước khi sửa phần mềm.

## 2. Mở project

Khuyến nghị ESP-IDF 5.5.x.

```bash
cd ESP32S3_OV7670_AI_Vision
idf.py set-target esp32s3
idf.py reconfigure
idf.py build
```

Component Manager sẽ tự tải `espressif/esp32-camera` 2.1.7.

## 3. Nạp firmware

Ví dụ COM5:

```bash
idf.py -p COM5 flash monitor
```

Thoát monitor bằng `Ctrl+]`.

## 4. Xem camera

Sau khi ESP32-S3 khởi động:

1. Trên máy tính/điện thoại, kết nối Wi-Fi:
   - SSID: `ESP32S3-OV7670`
   - Password: `12345678`
2. Mở:
   - `http://192.168.4.1`

Web hiển thị video, thông số RGB, độ sáng và motion score.

## 5. API

- `GET /` — giao diện web.
- `GET /capture.jpg` — chụp một ảnh JPEG.
- `GET /api/stats` — JSON trạng thái xử lý ảnh.
- `GET http://192.168.4.1:81/stream` — MJPEG stream.

Ví dụ JSON:

```json
{
  "frames": 125,
  "brightness": 112.4,
  "r": 118.2,
  "g": 115.8,
  "b": 101.3,
  "motion": 8.7
}
```

## 6. Thay đổi độ phân giải

Trong `main/camera_board.h`:

```c
#define CAM_FRAME_SIZE FRAMESIZE_QVGA
```

Có thể thử `FRAMESIZE_VGA`, nhưng OV7670 không có JPEG phần cứng. ESP32-S3 phải nhận RGB/YUV rồi nén JPEG bằng phần mềm để truyền web, do đó VGA sẽ nặng hơn đáng kể.

## 7. Khi tích hợp vào robot AI

Project đang dùng GPIO4..17 liên tục để dây camera dễ kiểm tra. Khi ghép vào robot hiện có, chỉ cần sửa các `CAM_PIN_*` trong `main/camera_board.h`.

Lưu ý: OV7670 dùng tới 14 tín hiệu cho bus camera + SCCB/XCLK. Khi ghép đồng thời TFT, micro I2S, amplifier I2S, động cơ và cảm biến, cần lập lại toàn bộ bảng GPIO trước khi làm PCB.

## 8. Nếu camera báo không hỗ trợ

Kiểm tra theo thứ tự:

1. VCC đúng 3.3V, GND chung.
2. SIOD/SIOC đúng GPIO16/17.
3. D0..D7 có đúng thứ tự, không đảo D0 và D7.
4. XCLK có trên GPIO15.
5. RESET đang HIGH và PWDN LOW.
6. Dây PCLK/HREF/VSYNC ngắn.
7. Xóa build và cấu hình lại:

```bash
idf.py fullclean
idf.py set-target esp32s3
idf.py build
```

## 9. Cấu trúc project

```text
ESP32S3_OV7670_AI_Vision/
├── CMakeLists.txt
├── sdkconfig.defaults
├── README.md
└── main/
    ├── CMakeLists.txt
    ├── idf_component.yml
    ├── main.c
    ├── camera_board.h
    ├── camera_driver.c
    ├── camera_driver.h
    ├── vision.c
    ├── vision.h
    ├── wifi_ap.c
    ├── wifi_ap.h
    ├── web_server.c
    └── web_server.h
```
