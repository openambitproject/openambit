/*
 * (C) Copyright 2017 Lars Andre Landås
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
 * Lars Andre Landås (landas@gmail.com)
 * Kristoffer Tonheim (kristoffer.tonheim@gmail.com)
 *
 */

#ifndef __DEVICE_DRIVER_AMBIT_NAVIGATION_H__
#define __DEVICE_DRIVER_AMBIT_NAVIGATION_H__

#include <stddef.h>
#include <stdint.h>
#include "libambit.h"

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

// Altitude is not stored inside Ambit2 S

typedef struct __attribute__((__packed__)) ambit_pack_waypoint_s {
    uint16_t      index;
    uint16_t      unknown;
    char          name[16];
    char          route_name[16];
    uint8_t       ctime_second;
    uint8_t       ctime_minute;
    uint8_t       ctime_hour;
    uint8_t       ctime_day;
    uint8_t       ctime_month;
    uint16_t      ctime_year;
    int32_t       latitude;
    int32_t       longitude;
    uint16_t      type;
    uint8_t       name_count;
    uint8_t       status; //0 - synced, 1 - (added in watch), 2 - (removed in watch)
} ambit_pack_waypoint_t;

// Cross reference table for converting to and from movescount waypoint type id
#define NUM_WAYPOINTS_MOVESCOUNT 32
#define NUM_WAYPOINTS_AMBIT 18

extern const uint8_t ambit_waypoint_types_from_movescount[NUM_WAYPOINTS_MOVESCOUNT];
extern const uint8_t ambit_waypoint_types_to_movescount[NUM_WAYPOINTS_AMBIT];

int ambit_navigation_read(ambit_object_t *object, ambit_pack_waypoint_t **waypoint_data, uint16_t *waypoint_count);
int ambit_navigation_write(ambit_object_t *object,ambit_pack_waypoint_t *waypoint_data, uint16_t waypoint_count);
void ambit_navigation_print_struct(ambit_pack_waypoint_t *str);

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif


#endif /* __DEVICE_DRIVER_AMBIT_NAVIGATION_H__ */
