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
#include "distance.h"
#include "crc16.h"

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
    uint8_t       type;
    uint8_t       unknown2;
    uint8_t       name_count;
    uint8_t       status; //0 - synced, 1 - (added in watch), 2 - (removed in watch)
} ambit_pack_waypoint_t;

typedef struct __attribute__((__packed__)) ambit_pack_write_head_s {
    uint32_t      memory_addr_start; // +1024
    uint16_t      pack_len; //bytes
    uint16_t      pack_sequence;
} ambit_pack_write_head_t;

typedef struct __attribute__((__packed__)) ambit_pack_route_info_head_s {
    uint16_t      unknown1; //always 12296 (can be two uint8_t?)
    uint8_t       unknown2; //always 0?
    uint8_t       unknown3; //always 1?
    uint16_t      route_count;
    uint16_t      unknown4; //always 0?
    uint32_t      routepoint_count;
    uint16_t      checksum;
    uint16_t      unknown6; //always 0
    uint16_t      unknown8; //always 0
    uint16_t      unknown9; //always 0
    uint16_t      unknown10; //always 0
    uint16_t      unknown11; //always 0
    uint16_t      unknown12; //always 0
    uint16_t      unknown13; //always 0
    uint16_t      unknown14; //always 0
    uint16_t      unknown15; //always 0
} ambit_pack_route_info_head_t;

typedef struct __attribute__((__packed__)) ambit_pack_route_info_s {
    char          name[16];
    uint32_t      routepoint_start_index;
    uint16_t      routepoint_count;
    uint32_t      distance; //length of route from first to last point in meters
    int32_t       latitude;
    int32_t       longitude;
    int32_t       max_x_axis_rel_eastern_point; //why??
    int32_t       max_y_axis_rel_nothern_point; //Why??
    uint16_t      unknown1; //Mostly 0xffff
    uint16_t      unknown2; //Mostly 0xffff
    uint16_t      unknown3; //Always 0??
} ambit_pack_route_info_t;

typedef struct __attribute__((__packed__)) ambit_pack_routepoints_s {
    int32_t       x_axis_rel;
    int32_t       y_axis_rel;
} ambit_pack_routepoints_t;

typedef struct ambit_pack_routes_s {
    ambit_pack_route_info_head_t    *data_head;
    ambit_pack_route_info_t         *data_route_info;
    ambit_pack_routepoints_t        *data_routepoints;

    size_t route_info_length; //bytes
    size_t route_info_offset; //bytes
    size_t routepoints_length; //bytes
    size_t routepoints_offset; //bytes
} ambit_pack_routes_t;

// Cross reference table for converting to and from movescount waypoint type id
#define NUM_WAYPOINTS_MOVESCOUNT 34
#define NUM_WAYPOINTS_AMBIT 18

const uint8_t ambit_waypoint_types_from_movescount[NUM_WAYPOINTS_MOVESCOUNT];
const uint8_t ambit_waypoint_types_to_movescount[NUM_WAYPOINTS_AMBIT];

int ambit_navigation_read(ambit_object_t *object, ambit_personal_settings_t *settings);
int ambit_navigation_write(ambit_object_t *object, ambit_personal_settings_t *settings);
int ambit_navigation_waypoint_read(ambit_object_t *object, ambit_pack_waypoint_t **waypoint_data, uint16_t *waypoint_count);
int ambit_navigation_waypoint_write(ambit_object_t *object,ambit_pack_waypoint_t *waypoint_data, uint16_t waypoint_count);
void ambit_navigation_print_struct(ambit_pack_waypoint_t *str);

ambit_pack_routes_t ambit_navigation_route_init(uint16_t route_count, uint32_t routepoints_count);
void ambit_navigation_route_free(ambit_pack_routes_t routes);

int ambit_navigation_route_write(ambit_object_t *object, ambit_personal_settings_t *ps);
int ambit_navigation_route_write_to_packs(ambit_object_t *object, ambit_pack_routes_t *routes);
void ambit_navigation_route_add_checksum(ambit_pack_routes_t *routes);


#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif


#endif /* __DEVICE_DRIVER_AMBIT_NAVIGATION_H__ */
