#include "libambit.h"
#include "libambit_int.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * Local definitions
 */
#define AMBIT_USB_VENDOR_ID 0x1493

/*
 * Static functions
 */

/*
 * Static variables
 */
uint16_t ambit_known_ids[] = { 0x10, 0x19 };
enum ambit_commands_e {
    ambit_command_device_info       = 0x0000,
    ambit_command_time              = 0x0300,
    ambit_command_date              = 0x0302,
    ambit_command_status            = 0x0306,
    ambit_command_personal_settings = 0x0b00,
    ambit_command_log_head_first    = 0x0b07,
    ambit_command_log_head_next     = 0x0b08,
    ambit_command_log_head_step     = 0x0b0a,
    ambit_command_log_head          = 0x0b0b,
    ambit_command_log_read          = 0x0b17,
};

/*
 * Public functions
 */
ambit_object_t *libambit_detect(void)
{
    hid_device *handle;
    struct hid_device_info *devs, *cur_dev;
    ambit_object_t *ret_object = NULL;
    int i;
    uint16_t product_id = 0;

    devs = hid_enumerate(0x0, 0x0);
    cur_dev = devs;
    while (cur_dev) {
        //printf("Device Found\n  type: %04hx %04hx\n  path: %s\n  serial_number: %ls", cur_dev->vendor_id, cur_dev->product_id, cur_dev->path, cur_dev->serial_number);
        //printf("\n");
        //printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
        //printf("  Product:      %ls\n", cur_dev->product_string);
        //printf("  Release:      %hx\n", cur_dev->release_number);
        //printf("  Interface:    %d\n",  cur_dev->interface_number);
        //printf("\n");
        if (cur_dev->vendor_id == AMBIT_USB_VENDOR_ID) {
            for (i=0; i<(sizeof(ambit_known_ids)/sizeof(ambit_known_ids[0])); i++) {
                if (cur_dev->product_id == ambit_known_ids[i]) {
                    product_id = cur_dev->product_id;
                    break;
                }
            }
            if (product_id != 0) {
                break;
            }
        }
        cur_dev = cur_dev->next;
    }
    hid_free_enumeration(devs);

    if (product_id != 0) {
        handle = hid_open(AMBIT_USB_VENDOR_ID, product_id, NULL);
        if (handle != NULL) {
            // Setup hid device correctly
            hid_set_nonblocking(handle, 1);

            ret_object = malloc(sizeof(ambit_object_t));
            memset(ret_object, 0, sizeof(ambit_object_t));
            ret_object->handle = handle;
            ret_object->vendor_id = AMBIT_USB_VENDOR_ID;
            ret_object->product_id = product_id;
        }
        else {
            printf("Failed to open device\n");
        }
    }

    return ret_object;
}

void libambit_close(ambit_object_t *object)
{
    if (object != NULL) {
        if (object->handle != NULL) {
            hid_close(object->handle);
        }

        free(object);
    }
}

int libambit_device_info_get(ambit_object_t *object, ambit_device_info_t *info)
{
    uint8_t send_data[] = { 0x01, 0x06, 0x14, 0x00 };
    uint8_t *reply_data = NULL;
    size_t replylen;
    int ret = -1;

    if (libambit_protocol_command(object, ambit_command_device_info, send_data, sizeof(send_data), &reply_data, &replylen, 1) == 0) {
        if (info != NULL) {
            memcpy(info->model, reply_data, 16);
            info->model[16] = 0;
            memcpy(info->serial, &reply_data[16], 16);
            info->serial[16] = 0;
            memcpy(info->fw_version, &reply_data[32], 4);
            memcpy(info->hw_version, &reply_data[36], 4);
        }
        ret = 0;
    }

    libambit_protocol_free(reply_data);

    return ret;
}

int libambit_date_time_set(ambit_object_t *object, struct tm *tm)
{
    uint8_t date_data[8];
    uint8_t time_data[8] = { 0x09, 0x00, 0x01, 0x00 };
    int ret = -1;

    // Set date
    *(uint16_t*)(&date_data[0]) = htole16(tm->tm_year);
    date_data[2] = 1 + tm->tm_mon;
    date_data[3] = tm->tm_mday;
    memset(&date_data[4], 0, 4); // ????? Unknown data

    // Set time
    time_data[4] = tm->tm_hour;
    time_data[5] = tm->tm_min;
    *(uint16_t*)(&time_data[6]) = htole16(1000*tm->tm_sec);

    if (libambit_protocol_command(object, ambit_command_date, date_data, sizeof(date_data), NULL, NULL, 0) == 0 &&
        libambit_protocol_command(object, ambit_command_time, time_data, sizeof(time_data), NULL, NULL, 0) == 0) {

        ret = 0;
    }

    return ret;
}

int libambit_device_status_get(ambit_object_t *object, ambit_device_status_t *status)
{
    uint8_t *reply_data = NULL;
    size_t replylen;
    int ret = -1;

    if (libambit_protocol_command(object, ambit_command_status, NULL, 0, &reply_data, &replylen, 0) == 0) {
        if (status != NULL) {
            status->charge = reply_data[1];
        }
        ret = 0;
    }

    libambit_protocol_free(reply_data);

    return ret;
}

int libambit_personal_settings_get(ambit_object_t *object, ambit_personal_settings_t *settings)
{
    uint8_t *reply_data = NULL;
    size_t replylen = 0;
    int ret = -1;

    if (libambit_protocol_command(object, ambit_command_personal_settings, NULL, 0, &reply_data, &replylen, 0) == 0) {

        ret = 0;
    }

    libambit_protocol_free(reply_data);

    return ret;
}

int libambit_log_read_get_next(ambit_object_t *object)
{
    uint8_t send_data[8];
    uint8_t *reply_data = NULL;
    size_t replylen = 0;

    uint32_t *address = (uint32_t*)&send_data[0];
    uint32_t *length = (uint32_t*)&send_data[4];
    int i=0, j, q;
    uint32_t more = 0x00000400;

    if (libambit_protocol_command(object, ambit_command_log_head_first, NULL, 0, &reply_data, &replylen, 0) == 0) {
        more = le32toh(*(uint32_t*)reply_data);
        libambit_protocol_free(reply_data);
    }

    while(more == 0x00000400) {
        printf("Round %d:\n", ++i);
        if (libambit_protocol_command(object, ambit_command_log_head_step, NULL, 0, &reply_data, &replylen, 0) == 0) {
            libambit_protocol_free(reply_data);
        }
        if (libambit_protocol_command(object, ambit_command_log_head, NULL, 0, &reply_data, &replylen, 0) == 0) {
            printf("1st, length: %lu\n", replylen);
            for(j=0; j<replylen; j+=16) {
                for(q=0; q<16; q++) {
                    printf("%02x ", reply_data[j+q]);
                }
                printf(" ");
                for(q=0; q<16; q++) {
                    if(reply_data[j+q] >= 0x20)
                        printf("%c", reply_data[j+q]);
                    else
                        printf(".");
                }
                printf("\n");
            }
            printf("\n");
            libambit_protocol_free(reply_data);
        }
        if (libambit_protocol_command(object, ambit_command_log_head, NULL, 0, &reply_data, &replylen, 0) == 0) {
            printf("2nd, length: %lu\n", replylen);
            for(j=0; j<replylen; j+=16) {
                for(q=0; q<16; q++) {
                    printf("%02x ", reply_data[j+q]);
                }
                printf(" ");
                for(q=0; q<16; q++) {
                    if(reply_data[j+q] >= 0x20)
                        printf("%c", reply_data[j+q]);
                    else
                        printf(".");
                }
                printf("\n");
            }
            printf("\n");
            libambit_protocol_free(reply_data);
        }
        if (libambit_protocol_command(object, ambit_command_log_head_next, NULL, 0, &reply_data, &replylen, 0) == 0) {
            more = le32toh(*(uint32_t*)reply_data);
            libambit_protocol_free(reply_data);
        }
        printf("\n\n\n");
    }

    *length = 1024;
/*
    for(i=0; i<5; i++) {
        *address = 0x000f4240 + i*1024;

        if (libambit_protocol_command(object, ambit_command_log_read, send_data, sizeof(send_data), &reply_data, &replylen, 0) == 0) {
            printf("Address: %08x\n", *address);
            for(j=8; j<replylen; j+=16) {
                for(q=0; q<16; q++) {
                    printf("%02x ", reply_data[j+q]);
                }
                printf(" ");
                for(q=0; q<16; q++) {
                    if(reply_data[j+q] >= 0x20)
                        printf("%c", reply_data[j+q]);
                    else
                        printf(".");
                }
                printf("\n");
            }
            printf("\n");
            libambit_protocol_free(reply_data);
        }
    }
*/

    return 0;
}

