#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#include "config.h"
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

device_info_t device_info_get(void);

void device_info_serialize(const device_info_t *info, uint8_t payload[DEVICE_INFO_PAYLOAD_SIZE]);

#endif
