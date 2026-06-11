#ifndef CONFIG_H
#define CONFIG_H

// WIFI_CONFIG
#define WIFI_DEFAULT_PASSWORD "1234567890"
#define WIFI_SSID       "FES_Device"
#define WIFI_CHANNEL    1
#define WIFI_MAX_CONN   1
#define WIFI_PASSWORD_MIN_LENGTH 8
#define WIFI_PASSWORD_MAX_LENGTH 63

// TCP_CONFIG
#define TCP_SERVER_PORT 5000

// GET_INFO / HANDSHAKE
// Расширенный payload рукопожатия (см. device_info_serialize):
// 6 байт версий + channel_count(1) + intensity_min/max(2) + freq_min/max(2)
// + pw_min/max(4) + state(1) = 16 байт
#define DEVICE_INFO_PAYLOAD_SIZE 16
#define FIRMWARE_VERSION_MAJOR 1
#define FIRMWARE_VERSION_MINOR 0
#define FIRMWARE_VERSION_PATCH 0
#define PROTOCOL_VERSION_MAJOR 1
#define PROTOCOL_VERSION_MINOR 0
#define PROTOCOL_VERSION_PATCH 0

#define PROTOCOL_SIGNATURE 0xAA55
#define PROTOCOL_MAX_PAYLOAD_SIZE 256

// ============================================================================
// АППАРАТНЫЙ МОДУЛЬ: I2C / PCA9685
// ============================================================================
#define PCA9685_SDA_GPIO        8
#define PCA9685_SCL_GPIO        9
#define PCA9685_I2C_FREQ_HZ     400000
#define PCA9685_I2C_ADDR        0x40
#define PCA9685_OSC_HZ          25000000
#define PCA9685_PWM_FREQ_HZ     1000
#define PCA9685_PWM_MAX         4095
#define PCA9685_I2C_TIMEOUT_MS  50

// ============================================================================
// СТИМУЛЯЦИЯ
// ============================================================================
#define FES_CHANNEL_COUNT       8
#define FES_DEAD_TIME_US        200

#define FES_INTENSITY_MIN       0
#define FES_INTENSITY_MAX       255
#define FES_FREQUENCY_MIN_HZ    1
#define FES_FREQUENCY_MAX_HZ    100
#define FES_PULSE_WIDTH_MIN_US  100
#define FES_PULSE_WIDTH_MAX_US  15000

#define FES_DEFAULT_FREQUENCY_HZ    50
#define FES_DEFAULT_PULSE_WIDTH_US  10000

// Задача движка стимуляции
#define FES_TASK_STACK_SIZE     4096
#define FES_TASK_PRIORITY       10
#define FES_TASK_CORE           1   // APP_CPU

// Маппинг канала ФЭС на выходы PCA9685 (IN1 = 2N, IN2 = 2N+1)
#define FES_CH_IN1(n)  ((uint8_t)(2 * (n)))
#define FES_CH_IN2(n)  ((uint8_t)(2 * (n) + 1))

// ============================================================================
// HEARTBEAT
// ============================================================================
#define HEARTBEAT_DEFAULT_INTERVAL_MS  500
#define HEARTBEAT_DEFAULT_TIMEOUT_MS   2000

#endif
