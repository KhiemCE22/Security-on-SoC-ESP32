/**
 * Tên File: main.cpp
 * Mô tả: Mã nguồn Firmware Demo cho đề tài Bảo mật IoT (Secure Boot & Flash Encryption).
 * Chức năng:
 * 1. Kết nối WiFi và MQTT Server (chứa thông tin nhạy cảm cần bảo vệ).
 * 2. Hiển thị trạng thái lên màn hình LCD I2C.
 * 3. Gửi tin nhắn "heartbeat" định kỳ lên Server để chứng minh hệ thống hoạt động ổn định.
 */

#include <Arduino.h>
#include "esp_log.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ============================================================
// DỮ LIỆU NHẠY CẢM (SENSITIVE DATA / SECRETS)
// Lưu ý: Đây là các thông tin Hardcoded dạng Plaintext.
// Mục tiêu của Flash Encryption là mã hóa các chuỗi này trong bộ nhớ Flash
// để ngăn chặn hacker trích xuất (Dump Flash).
// ============================================================
#define WIFI_SSID       "OPPO Reno4"    // SSID Wifi (Bị lộ trong Hình 14 nếu chưa mã hóa)
#define WIFI_PASSWORD   "hcmutk22"      // Password Wifi (Bị lộ trong Hình 14 nếu chưa mã hóa)

#define MQTT_BROKER     "app.coreiot.io"
#define MQTT_PORT       1883
#define MQTT_USER       "iot_device_test"
#define MQTT_PASSWORD   "123456"        // Password MQTT
#define MQTT_CLIENT_ID  "IOT_DEVICE_TEST_SECURE"

// TOPICS
#define MQTT_TOPIC_PING       "v1/devices/me/telemetry"

static const char *TAG = "SECURE_APP";

// Cấu hình LCD I2C (Địa chỉ 0x27 hoặc 0x21 tùy mạch)
LiquidCrystal_I2C lcd(0x21, 16, 2);

// Khởi tạo MQTT Client
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ============================================================
// HÀM: KẾT NỐI WIFI
// ============================================================
void connectWiFi() {
    ESP_LOGI(TAG, "Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    // Chờ kết nối
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, ".");
    }
    
    ESP_LOGI(TAG, "WiFi connected!");
    // In ra IP để kiểm tra (cũng là dữ liệu có thể bị lộ nếu không bảo mật)
    ESP_LOGI(TAG, "IP address: %s", WiFi.localIP().toString().c_str());
}

// ============================================================
// HÀM: XỬ LÝ TIN NHẮN MQTT NHẬN ĐƯỢC (CALLBACK)
// ============================================================
void mqttCallback(char *topic, byte *payload, unsigned int length) {
    ESP_LOGI(TAG, "MQTT message on topic: %s", topic);
    
    // Cấp phát bộ nhớ động để xử lý payload
    char* payload_str = (char*)malloc(length + 1);
    if (!payload_str) {
        ESP_LOGE(TAG, "Failed to allocate memory for payload");
        return;
    }
    memcpy(payload_str, payload, length);
    payload_str[length] = '\0'; // Null-terminate string
    
    ESP_LOGI(TAG, "Payload: %s", payload_str);
    free(payload_str); // Giải phóng bộ nhớ tránh memory leak
}

// ============================================================
// HÀM: KẾT NỐI MQTT BROKER
// ============================================================
void connectMQTT() {
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);

    while (!mqttClient.connected()) {
        ESP_LOGI(TAG, "Connecting to MQTT...");
        
        // Thử kết nối với Client ID và User/Pass
        if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
            ESP_LOGI(TAG, "MQTT connected!");
        } else {
            // Nếu lỗi, in mã lỗi và thử lại sau 2 giây
            ESP_LOGE(TAG, "Failed, rc=%d. Retrying in 2 seconds...", mqttClient.state());
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }
}

// ============================================================
// HÀM: SETUP (KHỞI TẠO HỆ THỐNG)
// ============================================================
void setup() {
    Serial.begin(115200);

    // 1. Khởi tạo màn hình LCD
    Wire.begin();              
    lcd.init();
    lcd.backlight();
    lcd.clear();

    // Hiển thị thông báo chào mừng lên LCD
    lcd.setCursor(0, 0);
    lcd.print("Secure Boot App");
    lcd.setCursor(0, 1);
    lcd.print("System Ready");

    // 2. Thực hiện kết nối mạng
    connectWiFi();
    connectMQTT();

    // 3. LOG QUAN TRỌNG: Đánh dấu hệ thống đã vượt qua Secure Boot check
    // Dòng này được sử dụng trong Hình 11 và Hình 20 của báo cáo 
    // để minh chứng Firmware đã được xác thực và chạy thành công.
    ESP_LOGI(TAG, "System ready for Secure Boot!");
}

// ============================================================
// HÀM: LOOP (VÒNG LẶP CHÍNH)
// ============================================================
void loop() {
    // Đảm bảo kết nối MQTT luôn duy trì
    if (!mqttClient.connected()) {
        connectMQTT();
    }
    mqttClient.loop();

    // Gửi heartbeat mỗi 5 giây để báo hiệu thiết bị "Alive"
    static unsigned long lastPing = 0;
    unsigned long now = millis();
    if (now - lastPing > 5000) {
        lastPing = now;
        // Payload JSON đơn giản
        const char *payload = "{\"status\":\"alive\",\"app\":\"secure-boot-test\"}";
        mqttClient.publish(MQTT_TOPIC_PING, payload);
        ESP_LOGI(TAG, "MQTT ping: %s", payload);
    }

    // Hiệu ứng nhấp nháy ký tự trên LCD để biết chương trình không bị treo
    static bool toggle = false;
    lcd.setCursor(15, 1);
    lcd.print(toggle ? "*" : " ");
    toggle = !toggle;

    delay(500);
}

// ============================================================
// APP MAIN: CẦU NỐI GIỮA ESP-IDF VÀ ARDUINO CORE
// Vì project dùng ESP-IDF framework nhưng viết code kiểu Arduino,
// cần hàm này để khởi chạy môi trường Arduino.
// ============================================================
extern "C" void app_main() {
    initArduino();       // Khởi tạo Arduino Core
    setup();             // Gọi hàm setup() của người dùng
    while (true) {
        loop();          // Gọi hàm loop() liên tục
        vTaskDelay(1 / portTICK_PERIOD_MS); // Nhường CPU cho các tác vụ FreeRTOS nền (Watchdog, WiFi...)
    }
}