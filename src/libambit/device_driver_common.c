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
#include "device_driver_common.h"
#include "protocol.h"
#include "debug.h"

#include <stdlib.h>
#include <string.h>

/*
 * Local definitions
 */

/*
 * Static functions
 */

/*
 * Public functions
 */
int libambit_device_driver_lock_log(ambit_object_t *object, bool lock)
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

int libambit_device_driver_date_time_set(ambit_object_t *object, struct tm *tm)
{
    uint8_t date_data[8] = { 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00 };
    uint8_t time_data[8];
    uint16_t year = htole16(1900 + tm->tm_year);
    uint16_t sec = htole16(1000*tm->tm_sec);
    int ret = -1;

    LOG_INFO("Writing date and time to clock");

    // Set date
    memcpy(&date_data[0], &year, sizeof(uint16_t));
    date_data[2] = 1 + tm->tm_mon;
    date_data[3] = tm->tm_mday;
    // byte[4-7] unknown (but set to 0x28000000 in moveslink)

    // Set time (+date)
    memcpy(&time_data[0], &year, sizeof(uint16_t));
    time_data[2] = 1 + tm->tm_mon;
    time_data[3] = tm->tm_mday;
    time_data[4] = tm->tm_hour;
    time_data[5] = tm->tm_min;
    memcpy(&time_data[6], &sec, sizeof(uint16_t));

    if (libambit_protocol_command(object, ambit_command_date, date_data, sizeof(date_data), NULL, NULL, 0) == 0 &&
        libambit_protocol_command(object, ambit_command_time, time_data, sizeof(time_data), NULL, NULL, 0) == 0) {

        ret = 0;
    }
    else {
        LOG_WARNING("Failed to write date and time");
    }

    return ret;
}

int libambit_device_driver_status_get(ambit_object_t *object, ambit_device_status_t *status)
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
