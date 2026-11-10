#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

// ============================================================
// QUẢN LÝ KẾT NỐI WIFI CHO ESP32
// ============================================================

// Cờ trạng thái kết nối WiFi (chia sẻ với FSM task)
extern bool wifiConnected;

/**
 * @brief Thiết lập và kết nối WiFi ban đầu
 * 
 * Thực hiện kết nối WiFi với số lần thử giới hạn.
 * Gọi một lần trong setup().
 */
void setupWiFi();

/**
 * @brief Task giám sát kết nối WiFi (chạy trên FreeRTOS)
 * 
 * Liên tục kiểm tra trạng thái WiFi và tự động
 * kết nối lại nếu mất kết nối.
 * 
 * @param pvParameters Tham số truyền vào task (không sử dụng)
 */
void wifiTask(void* pvParameters);

#endif // WIFI_MANAGER_H

