#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#include <stdint.h>

typedef struct {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
} version_t;

typedef struct {
    version_t firmware_version;
    version_t protocol_version;
} device_info_t;

device_info_t get_info(void);

#endif
