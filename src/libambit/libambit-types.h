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
#ifndef __LIBAMBIT_TYPES_H__
#define __LIBAMBIT_TYPES_H__

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

typedef struct ambit_object_s ambit_object_t;
typedef struct ambit_device_info_s ambit_device_info_t;
typedef struct ambit_device_status_s ambit_device_status_t;
typedef struct ambit_personal_settings_s ambit_personal_settings_t;
typedef struct ambit_log_date_time_s ambit_date_time_t;

typedef enum ambit_log_sample_type_e {
    ambit_log_sample_type_periodic = 0x0200,
    ambit_log_sample_type_logpause = 0x0304,
    ambit_log_sample_type_logrestart = 0x0305,
    ambit_log_sample_type_ibi = 0x0306,
    ambit_log_sample_type_ttff = 0x0307,
    ambit_log_sample_type_distance_source = 0x0308,
    ambit_log_sample_type_lapinfo = 0x0309,
    ambit_log_sample_type_altitude_source = 0x030d,
    ambit_log_sample_type_gps_base = 0x030f,
    ambit_log_sample_type_gps_small = 0x0310,
    ambit_log_sample_type_gps_tiny = 0x0311,
    ambit_log_sample_type_time = 0x0312,
    ambit_log_sample_type_swimming_turn = 0x0314,
    ambit_log_sample_type_swimming_stroke = 0x0315,
    ambit_log_sample_type_activity = 0x0318,
    ambit_log_sample_type_cadence_source = 0x031a,
    ambit_log_sample_type_position = 0x031b,
    ambit_log_sample_type_fwinfo = 0x031c,
    ambit_log_sample_type_unknown = 0xf000
} ambit_log_sample_type_t;

typedef enum ambit_log_sample_periodic_type_e {
    ambit_log_sample_periodic_type_latitude = 0x01,
    ambit_log_sample_periodic_type_longitude = 0x02,
    ambit_log_sample_periodic_type_distance = 0x03,
    ambit_log_sample_periodic_type_speed = 0x04,
    ambit_log_sample_periodic_type_hr = 0x05,
    ambit_log_sample_periodic_type_time = 0x06,
    ambit_log_sample_periodic_type_gpsspeed = 0x07,
    ambit_log_sample_periodic_type_wristaccspeed = 0x08,
    ambit_log_sample_periodic_type_bikepodspeed = 0x09,
    ambit_log_sample_periodic_type_ehpe = 0x0a,
    ambit_log_sample_periodic_type_evpe = 0x0b,
    ambit_log_sample_periodic_type_altitude = 0x0c,
    ambit_log_sample_periodic_type_abspressure = 0x0d,
    ambit_log_sample_periodic_type_energy = 0x0e,
    ambit_log_sample_periodic_type_temperature = 0x0f,
    ambit_log_sample_periodic_type_charge = 0x10,
    ambit_log_sample_periodic_type_gpsaltitude = 0x11,
    ambit_log_sample_periodic_type_gpsheading = 0x12,
    ambit_log_sample_periodic_type_gpshdop = 0x13,
    ambit_log_sample_periodic_type_gpsvdop = 0x14,
    ambit_log_sample_periodic_type_wristcadence = 0x15,
    ambit_log_sample_periodic_type_snr = 0x16,
    ambit_log_sample_periodic_type_noofsatellites = 0x17,
    ambit_log_sample_periodic_type_sealevelpressure = 0x18,
    ambit_log_sample_periodic_type_verticalspeed = 0x19,
    ambit_log_sample_periodic_type_cadence = 0x1a,
    ambit_log_sample_periodic_type_bikepower = 0x1f,
    ambit_log_sample_periodic_type_swimingstrokecnt = 0x20,
    ambit_log_sample_periodic_type_ruleoutput1 = 0x64,
    ambit_log_sample_periodic_type_ruleoutput2 = 0x65,
    ambit_log_sample_periodic_type_ruleoutput3 = 0x66,
    ambit_log_sample_periodic_type_ruleoutput4 = 0x67,
    ambit_log_sample_periodic_type_ruleoutput5 = 0x68
} ambit_log_sample_periodic_type_t;

typedef struct ambit_log_sample_periodic_value_s ambit_log_sample_periodic_value_t;

typedef struct ambit_log_gps_satellite_s ambit_log_gps_satellite_t;

typedef struct ambit_log_gps_satellite_s ambit_log_gps_satellite_t;
typedef struct ambit_log_sample_periodic_s ambit_log_sample_periodic_t;
typedef struct ambit_log_sample_ibi_s ambit_log_sample_ibi_t;
typedef struct ambit_log_sample_ttff_s ambit_log_sample_ttff_t;
typedef struct ambit_log_sample_distance_source_s ambit_log_sample_distance_source_t;
typedef struct ambit_log_sample_lap_info_s ambit_log_sample_lapinfo_t;
typedef struct ambit_log_sample_altitude_source_s ambit_log_sample_altitude_source_t;
typedef struct ambit_log_sample_gps_base_s ambit_log_sample_gps_base_t;
typedef struct ambit_log_sample_gps_small_s ambit_log_sample_gps_small_t;
typedef struct ambit_log_sample_gps_tiny_s ambit_log_sample_gps_tiny_t;
typedef struct ambit_log_sample_time_s ambit_log_sample_time_t;
typedef struct ambit_log_sample_swimming_turn_s ambit_log_sample_swimming_turn_t;
typedef struct ambit_log_sample_activity_s ambit_log_sample_activity_t;
typedef struct ambit_log_sample_cadence_source_s ambit_log_sample_cadence_source_t;
typedef struct ambit_log_sample_position_s ambit_log_sample_position_t;
typedef struct ambit_log_sample_fwinfo_s ambit_log_sample_fwinfo_t;
typedef struct ambit_log_sample_unknown_s ambit_log_sample_unknown_t;

typedef struct ambit_log_sample_s ambit_log_sample_t;

typedef struct ambit_log_header_s ambit_log_header_t;

typedef struct ambit_log_entry_s ambit_log_entry_t;

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif /* __LIBAMBIT_H_TYPES__ */
