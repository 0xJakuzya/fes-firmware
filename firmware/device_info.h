#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#include <stdint.h>

#include "data_struct.h"

#define FIRMWARE_VERSION_MAJOR  1
#define FIRMWARE_VERSION_MINOR  0
#define FIRMWARE_VERSION_PATCH  0

#define PROTOCOL_VERSION_MAJOR  1
#define PROTOCOL_VERSION_MINOR  0
#define PROTOCOL_VERSION_PATCH  0

#define DEVICE_INFO_PAYLOAD_SIZE 7

device_info_t device_info_get(void);
void device_info_serialize(const device_info_t *info, uint8_t *payload);

#endif
