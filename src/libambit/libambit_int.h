#ifndef __LIBAMBIT_INT_H__
#define __LIBAMBIT_INT_H__

#include <stdint.h>
#include "hidapi/hidapi.h"

typedef struct ambit_object_s {
    hid_device *handle;
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t sequence_no;
} ambit_object_t;

// protocol.c
int libambit_protocol_command(ambit_object_t *object, uint16_t command, uint8_t *data, size_t datalen, uint8_t **reply_data, size_t *replylen, uint8_t legacy_format);
void libambit_protocol_free(uint8_t *data);

// crc16.c
uint16_t crc16_ccitt_false(unsigned char *buf, size_t buflen);
uint16_t crc16_ccitt_false_init(unsigned char *buf, size_t buflen, uint16_t crc);

#endif /* __LIBAMBIT_INT_H__ */
