#include "pca9685.h"
#include "config.h"

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "pca9685";

// Регистры PCA9685
#define PCA9685_REG_MODE1     0x00
#define PCA9685_REG_MODE2     0x01
#define PCA9685_REG_LED0_ON_L 0x06
#define PCA9685_REG_PRESCALE  0xFE

// Биты MODE1
#define PCA9685_MODE1_RESTART 0x80
#define PCA9685_MODE1_AI      0x20  // авто-инкремент адреса регистра
#define PCA9685_MODE1_SLEEP   0x10

#define PCA9685_PRESCALE_MIN  3      // соответствует максимуму 1526 Гц

static i2c_master_bus_handle_t s_bus;
static i2c_master_dev_handle_t s_dev;
static bool s_ready;  // true только после успешной инициализации устройства

static esp_err_t pca9685_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t buffer[2] = { reg, value };
    return i2c_master_transmit(s_dev, buffer, sizeof(buffer), PCA9685_I2C_TIMEOUT_MS);
}

// Записывает 4 регистра LEDn_ON_L..LEDn_OFF_H одной транзакцией (нужен авто-инкремент).
static esp_err_t pca9685_write_led(uint8_t channel, uint16_t on, uint16_t off)
{
    uint8_t buffer[5] = {
        (uint8_t)(PCA9685_REG_LED0_ON_L + 4 * channel),
        (uint8_t)(on & 0xFF),
        (uint8_t)(on >> 8),
        (uint8_t)(off & 0xFF),
        (uint8_t)(off >> 8),
    };
    return i2c_master_transmit(s_dev, buffer, sizeof(buffer), PCA9685_I2C_TIMEOUT_MS);
}

// Вычисляет prescale из частоты осциллятора и целевой частоты ШИМ и применяет его.
// Требует перевода в SLEEP (бит SLEEP), записи PRESCALE, пробуждения и RESTART.
static esp_err_t pca9685_set_prescale(void)
{
    uint32_t prescale = (PCA9685_OSC_HZ + (4096UL * PCA9685_PWM_FREQ_HZ) / 2) /
                        (4096UL * PCA9685_PWM_FREQ_HZ);
    if (prescale > 0) {
        prescale -= 1;
    }
    if (prescale < PCA9685_PRESCALE_MIN) {
        prescale = PCA9685_PRESCALE_MIN;
    }

    esp_err_t err = pca9685_write_reg(PCA9685_REG_MODE1, PCA9685_MODE1_SLEEP);
    if (err != ESP_OK) {
        return err;
    }
    err = pca9685_write_reg(PCA9685_REG_PRESCALE, (uint8_t)prescale);
    if (err != ESP_OK) {
        return err;
    }
    // Пробуждение + авто-инкремент
    err = pca9685_write_reg(PCA9685_REG_MODE1, PCA9685_MODE1_AI);
    if (err != ESP_OK) {
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(1));  // >500 мкс на стабилизацию осциллятора
    return pca9685_write_reg(PCA9685_REG_MODE1, PCA9685_MODE1_AI | PCA9685_MODE1_RESTART);
}

esp_err_t pca9685_init(void)
{
    s_ready = false;

    i2c_master_bus_config_t bus_config = {
        .i2c_port = -1,  // авто-выбор свободного порта
        .sda_io_num = PCA9685_SDA_GPIO,
        .scl_io_num = PCA9685_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t err = i2c_new_master_bus(&bus_config, &s_bus);
    if (err != ESP_OK) {
        return err;
    }

    // Проба адреса на шине: отвечает ли PCA9685. Решающая диагностика I2C-линии.
    err = i2c_master_probe(s_bus, PCA9685_I2C_ADDR, PCA9685_I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG,
                 "PCA9685 НЕ ОТВЕЧАЕТ на 0x%02X (SDA=GPIO%d, SCL=GPIO%d): %s. "
                 "Проверь проводку I2C, подтяжки, питание и адрес микросхемы.",
                 PCA9685_I2C_ADDR, PCA9685_SDA_GPIO, PCA9685_SCL_GPIO,
                 esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "PCA9685 обнаружен на 0x%02X (SDA=GPIO%d, SCL=GPIO%d)",
             PCA9685_I2C_ADDR, PCA9685_SDA_GPIO, PCA9685_SCL_GPIO);

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = PCA9685_I2C_ADDR,
        .scl_speed_hz = PCA9685_I2C_FREQ_HZ,
    };
    err = i2c_master_bus_add_device(s_bus, &dev_config, &s_dev);
    if (err != ESP_OK) {
        return err;
    }

    err = pca9685_set_prescale();
    if (err != ESP_OK) {
        return err;
    }

    // Безопасное стартовое состояние: все 16 выходов LOW
    for (uint8_t channel = 0; channel < 16; ++channel) {
        err = pca9685_write_led(channel, 0, 0);
        if (err != ESP_OK) {
            return err;
        }
    }

    s_ready = true;
    ESP_LOGI(TAG, "PCA9685 инициализирован (ШИМ %d Гц), все выходы LOW", PCA9685_PWM_FREQ_HZ);
    return ESP_OK;
}

esp_err_t pca9685_set_pwm(uint8_t channel, uint16_t value)
{
    if (!s_ready) {
        return ESP_ERR_INVALID_STATE;  // нет устройства — мгновенный no-op, без I2C
    }
    if (value > PCA9685_PWM_MAX) {
        value = PCA9685_PWM_MAX;
    }
    return pca9685_write_led(channel, 0, value);
}

esp_err_t pca9685_set_off(uint8_t channel)
{
    if (!s_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    return pca9685_write_led(channel, 0, 0);
}
