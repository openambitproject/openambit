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

const uint8_t ambit_waypoint_types_from_movescount[NUM_WAYPOINTS_MOVESCOUNT] = { 0,0,3,3,2,2,7,7,7,10,10,10,16,16,16,16,12,12,12,12,8,4,13,5,6,9,17,14,14,15,11,1,5,6 };
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
            //pack_data[x].index = le16toh(pack_data[x].index);
            pack_data[x].index = 0;
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

        if(ret > -1 && personal_settings->routes.count>0) {
            ret = ambit_navigation_route_write(object, personal_settings);
        }
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
        reply_data = NULL;
        return -1;
    }

    *way_point_count = le16toh(*(uint16_t*)(reply_data));
    waypoint_return_list = malloc(sizeof(ambit_pack_waypoint_t)*(*way_point_count));

    libambit_protocol_free(reply_data);
    reply_data = NULL;

    sendlen = sizeof(ambit_pack_waypoint_t);

    for(x=0; x<(*way_point_count); ++x) {

        send_waypoint_data = malloc(sizeof(ambit_pack_waypoint_t));
        memset(send_waypoint_data, 0, sizeof(ambit_pack_waypoint_t));
        send_data = malloc(sizeof(ambit_pack_waypoint_t));

        send_waypoint_data->index = htole16(x);

        memcpy(send_data, send_waypoint_data, sizeof(ambit_pack_waypoint_t));
        free(send_waypoint_data);

        if( libambit_protocol_command(object, ambit_command_waypoint_read, send_data, sendlen, &reply_data ,&replylen , 0) == 0 ) {
            memcpy(&waypoint_return_list[x], reply_data, sizeof(ambit_pack_waypoint_t));

#ifdef DEBUG_PRINT_INFO
            ambit_navigation_print_struct(&waypoint_return_list[x]);
            printf("\n");
#endif

            if(waypoint_return_list[x].type < NUM_WAYPOINTS_AMBIT) {
                waypoint_return_list[x].type = ambit_waypoint_types_to_movescount[waypoint_return_list[x].type];
            } else {
                waypoint_return_list[x].type = movescount_waypoint_type_poi;
            }

        } else {
            LOG_WARNING("ambit_command_waypoint_read failed: %u\n", x);
        }

        if(send_data != NULL) {
            free(send_data);
            send_data = NULL;
        }
        libambit_protocol_free(reply_data);
        reply_data = NULL;

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
            waypoint_data[x].index = x;
            waypoint_data[x].index = 0;
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

#ifdef DEBUG_PRINT_INFO
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
	printf("unknown2: %u\n", str->unknown2);
	printf("name_count: %u\n", str->name_count);
	printf("status: %u\n", str->status);

}
#endif

ambit_pack_routes_t ambit_navigation_route_init(uint16_t route_count, uint32_t routepoints_count) {
    ambit_pack_routes_t routes;
    routes.data_head = (ambit_pack_route_info_head_t*)calloc(1, sizeof(ambit_pack_route_info_head_t));
    routes.data_head->unknown1 = htole16(12296);
    routes.data_head->unknown3 = 1;
    routes.data_head->route_count = htole16(route_count);
    routes.data_head->routepoint_count = htole32(routepoints_count);
    routes.data_route_info = (ambit_pack_route_info_t*)calloc(route_count, sizeof(ambit_pack_route_info_t));
    routes.data_routepoints = (ambit_pack_routepoints_t*)calloc(routepoints_count, sizeof(ambit_pack_routepoints_t));
    routes.route_info_length = sizeof(ambit_pack_route_info_head_t)+sizeof(ambit_pack_route_info_t)*route_count;
    routes.route_info_offset = 0;
    routes.routepoints_length = sizeof(ambit_pack_routepoints_t)*routepoints_count;
    routes.routepoints_offset = 0;
    return routes;
}
void ambit_navigation_route_free(ambit_pack_routes_t routes) {
    if(routes.data_head != NULL) {
        free(routes.data_head);
    }
    if(routes.data_route_info != NULL) {
        free(routes.data_route_info);
    }
    if(routes.data_routepoints!= NULL) {
        free(routes.data_routepoints);
    }
}

void debug_print_hex(uint8_t *data, size_t datalen) {
    printf("Data:");

    for(int x=0; x<datalen; ++x) {
        if(x%2==0) printf(" ");
        printf("%02x", data[x]);
    }

    printf("\n");
}

int ambit_navigation_route_write(ambit_object_t *object, ambit_personal_settings_t *ps) {
    //TODO: Split function in smaller functions

    uint32_t routepoints_count_tot = 0, routepoint_start_index_offset=0, current_point_offset = 0;


    //calculate total number of routepoints
    for(int x=0; x<ps->routes.count;++x) {
        routepoints_count_tot += ps->routes.data[x].points_count;
    }
    
    ambit_pack_routes_t routes = ambit_navigation_route_init(ps->routes.count, routepoints_count_tot);

    for(int x=0;x<ps->routes.count;++x) {
        ambit_pack_route_info_t *cur = &(routes.data_route_info[x]);
        ambit_route_t *cur_ps = &(ps->routes.data[x]);

        strncpy(cur->name, cur_ps->name, 15);

        cur->routepoint_start_index = htole32(routepoints_count_tot-cur_ps->points_count-routepoint_start_index_offset);
        cur->routepoint_count = htole16(cur_ps->points_count);
        cur->distance = htole32(cur_ps->distance);
        cur->latitude = htole32(cur_ps->mid_lat);
        cur->longitude = htole32(cur_ps->mid_lon);
        cur->max_x_axis_rel_eastern_point = 0; //Will be set later
        cur->max_y_axis_rel_nothern_point = 0; //Will be set later
        cur->unknown1 = 0xffff; //probably needs fix
        cur->unknown2 = 0xffff; //probably needs fix
        cur->unknown3 = 0;

        routepoint_start_index_offset += cur->routepoint_count;

        //pack routepoints
        current_point_offset = cur->routepoint_start_index;


        for(int y=0;y<cur_ps->points_count;++y) {
            int32_t rel_x = (int32_t)(distance_calc((double)(cur_ps->mid_lat)/10000000, (double)(cur_ps->mid_lon)/10000000, (double)(cur_ps->mid_lat)/10000000, (double)(cur_ps->points[y].lon)/10000000)*1000);

            if(cur_ps->points[y].lon<cur_ps->mid_lon) {
                rel_x *= -1;
            }

            int32_t rel_y = (int32_t)(distance_calc((double)(cur_ps->mid_lat)/10000000, (double)(cur_ps->mid_lon)/10000000, (double)(cur_ps->points[y].lat)/10000000, (double)(cur_ps->mid_lon)/10000000)*1000);

            if(cur_ps->points[y].lat<cur_ps->mid_lat) {
                rel_y *= -1;
            }

            if(rel_x > cur->max_x_axis_rel_eastern_point) {
                cur->max_x_axis_rel_eastern_point = htole32(rel_x);
            }
            if(rel_y > cur->max_y_axis_rel_nothern_point) {
                cur->max_y_axis_rel_nothern_point = htole32(rel_y);
            }

            //printf("x_axis_rel: %imeters (%i)\n", rel_x, cur_ps->points[y].lon);
            //printf("y_axis_rel: %imeters (%i)\n", rel_y, cur_ps->points[y].lat);

            routes.data_routepoints[current_point_offset].x_axis_rel = htole32(rel_x);
            routes.data_routepoints[current_point_offset].y_axis_rel = htole32(rel_y);
            ++current_point_offset;
        }
    }

    ambit_navigation_route_add_checksum(&routes);
    ambit_navigation_route_write_to_packs(object, &routes);

    ambit_navigation_route_free(routes);
    return 0;
}

int ambit_navigation_route_write_to_packs(ambit_object_t *object, ambit_pack_routes_t *routes) {

    int ret = 0;
    size_t max_pack_len = 1024;
    size_t pack_len = 0;
    uint16_t pack_sequence = 0;
    uint8_t *data;
    uint32_t device_memory_addr = 0x041EB0;
    ambit_pack_write_head_t write_head;

    //first info pack
    size_t writelen = fmin(max_pack_len, (routes->route_info_length));
    pack_len = sizeof(ambit_pack_write_head_t)+writelen;
    write_head.memory_addr_start = htole32(device_memory_addr);
    write_head.pack_len = htole16(writelen);
    write_head.pack_sequence = 0;

    data = (uint8_t*)malloc(pack_len);
    memcpy(data, &write_head, sizeof(ambit_pack_write_head_t));
    memcpy(data+sizeof(ambit_pack_write_head_t),
            routes->data_head,
            sizeof(ambit_pack_route_info_head_t));
    memcpy(data+sizeof(ambit_pack_write_head_t)+sizeof(ambit_pack_route_info_head_t),
            routes->data_route_info,
            writelen-sizeof(ambit_pack_route_info_head_t));


    //Write to device;
    //debug_print_hex(data, pack_len);
    ret = libambit_protocol_command(object, ambit_command_data_write, data, pack_len, NULL, NULL, 0);
    routes->route_info_offset += writelen;

    free(data);

    if(ret == -1) {
        return ret;
    }

    while(routes->route_info_offset < routes->route_info_length) {
        device_memory_addr += max_pack_len;
        ++pack_sequence;
        writelen = fmin(max_pack_len, routes->route_info_length-routes->route_info_offset);
        pack_len = sizeof(ambit_pack_write_head_t)+writelen;
        write_head.memory_addr_start = htole32(device_memory_addr);
        write_head.pack_len = htole16(writelen);
        //write_head.pack_sequence = htole16(pack_sequence);
        write_head.pack_sequence = 0;

        data = (uint8_t*)malloc(sizeof(ambit_pack_write_head_t)+writelen);

        memcpy(data, &write_head, sizeof(ambit_pack_write_head_t));
        memcpy(data+sizeof(ambit_pack_write_head_t),
            (uint8_t*)(routes->data_route_info) + (routes->route_info_offset - sizeof(ambit_pack_route_info_head_t)),
            writelen);


        //Write to device
        //debug_print_hex(data, pack_len);
        ret = libambit_protocol_command(object, ambit_command_data_write, data, pack_len, NULL, NULL, 0);
        routes->route_info_offset += writelen;
        free(data);

        if(ret == -1) {
            return ret;
        }
    }

    //Write route points
    device_memory_addr = 0x042830;

    while(routes->routepoints_offset < routes->routepoints_length) {
        writelen = fmin(max_pack_len, routes->routepoints_length-routes->routepoints_offset);
        write_head.memory_addr_start = htole32(device_memory_addr);
        write_head.pack_len = htole16(writelen);
        data = (uint8_t*)malloc(sizeof(ambit_pack_write_head_t)+writelen);

        memcpy(data, &write_head, sizeof(ambit_pack_write_head_t));
        memcpy(data+sizeof(ambit_pack_write_head_t),
                (uint8_t*)(routes->data_routepoints) + routes->routepoints_offset,
                writelen);
        pack_len = sizeof(ambit_pack_write_head_t)+writelen;
 
        //Write to device
        //debug_print_hex(data, pack_len);
        ret = libambit_protocol_command(object, ambit_command_data_write, data, pack_len, NULL, NULL, 0);
        routes->routepoints_offset += writelen;
        free(data);
        device_memory_addr += max_pack_len;
    }


    //Data tail len

    device_memory_addr = 0x041eb0;
    write_head.memory_addr_start = htole32(device_memory_addr);
    write_head.pack_len = htole16(writelen);
    pack_len = sizeof(ambit_pack_write_head_t);

    //Write to device
    //debug_print_hex((uint8_t*)&write_head, pack_len);
    ret = libambit_protocol_command(object, ambit_command_data_tail_len, (uint8_t*)&write_head, pack_len, NULL, NULL, 0);
    return 0;

}

void ambit_navigation_route_add_checksum(ambit_pack_routes_t *routes)
{
    uint8_t *data;
    size_t datalen_info_packs = sizeof(ambit_pack_route_info_t)*
                                routes->data_head->route_count;
    size_t datalen_point_packs = sizeof(ambit_pack_routepoints_t)*
                                routes->data_head->routepoint_count;

    size_t datalen = datalen_info_packs + datalen_point_packs;
    data = (uint8_t*)malloc(datalen);

    for(int x=0; x<routes->data_head->route_count;++x) {

        memcpy(data+(sizeof(ambit_pack_route_info_t)*x),
                &(routes->data_route_info[x]),
                sizeof(ambit_pack_route_info_t));
    }


    memcpy(data+datalen_info_packs,
            routes->data_routepoints,
            datalen_point_packs);

    routes->data_head->checksum = crc16_ccitt_false(data, datalen);
    free(data);
}
