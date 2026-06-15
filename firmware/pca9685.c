#include "pca9685.h"
#include "config.h"

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static i2c_master_bus_handle_t s_bus;
static i2c_master_dev_handle_t s_dev;

static void pca9685_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t buffer[2] = { reg, value };
    i2c_master_transmit(s_dev, buffer, sizeof(buffer), PCA9685_I2C_TIMEOUT_MS);
}

static void pca9685_write_led(uint8_t channel, uint16_t on, uint16_t off)
{
    uint8_t buffer[5] = {
        (uint8_t)(PCA9685_REG_LED0_ON_L + 4 * channel),
        (uint8_t)(on & 0xFF),
        (uint8_t)(on >> 8),
        (uint8_t)(off & 0xFF),
        (uint8_t)(off >> 8),
    };
    i2c_master_transmit(s_dev, buffer, sizeof(buffer), PCA9685_I2C_TIMEOUT_MS);
}

static void pca9685_set_prescale(void)
{
    uint32_t prescale = (PCA9685_OSC_HZ + (4096UL * PCA9685_PWM_FREQ_HZ) / 2) / (4096UL * PCA9685_PWM_FREQ_HZ);
    if (prescale > 0) {
        prescale -= 1;
    }
    if (prescale < PCA9685_PRESCALE_MIN) {
        prescale = PCA9685_PRESCALE_MIN;
    }
    pca9685_write_reg(PCA9685_REG_MODE1, PCA9685_MODE1_SLEEP);
    pca9685_write_reg(PCA9685_REG_PRESCALE, (uint8_t)prescale);
    pca9685_write_reg(PCA9685_REG_MODE1, PCA9685_MODE1_AI);
    vTaskDelay(pdMS_TO_TICKS(1));  // >500 мкс на стабилизацию осциллятора
    pca9685_write_reg(PCA9685_REG_MODE1, PCA9685_MODE1_AI | PCA9685_MODE1_RESTART);
}

void pca9685_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = -1,
        .sda_io_num = PCA9685_SDA_GPIO,
        .scl_io_num = PCA9685_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_new_master_bus(&bus_config, &s_bus);

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = PCA9685_I2C_ADDR,
        .scl_speed_hz = PCA9685_I2C_FREQ_HZ,
    };
    i2c_master_bus_add_device(s_bus, &dev_config, &s_dev);

    pca9685_set_prescale();

    for (uint8_t channel = 0; channel < 16; ++channel) {
        pca9685_write_led(channel, 0, 0);
    }
}

void pca9685_set_pwm(uint8_t channel, uint16_t value)
{
    if (value > PCA9685_PWM_MAX) {
        value = PCA9685_PWM_MAX;
    }
    pca9685_write_led(channel, 0, value);
}

void pca9685_set_off(uint8_t channel)
{
    pca9685_write_led(channel, 0, 0);
}
