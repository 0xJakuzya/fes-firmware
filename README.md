# FES — прошивка ESP32

Прошивка 8-канального устройства функциональной электростимуляции (ФЭС). ESP32
создаёт Wi-Fi точку доступа, принимает команды управляющего ПО по бинарному
TCP-протоколу и генерирует биполярные импульсы стимуляции через PCA9685 и
H-мосты DRV8871.

## Аппаратная часть

```
ESP32 (I2C: SDA=GPIO8, SCL=GPIO9, 400 кГц)
    │ I2C
PCA9685 (0x40) — 16-канальный ШИМ, несущая 1 кГц
    │ 2 выхода на канал (IN1, IN2)
DRV8871 × 8 — H-мосты
    │ OUT1/OUT2 → конденсатор 1 мкФ 63 В → электроды → мышца
```

Каждому каналу ФЭС соответствуют два выхода PCA9685: IN1 = PWM(2·N), IN2 = PWM(2·N+1).

**Параметры стимуляции:** интенсивность 0–255, частота 1–100 Гц, длительность
импульса 100–15000 мкс. Биполярный импульс — 5 фаз: Forward → dead-time →
Reverse → dead-time → ожидание; dead-time 200 мкс. Значения вне диапазона
зажимаются.

## Требования

- ESP32 / ESP32-S3;
- ESP-IDF v5.5.3, CMake 3.16+, Ninja;
- PCA9685 + 8×DRV8871, общая земля, подтяжки на линиях I2C.

## Структура проекта

```text
.
├── firmware/
│   ├── main.c            # Точка входа: wifi + pca9685 + стимуляция + tcp
│   ├── wifi.c/.h         # Wi-Fi AP, пароль в NVS
│   ├── tcp_server.c/.h   # TCP-сервер
│   ├── protocol.c/.h     # Бинарные пакеты + CRC-16-CCITT
│   ├── command_handler.c/.h # Разбор и выполнение команд
│   ├── device_info.c/.h  # Рукопожатие (версии, каналы, диапазоны)
│   ├── pca9685.c/.h      # Драйвер I2C ШИМ-контроллера
│   ├── stimulation.c/.h  # Движок биполярных импульсов (8 каналов)
│   ├── config.h          # Настройки Wi-Fi, TCP, I2C, стимуляции
│   └── CMakeLists.txt
├── CMakeLists.txt
├── sdkconfig.defaults
├── test.py               # Python-клиент протокола
└── не нужно пока/         # Отложенные модули (heartbeat, safety, device_state, hw_module)
```

## TCP-протокол

Адрес устройства: `192.168.4.1:5000`. Wi-Fi: SSID `FES_Device`, пароль `1234567890`.

**Формат пакета:**
```
[0xAA 0x55] | msg_type(1) | seq_id(2 LE) | payload_len(2 LE) | payload | CRC-16(2 LE)
```
CRC-16-CCITT, полином `0x1021`, начальное `0xFFFF`, по заголовку + payload.
При подключении устройство сразу шлёт рукопожатие (`GET_INFO`).

| Команда | type | payload запроса | ответ |
|---------|------|-----------------|-------|
| GET_INFO (рукопожатие) | 0x01 | — | версии(6) + каналы(1) + диапазоны(8) = 15 б |
| SET_WIFI_PASSWORD | 0x02 | пароль 8–63 б | `[saved]`, затем перезагрузка |
| ACK | 0x06 | — | `[orig_type][result]` |
| SET_CHANNEL | 0x10 | `[канал][param][value:2]` | ACK |
| SET_ALL | 0x11 | `[param][value:2]` | ACK |
| GET_CHANNEL | 0x12 | `[канал]` | `[I][F][pw:2]` |
| START / STOP | 0x20 / 0x21 | — | ACK |
| ERROR | 0xFF | — | `[error_code]` |

`param`: 0 = интенсивность, 1 = частота, 2 = длительность импульса.

## Сборка и прошивка

```bash
idf.py set-target esp32s3   # или esp32 — под свою плату
idf.py build
idf.py -p COM10 flash monitor
```

Для Windows — ESP-IDF PowerShell / Command Prompt. На Linux порт вида `/dev/ttyUSB0`.

## Проверка протокола

```bash
python test.py        # сценарий: рукопожатие → параметры → start/stop
python test.py -i     # интерактив: i/f/p/all/get/start/stop/info
```

Интерактивный режим:
```
fes> i 0 120     # интенсивность канала 0
fes> f 0 50      # частота
fes> p 0 5000    # длительность импульса
fes> start
fes> get 0
fes> stop
fes> q
```
