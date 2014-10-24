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
#include "libambit.h"
#include "libambit_int.h"

#include <errno.h>
#include <iconv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/*
 * Local definitions
 */
#define SUUNTO_USB_VENDOR_ID 0x1493

typedef struct ambit_known_device_s ambit_known_device_t;

struct ambit_known_device_s {
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
static uint32_t version_number(const uint8_t version[4]);
static ambit_device_info_t * ambit_device_info_new(const struct hid_device_info *dev);

/*
 * Static variables
 */
static ambit_known_device_t known_devices[] = {
    { SUUNTO_USB_VENDOR_ID, 0x001c, "Finch", {0x00,0x00,0x00,0x00}, "Suunto Ambit3 Sport", false, 0x0400 },
    { SUUNTO_USB_VENDOR_ID, 0x001b, "Emu", {0x00,0x00,0x00,0x00}, "Suunto Ambit3 Peak", false, 0x0400 },
    { SUUNTO_USB_VENDOR_ID, 0x001d, "Greentit", {0x00,0x00,0x00,0x00}, "Suunto Ambit2 R", true, 0x0400 },
    { SUUNTO_USB_VENDOR_ID, 0x001a, "Colibri", {0x01,0x01,0x02,0x00}, "Suunto Ambit2 S", true, 0x0400 },
    { SUUNTO_USB_VENDOR_ID, 0x0019, "Duck", {0x01,0x01,0x02,0x00}, "Suunto Ambit2", true, 0x0400 },
    { SUUNTO_USB_VENDOR_ID, 0x001a, "Colibri", {0x00,0x02,0x03,0x00}, "Suunto Ambit2 S", false, 0x0400 },
    { SUUNTO_USB_VENDOR_ID, 0x0019, "Duck", {0x00,0x02,0x03,0x00}, "Suunto Ambit2", false, 0x0400 },
    { SUUNTO_USB_VENDOR_ID, 0x001a, "Colibri", {0x00,0x02,0x02,0x00}, "Suunto Ambit2 S (up to 0.2.2)", false, 0x0200 },
    { SUUNTO_USB_VENDOR_ID, 0x0019, "Duck", {0x00,0x02,0x02,0x00}, "Suunto Ambit2 (up to 0.2.2)", false, 0x0200 },
    { SUUNTO_USB_VENDOR_ID, 0x0010, "Bluebird", {0x02,0x01,0x00,0x00}, "Suunto Ambit", true, 0x0200 },
    { SUUNTO_USB_VENDOR_ID, 0x0010, "Bluebird", {0x01,0x09,0x00,0x00}, "Suunto Ambit", false, 0x0200 }, /* First with PMEM 2.0!? */
    { SUUNTO_USB_VENDOR_ID, 0x0010, "Bluebird", {0x01,0x06,0x00,0x00}, "Suunto Ambit", false, 0 },
    { SUUNTO_USB_VENDOR_ID, 0x0010, "Bluebird", {0x01,0x01,0x00,0x00}, "Suunto Ambit", false, 0 },
    { SUUNTO_USB_VENDOR_ID, 0x0010, "Bluebird", {0x00,0x00,0x00,0x00}, "Suunto Ambit", false, 0 },
    { 0x0000, 0x0000, NULL, {0x00,0x00,0x00,0x00}, NULL, false }
};

static uint8_t komposti_version[] = { 0x01, 0x08, 0x01, 0x00 };

/*
 * Public functions
 */
ambit_device_info_t * libambit_enumerate(void)
{
    ambit_device_info_t *devices = NULL;

    struct hid_device_info *devs = hid_enumerate(0, 0);
    struct hid_device_info *current;

    if (!devs) {
      LOG_ERROR("HID: something went wrong");
      return NULL;
    }

    current = devs;
    while (current) {
        ambit_device_info_t *tmp = ambit_device_info_new(current);

        if (tmp) {
            if (devices) {
                tmp->next = devices;
            }
            else {
                devices = tmp;
            }
        }
        current = current->next;
    }
    hid_free_enumeration(devs);

    return devices;
}

void libambit_free_enumeration(ambit_device_info_t *devices)
{
    while (devices) {
        ambit_device_info_t *next = devices->next;
        free((char *) devices->path);
        free(devices);
        devices = next;
    }
}

ambit_object_t * libambit_new(const ambit_device_info_t *device)
{
    ambit_object_t *object = NULL;
    const char *path = NULL;

    if (!device || !device->path) {
        LOG_ERROR("%s", strerror(EINVAL));
        return NULL;
    }

    path = strdup (device->path);
    if (!path) return NULL;

    if (0 == device->access_status && device->is_supported) {
        object = calloc(1, sizeof(*object));
        if (object) {
            object->handle = hid_open_path(path);
            memcpy(&object->device_info, device, sizeof(*device));
            object->device_info.path = path;
            libambit_pmem20_init(object, device->chunk_size);

            if (object->handle) {
                hid_set_nonblocking(object->handle, true);
            }
        }
    }
    if (!object) {
        free((char *) path);
    }

    return object;
}

ambit_object_t * libambit_new_from_pathname(const char* pathname)
{
    ambit_object_t *object = NULL;
    ambit_device_info_t *info;
    ambit_device_info_t *current;

    if (!pathname) {
        LOG_ERROR("%s", strerror(EINVAL));
        return NULL;
    }

    info = libambit_enumerate();
    current = info;
    while (!object && current) {
        if (0 == strcmp(pathname, current->path)) {
            object = libambit_new(current);
        }
        current = current->next;
    }
    libambit_free_enumeration(info);

    return object;
}

void libambit_close(ambit_object_t *object)
{
    LOG_INFO("Closing");
    if (object != NULL) {
        if (object->handle != NULL) {
            // Make sure to clear log lock (if possible)
            lock_log(object, false);
            hid_close(object->handle);
        }

        libambit_pmem20_deinit(object);
        free((char *) object->device_info.path);
        free(object);
    }
}

void libambit_sync_display_show(ambit_object_t *object)
{
    lock_log(object, true);
}

void libambit_sync_display_clear(ambit_object_t *object)
{
    lock_log(object, false);
}

int libambit_date_time_set(ambit_object_t *object, struct tm *tm)
{
    uint8_t date_data[8] = { 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00 };
    uint8_t time_data[8];
    int ret = -1;

    LOG_INFO("Writing date and time to clock");

    // Set date
    *(uint16_t*)(&date_data[0]) = htole16(1900 + tm->tm_year);
    date_data[2] = 1 + tm->tm_mon;
    date_data[3] = tm->tm_mday;
    // byte[4-7] unknown (but set to 0x28000000 in moveslink)

    // Set time (+date)
    *(uint16_t*)(&time_data[0]) = htole16(1900 + tm->tm_year);
    time_data[2] = 1 + tm->tm_mon;
    time_data[3] = tm->tm_mday;
    time_data[4] = tm->tm_hour;
    time_data[5] = tm->tm_min;
    *(uint16_t*)(&time_data[6]) = htole16(1000*tm->tm_sec);

    if (libambit_protocol_command(object, ambit_command_date, date_data, sizeof(date_data), NULL, NULL, 0) == 0 &&
        libambit_protocol_command(object, ambit_command_time, time_data, sizeof(time_data), NULL, NULL, 0) == 0) {

        ret = 0;
    }
    else {
        LOG_WARNING("Failed to write date and time");
    }

    return ret;
}

int libambit_device_status_get(ambit_object_t *object, ambit_device_status_t *status)
{
    uint8_t *reply_data = NULL;
    size_t replylen;
    int ret = -1;

    LOG_INFO("Reading device status");

    if (libambit_protocol_command(object, ambit_command_status, NULL, 0, &reply_data, &replylen, 0) == 0) {
        if (status != NULL) {
            status->charge = reply_data[1];
        }
        ret = 0;
    }
    else {
        LOG_WARNING("Failed to read device status");
    }

    libambit_protocol_free(reply_data);

    return ret;
}

int libambit_personal_settings_get(ambit_object_t *object, ambit_personal_settings_t *settings)
{
    uint8_t *reply_data = NULL;
    size_t replylen = 0;
    int ret = -1;

    LOG_INFO("Reading personal settings");

    if (libambit_protocol_command(object, ambit_command_personal_settings, NULL, 0, &reply_data, &replylen, 0) == 0) {
        ret = libambit_personal_settings_parse(reply_data, replylen, settings);
        libambit_protocol_free(reply_data);
    }
    else {
        LOG_WARNING("Failed to read personal settings");
    }

    return ret;
}

int libambit_gps_orbit_header_read(ambit_object_t *object, uint8_t data[8])
{
    uint8_t *reply_data = NULL;
    size_t replylen = 0;
    int ret = -1;

    if (libambit_protocol_command(object, ambit_command_gps_orbit_head, NULL, 0, &reply_data, &replylen, 0) == 0 && replylen >= 9) {
        memcpy(data, &reply_data[1], 8);
        libambit_protocol_free(reply_data);

        ret = 0;
    }
    else {
        LOG_WARNING("Failed to read GPS orbit header");
    }

    return ret;
}

int libambit_gps_orbit_write(ambit_object_t *object, uint8_t *data, size_t datalen)
{
    uint8_t header[8], cmpheader[8];
    int ret = -1;

    LOG_INFO("Writing GPS orbit data");

    libambit_protocol_command(object, ambit_command_write_start, NULL, 0, NULL, NULL, 0);

    if (libambit_gps_orbit_header_read(object, header) == 0) {
        cmpheader[0] = data[7]; // Year, swap bytes
        cmpheader[1] = data[6];
        cmpheader[2] = data[8];
        cmpheader[3] = data[9];
        cmpheader[4] = data[13]; // 4 byte swap
        cmpheader[5] = data[12];
        cmpheader[6] = data[11];
        cmpheader[7] = data[10];

        // Check if new data differs 
        if (memcmp(header, cmpheader, 8) != 0) {
            ret = libambit_pmem20_gps_orbit_write(object, data, datalen);
        }
        else {
            LOG_INFO("Current GPS orbit data is already up to date, skipping");
            ret = 0;
        }
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

    bool read_pmem = false;

    ambit_log_header_t log_header;
    ambit_log_entry_t *log_entry;

    LOG_INFO("Reading number of logs");

    /*
     * Read number of log entries
     */
    if (libambit_protocol_command(object, ambit_command_log_count, NULL, 0, &reply_data, &replylen, 0) != 0) {
        LOG_WARNING("Failed to read number of log entries");
        return -1;
    }
    log_entries_total = le16toh(*(uint16_t*)(reply_data + 2));
    libambit_protocol_free(reply_data);

    LOG_INFO("Number of logs=%d", log_entries_total);

    /*
     * First part walks through headers to check if there is any point in start
     * reading the PMEM content. If no skip callback is defined, there is no
     * point in checking the headers, because no one can tell us to not include
     * the logs...
     */

    if (skip_cb != NULL) {
        LOG_INFO("Look in headers for new logs");
        // Rewind
        if (libambit_protocol_command(object, ambit_command_log_head_first, NULL, 0, &reply_data, &replylen, 0) != 0) {
            LOG_WARNING("Failed to rewind header pointer");
            return -1;
        }
        more = le32toh(*(uint32_t*)reply_data);
        libambit_protocol_free(reply_data);

        // Loop through logs while more entries exists
        while (more == 0x00000400) {
            LOG_INFO("Reading next header");
            // Go to next entry
            if (libambit_protocol_command(object, ambit_command_log_head_step, NULL, 0, &reply_data, &replylen, 0) != 0) {
                LOG_WARNING("Failed to walk to next header");
                return -1;
            }
            libambit_protocol_free(reply_data);

            // Assume every header is composited by 2 parts, where only the
            // second is of interrest right now
            if (libambit_protocol_command(object, ambit_command_log_head, NULL, 0, &reply_data, &replylen, 0) != 0) {
                LOG_WARNING("Failed to read first part of header");
                return -1;
            }
            libambit_protocol_free(reply_data);

            if (libambit_protocol_command(object, ambit_command_log_head, NULL, 0, &reply_data, &replylen, 0) == 0) {
                if (replylen > 8 && libambit_pmem20_log_parse_header(reply_data + 8, replylen - 8, &log_header) == 0) {
                    if (skip_cb(userref, &log_header) != 0) {
                        // Header was NOT skipped, break out!
                        read_pmem = true;
                        LOG_INFO("Found new entry, start reading log data");
                        break;
                    }
                }
                else {
                    LOG_ERROR("Failed to parse log header");
                    return -1;
                }
                libambit_protocol_free(reply_data);
            }
            else {
                LOG_WARNING("Failed to read second part of header");
                return -1;
            }

            // Is there more entries to read?
            if (libambit_protocol_command(object, ambit_command_log_head_peek, NULL, 0, &reply_data, &replylen, 0) != 0) {
                LOG_WARNING("Failed to check for more headers");
                return -1;
            }
            more = le32toh(*(uint32_t*)reply_data);
            libambit_protocol_free(reply_data);
        }
    }
    else {
        LOG_INFO("No skip callback defined, reading log data");
        read_pmem = true;
    }

    if (read_pmem) {
        if (libambit_pmem20_log_init(object) != 0) {
            return -1;
        }

        // Loop through all log entries, first check headers
        while (log_entries_walked < log_entries_total && libambit_pmem20_log_next_header(object, &log_header) == 1) {
            LOG_INFO("Reading header of log %d of %d", log_entries_walked + 1, log_entries_total);
            if (progress_cb != NULL) {
                progress_cb(userref, log_entries_total, log_entries_walked+1, 100*log_entries_walked/log_entries_total);
            }
            // Check if this entry needs to be read
            if (skip_cb == NULL || skip_cb(userref, &log_header) != 0) {
                LOG_INFO("Reading data of log %d of %d", log_entries_walked + 1, log_entries_total);
                log_entry = libambit_pmem20_log_read_entry(object);
                if (log_entry != NULL) {
                    if (push_cb != NULL) {
                        push_cb(userref, log_entry);
                    }
                    entries_read++;
                }
            }
            else {
                LOG_INFO("Log %d of %d already exists, skip reading data", log_entries_walked + 1, log_entries_total);
            }
            log_entries_walked++;
            if (progress_cb != NULL) {
                progress_cb(userref, log_entries_total, log_entries_walked, 100*log_entries_walked/log_entries_total);
            }
        }
    }

    LOG_INFO("%d entries read", entries_read);

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
    uint8_t *reply_data = NULL;
    size_t replylen;
    int ret = -1;

    LOG_INFO("Reading device info");

    if (libambit_protocol_command(object, ambit_command_device_info, komposti_version, sizeof(komposti_version), &reply_data, &replylen, 1) == 0) {
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
    else {
        LOG_WARNING("Failed to device info");
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
        LOG_INFO("Setting Sync message to device display");
        send_data[0] = 1;
        ret = libambit_protocol_command(object, ambit_command_lock_set, send_data, sizeof(send_data), &reply_data, &replylen, 0);
        libambit_protocol_free(reply_data);
    }
    else if (!lock && current_lock == 1) {
        LOG_INFO("Clearing Sync message to device display");
        send_data[0] = 0;
        ret = libambit_protocol_command(object, ambit_command_lock_set, send_data, sizeof(send_data), &reply_data, &replylen, 0);
        libambit_protocol_free(reply_data);
    }

    return ret;
}

static uint32_t version_number(const uint8_t version[4])
{
    return (  (version[0] << 24)
            | (version[1] << 16)
            | (version[2] <<  0)
            | (version[3] <<  8));
}

const size_t LIBAMBIT_VERSION_LENGTH = 13;      /* max: 255.255.65535 */

static inline void version_string(char string[LIBAMBIT_VERSION_LENGTH+1],
                                  const uint8_t version[4])
{
  if (!string || !version) return;

  snprintf(string, LIBAMBIT_VERSION_LENGTH+1, "%d.%d.%d",
           version[0], version[1], (version[2] << 0) | (version[3] << 8));
}

static bool is_known_vid_pid(uint16_t vid, uint16_t pid)
{
    bool found = false;
    size_t i;
    size_t count = sizeof(known_devices) / sizeof(*known_devices);

    for (i = 0; !found && i < count; ++i) {
        found = (   known_devices[i].vid == vid
                 && known_devices[i].pid == pid);
    }

    return found;
}

/* Converts a wide-character string to a limited length UTF-8 string.
 * This produces the longest valid UTF-8 string that doesn't exceed n
 * bytes.  If the converted string would be too long, it is shortened
 * one wchar_t at a time until the result is short enough.  Invalid
 * and incomplete multibyte sequences will result in an empty string.
 */
static const char * wcs2nutf8(char *dest, const wchar_t *src, size_t n)
{
    const char *rv = NULL;

    iconv_t cd = iconv_open("UTF-8", "WCHAR_T");

    if ((iconv_t) -1 == cd) {
        LOG_ERROR("iconv_open: %s", strerror(errno));
    }
    else {
        char  *s = (char *) malloc((n + 1) * sizeof(char));
        size_t m = wcslen(src) + 1;

        if (s) {
            size_t sz;

            do {
                char  *ibuf = (char *) src;
                char  *obuf = s;
                size_t ilen = --m * sizeof(wchar_t);
                size_t olen = n;

                sz = iconv(cd, &ibuf, &ilen, &obuf, &olen);

                if ((size_t) -1 == sz) {
                    s[0] = '\0';
                }
                else {          /* we're good, terminate string */
                    s[n - olen] = '\0';
                }
            } while ((size_t) -1 == sz && E2BIG == errno && 0 < m);

            if ((size_t) -1 == sz && E2BIG != errno) {
                LOG_ERROR("iconv: %s", strerror(errno));
            }

            strncpy(dest, s, n);
            rv = s;
        }

        iconv_close(cd);
    }

    return rv;
}

/* Tacitly assumes that minimally required software versions are
 * listed in decreasing order in the known_devices array!
 */
static int find_known_device(const ambit_device_info_t *info)
{
    bool found = false;
    int i = -1;
    int count = sizeof(known_devices) / sizeof(*known_devices);

    if (!info) return -1;

    while (!found && ++i < count) {
        found = (   known_devices[i].vid == info->vendor_id
                 && known_devices[i].pid == info->product_id
                 && 0 == strcmp(known_devices[i].name, info->name)
                 && 0 == strcmp(known_devices[i].model, info->model)
                 && (   version_number(known_devices[i].min_sw_version)
                     <= version_number(info->fw_version)));
    }

    return (found ? i : -1);
}

static ambit_device_info_t * ambit_device_info_new(const struct hid_device_info *dev)
{
    ambit_device_info_t *device = NULL;

    const char *dev_path;
    const char *name = NULL;
    const char *uniq = NULL;

    uint16_t vid;
    uint16_t pid;

    hid_device *hid;

    if (!dev || !dev->path) {
        LOG_ERROR("internal error: expecting hidraw device");
        return NULL;
    }

    dev_path = dev->path;
    vid = dev->vendor_id;
    pid = dev->product_id;

    if (!is_known_vid_pid(vid, pid)) {
        LOG_WARNING("unknown device (VID/PID: %04x/%04x)", vid, pid);
        return NULL;
    }

    dev_path = strdup(dev_path);
    if (!dev_path) return NULL;

    device = calloc(1, sizeof(*device));
    if (!device) {
        free ((char *) dev_path);
        return NULL;
    }

    device->path = dev_path;
    device->vendor_id  = vid;
    device->product_id = pid;

    if (dev->product_string) {
        name = wcs2nutf8(device->name, dev->product_string,
                         LIBAMBIT_PRODUCT_NAME_LENGTH);
    }

    if (dev->serial_number) {
        uniq = wcs2nutf8(device->serial, dev->serial_number,
                         LIBAMBIT_SERIAL_LENGTH);
    }

    LOG_INFO("HID  : %s: '%s' (serial: %s, VID/PID: %04x/%04x)",
             device->path, device->name, device->serial,
             device->vendor_id, device->product_id);

    hid = hid_open_path(device->path);
    if (hid) {
        /* HACK ALERT: minimally initialize an ambit object so we can
         * call device_info_get() */
        ambit_object_t obj;
        obj.handle = hid;
        obj.sequence_no = 0;
        if (0 == device_info_get(&obj, device)) {
            int index;
            char fw_version[LIBAMBIT_VERSION_LENGTH+1];
            char hw_version[LIBAMBIT_VERSION_LENGTH+1];

            if (name && 0 != strcmp(name, device->name)) {
                LOG_INFO("preferring F/W name over '%s'", name);
            }
            if (uniq && 0 != strcmp(uniq, device->serial)) {
                LOG_INFO("preferring F/W serial number over '%s'", uniq);
            }

            index = find_known_device(device);
            if (0 <= index) {
                device->is_supported = known_devices[index].supported;
                device->chunk_size = known_devices[index].pmem20_chunksize;
            }

            version_string(fw_version, device->fw_version);
            version_string(hw_version, device->hw_version);

            LOG_INFO("Ambit: %s: '%s' (serial: %s, VID/PID: %04x/%04x, "
                     "nick: %s, F/W: %s, H/W: %s, supported: %s)",
                     device->path, device->name, device->serial,
                     device->vendor_id, device->product_id,
                     device->model, fw_version, hw_version,
                     (device->is_supported ? "YES" : "NO"));
        }
        else {
            LOG_ERROR("cannot get device info from %s", device->path);
        }
        hid_close(hid);
    }
    else {
        /* Store an educated guess as to why we cannot open the HID
         * device.  Without read/write access we cannot communicate
         * to begin with but there may be other reasons.
         */
        int fd = open(device->path, O_RDWR);

        if (-1 == fd) {
            device->access_status = errno;
            LOG_ERROR("cannot open HID device (%s): %s", device->path,
                      strerror (device->access_status));
        }
        else {
            LOG_WARNING("have read/write access to %s but cannot open HID "
                        "device", device->path);
            close(fd);
        }
    }

    if (name) free((char *) name);
    if (uniq) free((char *) uniq);

    return device;
}
