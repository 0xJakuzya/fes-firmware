#include "device_info.h"
#include "config.h"

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
    };
}

void device_info_serialize(const device_info_t *info, uint8_t payload[DEVICE_INFO_PAYLOAD_SIZE])
{
    payload[0] = info->firmware_version.major;
    payload[1] = info->firmware_version.minor;
    payload[2] = info->firmware_version.patch;
    payload[3] = info->protocol_version.major;
    payload[4] = info->protocol_version.minor;
    payload[5] = info->protocol_version.patch;
    payload[6] = FES_CHANNEL_COUNT;
    payload[7] = FES_INTENSITY_MIN;
    payload[8] = FES_INTENSITY_MAX;
    payload[9] = FES_FREQUENCY_MIN_HZ;
    payload[10] = FES_FREQUENCY_MAX_HZ;
    payload[11] = (uint8_t)(FES_PULSE_WIDTH_MIN_US & 0xFF);
    payload[12] = (uint8_t)(FES_PULSE_WIDTH_MIN_US >> 8);
    payload[13] = (uint8_t)(FES_PULSE_WIDTH_MAX_US & 0xFF);
    payload[14] = (uint8_t)(FES_PULSE_WIDTH_MAX_US >> 8);
}
