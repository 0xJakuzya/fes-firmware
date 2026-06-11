#include "heartbeat.h"
#include "config.h"
#include "device_state.h"
#include "safety.h"
#include "tcp_server.h"

#include "esp_timer.h"

#define HEARTBEAT_CHECK_PERIOD_US (100 * 1000)  // период опроса монитора, 100 мс

static volatile int64_t s_last_feed_us;
static volatile uint16_t s_interval_ms = HEARTBEAT_DEFAULT_INTERVAL_MS;
static volatile uint16_t s_timeout_ms = HEARTBEAT_DEFAULT_TIMEOUT_MS;
static volatile bool s_active;
static esp_timer_handle_t s_timer;

static void heartbeat_check(void *arg)
{
    (void)arg;
    if (!s_active) {
        return;
    }
    int64_t now = esp_timer_get_time();
    if (now - s_last_feed_us > (int64_t)s_timeout_ms * 1000) {
        // Таймаут heartbeat. Аварийная остановка только при активной стимуляции.
        if (device_state_get() == STATE_RUNNING) {
            safety_emergency_stop(SAFETY_REASON_HEARTBEAT_TIMEOUT);
        }
    }
}

void heartbeat_init(void)
{
    const esp_timer_create_args_t args = {
        .callback = heartbeat_check,
        .name = "heartbeat",
    };
    esp_timer_create(&args, &s_timer);
}

void heartbeat_configure(uint16_t interval_ms, uint16_t timeout_ms)
{
    s_interval_ms = interval_ms;
    s_timeout_ms = timeout_ms;
}

void heartbeat_feed(void)
{
    s_last_feed_us = esp_timer_get_time();
    // Первый валидный heartbeat → готовность к командам
    if (device_state_get() == STATE_CONNECTED) {
        device_state_set(STATE_READY_FOR_COMMANDS);
        tcp_server_push_status();
    }
}

void heartbeat_start(void)
{
    s_last_feed_us = esp_timer_get_time();
    s_active = true;
    if (!esp_timer_is_active(s_timer)) {
        esp_timer_start_periodic(s_timer, HEARTBEAT_CHECK_PERIOD_US);
    }
}

void heartbeat_stop(void)
{
    s_active = false;
    if (esp_timer_is_active(s_timer)) {
        esp_timer_stop(s_timer);
    }
}
