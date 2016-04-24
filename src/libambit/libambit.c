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
#include "device_support.h"
#include "device_driver.h"
#include "protocol.h"
#include "utils.h"
#include "debug.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/*
 * Local definitions
 */
#define LIBAMBIT_MODEL_LENGTH    16
#define LIBAMBIT_SERIAL_LENGTH   16

/*
 * Static functions
 */
static int device_info_get(ambit_object_t *object, ambit_device_info_t *info);
static ambit_device_info_t * ambit_device_info_new(const struct hid_device_info *dev);

/*
 * Static variables
 */
static uint8_t komposti_version_default[] = { 0x02, 0x00, 0x2d, 0x00 };
static uint8_t komposti_version_ambit3[] = { 0x02, 0x03, 0x06, 0x00 };

/*
 * Public functions
 */
ambit_device_info_t * libambit_enumerate(void)
{
    ambit_device_info_t *devices = NULL;

    struct hid_device_info *devs = hid_enumerate(0, 0);
    struct hid_device_info *current;

    if (!devs) {
      LOG_WARNING("HID: no USB HID devices found");
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
        if (devices->name)   free(devices->name);
        if (devices->model)  free(devices->model);
        if (devices->serial) free(devices->serial);
        free((char *) devices->path);
        free(devices);
        devices = next;
    }
}

ambit_object_t * libambit_new(const ambit_device_info_t *device)
{
    ambit_object_t *object = NULL;
    const ambit_known_device_t *known_device = NULL;
    const char *path = NULL;

    if (!device || !device->path) {
        LOG_ERROR("%s", strerror(EINVAL));
        return NULL;
    }

    path = strdup (device->path);
    if (!path) return NULL;

    if (0 == device->access_status && device->is_supported) {
        // Note, this should never fail if device was properly received with libambit_enumerate
        known_device = libambit_device_support_find(device->vendor_id, device->product_id, device->model, device->fw_version);
        if (known_device != NULL) {
            object = calloc(1, sizeof(*object));
            if (object) {
                object->handle = hid_open_path(path);
                memcpy(&object->device_info, device, sizeof(*device));
                object->device_info.path = path;
                object->driver = known_device->driver;

                if (object->handle) {
                    hid_set_nonblocking(object->handle, true);
                }

                // Initialize driver
                object->driver->init(object, known_device->driver_param);
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
        if (object->driver != NULL) {
            // Make sure to clear log lock (if possible)
            if (object->driver->lock_log != NULL) {
                object->driver->lock_log(object, false);
            }
            if (object->driver->deinit != NULL) {
                object->driver->deinit(object);
            }
        }
        if (object->handle != NULL) {
            hid_close(object->handle);
        }

        free((char *) object->device_info.path);
        free(object);
    }
}

void libambit_sync_display_show(ambit_object_t *object)
{
    if (object->driver != NULL && object->driver->lock_log != NULL) {
        object->driver->lock_log(object, true);
    }
}

void libambit_sync_display_clear(ambit_object_t *object)
{
    if (object->driver != NULL && object->driver->lock_log != NULL) {
        object->driver->lock_log(object, false);
    }
}

int libambit_date_time_set(ambit_object_t *object, struct tm *tm)
{
    int ret = -1;

    if (object->driver != NULL && object->driver->date_time_set != NULL) {
        ret = object->driver->date_time_set(object, tm);
    }
    else {
        LOG_WARNING("Driver does not support date_time_set");
    }

    return ret;
}

int libambit_device_status_get(ambit_object_t *object, ambit_device_status_t *status)
{
    int ret = -1;

    if (object->driver != NULL && object->driver->status_get != NULL) {
        ret = object->driver->status_get(object, status);
    }
    else {
        LOG_WARNING("Driver does not support status_get");
    }

    return ret;
}

int libambit_personal_settings_get(ambit_object_t *object, ambit_personal_settings_t *settings)
{
    int ret = -1;

    if (object->driver != NULL && object->driver->personal_settings_get != NULL) {
        ret = object->driver->personal_settings_get(object, settings);
    }
    else {
        LOG_WARNING("Driver does not support personal_settings_get");
    }

    return ret;
}

int libambit_gps_orbit_header_read(ambit_object_t *object, uint8_t data[8])
{
    int ret = -1;

    if (object->driver != NULL && object->driver->gps_orbit_header_read != NULL) {
        ret = object->driver->gps_orbit_header_read(object, data);
    }
    else {
        LOG_WARNING("Driver does not support gps_orbit_header_read");
    }

    return ret;
}

int libambit_gps_orbit_write(ambit_object_t *object, uint8_t *data, size_t datalen)
{
    int ret = -1;

    if (object->driver != NULL && object->driver->gps_orbit_write != NULL) {
        ret = object->driver->gps_orbit_write(object, data, datalen);
    }
    else {
        LOG_WARNING("Driver does not support gps_orbit_write");
    }

    return ret;
}

int libambit_log_read(ambit_object_t *object, ambit_log_skip_cb skip_cb, ambit_log_push_cb push_cb, ambit_log_progress_cb progress_cb, void *userref)
{
    int ret = -1;

    if (object->driver != NULL && object->driver->log_read != NULL) {
        ret = object->driver->log_read(object, skip_cb, push_cb, progress_cb, userref);
    }
    else {
        LOG_WARNING("Driver does not support log_read");
    }

    return ret;
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
        if (log_entry->header.activity_name) {
            free(log_entry->header.activity_name);
        }
        free(log_entry);
    }
}

static int device_info_get(ambit_object_t *object, ambit_device_info_t *info)
{
    uint8_t *komposti_version;
    size_t komposti_version_size;
    uint8_t *reply_data = NULL;
    size_t replylen;
    int ret = -1;

    LOG_INFO("Reading device info");

    switch (info->product_id) {
      case 0x001c:
      case 0x001b:
        komposti_version = komposti_version_ambit3;
        komposti_version_size = sizeof(komposti_version_ambit3);
        break;
      default:
        komposti_version = komposti_version_default;
        komposti_version_size = sizeof(komposti_version_default);
        break;
    }

    if (libambit_protocol_command(object, ambit_command_device_info, komposti_version, komposti_version_size, &reply_data, &replylen, 1) == 0) {
        if (info != NULL) {
            const char *p = (char *)reply_data;

            info->model  = utf8memconv(p, LIBAMBIT_MODEL_LENGTH, NULL);
            p += LIBAMBIT_MODEL_LENGTH;
            info->serial = utf8memconv(p, LIBAMBIT_SERIAL_LENGTH, NULL);
            p += LIBAMBIT_SERIAL_LENGTH;
            memcpy(info->fw_version, p, 4);
            memcpy(info->hw_version, p + 4, 4);
        }
        ret = 0;
    }
    else {
        LOG_WARNING("Failed to device info");
    }

    libambit_protocol_free(reply_data);

    return ret;
}

const size_t LIBAMBIT_VERSION_LENGTH = 13;      /* max: 255.255.65535 */
static inline void version_string(char string[LIBAMBIT_VERSION_LENGTH+1],
                                  const uint8_t version[4])
{
  if (!string || !version) return;

  snprintf(string, LIBAMBIT_VERSION_LENGTH+1, "%d.%d.%d",
           version[0], version[1], (version[2] << 0) | (version[3] << 8));
}

static ambit_device_info_t * ambit_device_info_new(const struct hid_device_info *dev)
{
    ambit_device_info_t *device = NULL;
    const ambit_known_device_t *known_device = NULL;

    const char *dev_path;

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

    if (!libambit_device_support_known(vid, pid)) {
        LOG_INFO("ignoring unknown device (VID/PID: %04x/%04x)", vid, pid);
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

    {                           /* create name for display purposes */
        char *vendor  = utf8wcsconv(dev->manufacturer_string);
        char *product = utf8wcsconv(dev->product_string);
        if (vendor && product) {
            char *name = (char *)malloc((strlen(vendor) + 1
                                         + strlen(product) + 1)
                                        * sizeof(char));
            if (name) {
                strcpy(name, vendor);
                strcat(name, " ");
                strcat(name, product);
                free(vendor);
                free(product);
                device->name = name;
            }
            else {
                device->name = product;
                if (vendor) free(vendor);
            }
        }
        else {
            device->name = product;
            if (vendor) free(vendor);
        }
        device->serial = utf8wcsconv(dev->serial_number);
    }

    LOG_INFO("HID  : %s: '%s' (serial: %s, VID/PID: %04x/%04x)",
             device->path, device->name, device->serial,
             device->vendor_id, device->product_id);

    hid = hid_open_path(device->path);
    if (hid) {
        /* HACK ALERT: minimally initialize an ambit object so we can
         * call device_info_get().  Note that this function sets the
         * device's model and serial string fields.  Above the latter
         * has been set already using the HID information.
         */
        char *serial = device->serial;
        ambit_object_t obj;
        obj.handle = hid;
        obj.sequence_no = 0;
        if (0 == device_info_get(&obj, device)) {

            if (!device->serial) { /* fall back to HID information */
                device->serial = serial;
            }
            else {
                if (serial && 0 != strcmp(device->serial, serial)) {
                  LOG_INFO("preferring F/W serial number over HID '%s'",
                           serial);
                }
                if (serial) free(serial);
            }

            known_device = libambit_device_support_find(device->vendor_id, device->product_id, device->model, device->fw_version);
            if (known_device != NULL) {
                device->is_supported = known_device->supported;
                if (device->name && known_device->name
                    && 0 != strcmp(device->name, known_device->name)) {
                    char *name = strdup(known_device->name);
                    if (name) {
                        LOG_INFO("preferring known name over HID '%s'",
                                 device->name);
                        free(device->name);
                        device->name = name;
                    }
                }
            }

#ifdef DEBUG_PRINT_INFO
            {
                char fw_version[LIBAMBIT_VERSION_LENGTH+1];
                char hw_version[LIBAMBIT_VERSION_LENGTH+1];
                version_string(fw_version, device->fw_version);
                version_string(hw_version, device->hw_version);

                LOG_INFO("Ambit: %s: '%s' (serial: %s, VID/PID: %04x/%04x, "
                         "nick: %s, F/W: %s, H/W: %s, supported: %s)",
                         device->path, device->name, device->serial,
                         device->vendor_id, device->product_id,
                         device->model, fw_version, hw_version,
                         (device->is_supported ? "YES" : "NO"));
            }
#endif
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

    return device;
}
