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
#ifndef __DEVICE_DRIVER_H__
#define __DEVICE_DRIVER_H__

#include <stddef.h>
#include <stdint.h>

#include "libambit.h"
#include "device_driver_ambit_navigation.h"

typedef struct ambit_device_driver_s {
    void (*init)(ambit_object_t *object, uint32_t driver_param);
    void (*deinit)(ambit_object_t *object);
    int (*lock_log)(ambit_object_t *object, bool lock);
    int (*date_time_set)(ambit_object_t *object, struct tm *tm);
    int (*status_get)(ambit_object_t *object, ambit_device_status_t *status);
    int (*personal_settings_get)(ambit_object_t *object, ambit_personal_settings_t *settings);
    int (*log_read)(ambit_object_t *object, ambit_log_skip_cb skip_cb, ambit_log_push_cb push_cb, ambit_log_progress_cb progress_cb, void *userref);
    int (*gps_orbit_header_read)(ambit_object_t *object, uint8_t data[8]);
    int (*gps_orbit_write)(ambit_object_t *object, uint8_t *data, size_t datalen);
    int (*navigation_read)(ambit_object_t *object, ambit_personal_settings_t *settings);
    int (*navigation_write)(ambit_object_t *object, ambit_personal_settings_t *settings);
    int (*sport_mode_write)(ambit_object_t *object, ambit_sport_mode_device_settings_t *ambitCustomModes);
    int (*app_data_write)(ambit_object_t *object, ambit_sport_mode_device_settings_t *ambitCustomModes, ambit_app_rules_t* ambit_apps);
} ambit_device_driver_t;

extern ambit_device_driver_t ambit_device_driver_ambit;  // Ambit & Ambit2
extern ambit_device_driver_t ambit_device_driver_ambit3; // Ambit3

#endif /* __DEVICE_DRIVER_H__ */
