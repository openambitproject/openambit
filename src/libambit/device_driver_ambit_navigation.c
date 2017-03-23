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


int ambit_navigation_poi_read(ambit_object_t *object, ambit_pack_poi_t *pois_data, uint16_t *poi_count) {

    uint8_t *reply_data = NULL;
    size_t replylen = 0;
    ambit_pack_poi_t *poidata = NULL;
    ambit_pack_poi_t *poi_return_list = NULL;
    uint8_t *send_data = NULL;
    size_t sendlen = 0;
    uint16_t x;

    *poi_count = 0;
    
    //get number of POI
    if( libambit_protocol_command(object, ambit_command_poi_count, NULL, 0, &reply_data, &replylen, 0) != 0) {
        LOG_WARNING("Failed to read number of POI entries");
        libambit_protocol_free(reply_data);
        return -1;
    }

    *poi_count = le16toh(*(uint16_t*)(reply_data));
    poi_return_list = malloc(sizeof(ambit_pack_poi_t)*(*poi_count));
    
    libambit_protocol_free(reply_data);

    printf("poi_count: %u\n", *poi_count);

    sendlen = sizeof(ambit_pack_poi_t);

    for(x=0; x<(*poi_count); ++x) {
    
        poidata = malloc(sizeof(ambit_pack_poi_t));
        send_data = malloc(sizeof(ambit_pack_poi_t));
        
        poidata->poi_index = htole16(x);
        memcpy(send_data, poidata, sizeof(ambit_pack_poi_t));

        if( libambit_protocol_command(object, ambit_command_poi_read, send_data, sendlen, &reply_data ,&replylen , 0) == 0 ) {
            printf("\n");
            printf("ambit_command_poi_read(%u) size: %zu\n", x, replylen);
            memcpy(&poi_return_list[x], reply_data, sizeof(ambit_pack_poi_t));
            ambit_navigation_print_struct(&poi_return_list[x]);
        } else {
            printf("ambit_command_poi_read failed: %u\n", x);
        }
        
        pois_data = poidata;
        free(send_data);
        send_data = NULL;
        libambit_protocol_free(reply_data);

    }

    return 0;
}

int ambit_navigation_poi_write(ambit_object_t *object,ambit_pack_poi_t *poidata, uint16_t poi_count) {
	return 0;
}

void ambit_navigation_print_struct(ambit_pack_poi_t *str) {
	
	printf("poi_index: %u\n", le16toh(str->poi_index));
	printf("unknown: %u\n", str->unknown);
	printf("name: %s\n", str->name);
	printf("route_name: %s\n", str->route_name);
	printf("ctime_second: %u\n", str->poi_index);
	printf("ctime_minute: %u\n", str->ctime_minute);
	printf("ctime_hour: %u\n", str->ctime_hour);
	printf("ctime_day: %u\n", str->ctime_day);
	printf("ctime_month: %u\n", str->ctime_month);
	printf("ctime_year: %u\n", le16toh(str->ctime_year));
	printf("latitude: %i\n", le16toh(str->latitude));
	printf("longitude: %i\n", le16toh(str->longitude));
	printf("poitype: %u\n", le16toh(str->poitype));
	printf("poi_name_count: %u\n", str->poi_name_count);
	printf("status: %u\n", str->status);

}
