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
#include "libambit.h"
#include "libambit_int.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * Local definitions
 */

/*
 * Public functions
 */
int libambit_personal_settings_parse(uint8_t *data, size_t datalen, ambit_personal_settings_t *settings)
{
    size_t offset = 0;

    // Check that data is long enough
    if (datalen < 132) {
        return -1;
    }

    offset = 1;
    settings->sportmode_button_lock = read8inc(data, &offset);
    settings->timemode_button_lock = read8inc(data, &offset);
    offset += 1;
    settings->compass_declination = read16inc(data, &offset);
    offset += 2;
    settings->units_mode = read8inc(data, &offset);

    settings->units.pressure = read8inc(data, &offset);
    settings->units.altitude = read8inc(data, &offset);
    settings->units.distance = read8inc(data, &offset);
    settings->units.height = read8inc(data, &offset);
    settings->units.temperature = read8inc(data, &offset);
    settings->units.verticalspeed = read8inc(data, &offset);
    settings->units.weight = read8inc(data, &offset);
    settings->units.compass = read8inc(data, &offset);
    settings->units.heartrate = read8inc(data, &offset);
    settings->units.speed = read8inc(data, &offset);

    settings->gps_position_format = read8inc(data, &offset);
    settings->language = read8inc(data, &offset);
    settings->navigation_style = read8inc(data, &offset);
    offset += 2;
    settings->sync_time_w_gps = read8inc(data, &offset);
    settings->time_format = read8inc(data, &offset);

    settings->alarm.hour = read8inc(data, &offset);
    settings->alarm.minute = read8inc(data, &offset);

    settings->alarm_enable = read8inc(data, &offset);

    offset += 2;
    settings->dual_time.hour = read8inc(data, &offset);
    settings->dual_time.minute = read8inc(data, &offset);

    offset += 3;

    settings->date_format = read8inc(data, &offset);

    offset += 3;

    settings->tones_mode = read8inc(data, &offset);

    offset += 3;

    settings->backlight_mode = read8inc(data, &offset);
    settings->backlight_brightness = read8inc(data, &offset);
    settings->display_brightness = read8inc(data, &offset);
    settings->display_is_negative = read8inc(data, &offset);
    settings->weight = read16inc(data, &offset);
    settings->birthyear = read16inc(data, &offset);
    settings->max_hr = read8inc(data, &offset);
    settings->rest_hr = read8inc(data, &offset);
    settings->fitness_level = read8inc(data, &offset);
    settings->is_male = read8inc(data, &offset);
    settings->length = read8inc(data, &offset);

    offset += 3;

    settings->alti_baro_mode = read8inc(data, &offset);

    offset += 1;

    settings->fused_alti_disabled = read8inc(data, &offset);

    offset = 0x80;
    settings->bikepod_calibration = read16inc(data, &offset);
    settings->bikepod_calibration2 = read16inc(data, &offset);

    if (datalen >= 137) {
        // Only Ambit 2 got this!
        settings->bikepod_calibration3 = read16inc(data, &offset);
        settings->footpod_calibration = read16inc(data, &offset);
        settings->automatic_bikepower_calib = read8inc(data, &offset);
    }

    return 0;
}
