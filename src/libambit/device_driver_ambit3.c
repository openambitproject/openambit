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
#include "device_support.h"
#include "libambit_int.h"
#include "protocol.h"
#include "pmem20.h"
#include "personal.h"
#include "sbem0102.h"
#include "utils.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*
 * Local definitions
 */

typedef struct memory_map_entry_s {
    uint32_t start;
    uint32_t size;
    uint8_t hash[32];
} memory_map_entry_t;

enum ambit3_fw_gen {
    AMBIT3_FW_GEN1,
    AMBIT3_FW_GEN2,
    AMBIT3_FW_GEN3,
    AMBIT3_FW_GEN4,
    AMBIT3_VERT_FW_GEN1,
    AMBIT3_VERT_FW_GEN2,
    AMBIT3_VERT_FW_GEN3,
    TRAVERSE_FW_GEN1,
    TRAVERSE_FW_GEN2,
};

typedef struct ambit3_driver_params_s {
    uint8_t mm_legacy_format;
    uint8_t mm_entry_data_id;
    uint8_t log_header_request_data_id;
    uint8_t log_entries_total_data_id;
    uint8_t log_entries_notsynced_data_id;
    uint8_t log_header_data_id;
    uint8_t log_header_tail_length;
} ambit3_driver_params_t;

struct ambit_device_driver_data_s {
    libambit_pmem20_t pmem20;
    libambit_sbem0102_t sbem0102;
    struct {
        uint8_t initialized;
        memory_map_entry_t waypoints;
        memory_map_entry_t routes;
        memory_map_entry_t rules;
        memory_map_entry_t gps;
        memory_map_entry_t sport_modes;
        memory_map_entry_t training_program;
        memory_map_entry_t exercise_log;
        memory_map_entry_t event_log;
        memory_map_entry_t ble_pairing;
        memory_map_entry_t apps;
        memory_map_entry_t glonass; // Ambit3 Vertical / Traverse
        memory_map_entry_t track_log; // Traverse
    } memory_maps;
    enum ambit3_fw_gen fw_gen;
    ambit3_driver_params_t driver_params;
};

typedef struct ambit3_log_header_s {
    ambit_log_header_t header;
    uint32_t address;
    uint32_t end_address;
    uint32_t address2;
    uint32_t end_address2;
    uint8_t synced;
} ambit3_log_header_t;

/*
 * Static functions
 */
static void init(ambit_object_t *object, uint32_t driver_param);
static void deinit(ambit_object_t *object);
static int personal_settings_get(ambit_object_t *object, ambit_personal_settings_t *settings);
static int log_read(ambit_object_t *object, ambit_log_skip_cb skip_cb, ambit_log_push_cb push_cb, ambit_log_progress_cb progress_cb, void *userref);
static int gps_orbit_header_read(ambit_object_t *object, uint8_t data[8]);
static int gps_orbit_write(ambit_object_t *object, uint8_t *data, size_t datalen);

static int parse_log_header_block(ambit_object_t *object, libambit_sbem0102_data_t *reply_data_object, ambit_log_skip_cb skip_cb, ambit_log_push_cb push_cb, ambit_log_progress_cb progress_cb, void *userref, uint16_t *log_entries_walked, uint16_t log_entries_total);
static size_t parse_log_entry(ambit_object_t *object, const uint8_t *log_data, ambit3_log_header_t *log_header);
static int get_memory_maps(ambit_object_t *object);

/*
 * Global variables
 */
ambit_device_driver_t ambit_device_driver_ambit3 = {
    init,
    deinit,
    libambit_device_driver_lock_log,
    libambit_device_driver_date_time_set,
    libambit_device_driver_status_get,
    personal_settings_get,
    log_read,
    gps_orbit_header_read,
    gps_orbit_write,
    NULL,
    NULL,
    NULL,
    NULL
};


/*
 * Static functions implementation
 */

/**
 * Performs a lookup of the ambit3 firmware generation for the watch product id and firmware version.
 *
 * \param device_info Device info structure of the watch.
 * \return Enumeration of the watch firmware generation.
 */
static enum ambit3_fw_gen get_ambit3_fw_gen(ambit_device_info_t *device_info)
{
    struct generation {
        uint8_t fw_version[4];
        enum ambit3_fw_gen gen;
    };

    struct generation ambit3_gen[] =  {
        {{2, 4, 1, 0}, AMBIT3_FW_GEN4},
        {{2, 2, 16, 0}, AMBIT3_FW_GEN3},
        {{2, 0, 4, 0}, AMBIT3_FW_GEN2},
        {{0, 0, 0, 0}, AMBIT3_FW_GEN1},
    };

    struct generation ambit3_vert_gen[] = {
        {{1, 1, 22, 0}, AMBIT3_VERT_FW_GEN3},
        {{1, 0, 27, 0}, AMBIT3_VERT_FW_GEN2},
        {{1, 0, 0, 0}, AMBIT3_VERT_FW_GEN1},
    };

    struct generation traverse_gen[] = {
        {{2, 0, 18, 0}, TRAVERSE_FW_GEN2},
        {{1, 0, 4, 0}, TRAVERSE_FW_GEN1},
    };

    struct generation* iter;

    switch (device_info->product_id) {
      case 0x1e:
      case 0x1c:
      case 0x1b:
        for (size_t i = 0; i < sizeof (ambit3_gen) / sizeof ((ambit3_gen)[0]) && (iter = &ambit3_gen[i]); i++) {
            if (libambit_fw_version_number(iter->fw_version) <= libambit_fw_version_number(device_info->fw_version))
                return iter->gen;
        }
        break;
      case 0x2c: // Ambit3 Vertical
        for (size_t i = 0; i < sizeof (ambit3_vert_gen) / sizeof ((ambit3_vert_gen)[0]) && (iter = &ambit3_vert_gen[i]); i++) {
            if (libambit_fw_version_number(iter->fw_version) <= libambit_fw_version_number(device_info->fw_version))
                return iter->gen;
        }
        break;
      case 0x2b: // Traverse
      case 0x2d: // Traverse Alpha
        for (size_t i = 0; i < sizeof (traverse_gen) / sizeof ((traverse_gen)[0]) && (iter = &traverse_gen[i]); i++) {
            if (libambit_fw_version_number(iter->fw_version) <= libambit_fw_version_number(device_info->fw_version))
                return iter->gen;
        }
        break;
    }

    abort();
}

/**
 * Performs a lookup of the ambit3 driver parameters for the watch firmware generation.
 *
 * \param fw_gen Firmware version of the watch.
 * \return ambit3_driver_params_t structure containing the driver parameters.
 */
static ambit3_driver_params_t get_ambit3_driver_params(enum ambit3_fw_gen fw_gen) {

    struct ambit3_driver_params_lookup {
        enum ambit3_fw_gen gen;
        ambit3_driver_params_t driver_params;
    };

    struct ambit3_driver_params_lookup dp_lookup[] =  {
            {AMBIT3_FW_GEN1,        {0x02, 0x3f, 0x81, 0x4e, 0x4f, 0x7e, 0x00}},
            {AMBIT3_FW_GEN2,        {0x00, 0x4b, 0x8d, 0x5a, 0x5b, 0x8a, 0x1a}},
            {AMBIT3_FW_GEN3,        {0x00, 0x4a, 0x8c, 0x59, 0x5a, 0x89, 0x1a}},
            {AMBIT3_FW_GEN4,        {0x00, 0x4a, 0x8d, 0x59, 0x5a, 0x8a, 0x1c}},
            {AMBIT3_VERT_FW_GEN1,   {0x00, 0x4a, 0x8c, 0x59, 0x5a, 0x8a, 0x1c}},
            {AMBIT3_VERT_FW_GEN2,   {0x00, 0x4a, 0x8b, 0x58, 0x59, 0x89, 0x1c}},
            {AMBIT3_VERT_FW_GEN3,   {0x00, 0x4a, 0x8a, 0x57, 0x58, 0x88, 0x1c}},
            {TRAVERSE_FW_GEN1,      {0x00, 0x4a, 0x8a, 0x56, 0x57, 0x87, 0x1c}},
            {TRAVERSE_FW_GEN2,      {0x00, 0x4a, 0x88, 0x55, 0x56, 0x86, 0x1c}},
    };

    ambit3_driver_params_t params;

    struct ambit3_driver_params_lookup* iter;

    for (size_t i = 0; i < sizeof (dp_lookup) / sizeof ((dp_lookup)[0]) && (iter = &dp_lookup[i]); i++) {
        if (iter->gen == fw_gen) {
            params = iter->driver_params;
            break;
        }
    }

    return params;
}

/**
 * Initialises the ambit3 driver.
 *
 * \param object
 * \param PMEM20 chunk size.
 */
static void init(ambit_object_t *object, uint32_t driver_param)
{
    struct ambit_device_driver_data_s *data;

    if ((data = calloc(1, sizeof(struct ambit_device_driver_data_s))) != NULL) {
        object->driver_data = data;
        libambit_pmem20_init(&object->driver_data->pmem20, object, driver_param);
        libambit_sbem0102_init(&object->driver_data->sbem0102, object, driver_param);

        // get fw generation specific parameters
        object->driver_data->fw_gen = get_ambit3_fw_gen(&object->device_info);
        object->driver_data->driver_params = get_ambit3_driver_params(object->driver_data->fw_gen);
    }
}

/**
 * De-initialises the ambit3 driver.
 *
 * \param object
 */
static void deinit(ambit_object_t *object)
{
    if (object->driver_data != NULL) {
        libambit_pmem20_deinit(&object->driver_data->pmem20);
        libambit_sbem0102_deinit(&object->driver_data->sbem0102);
    }
}

static float ieee754_to_float(uint32_t bits)
{
    int sign = bits >> 31 ? -1 : 1;
    int exp = (int)(((bits >> 23) & 0xff) - 127);
    int frac = (int)((bits & 0x7fffff) | 0x800000);

    return sign * frac * powf(2.0f, exp - 23);
}

/**
 * Gets the personal settings from the watch.
 *
 * \param object
 * \param settings Setting structure to populate.
 * \return 0 if successful.
 */
static int personal_settings_get(ambit_object_t *object, ambit_personal_settings_t *settings)
{
    uint8_t send_data[4] = { 0x00, 0x00, 0x00, 0x00 };
    libambit_sbem0102_data_t reply_data_object;
    uint32_t alarm_num;
    uint32_t decli_num;

    LOG_INFO("Reading personal settings");

    libambit_sbem0102_data_init(&reply_data_object);
    if (libambit_sbem0102_command_request_raw(&object->driver_data->sbem0102, ambit_command_ambit3_settings, send_data, sizeof(send_data), &reply_data_object) != 0) {
        LOG_WARNING("Failed to read personal settings");
        return -1;
    }

    memset(settings, 0, sizeof(ambit_personal_settings_t));

    while (libambit_sbem0102_data_next(&reply_data_object) == 0) {
        switch (libambit_sbem0102_data_id(&reply_data_object)) {
          case 0x01:
            settings->date_format = libambit_sbem0102_data_ptr(&reply_data_object)[0];
            break;
          case 0x02:
            settings->tones_mode = libambit_sbem0102_data_ptr(&reply_data_object)[0];
            break;
          case 0x03:
            settings->gps_position_format = libambit_sbem0102_data_ptr(&reply_data_object)[0];
            break;
          case 0x04:
            decli_num = read32(libambit_sbem0102_data_ptr(&reply_data_object), 0);
            settings->compass_declination_f = ieee754_to_float(decli_num);
            break;
          case 0x08:
            settings->units_mode = libambit_sbem0102_data_ptr(&reply_data_object)[0];
            break;
          case 0x12:
            settings->language = libambit_sbem0102_data_ptr(&reply_data_object)[0];
            break;
          case 0x13: // Map orientation
            settings->navigation_style = libambit_sbem0102_data_ptr(&reply_data_object)[0];
            break;
          case 0x14:
            settings->time_format = libambit_sbem0102_data_ptr(&reply_data_object)[0];
            break;
          case 0x15:
            settings->sync_time_w_gps = !libambit_sbem0102_data_ptr(&reply_data_object)[0];
            break;
          case 0x16: // Dual time enabled
            break;
          case 0x17:
            settings->alarm_enable = libambit_sbem0102_data_ptr(&reply_data_object)[0];
            break;
          case 0x18:
            alarm_num = read32(libambit_sbem0102_data_ptr(&reply_data_object), 0);
            settings->alarm.hour = (alarm_num / 60 / 60);
            settings->alarm.minute = (alarm_num / 60) % 60;
            break;
          case 0x19:
            settings->is_male = libambit_sbem0102_data_ptr(&reply_data_object)[0];
            break;
          case 0x1a:
            settings->weight = read16(libambit_sbem0102_data_ptr(&reply_data_object), 0);
            break;
          case 0x1b:
            settings->max_hr = libambit_sbem0102_data_ptr(&reply_data_object)[0];
            break;
          case 0x1f:
            if (libambit_sbem0102_data_len(&reply_data_object) == 11) {
                sscanf((const char*)libambit_sbem0102_data_ptr(&reply_data_object), "%04hu-", &settings->birthyear);
            }
            break;
          case 0x20: // Display contrast
            settings->display_brightness = libambit_sbem0102_data_ptr(&reply_data_object)[0];
            break;
          case 0x21:
            settings->display_is_negative = libambit_sbem0102_data_ptr(&reply_data_object)[0];
            break;
          case 0x22:
            settings->backlight_mode = libambit_sbem0102_data_ptr(&reply_data_object)[0];
            break;
          case 0x23:
            settings->backlight_brightness = libambit_sbem0102_data_ptr(&reply_data_object)[0];
            break;
          case 0x26:
            settings->alti_baro_mode = libambit_sbem0102_data_ptr(&reply_data_object)[0];
            break;
          case 0x27:
            settings->fused_alti_disabled = libambit_sbem0102_data_ptr(&reply_data_object)[0];
            break;
          case 0x28:
            settings->storm_alarm = libambit_sbem0102_data_ptr(&reply_data_object)[0];
            break;
          default:
            /*
            printf("Got id=%02x: ", libambit_sbem0102_data_id(&reply_data_object));
            switch(libambit_sbem0102_data_len(&reply_data_object)) {
              case 1:
                printf("%d", libambit_sbem0102_data_ptr(&reply_data_object)[0]);
                break;
              case 2:
                printf("%d", read16(libambit_sbem0102_data_ptr(&reply_data_object), 0));
                break;
              case 4:
                printf("%d", read32(libambit_sbem0102_data_ptr(&reply_data_object), 0));
                break;
              default:
                {
                    int q;
                for(q=0; q<libambit_sbem0102_data_len(&reply_data_object); q++)
                    printf("%02x", libambit_sbem0102_data_ptr(&reply_data_object)[q]);
                }
                break;
            }
            printf("\n");
            */
            break;
        }
    }

    return 0;
}

/**
 * Processes the log read replies for gen1 firmware. Gen1 has a log_header_data_id byte for each log header.
 *
 * \param object
 * \param reply_data_object Data received from the watch.
 * \param skip_cb Callback function when log header received.
 * \param push_cb Callback function when log received.
 * \param progress_cb Callback function when progress received.
 * \param userref
 * \return Number of entries read or -1 if error.
 */
static int process_log_read_replies_gen1(ambit_object_t *object, libambit_sbem0102_data_t *reply_data_object,
                                         ambit_log_skip_cb skip_cb, ambit_log_push_cb push_cb, ambit_log_progress_cb progress_cb, void *userref)
{
    ambit3_log_header_t log_header;
    ambit_log_entry_t *log_entry;

    int entries_read = 0;

    uint16_t log_entries_total = 0;
    uint16_t log_entries_walked = 0;
    uint16_t log_entries_notsynced;
    ONLYDEBUGVAR(log_entries_notsynced);

    log_header.header.activity_name = NULL;

    while (libambit_sbem0102_data_next(reply_data_object) == 0) {
        if (libambit_sbem0102_data_id(reply_data_object) == object->driver_data->driver_params.log_entries_total_data_id) {
            log_entries_total = read16(libambit_sbem0102_data_ptr(reply_data_object), 0);
            LOG_INFO("Number of logs=%d", log_entries_total);
        }
        else if (libambit_sbem0102_data_id(reply_data_object) == object->driver_data->driver_params.log_entries_notsynced_data_id) {
            log_entries_notsynced = read16(libambit_sbem0102_data_ptr(reply_data_object), 0);
            LOG_INFO("Number of logs marked as not synchronized=%d", log_entries_notsynced);
        }
        else if (libambit_sbem0102_data_id(reply_data_object) == object->driver_data->driver_params.log_header_data_id) {
            const uint8_t *data = libambit_sbem0102_data_ptr(reply_data_object);

            if(parse_log_entry(object, data, &log_header) != 0) {
                LOG_INFO("Log header parsed successfully");
                if (!skip_cb || skip_cb(userref, &log_header.header) != 0) {
                    LOG_INFO("Reading data of log %d of %d", log_entries_walked + 1, log_entries_total);
                    log_entry = libambit_pmem20_log_read_entry_address(&object->driver_data->pmem20,
                                                                       log_header.address,
                                                                       log_header.end_address - log_header.address,
                                                                       0, 0,
                                                                       LIBAMBIT_PMEM20_FLAGS_NONE);
                    if (log_entry != NULL) {
                        if (push_cb != NULL) {
                            push_cb(userref, log_entry);
                        }
                        entries_read++;

                        libambit_log_entry_free(log_entry);
                    }
                }
                else {
                    LOG_INFO("Log entry already exists, skipping");
                }
            }
            else {
                LOG_INFO("Failed to parse log header");
            }
            log_entries_walked++;
            if (progress_cb != NULL && log_entries_total != 0) {
                progress_cb(userref, log_entries_total, log_entries_walked, 100*log_entries_walked/log_entries_total);
            }
        }
        else {
            LOG_INFO("Unknown data id 0x%x", libambit_sbem0102_data_id(reply_data_object));
        }
    }
    
    return entries_read;
}

/**
 * Processes the log read replies for all firmware except gen1. There is a log_header_data_id byte for a blick
 * of log headers.
 *
 * \param object
 * \param reply_data_object Data received from the watch.
 * \param skip_cb Callback function when log header received.
 * \param push_cb Callback function when log received.
 * \param progress_cb Callback function when progress received.
 * \param userref
 * \return Number of entries read or -1 if error.
 */
static int process_log_read_replies(ambit_object_t *object, libambit_sbem0102_data_t *reply_data_object,
                                         ambit_log_skip_cb skip_cb, ambit_log_push_cb push_cb, ambit_log_progress_cb progress_cb, void *userref)
{
    uint16_t log_entries_total = 0;
    uint16_t log_entries_notsynced;
    uint16_t log_entries_walked = 0;
    int entries_read = 0;
    ONLYDEBUGVAR(log_entries_notsynced);

    while (libambit_sbem0102_data_next(reply_data_object) == 0) {
        if (libambit_sbem0102_data_id(reply_data_object) == object->driver_data->driver_params.log_entries_total_data_id) {
            log_entries_total = read16(libambit_sbem0102_data_ptr(reply_data_object), 0);
        }
        else if (libambit_sbem0102_data_id(reply_data_object) == object->driver_data->driver_params.log_entries_notsynced_data_id) {
            log_entries_notsynced = read16(libambit_sbem0102_data_ptr(reply_data_object), 0);
            LOG_INFO("Number of logs marked as not synchronized=%d", log_entries_notsynced);
        }
        else if (libambit_sbem0102_data_id(reply_data_object) == object->driver_data->driver_params.log_header_data_id) {
            entries_read = parse_log_header_block(object, reply_data_object, skip_cb, push_cb, progress_cb, userref, &log_entries_walked, log_entries_total);
        }
        else {
            LOG_INFO("Unknown data id 0x%x", libambit_sbem0102_data_id(reply_data_object));
        }
    }

    return entries_read;
}

/**
 * Gets the logs from the watch.
 *
 * \param skip_cb Callback function when log header received.
 * \param push_cb Callback function when log received.
 * \param progress_cb Callback function when progress received.
 * \param userref
 * \return Number of entries read or -1 if error.
 */
static int log_read(ambit_object_t *object, ambit_log_skip_cb skip_cb, ambit_log_push_cb push_cb, ambit_log_progress_cb progress_cb, void *userref)
{
    int entries_read = 0;
    libambit_sbem0102_data_t send_data_object, reply_data_object;
    LOG_INFO("Reading log headers");

    libambit_sbem0102_data_init(&reply_data_object);
    libambit_sbem0102_data_init(&send_data_object);
    libambit_sbem0102_data_add(&send_data_object, object->driver_data->driver_params.log_header_request_data_id, NULL, 0);

    if (libambit_sbem0102_command_request(&object->driver_data->sbem0102, ambit_command_ambit3_log_headers, &send_data_object, &reply_data_object) != 0) {
        LOG_WARNING("Failed to read log headers");
        return -1;
    }

    if (object->driver_data->memory_maps.initialized == 0) {
        if (get_memory_maps(object) != 0) {
            return -1;
        }
    }

    // Initialize PMEM20 log before starting to read logs
    libambit_pmem20_log_init(&object->driver_data->pmem20, object->driver_data->memory_maps.exercise_log.start, object->driver_data->memory_maps.exercise_log.size);

    if (object->driver_data->fw_gen == AMBIT3_FW_GEN1) {
        entries_read = process_log_read_replies_gen1(object, &reply_data_object, skip_cb, push_cb, progress_cb, userref);
    }
    else {
        entries_read = process_log_read_replies(object, &reply_data_object, skip_cb, push_cb, progress_cb, userref);
    }

    printf("Finished reading logs... I think...\n");

    libambit_sbem0102_data_free(&send_data_object);
    printf("Finished freeing data 1\n");
    libambit_sbem0102_data_free(&reply_data_object);

    printf("Finished freeing data 2\n");

    return entries_read;
}

/**
 * Gets the gps orbit header from the watch.
 *
 * \param object
 * \param data Buffer to populate with gps orbit data.
 * \return 0 if successful.
 */
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

/**
 * Writes the gps orbit header to the watch.
 *
 * \param object
 * \param data Buffer containing gps orbit data.
 * \param datalen Size of gps orbit data buiffer.
 * \return 0 if successful.
 */
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
            ret = libambit_pmem20_gps_orbit_write(&object->driver_data->pmem20, data, datalen, true);
        }
        else {
            LOG_INFO("Current GPS orbit data is already up to date, skipping");
            ret = 0;
        }
    }

    return ret;
}

/**
 * Processes a block of log headers (not gen1).
 *
 * \param object
 * \param reply_data_object Data received from the watch.
 * \param skip_cb Callback function when log header received.
 * \param push_cb Callback function when log received.
 * \param progress_cb Callback function when progress received.
 * \param userref
 * \param log_entries_walked Log entries walked.
 * \param log_entries_total Log entries total.
 * \return Number of entries read in this block or -1 if error.
 */
static int parse_log_header_block(ambit_object_t *object, libambit_sbem0102_data_t *reply_data_object, ambit_log_skip_cb skip_cb, ambit_log_push_cb push_cb, ambit_log_progress_cb progress_cb, void *userref,  uint16_t *log_entries_walked, uint16_t log_entries_total)
{
    ambit3_log_header_t log_header;
    ambit_log_entry_t *log_entry;
    const uint8_t *data;
    size_t length = 0;
    size_t offset = 0;
    size_t log_read_len = 0;
    int current_parse_num_log_read = 0;

    length = libambit_sbem0102_data_len(reply_data_object);
    data = libambit_sbem0102_data_ptr(reply_data_object);

    while(offset<length) {

        log_header.header.activity_name = NULL;
        log_read_len = parse_log_entry(object, &data[offset], &log_header);

        if(log_read_len == 0) {
            LOG_ERROR("Could not parse log header");
            return -1;
        }

        offset += log_read_len;

        LOG_INFO ("Next offset: %d of %d\n", offset, length);
        
        if (!skip_cb || skip_cb(userref, &log_header.header) != 0) {
            LOG_INFO("Reading data of log %d of %d", *log_entries_walked + 1, log_entries_total);
            log_entry = libambit_pmem20_log_read_entry_address(&object->driver_data->pmem20,
                                                               log_header.address,
                                                               log_header.end_address - log_header.address,
                                                               log_header.address2,
                                                               log_header.end_address2 - log_header.address2,
                                                               LIBAMBIT_PMEM20_FLAGS_UNKNOWN2_PADDING_48);
            LOG_INFO("Completed data of log %d of %d", *log_entries_walked + 1, log_entries_total);
            if (log_entry != NULL) {
                if (push_cb != NULL) {
                    push_cb(userref, log_entry);
                    LOG_INFO("Completed push_cb");
                }
            }
        }
        else {
            LOG_INFO("Log entry already exists, skipping");
        }

        (*log_entries_walked)++;
        current_parse_num_log_read++;

        if(*log_entries_walked > log_entries_total) {
            log_entries_total = *log_entries_walked; // Handle situations where ambit reports wrong number of total entries
        }

        if (progress_cb != NULL && log_entries_total != 0) {
            LOG_INFO("Do progress_cb");
            progress_cb(userref, log_entries_total, *log_entries_walked, 100*(*log_entries_walked)/log_entries_total);
        }
    }

    return current_parse_num_log_read;
}

/**
 * Parses the log header data for one log entry..
 *
 * \param data Raw log header data buffer.
 * \param log_header Structure to populate with header field values.
 * \return Buffer offset at the end of the log header.
 */
static size_t parse_log_entry(ambit_object_t *object, const uint8_t *data, ambit3_log_header_t *log_header)
{
    struct tm tm;
    char *ptr;
    size_t offset = 0;

    // Start with parsing the time
    if ((ptr = libambit_strptime((const char *)data, "%Y-%m-%dT%H:%M:%S", &tm)) == NULL) {
        return 0;
    }

    log_header->header.date_time.year = 1900 + tm.tm_year;
    log_header->header.date_time.month = tm.tm_mon + 1;
    log_header->header.date_time.day = tm.tm_mday;
    log_header->header.date_time.hour = tm.tm_hour;
    log_header->header.date_time.minute = tm.tm_min;
    log_header->header.date_time.msec = tm.tm_sec*1000;
    offset += (size_t)ptr - (size_t)data + 1;

    log_header->synced = read8inc(data, &offset);

    log_header->address = read32inc(data, &offset);
    log_header->end_address = read32inc(data, &offset);
    log_header->address2 = read32inc(data, &offset);
    log_header->end_address2 = read32inc(data, &offset);
    log_header->header.heartrate_min = read8inc(data, &offset);
    log_header->header.heartrate_avg = read8inc(data, &offset);
    log_header->header.heartrate_max = read8inc(data, &offset);

    if (object->driver_data->fw_gen == AMBIT3_FW_GEN1 || object->driver_data->fw_gen == AMBIT3_FW_GEN2) {
        log_header->header.heartrate_max_time = read32inc(data, &offset);
        log_header->header.heartrate_min_time = read32inc(data, &offset);
    }
    else {
        log_header->header.heartrate_min_time = read32inc(data, &offset);
        log_header->header.heartrate_max_time = read32inc(data, &offset);
    }

    // temperature format is messed up, 1 byte is missing, just skip for now
    log_header->header.temperature_min = 0;
    log_header->header.temperature_max = 0;
    offset += 2;
    log_header->header.temperature_min_time = read32inc(data, &offset);
    log_header->header.temperature_max_time = read32inc(data, &offset);
    log_header->header.altitude_min = read16inc(data, &offset);
    log_header->header.altitude_max = read16inc(data, &offset);
    log_header->header.altitude_min_time = read32inc(data, &offset);
    log_header->header.altitude_max_time = read32inc(data, &offset);
    log_header->header.cadence_avg = read8inc(data, &offset);
    log_header->header.cadence_max = read8inc(data, &offset);
    log_header->header.cadence_max_time = read32inc(data, &offset);
    log_header->header.speed_avg = read16inc(data, &offset); // 10 m/h
    log_header->header.speed_max = read16inc(data, &offset); // 10 m/h
    log_header->header.speed_max_time = read32inc(data, &offset);
    offset += 4; // Unknown bytes
    log_header->header.duration = read32inc(data, &offset)*100; // seconds 0.1
    log_header->header.ascent = read16inc(data, &offset);
    log_header->header.descent = read16inc(data, &offset);
    log_header->header.ascent_time = read32inc(data, &offset)*1000;
    log_header->header.descent_time = read32inc(data, &offset)*1000;
    log_header->header.recovery_time = read16inc(data, &offset)*60*1000;
    log_header->header.peak_training_effect = read8inc(data, &offset);

    if (log_header->header.activity_name) {
        free(log_header->header.activity_name);
    }
    log_header->header.activity_name = utf8memconv((const char*)(data + offset), 16, "ISO-8859-15");

    offset += (strnlen((const char*)(data + offset), 20)+1);

    log_header->header.distance = read32inc(data, &offset);
    log_header->header.energy_consumption = read16inc(data, &offset);

    offset += object->driver_data->driver_params.log_header_tail_length;

    return offset;
}

/**
 * Gets the memory maps from the watch.
 *
 * \param object
 * \return 0 if successful.
 */
static int get_memory_maps(ambit_object_t *object)
{
    uint8_t legacy_format = 0;
    uint8_t *reply_data = NULL;
    size_t replylen = 0;
    uint8_t send_data[4] = { 0x00, 0x00, 0x00, 0x00 };
    libambit_sbem0102_data_t reply_data_object;
    uint8_t mm_entry_data_id = 0;
    memory_map_entry_t *mm_entry;
    const uint8_t *ptr;

    legacy_format = object->driver_data->driver_params.mm_legacy_format;

    if (libambit_protocol_command(object, ambit_command_waypoint_count, NULL, 0, &reply_data, &replylen, legacy_format) != 0 || replylen < 4) {
        libambit_protocol_free(reply_data);
        LOG_WARNING("Failed to read memory map key");
        return -1;
    }
    libambit_protocol_free(reply_data);

    libambit_sbem0102_data_init(&reply_data_object);
    if (libambit_sbem0102_command_request_raw(&object->driver_data->sbem0102, ambit_command_ambit3_memory_map, send_data, sizeof(send_data), &reply_data_object) != 0) {
        LOG_WARNING("Failed to read memory map");
        return -1;
    }

    mm_entry_data_id = object->driver_data->driver_params.mm_entry_data_id;

    while (libambit_sbem0102_data_next(&reply_data_object) == 0) {
        if (libambit_sbem0102_data_id(&reply_data_object) == mm_entry_data_id) {
            ptr = libambit_sbem0102_data_ptr(&reply_data_object);
            LOG_INFO("Memory map entry \"%s\"", ptr);
            mm_entry = NULL;
            if (strcmp((char*)ptr, "Waypoints") == 0) {
                mm_entry = &object->driver_data->memory_maps.waypoints;
            }
            else if (strcmp((char*)ptr, "Routes") == 0) {
                mm_entry = &object->driver_data->memory_maps.routes;
            }
            else if (strcmp((char*)ptr, "Rules") == 0) {
                mm_entry = &object->driver_data->memory_maps.rules;
            }
            else if (strcmp((char*)ptr, "GpsSGEE") == 0) {
                mm_entry = &object->driver_data->memory_maps.gps;
            }
            else if (strcmp((char*)ptr, "CustomModes") == 0) {
                mm_entry = &object->driver_data->memory_maps.sport_modes;
            }
            else if (strcmp((char*)ptr, "TrainingProgram") == 0) {
                mm_entry = &object->driver_data->memory_maps.training_program;
            }
            else if (strcmp((char*)ptr, "ExerciseLog") == 0) {
                mm_entry = &object->driver_data->memory_maps.exercise_log;
            }
            else if (strcmp((char*)ptr, "EventLog") == 0) {
                mm_entry = &object->driver_data->memory_maps.event_log;
            }
            else if (strcmp((char*)ptr, "BlePairingInfo") == 0) {
                mm_entry = &object->driver_data->memory_maps.ble_pairing;
            }
            else if (strcmp((char*)ptr, "Apps") == 0) {
                mm_entry = &object->driver_data->memory_maps.apps;
            }
            else {
                LOG_WARNING("Unknown memory map type \"%s\"", (char*)ptr);
            }

            if (mm_entry != NULL) {
                // We have dealed with the name, advance to hash
                ptr += strlen((char*)ptr) + 1;

                if (libambit_htob((const char*)ptr, mm_entry->hash, sizeof(mm_entry->hash)) < 0) {
                    LOG_ERROR("Failed to read memory map hash");
                }
                ptr += strlen((char*)ptr) + 1;

                mm_entry->start = read32(ptr, 0);
                ptr += 4;
                mm_entry->size = read32(ptr, 0);
            }
        }
    }

    object->driver_data->memory_maps.initialized = 1;
    libambit_sbem0102_data_free(&reply_data_object);

    LOG_INFO("Memory map successfully parsed");

    return 0;
}
