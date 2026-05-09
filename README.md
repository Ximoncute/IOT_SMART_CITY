# 🏙️ SmartCity Monitoring System

Hệ thống giám sát khí gas và âm thanh dùng ESP8266, MQTT và Web Dashboard.

## 🔧 Tính năng

- Đọc cảm biến khí gas MQ2 (Digital D0-GPIO16) → phát hiện gas → bật còi (D5-GPIO14)
- Đọc cảm biến âm thanh MAX9814 (A0) → vượt ngưỡng 600 → bật LED (D8-GPIO15)
- Hiển thị OLED I2C 0.96" (SDA-GPIO4, SCL-GPIO5)
- Điều khiển qua MQTT (EMQX broker)
- Web Dashboard: biểu đồ, log, xuất CSV, gửi tin nhắn OLED
- 2 chế độ: Auto (tự động) / Manual (bấm web điều khiển)

## 📌 Sơ đồ kết nối
```
ESP8266 → Linh kiện
GPIO16 (D0) → MQ2 (DO)
A0 → MAX9814 (OUT)
GPIO14 (D5) → Buzzer (+)
GPIO15 (D8) → LED (+) + trở 220Ω
GPIO4 (D2) → OLED SDA
GPIO5 (D1) → OLED SCL
3.3V/5V → VCC
GND → GND
```


## 🚀 Cài đặt

### Arduino IDE
1. Cài board ESP8266 (Boards Manager → esp8266)
2. Cài thư viện: `Adafruit GFX`, `Adafruit SSD1306`, `PubSubClient`, `ArduinoJson`
3. Sửa WiFi trong code:
```cpp
const char* ssid = "WIFI_CUA_BAN";
const char* password = "MAT_KHAU";
```
### Web Dashboard
```
Mở file dashboard.html bằng Chrome/Edge

Đợi hiển thị "🟢 CONNECTED"

Gửi tin nhắn, bấm nút điều khiển, xuất CSV
```
### 👤 Tác giả : Ximoncute - Hiếu Ka Ka