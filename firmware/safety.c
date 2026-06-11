#include "safety.h"
#include "hw_module.h"
#include "device_state.h"
#include "tcp_server.h"

void safety_init(void)
{
    // Зарезервировано: регистрация колбэков критических ошибок (Этап 2).
}

void safety_emergency_stop(safety_reason_t reason)
{
    if (reason == SAFETY_REASON_DISCONNECT) {
        // Разрыв связи — не авария: остановить вывод без защёлки ошибки.
        hw_module_stop();
        device_state_set(STATE_DISCONNECTED);
        return;
    }

    // Команда alarm / таймаут heartbeat / ошибка канала: блок + защёлка ошибки.
    hw_module_alarm();
    device_state_set(STATE_ERROR);
    tcp_server_push_status();  // уведомить верхний уровень о переходе в ERROR
}
