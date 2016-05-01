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
#ifndef __LIBAMBIT_INT_H__
#define __LIBAMBIT_INT_H__

#include <stdint.h>
#include "hidapi/hidapi.h"
#include "libambit.h"
#include "libambit-structs.h"

struct ambit_object_s {
    hid_device *handle;
    uint16_t sequence_no;
    ambit_device_info_t device_info;

    struct ambit_device_driver_s *driver;
    struct ambit_device_driver_data_s *driver_data; // Driver specific struct,
                                                    // should be defined
                                                    // locally for each driver
};

#endif /* __LIBAMBIT_INT_H__ */
