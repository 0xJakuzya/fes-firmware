#include "hw_module.h"
#include "config.h"
#include "pca9685.h"
#include "stimulation.h"

#include "esp_log.h"

static const char *TAG = "hw_module";

// Защёлка ошибки: после аварии запуск запрещён до явного снятия (reset_error).
// Реализует требование «нет авто-восстановления после ошибки канала».
static volatile bool s_error_latched;

esp_err_t hw_module_init(void)
{
    s_error_latched = false;

    // Инициализация PCA9685 некритична: при отсутствии платы (нет ACK по I2C)
    // продолжаем работу — транспортный слой (Wi-Fi/TCP/протокол) поднимается,
    // а запись ШИМ становится no-op. Это исключает boot-loop без аппаратуры.
    esp_err_t err = pca9685_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "PCA9685 init failed (%s): стимуляция отключена, сеть работает",
                 esp_err_to_name(err));
    }

    // Фатальна только невозможность создать задачу движка (нехватка памяти).
    return stimulation_start();
}

hw_result_t hw_module_validate(void)
{
    // Параметры уже клампятся в допустимые диапазоны при установке, поэтому
    // загруженная конфигурация всегда корректна. Проверяем, что задан хотя бы
    // один активный канал (иначе запускать нечего).
    for (uint8_t ch = 0; ch < FES_CHANNEL_COUNT; ++ch) {
        stimulation_channel_state_t state;
        if (stimulation_get_channel(ch, &state) && state.intensity > 0) {
            return HW_RESULT_OK;
        }
    }
    return HW_RESULT_VALIDATION_FAILED;
}

bool hw_module_start(void)
{
    if (s_error_latched) {
        return false;
    }
    stimulation_set_running(true);
    return true;
}

bool hw_module_stop(void)
{
    stimulation_set_running(false);
    return true;
}

void hw_module_alarm(void)
{
    // Немедленная блокировка вывода всех каналов + защёлка ошибки.
    stimulation_stop_all();
    s_error_latched = true;
}

bool hw_module_reset_error(void)
{
    stimulation_set_running(false);
    s_error_latched = false;
    return true;
}

bool hw_module_set_channel(uint8_t channel, stim_param_t param, uint16_t value)
{
    return stimulation_set_param(channel, param, value);
}

void hw_module_set_all(stim_param_t param, uint16_t value)
{
    stimulation_set_all(param, value);
}

bool hw_module_get_channel(uint8_t channel, stimulation_channel_state_t *out)
{
    return stimulation_get_channel(channel, out);
}
