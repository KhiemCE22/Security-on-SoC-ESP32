#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ⚙️ WiFi & MQTT (giữ để sau dùng OTA)
#define WIFI_SSID       "OPPO Reno4"
#define WIFI_PASSWORD   "hcmutk22"

#define MQTT_BROKER     "app.coreiot.io"
#define MQTT_PORT       1883
#define MQTT_USER       "iot_device_test"
#define MQTT_PASSWORD   "123456"
#define MQTT_CLIENT_ID  "IOT_DEVICE_TEST_OTA"

// Nếu sau này cần topic riêng cho OTA thì đổi ở đây
#define MQTT_TOPIC_PING "v1/devices/me/telemetry"

// LCD I2C: giữ đúng như code cũ của bạn
LiquidCrystal_I2C lcd(0x21, 16, 2);

// MQTT client (Arduino-style)
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ===============================
// 🔹 WIFI + MQTT
// ===============================
void connectWiFi() {
    Serial.print("🔗 Kết nối WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\n✅ WiFi đã kết nối!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    // Tạm thời chưa xử lý gì – sau này bạn có thể parse JSON cho OTA command
    Serial.print("📩 MQTT message on [");
    Serial.print(topic);
    Serial.print("]: ");
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

void connectMQTT() {
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);

    while (!mqttClient.connected()) {
        Serial.print("🔌 Kết nối MQTT...");
        if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
            Serial.println("✅ MQTT đã kết nối!");
            // Sau này nếu có topic OTA thì subscribe ở đây
            // mqttClient.subscribe("v1/devices/me/rpc/request/+");
        } else {
            Serial.print("❌ Thất bại, state = ");
            Serial.println(mqttClient.state());
            delay(2000);
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
    lcd.print("Hello world!");
    lcd.setCursor(0, 1);
    lcd.print("OTA test ready");

    // WiFi + MQTT
    connectWiFi();
    connectMQTT();

    Serial.println("🚀 System ready for OTA tests!");
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
        const char *payload = "{\"status\":\"alive\",\"app\":\"ota-test\"}";
        mqttClient.publish(MQTT_TOPIC_PING, payload);
        Serial.println(String("📡 MQTT ping: ") + payload);
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
