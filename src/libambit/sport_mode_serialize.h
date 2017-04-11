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
#ifndef __SPORT_MODE_SERIALIZE_H__
#define __SPORT_MODE_SERIALIZE_H__

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

#include <stdlib.h>
#include "libambit.h"


static const int HEADER_SIZE = 4;
static const int ACTIVITY_NAME_SIZE = 16;
static const int GROUP_NAME_SIZE = 24;
static const int SETTINGS_SIZE = 90;

static const int SPORT_MODE_START_HEADER = 0x0100;
static const int SPORT_MODE_HEADER = 0x0101;
static const int SETTINGS_HEADER = 0x0102;
static const int DISPLAYS_HEADER = 0x0105;
static const int DISPLAY_HEADER = 0x0106;
static const int DISPLAY_LAYOUT_HEADER = 0x0107;
static const int ROWS_HEADER = 0x0108;
static const int ROW_HEADER = 0x0109;
static const int VIEW_HEADER = 0x010a;

static const int SPORT_MODE_GROUP_START_HEADER = 0x0200;
static const int SPORT_MODE_GROUP_HEADER = 0x0210;
static const int NAME_HEADER = 0x0212;
static const int ACTIVITY_ID_HEADER = 0x0213;
static const int MODES_ID_HEADER = 0x0214;

#define SINGLE_ROW_DISPLAY_TYPE 0x0106
#define DOUBLE_ROWS_DISPLAY_TYPE 0x0105
#define TRIPLE_ROWS_DISPLAY_TYPE 0x0104
#define GRAPH_DISPLAY_TYPE 0x0101

int calculate_size_for_serialize_sport_mode_device_settings(ambit_sport_mode_device_settings_t *ambit_device_settings);
int serialize_sport_mode_device_settings(ambit_sport_mode_device_settings_t *ambit_settings, uint8_t *data);
int calculate_size_for_serialize_app_data(ambit_sport_mode_device_settings_t *ambit_settings, ambit_app_rules_t *ambit_apps);
int serialize_app_data(ambit_sport_mode_device_settings_t *ambit_settings, ambit_app_rules_t *ambit_apps, uint8_t *data);


#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif /* __SPORT_MODE_SERIALIZE_H__ */
