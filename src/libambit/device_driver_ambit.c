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
#include "device_driver.h"
#include "device_driver_common.h"
#include "device_driver_ambit_navigation.h"
#include "libambit_int.h"
#include "protocol.h"
#include "pmem20.h"
#include "personal.h"
#include "sport_mode_serialize.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
 * Local definitions
 */
#define PMEM20_LOG_START                  0x000f4240
#define PMEM20_LOG_SIZE                   0x0029f630 /* 2 750 000 */

struct ambit_device_driver_data_s {
    libambit_pmem20_t pmem20;
};

/*
 * Static functions
 */
static void init(ambit_object_t *object, uint32_t driver_param);
static void deinit(ambit_object_t *object);
static int personal_settings_get(ambit_object_t *object, ambit_personal_settings_t *settings);
static int log_read(ambit_object_t *object, ambit_log_skip_cb skip_cb, ambit_log_push_cb push_cb, ambit_log_progress_cb progress_cb, void *userref);
static int gps_orbit_header_read(ambit_object_t *object, uint8_t data[8]);
static int gps_orbit_write(ambit_object_t *object, uint8_t *data, size_t datalen);

static int sport_mode_write(ambit_object_t *object, ambit_sport_mode_device_settings_t *ambit_sport_modes);
static int app_data_write(ambit_object_t *object, ambit_sport_mode_device_settings_t *ambit_device_settings, ambit_app_rules_t* ambit_apps);

/*
 * Global variables
 */
ambit_device_driver_t ambit_device_driver_ambit = {
    init,
    deinit,
    libambit_device_driver_lock_log,
    libambit_device_driver_date_time_set,
    libambit_device_driver_status_get,
    personal_settings_get,
    log_read,
    gps_orbit_header_read,
    gps_orbit_write,
    ambit_navigation_read,
    ambit_navigation_write,
    sport_mode_write,
    app_data_write
};

/*
 * Static functions implementation
 */
/**
 * Init function
 * \param object to initialize
 * \param driver_param PMEM20 chunk size
 */
static void init(ambit_object_t *object, uint32_t driver_param)
{
    struct ambit_device_driver_data_s *data;

    if ((data = calloc(1, sizeof(struct ambit_device_driver_data_s))) != NULL) {
        object->driver_data = data;
        libambit_pmem20_init(&object->driver_data->pmem20, object, driver_param);
    }
}

static void deinit(ambit_object_t *object)
{
    if (object->driver_data != NULL) {
        libambit_pmem20_deinit(&object->driver_data->pmem20);
    }
}

static int personal_settings_get(ambit_object_t *object, ambit_personal_settings_t *settings)
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

static int log_read(ambit_object_t *object, ambit_log_skip_cb skip_cb, ambit_log_push_cb push_cb, ambit_log_progress_cb progress_cb, void *userref)
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
    log_header.activity_name = NULL;

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
        if (libambit_pmem20_log_init(&object->driver_data->pmem20, PMEM20_LOG_START, PMEM20_LOG_SIZE) != 0) {
            return -1;
        }

        // Loop through all log entries, first check headers
        while (log_entries_walked < log_entries_total && libambit_pmem20_log_next_header(&object->driver_data->pmem20, &log_header) == 1) {
            LOG_INFO("Reading header of log %d of %d", log_entries_walked + 1, log_entries_total);
            if (progress_cb != NULL) {
                progress_cb(userref, log_entries_total, log_entries_walked+1, 100*log_entries_walked/log_entries_total);
            }
            // Check if this entry needs to be read
            if (skip_cb == NULL || skip_cb(userref, &log_header) != 0) {
                LOG_INFO("Reading data of log %d of %d", log_entries_walked + 1, log_entries_total);
                log_entry = libambit_pmem20_log_read_entry(&object->driver_data->pmem20);
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

static int gps_orbit_header_read(ambit_object_t *object, uint8_t data[8])
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

static int gps_orbit_write(ambit_object_t *object, uint8_t *data, size_t datalen)
{
    uint8_t header[8], cmpheader[8];
    int ret = -1;

    LOG_INFO("Writing GPS orbit data");

    libambit_protocol_command(object, ambit_command_write_start, NULL, 0, NULL, NULL, 0);

    if (object->driver->gps_orbit_header_read(object, header) == 0) {
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
            ret = libambit_pmem20_gps_orbit_write(&object->driver_data->pmem20, data, datalen, false);
        }
        else {
            LOG_INFO("Current GPS orbit data is already up to date, skipping");
            ret = 0;
        }
    }

    return ret;
}

static int sport_mode_write(ambit_object_t *object, ambit_sport_mode_device_settings_t *ambit_device_settings)
{
    int ret = -1;

    LOG_INFO("Writing Custom mode data");

//    libambit_protocol_command(object, ambit_command_write_start, NULL, 0, NULL, NULL, 0);

    int dataBufferSize = calculate_size_for_serialize_sport_mode_device_settings(ambit_device_settings);
    if (dataBufferSize==0) {
        return 0;
    }

    uint8_t *data = (uint8_t*)malloc(dataBufferSize);

    if (data != NULL) {
        int dataLen = serialize_sport_mode_device_settings(ambit_device_settings, data);
        ret = libambit_pmem20_sport_mode_write(&object->driver_data->pmem20, data, dataLen, false);
    }

    return ret;
}

static int app_data_write(ambit_object_t *object, ambit_sport_mode_device_settings_t *ambit_device_settings, ambit_app_rules_t* ambit_apps)
{
    int ret = -1;

    LOG_INFO("Writing App data");

//    libambit_protocol_command(object, ambit_command_write_start, NULL, 0, NULL, NULL, 0);

    int dataBufferSize = calculate_size_for_serialize_app_data(ambit_device_settings, ambit_apps);
    if (dataBufferSize==0) {
        return 0;
    }

    uint8_t *data = (uint8_t*)malloc(dataBufferSize);

    if (data != NULL) {
        int dataLen = serialize_app_data(ambit_device_settings, ambit_apps, data);
        ret = libambit_pmem20_app_data_write(&object->driver_data->pmem20, data, dataLen, false);
    }

    return ret;
}
