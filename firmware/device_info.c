#include "device_info.h"
#include "stimulation.h"

device_info_t device_info_get(void)
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
        .channel_count = FES_CHANNEL_COUNT,
        .status        = 0x00,
    };
}

void device_info_serialize(const device_info_t *info, uint8_t *payload)
{
    payload[0] = info->protocol_version.major;
    payload[1] = info->protocol_version.minor;
    payload[2] = info->firmware_version.major;
    payload[3] = info->firmware_version.minor;
    payload[4] = info->firmware_version.patch;
    payload[5] = info->channel_count;
    payload[6] = info->status;
}
