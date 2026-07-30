#ifndef CONFIG_SPU_H
#define CONFIG_SPU_H

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════════════
//                              DEVICE & SYSTEM IDENTIFICATION
// ═══════════════════════════════════════════════════════════════════════════════════

#define SPU_DEVICE_ID           3                // Unique sensor node ID
#define SPU_FIRMWARE_VERSION    "v1.0-SPU"       // Firmware version
#define SPU_DEBUG_ENABLE        true             // Enable Serial Debug output

// Wi-Fi SoftAP & Local Web Server Configuration
#define ENABLE_WEB_SERVER       true             // Host local web dashboard on ESP32
#define WIFI_AP_SSID            "LifeLine-Sensor-Node" // Access Point SSID
#define WIFI_AP_PASS            "lifeline123"    // Access Point Password
#define WEB_SERVER_PORT         80               // HTTP Port 80

// Wi-Fi Station & Remote Server Telemetry Configuration
#define ENABLE_WIFI_STA         true             // Enable Wi-Fi Station mode to connect to router/hotspot
#define WIFI_STA_SSID           "LifeLine-Network" // Target Wi-Fi SSID
#define WIFI_STA_PASS           "lifeline2026"   // Target Wi-Fi Password

#define ENABLE_SERVER_UPLOAD    true             // Transmit telemetry to remote server
#define SERVER_TELEMETRY_URL    "http://192.168.1.100:5000/api/telemetry" // Server HTTP POST endpoint
#define SERVER_UPLOAD_INTERVAL  2000             // ms - Upload interval to server

// ═══════════════════════════════════════════════════════════════════════════════════
//                              PIN DEFINITIONS (ESP32 #1 SPU)
// ═══════════════════════════════════════════════════════════════════════════════════

// Hardware UART2 for SPU -> CCU Communication
#define UART_CCU_TX_PIN         17               // SPU TX2 -> CCU RX2 (GPIO 16)
#define UART_CCU_RX_PIN         16               // SPU RX2 -> CCU TX2 (GPIO 17)
#define UART_CCU_BAUD           115200           // High-speed UART link

// Hardware UART1 for GPS (NEO-6M)
#define GPS_TX_PIN              15               // ESP32 RX1 connected to GPS TX
#define GPS_RX_PIN              4                // ESP32 TX1 connected to GPS RX
#define GPS_BAUD                9600             // Standard NMEA baudrate

// Environmental Sensor Pin (DHT11 / DHT22)
#define DHT_PIN                 23               // Digital pin for DHT sensor
#define DHT_TYPE                DHT11            // DHT11 or DHT22

// I2C Bus Pins (MPU6050 Accelerometer / Gyro)
#define I2C_SDA_PIN             21               // SDA
#define I2C_SCL_PIN             22               // SCL

// Analog Sensor Pins
#define MQ135_PIN               34               // Analog pin for MQ135 Gas Sensor

// Digital Inputs & Controls
#define STATUS_LED_PIN          2                // Built-in status LED indicator

// ═══════════════════════════════════════════════════════════════════════════════════
//                              THRESHOLD CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════════

// Environmental Thresholds
#define THRESHOLD_HEAT_TEMP     45.0f            // °C - Extreme heat wave
#define THRESHOLD_COLD_TEMP      0.0f            // °C - Extreme cold wave
#define THRESHOLD_HIGH_HUMIDITY 85.0f            // %  - High humidity
#define THRESHOLD_DRY_HUMIDITY  20.0f            // %  - Dry environment

// Air Quality / Gas Thresholds
#define THRESHOLD_GAS_WARNING   300              // PPM - Air pollution / Poor air quality
#define THRESHOLD_GAS_DANGER    600              // PPM - High gas leak / Smoke
#define THRESHOLD_FIRE_TEMP     42.0f            // °C - Temp component for fire fusion

// Motion & Seismic Thresholds (MPU6050)
#define MPU_MOVING_AVG_WINDOW   10               // Moving average window size
#define THRESHOLD_FREE_FALL_G   0.3f             // G force below which free fall is triggered
#define THRESHOLD_IMPACT_G      3.0f             // G force spike for sudden impact / fall
#define THRESHOLD_TILT_DEG      60.0f            // Tilt angle threshold in degrees
#define THRESHOLD_VIBE_EARTHQ   1.8f             // Acceleration magnitude G for earthquake vibration
#define EARTHQUAKE_VIBE_WINDOW  15               // Consecutive samples above threshold for Earthquake

// ═══════════════════════════════════════════════════════════════════════════════════
//                              TIMING & SAMPLING INTERVALS
// ═══════════════════════════════════════════════════════════════════════════════════

#define SENSOR_SAMPLE_INTERVAL  200              // ms - High frequency sensor loop (5 Hz)
#define TELEMETRY_SEND_INTERVAL 2000             // ms - Periodic sensor telemetry broadcast (Every 2 seconds)
#define EMERGENCY_SEND_INTERVAL 500              // ms - Rapid emergency dispatch interval (0.5s)

#endif // CONFIG_SPU_H
