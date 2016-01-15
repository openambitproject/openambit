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
#ifndef __CUSTOM_MODE_SERIALIZE_H__
#define __CUSTOM_MODE_SERIALIZE_H__

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

#include <stdlib.h>
#include "libambit.h"


static const int HEADER_SIZE = 4;
static const int ACTIVITY_NAME_SIZE = 16;
static const int GROUP_NAME_SIZE = 24;
static const int SETTINGS_SIZE = 90;

static const int CUSTOM_MODE_START_HEADER = 0x0100;
static const int CUSTOM_MODE_HEADER = 0x0101;
static const int SETTINGS_HEADER = 0x0102;

static const int CUSTOM_MODE_GROUP_START_HEADER = 0x0200;
static const int CUSTOM_MODE_GROUP_HEADER = 0x0210;
static const int NAME_HEADER = 0x0212;
static const int ACTIVITY_ID_HEADER = 0x0213;
static const int MODES_ID_HEADER = 0x0214;

int calculate_size_for_serialize_device_settings(ambit_device_settings_t *ambit_device_settings);
int serialize_device_settings(ambit_device_settings_t *ambit_settings, uint8_t *data);


#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif /* __CUSTOM_MODE_SERIALIZE_H__ */
