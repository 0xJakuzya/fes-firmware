#include "device_info.h"
#include "config.h"

device_info_t get_info(void)
{
    return (device_info_t) {
        .firmware_version = {
            .major = FIRMWARE_VERSION_MAJOR,
            .minor = FIRMWARE_VERSION_MINOR,
            .patch = FIRMWARE_VERSION_PATCH,
        },
        .protocol_version = {
            .major = PROTOCOL_VERSION_MAJOR,
            .minor = PROTOCOL_VERSION_MINOR,
            .patch = PROTOCOL_VERSION_PATCH,
        },
    };
}