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
#include "sbem0102.h"
#include "protocol.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * Local definitions
 */


/*
 * Static functions
 */


/*
 * Static data
 */


/*
 * Public functions
 */
int libambit_sbem0102_init(libambit_sbem0102_t *object, ambit_object_t *ambit_object, uint16_t chunk_size)
{
    object->ambit_object = ambit_object;
    object->chunk_size = chunk_size;

    return 0;
}

int libambit_sbem0102_deinit(libambit_sbem0102_t *object)
{
    return 0;
}

int libambit_sbem0102_write(libambit_sbem0102_t *object, uint16_t command, libambit_sbem0102_data_t *data)
{
    int ret = -1;
    uint8_t *send_data;
    size_t offset = 0;
    uint8_t *reply = NULL;
    size_t replylen = 0;
    static uint8_t header[] = { 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 'S', 'B', 'E', 'M', '0', '1', '0', '2' };

    // TODO: We have no idea how to deal with multiple packets at the moment,
    // just fail for now
    if (data != NULL && data->size > object->chunk_size) {
        return -1;
    }

    // Calculate size of buffer, and allocate it
    send_data = malloc(sizeof(header) + (data != NULL ? data->size : 0));
    if (send_data == NULL) {
        return -1;
    }

    // Prepare initial header
    memcpy(send_data, header, sizeof(header));
    offset += sizeof(header);

    if (data != NULL && data->data != NULL && data->size > 0) {
        memcpy(send_data+offset, data->data, data->size);
        offset += data->size;
    }

    ret = libambit_protocol_command(object->ambit_object, command, send_data, offset, &reply, &replylen, 0);

    free(send_data);
    libambit_protocol_free(reply);

    return ret;
}

int libambit_sbem0102_command_request(libambit_sbem0102_t *object, uint16_t command, libambit_sbem0102_data_t *data_objects, libambit_sbem0102_data_t *reply_data)
{
    int ret = -1;
    uint8_t *send_data = NULL;
    size_t offset = 0;
    uint8_t *reply = NULL;
    size_t replylen = 0;

    static uint8_t header[] = { 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 'S', 'B', 'E', 'M', '0', '1', '0', '2' };
    static uint8_t special_header[] = { 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0a, 0x00, 'S', 'B', 'E', 'M', '0', '1', '0', '2' };

    // TODO: We have no idea how to deal with multiple packets at the moment,
    // just fail for now
    if (data_objects != NULL && data_objects->size > object->chunk_size) {
        return -1;
    }

    // Calculate size of buffer, and allocate it
    // TODO: log headers seems to have a different format than the rest, treat
    // it here until the mystery of the 2 extra bytes is really solved
    if (command == ambit_command_ambit3_log_headers) {
        send_data = malloc(sizeof(special_header) + (data_objects != NULL ? data_objects->size : 0));
        if (send_data == NULL) {
            return -1;
        }
        memcpy(send_data, special_header, sizeof(special_header));
        offset += sizeof(special_header);
    }
    else {
        send_data = malloc(sizeof(header) + (data_objects != NULL ? data_objects->size : 0));
        if (send_data == NULL) {
            return -1;
        }
        memcpy(send_data, header, sizeof(header));
        offset += sizeof(header);
    }

    // Add data objects
    if (data_objects != NULL && data_objects->data != NULL && data_objects->size > 0) {
        memcpy(send_data+offset, data_objects->data, data_objects->size);
        offset += data_objects->size;
    }

    // Reset reply data before starting to fill it
    libambit_sbem0102_data_free(reply_data);

    if (libambit_protocol_command(object->ambit_object, command, send_data, offset, &reply, &replylen, 0) == 0) {
        // Check that the reply contains an SBEM0102 header
        if (replylen >= sizeof(header) && memcmp(reply + 6, header + 6, 8) == 0) {
            if (replylen > sizeof(header)) {
                // Copy message to reply_data object
                reply_data->data = malloc(replylen - sizeof(header));
                memcpy(reply_data->data, reply + sizeof(header), replylen - sizeof(header));
                reply_data->size = replylen - sizeof(header);

                // Check if this reply was just a part (5th byte is the current
                // guess on how to determine)
                while (reply[4] != 0x01) {
                    // First byte may be 2
                    // Second byte is (number of read log headers) % 256
                    memcpy(send_data, reply, 4);

                    // Free old reply before calling again
                    libambit_protocol_free(reply);

                    if (libambit_protocol_command(object->ambit_object, command, send_data, offset, &reply, &replylen, 0) != 0 ||
                        replylen < 6) {
                        libambit_sbem0102_data_free(reply_data);
                        break;
                    }

                    if (replylen > 6) {
                        reply_data->data = realloc(reply_data->data, reply_data->size + replylen - 6);
                        memcpy(reply_data->data + reply_data->size, reply + 6, replylen - 6);
                        reply_data->size += replylen - 6;
                    }
                }
            }

            ret = 0;
        }

        libambit_protocol_free(reply);
    }

    free(send_data);

    return ret;
}

int libambit_sbem0102_command_request_raw(libambit_sbem0102_t *object, uint16_t command, uint8_t *data, size_t datalen, libambit_sbem0102_data_t *reply_data)
{
    int ret = -1;
    uint8_t *reply = NULL;
    size_t replylen = 0;

    static uint8_t header[] = { 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 'S', 'B', 'E', 'M', '0', '1', '0', '2' };

    // Reset reply data before starting to fill it
    libambit_sbem0102_data_free(reply_data);

    if (libambit_protocol_command(object->ambit_object, command, data, datalen, &reply, &replylen, 0) == 0) {
        // Check that the reply contains an SBEM0102 header
        if (replylen >= sizeof(header) && memcmp(reply + 6, header + 6, 8) == 0) {
            if (replylen > sizeof(header)) {
                // Copy message to reply_data object
                reply_data->data = malloc(replylen - sizeof(header));
                memcpy(reply_data->data, reply + sizeof(header), replylen - sizeof(header));
                reply_data->size = replylen - sizeof(header);
            }

            ret = 0;
        }

        libambit_protocol_free(reply);
    }

    return ret;
}

void libambit_sbem0102_data_init(libambit_sbem0102_data_t *data)
{
    if (data != NULL) {
        memset(data, 0, sizeof(libambit_sbem0102_data_t));
    }
}

void libambit_sbem0102_data_free(libambit_sbem0102_data_t *data)
{
    if (data != NULL) {
        if (data->data != NULL) {
            free(data->data);
        }
        memset(data, 0, sizeof(libambit_sbem0102_data_t));
    }
}

void libambit_sbem0102_data_add(libambit_sbem0102_data_t *object, uint8_t id, uint8_t *data, uint8_t datalen)
{
    if (object != NULL) {
        object->data = realloc(object->data, object->size + 2 + datalen);
        if (object->data != NULL) {
            object->data[object->size] = id;
            object->data[object->size+1] = datalen;
            if (datalen > 0 && data != NULL) {
                memcpy(object->data+2+object->size, data, datalen);
            }
            object->size += 2 + datalen;
        }
    }
}
