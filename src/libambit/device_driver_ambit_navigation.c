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

#include "libambit.h"
#include "device_driver_ambit_navigation.h"
#include "protocol.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const uint8_t ambit_waypoint_types_from_movescount[NUM_WAYPOINTS_MOVESCOUNT] = { 0,0,3,3,2,2,7,7,7,10,10,10,16,16,16,16,12,12,12,12,8,4,13,5,6,9,17,14,14,15,11,1 };
const uint8_t ambit_waypoint_types_to_movescount[NUM_WAYPOINTS_AMBIT] = { 0,31,4,2,21,23,24,6,20,25,9,30,16,22,27,29,12,26 };

int ambit_navigation_read(ambit_object_t *object, ambit_personal_settings_t *personal_settings) {

    ambit_pack_waypoint_t *pack_data = NULL;
    uint16_t ambit_pack_count = 0;
    int ret = ambit_navigation_waypoint_read(object, &pack_data, &ambit_pack_count);

    if(personal_settings->waypoints.data != NULL) {
        free(personal_settings->waypoints.data);
        personal_settings->waypoints.data = NULL;
    }

    personal_settings->waypoints.data = (ambit_waypoint_t*)malloc(sizeof(ambit_waypoint_t)*ambit_pack_count);
    personal_settings->waypoints.count = ambit_pack_count;

    for(int x=0;x<ambit_pack_count;x++) {
        personal_settings->waypoints.data[x].altitude = 0;
        personal_settings->waypoints.data[x].index = le16toh(pack_data[x].index);
        strncpy(personal_settings->waypoints.data[x].name, pack_data[x].name, 15);
        strncpy(personal_settings->waypoints.data[x].route_name, pack_data[x].route_name, 15);
        personal_settings->waypoints.data[x].ctime_second = pack_data[x].ctime_second;
        personal_settings->waypoints.data[x].ctime_minute = pack_data[x].ctime_minute;
        personal_settings->waypoints.data[x].ctime_hour = pack_data[x].ctime_hour;
        personal_settings->waypoints.data[x].ctime_day = pack_data[x].ctime_day;
        personal_settings->waypoints.data[x].ctime_month = pack_data[x].ctime_month;
        personal_settings->waypoints.data[x].ctime_year = le16toh(pack_data[x].ctime_year);
        personal_settings->waypoints.data[x].latitude = le32toh(pack_data[x].latitude);
        personal_settings->waypoints.data[x].longitude = le32toh(pack_data[x].longitude);
        personal_settings->waypoints.data[x].type = pack_data[x].type;
        personal_settings->waypoints.data[x].status = pack_data[x].status;
    }

    if(pack_data != NULL) {
        free(pack_data);
    }

    return ret;
}

int ambit_navigation_write(ambit_object_t *object, ambit_personal_settings_t *personal_settings) {

    int ret = -1;

    if(personal_settings->waypoints.count>0) {
        ambit_pack_waypoint_t *pack_data;
        pack_data = (ambit_pack_waypoint_t*)calloc(personal_settings->waypoints.count,sizeof(ambit_pack_waypoint_t));

        for(int x=0; x<personal_settings->waypoints.count; x++) {
            pack_data[x].index = le16toh(pack_data[x].index);
            strncpy(pack_data[x].name, personal_settings->waypoints.data[x].name, 15);
            strncpy(pack_data[x].route_name, personal_settings->waypoints.data[x].route_name, 15);
            pack_data[x].ctime_second = personal_settings->waypoints.data[x].ctime_second;
            pack_data[x].ctime_minute = personal_settings->waypoints.data[x].ctime_minute;
            pack_data[x].ctime_hour = personal_settings->waypoints.data[x].ctime_hour;
            pack_data[x].ctime_day = personal_settings->waypoints.data[x].ctime_day;
            pack_data[x].ctime_month = personal_settings->waypoints.data[x].ctime_month;
            pack_data[x].ctime_year = htole16(personal_settings->waypoints.data[x].ctime_year);
            pack_data[x].latitude = htole32(personal_settings->waypoints.data[x].latitude);
            pack_data[x].longitude = htole32(personal_settings->waypoints.data[x].longitude);
            pack_data[x].type = personal_settings->waypoints.data[x].type;
            pack_data[x].status = personal_settings->waypoints.data[x].status;
        }

        ret = ambit_navigation_waypoint_write(object, pack_data, personal_settings->waypoints.count);

        free(pack_data);
    } else {
        ret = ambit_navigation_waypoint_write(object, NULL, 0);
    }

    return ret;
}

int ambit_navigation_waypoint_read(ambit_object_t *object, ambit_pack_waypoint_t **waypoint_data, uint16_t *way_point_count) {

    uint8_t *reply_data = NULL;
    size_t replylen = 0;
    ambit_pack_waypoint_t *send_waypoint_data = NULL;
    ambit_pack_waypoint_t *waypoint_return_list = NULL;
    uint8_t *send_data = NULL;
    size_t sendlen = 0;
    uint16_t x;

    *way_point_count = 0;

    //get number of waypoints
    if( libambit_protocol_command(object, ambit_command_waypoint_count, NULL, 0, &reply_data, &replylen, 0) != 0) {
        LOG_WARNING("Failed to read number of waypoints entries");
        libambit_protocol_free(reply_data);
        return -1;
    }

    *way_point_count = le16toh(*(uint16_t*)(reply_data));
    waypoint_return_list = malloc(sizeof(ambit_pack_waypoint_t)*(*way_point_count));
    
    libambit_protocol_free(reply_data);

    sendlen = sizeof(ambit_pack_waypoint_t);

    for(x=0; x<(*way_point_count); ++x) {

        send_waypoint_data = malloc(sizeof(ambit_pack_waypoint_t));
        send_data = malloc(sizeof(ambit_pack_waypoint_t));

        send_waypoint_data->index = htole16(x);

        memcpy(send_data, send_waypoint_data, sizeof(ambit_pack_waypoint_t));
        free(send_waypoint_data);

        if( libambit_protocol_command(object, ambit_command_waypoint_read, send_data, sendlen, &reply_data ,&replylen , 0) == 0 ) {
            memcpy(&waypoint_return_list[x], reply_data, sizeof(ambit_pack_waypoint_t));

            if(waypoint_return_list[x].type < NUM_WAYPOINTS_AMBIT) {
                waypoint_return_list[x].type = ambit_waypoint_types_to_movescount[waypoint_return_list[x].type];
            }

            ambit_navigation_print_struct(&waypoint_return_list[x]);
            printf("\n");
        } else {
            LOG_WARNING("ambit_command_waypoint_read failed: %u\n", x);
        }

        free(send_data);
        send_data = NULL;
        libambit_protocol_free(reply_data);

    }

    *waypoint_data = waypoint_return_list;

    return 0;
}

int ambit_navigation_waypoint_write(ambit_object_t *object,ambit_pack_waypoint_t *waypoint_data, uint16_t waypoint_count) {

    //Notify device, start write
    if( libambit_protocol_command(object, ambit_command_write_start, NULL, 0, NULL, NULL, 0) != 0) {
        LOG_WARNING("Device denied writing (navigation)");
        return -1;
    }

    //Remove all device navigation elements before write
    if( libambit_protocol_command(object, ambit_command_nav_memory_delete, NULL, 0, NULL, NULL, 0) != 0) {
        LOG_WARNING("Failed to remove navigation points from memory");
        return -1;
    }

    if(waypoint_count>0 && waypoint_data != NULL) {
        uint8_t *send_data = NULL;
        ambit_pack_waypoint_t send_pack;
        send_data = malloc(sizeof(ambit_pack_waypoint_t));

        for(int x=0; x < waypoint_count; x++) {
            send_pack = waypoint_data[x];
            send_pack.type = ambit_waypoint_types_from_movescount[send_pack.type];
            send_pack.status = 0;
            memcpy(send_data, &send_pack, sizeof(ambit_pack_waypoint_t));
            libambit_protocol_command(object, ambit_command_waypoint_write, send_data, sizeof(ambit_pack_waypoint_t), NULL, NULL, 0);
        }

        free(send_data);
    }

    return 0;
}

void ambit_navigation_print_struct(ambit_pack_waypoint_t *str) {

	printf("index: %u\n", le16toh(str->index));
	printf("unknown: %u\n", str->unknown);
	printf("name: %s\n", str->name);
	printf("route_name: %s\n", str->route_name);
	printf("ctime_second: %u\n", str->ctime_second);
	printf("ctime_minute: %u\n", str->ctime_minute);
	printf("ctime_hour: %u\n", str->ctime_hour);
	printf("ctime_day: %u\n", str->ctime_day);
	printf("ctime_month: %u\n", str->ctime_month);
	printf("ctime_year: %u\n", le16toh(str->ctime_year));
	printf("latitude: %i\n", le16toh(str->latitude));
	printf("longitude: %i\n", le16toh(str->longitude));
	printf("type: %u\n", le16toh(str->type));
	printf("name_count: %u\n", str->name_count);
	printf("status: %u\n", str->status);

}
