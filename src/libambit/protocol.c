/*
 * (C) Copyright 2014 Emil Ljungdahl
 *
 * This file is part of libambit.
 *
 * libambit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contributors:
 *
 */
#include "protocol.h"
#include "libambit_int.h"
#include "crc16.h"

#include "hidapi/hidapi.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <stdio.h>

/*
 * Local definitions
 */
#define READ_TIMEOUT       20000 // ms
#define READ_POLL_INTERVAL 100  // ms
#define READ_POLL_RETRY    (READ_TIMEOUT / READ_POLL_INTERVAL)

typedef struct __attribute__((__packed__)) ambit_msg_header_s {
    uint8_t UId;
    uint8_t UL;
    uint8_t MP;
    uint8_t ML;
    uint16_t parts_seq;
    uint16_t checksum;
    uint16_t command;
    uint16_t send_recv;
    uint16_t format;
    uint16_t sequence;
    uint32_t payload_len;
} ambit_msg_header_t;

/*
 * Static functions
 */
/**
 * Write packet to bus. The data buffer should include space for headers
 * which is automatically filled in.
 * \param object Connection object
 * \param data Data buffer to write (64 byte)
 * \return 0 on success, else -1
 */
static int protocol_write_packet(ambit_object_t *object, uint8_t *data);

/**
 * Read packet from bus.
 * \param object Connection object
 * \param data Data buffer to write (64 byte)
 * \return 0 on success, else -1
 */
static int protocol_read_packet(ambit_object_t *object, uint8_t *data);

/**
 * Finalize packet. Add lengths and calculate checksums
 * \param data Data buffer
 * \param payload_len Length of payload
 */
static void finalize_packet(uint8_t *data, uint8_t payload_len);

/*
 * Static variables
 */

/*
 * Public functions
 */
int libambit_protocol_command(ambit_object_t *object, uint16_t command, uint8_t *data, size_t datalen, uint8_t **reply_data, size_t *replylen, uint8_t legacy_format)
{
    int ret = 0;
    uint8_t buf[64];
    int packet_count = 1;
    ambit_msg_header_t *msg = (ambit_msg_header_t *)buf;
    uint8_t packet_payload_len;
    int i;
    uint32_t dataoffset = 0, reply_data_len;
    uint16_t msg_parts;

    // Calculate number of packets
    if (datalen > 42) {
        packet_count = 2 + (datalen - 42)/54;
    }

    // Create first packet
    msg->MP = 0x5d;
    msg->parts_seq = htole16(packet_count);
    msg->command = htobe16(command);
    msg->send_recv = htole16(legacy_format == 1 ? 1 : legacy_format == 2 ? 0x15 : legacy_format == 3 ? 0x0a : 5);
    msg->format = htole16(legacy_format == 1 ? 0 : 9);
    msg->sequence = htole16(object->sequence_no);
    msg->payload_len = htole32(datalen);
    packet_payload_len = fmin(42, datalen);
    memcpy(&buf[20], &data[dataoffset], packet_payload_len);
    finalize_packet(buf, packet_payload_len + 12);
    protocol_write_packet(object, buf);

    datalen -= packet_payload_len;
    dataoffset += packet_payload_len;

    // Send additional packets
    for(i=1; i<packet_count; i++) {
        msg->MP = 0x5e;
        msg->parts_seq = htole16(i);
        packet_payload_len = fmin(54, datalen);
        memcpy(&buf[8], &data[dataoffset], packet_payload_len);
        finalize_packet(buf, packet_payload_len);
        protocol_write_packet(object, buf);

        datalen -= packet_payload_len;
        dataoffset += packet_payload_len;
    }

    // Retrieve reply packets
    if (protocol_read_packet(object, buf) == 0 &&
        msg->MP == 0x5d && le16toh(msg->sequence) == object->sequence_no) {
        reply_data_len = le32toh(msg->payload_len);
        dataoffset = 0;
        packet_payload_len = fmin(42, reply_data_len);
        if (reply_data != NULL && replylen != NULL) {
            *replylen = reply_data_len;
            *reply_data = malloc(reply_data_len);
            memcpy(&(*reply_data)[dataoffset], &buf[20], packet_payload_len);
        }
        dataoffset += packet_payload_len;
        reply_data_len -= packet_payload_len;

        msg_parts = le16toh(msg->parts_seq);

        for (i=2; ret == 0 && i<=msg_parts; i++) {
            if (protocol_read_packet(object, buf) == 0 && msg->MP == 0x5e && le16toh(msg->parts_seq) < msg_parts) {
                packet_payload_len = fmin(54, reply_data_len);
                if (reply_data != NULL) {
                    memcpy(&(*reply_data)[42+(le16toh(msg->parts_seq)-1)*54], &buf[8], packet_payload_len);
                }
                dataoffset += packet_payload_len;
                reply_data_len -= packet_payload_len;
            }
            else {
                ret = -1;
            }
        }
    }
    else {
        ret = -1;
    }

    // Increment sequence number for next run
    object->sequence_no++;

    return ret;
}

void libambit_protocol_free(uint8_t *data)
{
    if (data != NULL) {
        free(data);
    }
}

static int protocol_write_packet(ambit_object_t *object, uint8_t *data)
{
    hid_write(object->handle, data, 64);

    return 0;
}

static int protocol_read_packet(ambit_object_t *object, uint8_t *data)
{
    int i, res = -1;
    for (i=0; i<READ_POLL_RETRY; i++) {
        res = hid_read(object->handle, data, 64);
        if (res != 0) {
            break;
        }
        usleep(READ_POLL_INTERVAL * 1000);
    }

    return (res > 0 ? 0 : -1);
}

static void finalize_packet(uint8_t *data, uint8_t payload_len)
{
    ambit_msg_header_t *msg = (ambit_msg_header_t *)data;
    uint16_t *payload_crc, tmpcrc;

    msg->UId = 0x3f;
    msg->UL = payload_len + 8;
    msg->ML = payload_len;
    tmpcrc = crc16_ccitt_false(&data[2], 4);
    msg->checksum = htole16(tmpcrc);
    payload_crc = (uint16_t *)&data[msg->UL];
    *payload_crc = htole16(crc16_ccitt_false_init(&data[8], payload_len, tmpcrc));
}
