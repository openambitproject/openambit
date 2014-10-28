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
#ifndef __DEVICE_DRIVER_COMMON_H__
#define __DEVICE_DRIVER_COMMON_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "libambit.h"

int libambit_device_driver_lock_log(ambit_object_t *object, bool lock);
int libambit_device_driver_date_time_set(ambit_object_t *object, struct tm *tm);
int libambit_device_driver_status_get(ambit_object_t *object, ambit_device_status_t *status);

#endif /* DEVICE_DRIVER_COMMON */
