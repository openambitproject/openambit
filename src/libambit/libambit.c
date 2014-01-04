/*
 * (C) Copyright 2013 Emil Ljungdahl
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
#include "libambit.h"
#include "libambit_int.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * Local definitions
 */
#define SUUNTO_USB_VENDOR_ID 0x1493

struct ambit_supported_device_s {
    uint16_t vid;
    uint16_t pid;
    char *model;
    uint8_t min_sw_version[4];
    char *name;
    bool supported;
    uint16_t pmem20_chunksize;
};

/*
 * Static functions
 */
static int device_info_get(ambit_object_t *object, ambit_device_info_t *info);
static int lock_log(ambit_object_t *object, bool lock);

/*
 * Static variables
 */
static ambit_supported_device_t supported_devices[] = {
    { SUUNTO_USB_VENDOR_ID, 0x001a, "Colibri", {0x01,0x01,0x02,0x00}, "Suunto Ambit2 S", false, 0x0400 },
    { SUUNTO_USB_VENDOR_ID, 0x0019, "Duck", {0x01,0x01,0x02,0x00}, "Suunto Ambit2", true, 0x0400 },
    { SUUNTO_USB_VENDOR_ID, 0x001a, "Colibri", {0x00,0x02,0x03,0x00}, "Suunto Ambit2 S", false, 0x0400 },
    { SUUNTO_USB_VENDOR_ID, 0x0019, "Duck", {0x00,0x02,0x03,0x00}, "Suunto Ambit2", false, 0x0400 },
    { SUUNTO_USB_VENDOR_ID, 0x001a, "Colibri", {0x00,0x02,0x02,0x00}, "Suunto Ambit2 S (up to 0.2.2)", false, 0x0200 },
    { SUUNTO_USB_VENDOR_ID, 0x0019, "Duck", {0x00,0x02,0x02,0x00}, "Suunto Ambit2 (up to 0.2.2)", false, 0x0200 },
    { SUUNTO_USB_VENDOR_ID, 0x0010, "Bluebird", {0x02,0x01,0x00,0x00}, "Suunto Ambit", false, 0x0200 },
    { SUUNTO_USB_VENDOR_ID, 0x0010, "Bluebird", {0x01,0x09,0x00,0x00}, "Suunto Ambit", false, 0x0200 }, /* First with PMEM 2.0!? */
    { SUUNTO_USB_VENDOR_ID, 0x0010, "Bluebird", {0x01,0x06,0x00,0x00}, "Suunto Ambit", false, 0 },
    { SUUNTO_USB_VENDOR_ID, 0x0010, "Bluebird", {0x01,0x01,0x00,0x00}, "Suunto Ambit", false, 0 },
    { SUUNTO_USB_VENDOR_ID, 0x0010, "Bluebird", {0x00,0x00,0x00,0x00}, "Suunto Ambit", false, 0 },
    { 0x0000, 0x0000, NULL, {0x00,0x00,0x00,0x00}, NULL, false }
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
    ambit_supported_device_t *device = NULL;

    devs = hid_enumerate(0x0, 0x0);
    cur_dev = devs;
    while (cur_dev) {
        for (i=0; i<sizeof(supported_devices)/sizeof(supported_devices[0]); i++) {
            if (cur_dev->vendor_id == supported_devices[i].vid && cur_dev->product_id == supported_devices[i].pid) {
                // Found at least one supported row, lets remember that!
                device = &supported_devices[i];
                break;
            }
        }
        if (device != NULL) {
            // Devcice was found, we can stop looping through devices now...
            break;
        }
        cur_dev = cur_dev->next;
    }
    hid_free_enumeration(devs);

    if (device != NULL) {
        handle = hid_open(device->vid, device->pid, NULL);
        if (handle != NULL) {
            // Setup hid device correctly
            hid_set_nonblocking(handle, 1);

            ret_object = malloc(sizeof(ambit_object_t));
            memset(ret_object, 0, sizeof(ambit_object_t));
            ret_object->handle = handle;
            ret_object->vendor_id = device->vid;
            ret_object->product_id = device->pid;

            // Get device info to resolve supported functionality
            if (device_info_get(ret_object, &ret_object->device_info) == 0) {
                // Let's resolve the correct device
                for (i=0; i<sizeof(supported_devices)/sizeof(supported_devices[0]); i++) {
                    if (device->vid == supported_devices[i].vid &&
                        device->pid == supported_devices[i].pid &&
                        strncmp(device->model, ret_object->device_info.model, LIBAMBIT_MODEL_NAME_LENGTH) == 0 &&
                        (ret_object->device_info.fw_version[0] > device->min_sw_version[0] ||
                         (ret_object->device_info.fw_version[0] == device->min_sw_version[0] &&
                          (ret_object->device_info.fw_version[1] > device->min_sw_version[1] ||
                           (ret_object->device_info.fw_version[1] == device->min_sw_version[1] &&
                            (ret_object->device_info.fw_version[2] > device->min_sw_version[2] ||
                             (ret_object->device_info.fw_version[2] == device->min_sw_version[2] &&
                              (ret_object->device_info.fw_version[3] >= device->min_sw_version[3])))))))) {
                        // Found matching entry, reset to this one!
                        device = &supported_devices[i];
                        break;
                    }
                }
                ret_object->device = device;
                strncpy(ret_object->device_info.name, device->name, LIBAMBIT_PRODUCT_NAME_LENGTH);
            }
            else {
                free(ret_object);
                ret_object = NULL;
                printf("Failed to get device info\n");
            }
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

        libambit_pmem20_deinit(object);
        free(object);
    }
}

bool libambit_device_supported(ambit_object_t *object)
{
    bool ret = false;

    if (object != NULL && object->device != NULL) {
        ret = object->device->supported;
    }

    return ret;
}

int libambit_device_info_get(ambit_object_t *object, ambit_device_info_t *info)
{
    int ret = -1;

    if (object != NULL && object->device != NULL) {
        if (info != NULL) {
            memcpy(info, &object->device_info, sizeof(ambit_device_info_t));
        }
        ret = 0;
    }

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
        ret = libambit_personal_settings_parse(reply_data, replylen, settings);
        libambit_protocol_free(reply_data);
    }

    return ret;
}

int libambit_log_read(ambit_object_t *object, ambit_log_skip_cb skip_cb, ambit_log_push_cb push_cb, ambit_log_progress_cb progress_cb, void *userref)
{
    int entries_read = 0;

    uint8_t *reply_data = NULL;
    size_t replylen = 0;
    uint16_t log_entries_total = 0;
    uint16_t log_entries_walked = 0;

    uint32_t more = 0x00000400;

    int i=0, j, q;
    bool read_pmem = false;

    ambit_log_header_t log_header;
    ambit_log_entry_t *log_entry;

    lock_log(object, true);

    /*
     * Read number of log entries
     */
    if (libambit_protocol_command(object, ambit_command_log_count, NULL, 0, &reply_data, &replylen, 0) != 0) {
        lock_log(object, false);
        return -1;
    }
    log_entries_total = le16toh(*(uint16_t*)(reply_data + 2));
    libambit_protocol_free(reply_data);

    /*
     * First part walks through headers to check if there is any point in start
     * reading the PMEM content. If no skip callback is defined, there is no
     * point in checking the headers, because no one can tell us to not include
     * the logs...
     */

    if (skip_cb != NULL) {
        // Rewind
        if (libambit_protocol_command(object, ambit_command_log_head_first, NULL, 0, &reply_data, &replylen, 0) != 0) {
            lock_log(object, false);
            return -1;
        }
        more = le32toh(*(uint32_t*)reply_data);
        libambit_protocol_free(reply_data);

        // Loop through logs while more entries exists
        while (more == 0x00000400) {
            // Go to next entry
            if (libambit_protocol_command(object, ambit_command_log_head_step, NULL, 0, &reply_data, &replylen, 0) != 0) {
                lock_log(object, false);
                return -1;
            }
            libambit_protocol_free(reply_data);

            // Assume every header is composited by 2 parts, where only the
            // second is of interrest right now
            if (libambit_protocol_command(object, ambit_command_log_head, NULL, 0, &reply_data, &replylen, 0) != 0) {
                lock_log(object, false);
                return -1;
            }
            libambit_protocol_free(reply_data);

            if (libambit_protocol_command(object, ambit_command_log_head, NULL, 0, &reply_data, &replylen, 0) == 0) {
                if (replylen > 8 && libambit_pmem20_parse_header(reply_data + 8, replylen - 8, &log_header) == 0) {
                    if (skip_cb(userref, &log_header) != 0) {
                        // Header was NOT skipped, break out!
                        read_pmem = true;
                        break;
                    }
                }
                else {
                    printf("Failed to parse log header\n");
                    lock_log(object, false);
                    return -1;
                }
                libambit_protocol_free(reply_data);
            }
            else {
                lock_log(object, false);
                return -1;
            }

            // Is there more entries to read?
            if (libambit_protocol_command(object, ambit_command_log_head_peek, NULL, 0, &reply_data, &replylen, 0) != 0) {
                lock_log(object, false);
                return -1;
            }
            more = le32toh(*(uint32_t*)reply_data);
            libambit_protocol_free(reply_data);
        }
    }
    else {
        read_pmem = true;
    }

    if (read_pmem) {
        if (libambit_pmem20_init(object, object->device->pmem20_chunksize) != 0) {
            lock_log(object, false);
            return -1;
        }

        // Loop through all log entries, first check headers
        while (log_entries_walked < log_entries_total && libambit_pmem20_next_header(object, &log_header) == 1) {
            if (progress_cb != NULL) {
                progress_cb(userref, log_entries_total, log_entries_walked+1, 100*log_entries_walked/log_entries_total);
            }
            // Check if this entry needs to be read
            if (skip_cb == NULL || skip_cb(userref, &log_header) != 0) {
                log_entry = libambit_pmem20_read_entry(object);
                if (log_entry != NULL) {
                    if (push_cb != NULL) {
                        push_cb(userref, log_entry);
                    }
                    entries_read++;
                }
            }
            log_entries_walked++;
            if (progress_cb != NULL) {
                progress_cb(userref, log_entries_total, log_entries_walked, 100*log_entries_walked/log_entries_total);
            }
        }
    }

    lock_log(object, false);

    return entries_read;
}

void libambit_log_entry_free(ambit_log_entry_t *log_entry)
{
    int i;

    if (log_entry != NULL) {
        if (log_entry->samples != NULL) {
            for (i=0; i<log_entry->samples_count; i++) {
                if (log_entry->samples[i].type == ambit_log_sample_type_periodic) {
                    if (log_entry->samples[i].u.periodic.values != NULL) {
                        free(log_entry->samples[i].u.periodic.values);
                    }
                }
                if (log_entry->samples[i].type == ambit_log_sample_type_gps_base) {
                    if (log_entry->samples[i].u.gps_base.satellites != NULL) {
                        free(log_entry->samples[i].u.gps_base.satellites);
                    }
                }
                if (log_entry->samples[i].type == ambit_log_sample_type_unknown) {
                    if (log_entry->samples[i].u.unknown.data != NULL) {
                        free(log_entry->samples[i].u.unknown.data);
                    }
                }
            }
            free(log_entry->samples);
        }
        free(log_entry);
    }
}

static int device_info_get(ambit_object_t *object, ambit_device_info_t *info)
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

static int lock_log(ambit_object_t *object, bool lock)
{
    int ret = -1;
    uint8_t send_data[] = { 0x00, 0x00, 0x00, 0x00 };
    uint8_t *reply_data = NULL;
    size_t replylen;

    uint32_t current_lock = 0xffffffff;

    if ((ret = libambit_protocol_command(object, ambit_command_lock_check, NULL, 0, &reply_data, &replylen, 0)) == 0) {
        current_lock = le32toh(*(uint32_t*)reply_data);
        libambit_protocol_free(reply_data);
    }

    if (lock && current_lock == 0) {
        send_data[0] = 1;
        ret = libambit_protocol_command(object, ambit_command_lock_set, send_data, sizeof(send_data), &reply_data, &replylen, 0);
        libambit_protocol_free(reply_data);
    }
    else if (!lock && current_lock == 1) {
        send_data[0] = 0;
        ret = libambit_protocol_command(object, ambit_command_lock_set, send_data, sizeof(send_data), &reply_data, &replylen, 0);
        libambit_protocol_free(reply_data);
    }

    return ret;
}

