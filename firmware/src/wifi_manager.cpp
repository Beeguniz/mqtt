#include "wifi_manager.h"
#include "config.h"
#include <WiFi.h>

// ============================================================
// QUẢN LÝ KẾT NỐI WIFI - KẾT NỐI VÀ TỰ ĐỘNG PHỤC HỒI
// ============================================================

// Cờ trạng thái kết nối WiFi - được FSM task theo dõi
bool wifiConnected = false;

/**
 * @brief Thiết lập và kết nối WiFi ban đầu
 * 
 * Cấu hình ESP32 ở chế độ Station (STA), thực hiện
 * kết nối WiFi với số lần thử tối đa WIFI_MAX_RETRIES.
 */
void setupWiFi() {
    Serial.println("[WIFI] Đang kết nối WiFi...");
    Serial.printf("[WIFI] SSID: %s\n", WIFI_SSID);

    // Cấu hình chế độ Station (kết nối tới Access Point)
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Chờ kết nối với số lần thử giới hạn
    int retryCount = 0;
    while (WiFi.status() != WL_CONNECTED && retryCount < WIFI_MAX_RETRIES) {
        delay(500);
        Serial.print(".");
        retryCount++;
    }
    Serial.println();

    // Kiểm tra kết quả kết nối
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Serial.println("[WIFI] Kết nối WiFi thành công!");
        Serial.printf("[WIFI] Địa chỉ IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[WIFI] Cường độ tín hiệu (RSSI): %d dBm\n", WiFi.RSSI());
    } else {
        wifiConnected = false;
        Serial.println("[WIFI] CẢNH BÁO: Không thể kết nối WiFi khi khởi động.");
        Serial.println("[WIFI] Task WiFi sẽ tiếp tục thử kết nối lại...");
    }
}

/**
 * @brief Task giám sát WiFi - chạy liên tục trên FreeRTOS
 * 
 * Thực hiện:
 * 1. Kiểm tra trạng thái kết nối WiFi định kỳ
 * 2. Tự động kết nối lại nếu mất kết nối
 * 3. Cập nhật cờ wifiConnected cho FSM task
 */
void wifiTask(void* pvParameters) {
    Serial.println("[WIFI] Task giám sát WiFi đã khởi động.");

    for (;;) {
        if (WiFi.status() == WL_CONNECTED) {
            // WiFi đang kết nối bình thường
            if (!wifiConnected) {
                // Vừa kết nối lại thành công
                Serial.println("[WIFI] Đã kết nối lại WiFi thành công!");
                Serial.printf("[WIFI] Địa chỉ IP: %s\n", WiFi.localIP().toString().c_str());
            }
            wifiConnected = true;
        } else {
            // WiFi bị mất kết nối
            if (wifiConnected) {
                // Vừa mất kết nối
                Serial.println("[WIFI] CẢNH BÁO: Mất kết nối WiFi!");
            }
            wifiConnected = false;

            // Thử kết nối lại
            Serial.println("[WIFI] Đang thử kết nối lại WiFi...");
            WiFi.disconnect();          // Ngắt kết nối cũ
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  // Kết nối lại

            // Chờ kết nối (tối đa 10 giây)
            int waitCount = 0;
            while (WiFi.status() != WL_CONNECTED && waitCount < 20) {
                vTaskDelay(pdMS_TO_TICKS(500));
                waitCount++;
            }

            if (WiFi.status() == WL_CONNECTED) {
                wifiConnected = true;
                Serial.println("[WIFI] Kết nối lại WiFi thành công!");
                Serial.printf("[WIFI] Địa chỉ IP: %s\n", WiFi.localIP().toString().c_str());
            } else {
                Serial.println("[WIFI] Kết nối lại WiFi thất bại. Sẽ thử lại...");
            }
        }

        // Kiểm tra mỗi 10 giây
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

