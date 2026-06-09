# FES - прошивка ESP32

Прошивка для системы функциональной электростимуляции (ФЭС). ESP32 создает
Wi-Fi точку доступа и принимает команды управляющего ПО через TCP.


## Требования

- плата: ESP32 Wrover, 4 MB Flash;
- ESP-IDF v5.5.3;
- CMake 3.16+ и Ninja;
- компилятор `xtensa-esp32-elf-gcc`, входящий в ESP-IDF.

## Структура проекта

```text
.
├── firmware/
│   ├── main.c          # Точка входа app_main
│   ├── wifi.c          # Wi-Fi AP, пароль и работа с NVS
│   ├── wifi.h
│   ├── tcp.c           # TCP-сервер и обработка команд
│   ├── tcp.h
│   ├── device_info.c   # Информация о версиях
│   ├── device_info.h
│   ├── config.h        # Настройки Wi-Fi, TCP и версии
│   └── CMakeLists.txt
├── CMakeLists.txt
├── sdkconfig.defaults
└── test.py             # Пример изменения пароля
```

## Сборка и прошивка

### Установка ESP-IDF

```bash
git clone -b v5.5.3 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32
. ./export.sh
```

Для Windows используйте ESP-IDF PowerShell или ESP-IDF Command Prompt.

### Сборка

```bash
idf.py set-target esp32
idf.py build
```

### Прошивка и монитор

```bash
# Windows
idf.py -p COM3 flash monitor

# Linux
idf.py -p /dev/ttyUSB0 flash monitor
```