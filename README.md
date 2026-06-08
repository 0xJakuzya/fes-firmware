# FES - Прошивка ESP32

Прошивка для системы функциональной электростимуляции (ФЭС). Реализует сетевое взаимодействие между управляющим ПО верхнего уровня и аппаратным модулем стимуляции.

## Требования

- Плата: ESP32 Wrover (4MB Flash)
- Фреймворк: ESP-IDF v5.5.3
- Инструменты: CMake 3.16+, Ninja
- Компилятор: xtensa-esp32-elf-gcc (входит в ESP-IDF)

## Структура проекта

firmware/
├── main/
│   ├── main.c          # Точка входа (app_main)
│   ├── wifi.c          # Wi-Fi AP инициализация
│   ├── wifi.h          # Заголовочный файл Wi-Fi модуля
│   ├── config.h        # Конфигурация (SSID, пароль, канал)
│   └── CMakeLists.txt  # Сборка компонента main
├── CMakeLists.txt      # Корневой CMake
└── sdkconfig.defaults  # Настройки ESP-IDF

## Сборка и прошивка

### 1. Установка ESD-IDF

```bash
git clone -b v5.5.3 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32
. ./export.sh
```

### 2. Сборка проекта

```bash
idf.py set-target esp32
idf.py build
```

### 3. Прошивка

```bash
# Windows
idf.py -p COM3 flash monitor

# Linux
idf.py -p /dev/ttyUSB0 flash monitor
```

## Конфигурация

```C
#define WIFI_SSID       "FES_Device"
#define WIFI_PASSWORD   "1234567890" 
#define WIFI_CHANNEL    1
#define WIFI_MAX_CONN   4
```