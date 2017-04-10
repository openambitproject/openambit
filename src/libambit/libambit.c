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
static uint8_t komposti_version[] = { 0x02, 0x00, 0x2d, 0x00 };

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

int libambit_sport_mode_write(ambit_object_t *object, ambit_sport_mode_device_settings_t *ambit_sport_modes)
{
    int ret = -1;

    if (object->driver != NULL && object->driver->sport_mode_write != NULL) {
        ret = object->driver->sport_mode_write(object, ambit_sport_modes);
    }
    else {
        LOG_WARNING("Driver does not support sport_mode_write");
    }

    return ret;
}

int libambit_app_data_write(ambit_object_t *object, ambit_sport_mode_device_settings_t *ambit_sport_modes, ambit_app_rules_t* ambit_apps)
{
    int ret = -1;

    if (object->driver != NULL && object->driver->app_data_write != NULL) {
        ret = object->driver->app_data_write(object, ambit_sport_modes, ambit_apps);
    }
    else {
        LOG_WARNING("Driver does not support app_data_write");
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

void libambit_sport_mode_device_settings_free(ambit_sport_mode_device_settings_t *settings)
{
    int i;

    if (settings->sport_modes != NULL) {
        for (i=0; i<settings->sport_modes_count; i++) {
            if (settings->sport_modes[i].display != NULL) {
                if (settings->sport_modes[i].display->view != NULL) {
                    free(settings->sport_modes[i].display->view);
                }
                free(settings->sport_modes[i].display);
            }
            if (settings->sport_modes[i].apps_list != NULL) {
                free(settings->sport_modes[i].apps_list);
            }
        }
        free(settings->sport_modes);
    }

    if (settings->sport_mode_groups != NULL) {
        for (i=0; i<settings->sport_mode_groups_count; i++) {
            if (settings->sport_mode_groups[i].sport_mode_index != NULL) {
                free(settings->sport_mode_groups[i].sport_mode_index);
            }
        }
        free(settings->sport_mode_groups);
    }
}

ambit_sport_mode_device_settings_t *libambit_malloc_sport_mode_device_settings(void)
{
    ambit_sport_mode_device_settings_t *ambit_device_settings = (ambit_sport_mode_device_settings_t *)malloc(sizeof(ambit_sport_mode_device_settings_t));
    ambit_device_settings->sport_modes = NULL;
    ambit_device_settings->sport_modes_count = 0;
    ambit_device_settings->sport_mode_groups = NULL;
    ambit_device_settings->sport_mode_groups_count = 0;
    ambit_device_settings->app_ids_count = 0;

    return ambit_device_settings;
}

bool libambit_malloc_sport_modes(uint16_t count, ambit_sport_mode_device_settings_t *ambit_settings)
{
    ambit_sport_mode_t *ambit_sport_modes = (ambit_sport_mode_t *)malloc(sizeof(ambit_sport_mode_t) * count);
    if (ambit_sport_modes != NULL) {
        ambit_settings->sport_modes = ambit_sport_modes;
        ambit_settings->sport_modes_count = count;

        int i;
        for (i=0; i<count; i++) {
            ambit_settings->sport_modes[i].display = NULL;
            ambit_settings->sport_modes[i].displays_count = 0;
            ambit_settings->sport_modes[i].apps_list = NULL;
            ambit_settings->sport_modes[i].apps_list_count = 0;
        }
    }
    else {
        ambit_settings->sport_modes = NULL;
        ambit_settings->sport_modes_count = 0;
    }

    return ambit_sport_modes != NULL;
}

bool libambit_malloc_sport_mode_groups(uint16_t count, ambit_sport_mode_device_settings_t *ambit_settings)
{
    ambit_sport_mode_group_t *ambit_sport_mode_groups = (ambit_sport_mode_group_t *)malloc(sizeof(ambit_sport_mode_group_t) * count);
    if (ambit_sport_mode_groups != NULL) {
        ambit_settings->sport_mode_groups = ambit_sport_mode_groups;
        ambit_settings->sport_mode_groups_count = count;

        int i;
        for (i=0; i<count; i++) {
            ambit_settings->sport_mode_groups[i].sport_mode_index = NULL;
            ambit_settings->sport_mode_groups[i].sport_mode_index_count = 0;
        }
    }
    else {
        ambit_settings->sport_mode_groups = NULL;
        ambit_settings->sport_mode_groups_count = 0;
    }

    return ambit_sport_mode_groups != NULL;
}

bool libambit_malloc_sport_mode_app_ids(uint16_t count, ambit_sport_mode_t *ambit_sport_mode)
{
    ambit_apps_list_t *ambit_app_ids = (ambit_apps_list_t *)malloc(sizeof(ambit_apps_list_t) * count);
    if (ambit_app_ids != NULL) {
        ambit_sport_mode->apps_list = ambit_app_ids;
        ambit_sport_mode->apps_list_count = count;
    }
    else {
        ambit_sport_mode->apps_list = NULL;
        ambit_sport_mode->apps_list_count = 0;
    }
    return ambit_app_ids != NULL;
}

bool libambit_malloc_sport_mode_displays(uint16_t count, ambit_sport_mode_t *ambit_sport_mode)
{
    ambit_sport_mode_display_t *ambit_displays = (ambit_sport_mode_display_t *)malloc(sizeof(ambit_sport_mode_display_t) * count);
    if (ambit_displays != NULL) {
        ambit_sport_mode->display = ambit_displays;
        ambit_sport_mode->displays_count = count;

        int i;
        for (i=0; i<count; i++) {
            ambit_sport_mode->display[i].view = NULL;
            ambit_sport_mode->display[i].views_count = 0;
        }
    }
    else {
        ambit_sport_mode->display = NULL;
        ambit_sport_mode->displays_count = 0;
    }
    return ambit_displays != NULL;
}

bool libambit_malloc_sport_mode_view(uint16_t count, ambit_sport_mode_display_t *ambit_displays)
{
    uint16_t *ambit_views = (uint16_t *)malloc(sizeof(uint16_t) * count);
    if (ambit_views != NULL) {
        ambit_displays->view = ambit_views;
        ambit_displays->views_count = count;
    }
    else {
        ambit_displays->view = NULL;
        ambit_displays->views_count = 0;
    }

    return ambit_views != NULL;
}

bool libambit_malloc_sport_mode_index(uint16_t count, ambit_sport_mode_group_t *ambit_sport_mode_group)
{
    uint16_t *ambit_sport_mode_index = (uint16_t *)malloc(sizeof(uint16_t) * count);
    if (ambit_sport_mode_index != NULL) {
        ambit_sport_mode_group->sport_mode_index = ambit_sport_mode_index;
        ambit_sport_mode_group->sport_mode_index_count = count;
    }
    else
    {
        ambit_sport_mode_group->sport_mode_index = NULL;
        ambit_sport_mode_group->sport_mode_index_count = 0;
    }

    return ambit_sport_mode_index != NULL;
}

void libambit_app_rules_free(ambit_app_rules_t *app_rules)
{
    int i;

    if (app_rules->app_rules != NULL) {
        for (i=0; i<app_rules->app_rules_count; i++) {
            free(app_rules->app_rules[i].app_rule_data);
        }
        free(app_rules->app_rules);
    }
    free(app_rules);
}

ambit_app_rules_t *liblibambit_malloc_app_rules(void)
{
    ambit_app_rules_t *ambit_app_rules = (ambit_app_rules_t *)malloc(sizeof(ambit_app_rules_t));
    ambit_app_rules->app_rules = NULL;
    ambit_app_rules->app_rules_count = 0;

    return ambit_app_rules;
}

bool libambit_malloc_app_rule(uint16_t count, ambit_app_rules_t *ambit_app_rules)
{
    ambit_app_rule_t *ambit_app_rule = (ambit_app_rule_t *)malloc(sizeof(ambit_app_rule_t) * count);
    if (ambit_app_rule != NULL) {
        ambit_app_rules->app_rules = ambit_app_rule;
        ambit_app_rules->app_rules_count = count;

        int i;
        for (i=0; i<count; i++) {
            ambit_app_rules->app_rules[i].app_rule_data = NULL;
            ambit_app_rules->app_rules[i].app_rule_data_length = 0;
            ambit_app_rules->app_rules[i].app_id = 0;
        }
    }
    else {
        ambit_app_rules->app_rules = NULL;
        ambit_app_rules->app_rules_count = 0;
    }

    return ambit_app_rule != NULL;
}

static int device_info_get(ambit_object_t *object, ambit_device_info_t *info)
{
    uint8_t *reply_data = NULL;
    size_t replylen;
    int ret = -1;

    LOG_INFO("Reading device info");

    if (libambit_protocol_command(object, ambit_command_device_info, komposti_version, sizeof(komposti_version), &reply_data, &replylen, 1) == 0) {
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

ambit_personal_settings_t* libambit_personal_settings_alloc() {
    ambit_personal_settings_t *ps;
    ps = (ambit_personal_settings_t*)calloc(1, sizeof(ambit_personal_settings_t));
    ps->routes.data = NULL;
    ps->waypoints.data = NULL;
    return ps;
}

void libambit_personal_settings_free(ambit_personal_settings_t *personal_settings) {
    if(personal_settings->waypoints.data != NULL) {
        free(personal_settings->waypoints.data);
    }

    if(personal_settings->routes.data != NULL) {
        libambit_route_free(personal_settings->routes.data, personal_settings->routes.count);
    }

    free(personal_settings);
}

ambit_route_t* libambit_route_alloc(uint16_t route_count) {
    ambit_route_t *routes;
    routes = (ambit_route_t*)calloc(route_count, sizeof(ambit_route_t));
    for(int x=0; x<route_count; ++x) {
        routes[x].points = NULL;
    }
    return routes;
}

void libambit_route_free(ambit_route_t *routes, uint16_t route_count) {

    if(route_count!=0) {
        for(int x=0; x<route_count; ++x) {
            if(routes[x].points != NULL) {
                free(routes[x].points);
            }
        }
    } else if(routes->points != NULL) {
        free(routes->points);
    }

    free(routes);
}

void libambit_waypoint_append(ambit_personal_settings_t *ps, ambit_waypoint_t *waypoints, uint8_t num_to_append) {

    if(num_to_append == 0) {
        //Do nothing
        return;
    }

    ambit_waypoint_t *old_array = ps->waypoints.data;
    uint8_t old_count = ps->waypoints.count;

    ps->waypoints.count += num_to_append;
    ps->waypoints.data = (ambit_waypoint_t*)malloc(sizeof(ambit_waypoint_t)*ps->waypoints.count);

    if(old_count>0) {
        memcpy(ps->waypoints.data, old_array, sizeof(ambit_waypoint_t)*old_count);
        if(old_array != NULL) {
            free(old_array);
        }
    }
    memcpy(&(ps->waypoints.data[old_count]), waypoints, sizeof(ambit_waypoint_t)*num_to_append);
}

int libambit_navigation_read(ambit_object_t *object, ambit_personal_settings_t *personal_settings) {
    int ret = -1;

    if (object->driver != NULL && object->driver->navigation_read != NULL) {
        ret = object->driver->navigation_read(object, personal_settings);
    }
    else {
        LOG_WARNING("Driver does not support navigation_waypoint_read");
    }

    return ret;
}

int libambit_navigation_write(ambit_object_t *object, ambit_personal_settings_t *personal_settings) {

    int ret = -1;

    if (object->driver != NULL && object->driver->navigation_write != NULL) {
        ret = object->driver->navigation_write(object, personal_settings);
    }
    return ret;
}

