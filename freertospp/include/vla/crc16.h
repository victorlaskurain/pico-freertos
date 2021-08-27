#ifndef VLA_CRC16_H
#define VLA_CRC16_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t vla_modbus_crc16(const uint8_t *data, uint16_t data_lengh);

#ifdef __cplusplus
}
#endif

#endif
