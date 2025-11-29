#include <Arduino.h>
#include "esp_log.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <PubSubClient.h>

// WiFi & MQTT
#define WIFI_SSID       "OPPO Reno4"
#define WIFI_PASSWORD   "hcmutk22"

#define MQTT_BROKER     "app.coreiot.io"
#define MQTT_PORT       1883
#define MQTT_USER       "iot_device_test"
#define MQTT_PASSWORD   "123456"
#define MQTT_CLIENT_ID  "IOT_DEVICE_TEST_SECURE"

// TOPICS
#define MQTT_TOPIC_PING       "v1/devices/me/telemetry"

static const char *TAG = "SECURE_APP";

// LCD I2C
LiquidCrystal_I2C lcd(0x21, 16, 2);

// MQTT client
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ===============================
// 🔹 WIFI + MQTT
// ===============================
void connectWiFi() {
    ESP_LOGI(TAG, "Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, ".");
    }
    ESP_LOGI(TAG, "WiFi connected!");
    ESP_LOGI(TAG, "IP address: %s", WiFi.localIP().toString().c_str());
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    ESP_LOGI(TAG, "MQTT message on topic: %s", topic);
    char* payload_str = (char*)malloc(length + 1);
    if (!payload_str) {
        ESP_LOGE(TAG, "Failed to allocate memory for payload");
        return;
    }
    memcpy(payload_str, payload, length);
    payload_str[length] = '\0';
    ESP_LOGI(TAG, "Payload: %s", payload_str);
    free(payload_str);
}

void connectMQTT() {
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);

    while (!mqttClient.connected()) {
        ESP_LOGI(TAG, "Connecting to MQTT...");
        if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
            ESP_LOGI(TAG, "MQTT connected!");
            // Không cần subscribe topic OTA nữa
        } else {
            ESP_LOGE(TAG, "Failed, rc=%d. Retrying in 2 seconds...", mqttClient.state());
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }
}

// ===============================
// 🔹 APP SETUP / LOOP (Arduino style)
// ===============================
void setup() {
    Serial.begin(115200);

    // I2C + LCD
    Wire.begin();              // dùng default SDA/SCL, đỡ phải define chân
    lcd.init();
    lcd.backlight();
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Secure Boot App");
    lcd.setCursor(0, 1);
    lcd.print("System Ready");

    // WiFi + MQTT
    connectWiFi();
    connectMQTT();

    ESP_LOGI(TAG, "System ready for Secure Boot!");
}

void loop() {
    // Duy trì MQTT
    if (!mqttClient.connected()) {
        connectMQTT();
    }
    mqttClient.loop();

    // Gửi heartbeat đơn giản cho CoreIoT (không cần ArduinoJson)
    static unsigned long lastPing = 0;
    unsigned long now = millis();
    if (now - lastPing > 5000) {  // 5s một lần
        lastPing = now;
        const char *payload = "{\"status\":\"alive\",\"app\":\"secure-boot-test\"}";
        mqttClient.publish(MQTT_TOPIC_PING, payload);
        ESP_LOGI(TAG, "MQTT ping: %s", payload);
    }

    // Nhấp nháy 1 ký tự cuối dòng 2 cho vui
    static bool toggle = false;
    lcd.setCursor(15, 1);
    lcd.print(toggle ? "*" : " ");
    toggle = !toggle;

    delay(500);
}

// ===============================
// 🔹 Cầu nối sang ESP-IDF
// ===============================
extern "C" void app_main() {
    initArduino();   // bắt buộc để Arduino core hoạt động trong IDF
    setup();
    while (true) {
        loop();
        vTaskDelay(1 / portTICK_PERIOD_MS);  // nhường CPU cho FreeRTOS
    }
}
