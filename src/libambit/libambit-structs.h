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
#ifndef __LIBAMBIT_STRUCTS_H__
#define __LIBAMBIT_STRUCTS_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "libambit-types.h"

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

struct ambit_device_info_s {
    char *name;                                        /* UTF-8 */
    char *model;                                       /* UTF-8 */
    char *serial;                                      /* UTF-8 */
    uint8_t fw_version[4];
    uint8_t hw_version[4];

    const char *path;                   /* file system encoding */
    uint16_t    vendor_id;
    uint16_t    product_id;
    bool        is_supported;
    int         access_status;

    struct ambit_device_info_s *next;
};

struct ambit_device_status_s {
    uint8_t charge;
};

struct ambit_time_s {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

struct ambit_personal_settings_s {
    uint8_t  sportmode_button_lock;
    uint8_t  timemode_button_lock;
    uint16_t compass_declination;
    float compass_declination_f;      /* Ambit3: 32 bit float containing the radian */
    uint8_t  units_mode;
    struct {
        uint8_t pressure;
        uint8_t altitude;
        uint8_t distance;
        uint8_t height;
        uint8_t temperature;
        uint8_t verticalspeed;
        uint8_t weight;
        uint8_t compass;
        uint8_t heartrate;
        uint8_t speed;
    } units;
    uint8_t  gps_position_format;
    uint8_t  language;
    uint8_t  navigation_style;
    uint8_t  sync_time_w_gps;
    uint8_t  time_format;
    struct {
        uint8_t hour;
        uint8_t minute;
    } alarm;
    uint8_t  alarm_enable;
    struct {
        uint8_t hour;
        uint8_t minute;
    } dual_time;
    uint8_t  date_format;
    uint8_t  tones_mode;
    uint8_t  backlight_mode;
    uint8_t  backlight_brightness;
    uint8_t  display_brightness;
    uint8_t  display_is_negative;
    uint16_t weight;                  /* kg scale 0.01 */
    uint16_t birthyear;
    uint8_t  max_hr;
    uint8_t  rest_hr;
    uint8_t  fitness_level;
    uint8_t  is_male;
    uint8_t  length;
    uint8_t  alti_baro_mode;
    uint8_t  storm_alarm;
    uint8_t  fused_alti_disabled;
    uint16_t bikepod_calibration;     /* scale 0.0001 */
    uint16_t bikepod_calibration2;    /* scale 0.0001 */
    uint16_t bikepod_calibration3;    /* scale 0.0001 */
    uint16_t footpod_calibration;     /* scale 0.0001 */
    uint8_t  automatic_bikepower_calib;
    uint8_t  automatic_footpod_calib;
    uint8_t  training_program;
};

struct ambit_log_date_time_s {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint16_t msec;
};

struct ambit_log_sample_periodic_value_s {
    ambit_log_sample_periodic_type_t type;
    union {
        int32_t  latitude;          /* degree, scale: 0.0000001, -90 <= latitude <= 90 */
        int32_t  longitude;         /* degree, scale: 0.0000001, -180 <= latitude <= 180 */
        uint32_t distance;          /* meter, scale: 1 */
        uint16_t speed;             /* m/s scale: 0.01, ignore if 0xffff */
        uint8_t  hr;                /* bpm, ignore if 0xff */
        uint32_t time;              /* msec */
        uint16_t gpsspeed;          /* m/s scale: 0.01 */
        uint16_t wristaccspeed;     /* m/s scale: 0.01 */
        uint16_t bikepodspeed;      /* m/s scale: 0.01 */
        uint32_t ehpe;              /* meters scale: 0.01 */
        uint32_t evpe;              /* meters scale: 0.01 */
        int16_t  altitude;          /* meters, -1000 <= altitude <= 10000 */
        uint16_t abspressure;       /* hpa scale 0.1 */
        uint16_t energy;            /* hcal/min */
        int16_t  temperature;       /* celsius scale 0.1, -100 <= temp <= 100 */
        uint8_t  charge;            /* percent */
        int32_t  gpsaltitude;       /* meters scale: 0.01, -1000 <= altitude <= 10000 */
        uint16_t gpsheading;        /* degrees, scale: 0.01, 0 <= heading <= 360 */
        uint8_t  gpshdop;           /* ? */
        uint8_t  gpsvdop;           /* ? */
        uint16_t wristcadence;      /* rpm, ignore if 0xffff */
        uint8_t  snr[16];           /* ? */
        uint8_t  noofsatellites;    /* ignore if 0xff */
        int16_t  sealevelpressure;  /* hpa scale: 0.1 */
        int16_t  verticalspeed;     /* m/s scale: 0.01 */
        uint8_t  cadence;           /* rpm, ignore if 0xff */
        uint16_t bikepower;         /* watt */
        uint32_t swimingstrokecnt;  /* counter */
        int32_t  ruleoutput1;       /* ignore if 0x80000000 */
        int32_t  ruleoutput2;       /* ignore if 0x80000000 */
        int32_t  ruleoutput3;       /* ignore if 0x80000000 */
        int32_t  ruleoutput4;       /* ignore if 0x80000000 */
        int32_t  ruleoutput5;       /* ignore if 0x80000000 */
    } u;
};

struct ambit_log_gps_satellite_s {
    uint8_t sv;
    uint8_t snr;
    uint8_t state;
};

struct ambit_log_sample_periodic_s {
    uint8_t value_count;
    ambit_log_sample_periodic_value_t *values;
};

struct ambit_log_sample_ibi_s {
    uint8_t ibi_count;
    uint16_t ibi[32];
};

struct ambit_log_sample_ttff_s {
    uint16_t value;
};

struct ambit_log_sample_distance_source_s {
    uint8_t  value;               /* 0x00 = Bikepod,
                                     0x01 = Footpod,
                                     0x02 = GPS,
                                     0x03 = Wrist,
                                     0x04 = Indoorswimming,
                                     0x05 = Outdoorswimming */
};

struct ambit_log_sample_lap_info_s {
    uint8_t event_type;                 /* 0x01 = manual lap,
                                           0x14 = high interval end,
                                           0x15 = low interval end,
                                           0x16 = interval start,
                                           0x1e = pause,
                                           0x1f = start */
    ambit_date_time_t date_time;
    uint32_t duration;                  /* ms */
    uint32_t distance;                  /* meters */
};

struct ambit_log_sample_altitude_source_s {
    uint8_t source_type;                /* 0x04 = pressure */
    int16_t altitude_offset;
    int16_t pressure_offset;
};

struct ambit_log_sample_gps_base_s {
    uint16_t navvalid;
    uint16_t navtype;
    ambit_date_time_t utc_base_time;
    int32_t  latitude;                  /* degree, scale: 0.0000001, -90 <= latitude <= 90 */
    int32_t  longitude;                 /* degree, scale: 0.0000001, -180 <= latitude <= 180 */
    int32_t  altitude;                  /* meters scale: 0.01, -1000 <= altitude <= 10000 */
    uint16_t speed;                     /* m/s scale: 0.01 */
    uint16_t heading;                   /* degrees, scale: 0.01, 0 <= heading <= 360 */
    uint32_t ehpe;                      /* meters scale: 0.01 */
    uint8_t  noofsatellites;            /* */
    uint8_t  hdop;                      /* ? scale: 0.2 */
    uint8_t  satellites_count;
    ambit_log_gps_satellite_t *satellites;
};

struct ambit_log_sample_gps_small_s {
    uint8_t  noofsatellites;            /* */
    int32_t  latitude;                  /* degree, scale: 0.0000001, -90 <= latitude <= 90 */
    int32_t  longitude;                 /* degree, scale: 0.0000001, -180 <= latitude <= 180 */
    uint32_t ehpe;                      /* meters scale: 0.01 */
};

struct ambit_log_sample_gps_tiny_s {
    int32_t  latitude;                  /* degree, scale: 0.0000001, -90 <= latitude <= 90 */
    int32_t  longitude;                 /* degree, scale: 0.0000001, -180 <= latitude <= 180 */
    uint32_t ehpe;                      /* meters scale: 0.01 */
    uint8_t  unknown;                   /* Unknown data */
};

struct ambit_log_sample_time_s {
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
};

struct ambit_log_sample_swimming_turn_s {
    uint32_t distance;                  /* Total distance, meters scale: 0.01 */
    uint16_t lengths;                   /* Total pool lengths */
    uint16_t classification[4];
    uint8_t  style;                     /* (style of previous length)
                                           0x00 = Other,
                                           0x01 = Butterfly,
                                           0x02 = Backstroke,
                                           0x03 = Breaststroke,
                                           0x04 = Freestyle,
                                           0x05 = Drill */
};

struct ambit_log_sample_activity_s {
    uint16_t activitytype;
    uint32_t custommode;
};

struct ambit_log_sample_cadence_source_s {
    uint8_t value;                 /* 0x40 = Wrist */
};

struct ambit_log_sample_position_s {
    int32_t  latitude;                  /* degree, scale: 0.0000001, -90 <= latitude <= 90 */
    int32_t  longitude;                 /* degree, scale: 0.0000001, -180 <= latitude <= 180 */
};

struct ambit_log_sample_fwinfo_s {
    uint8_t version[4];
    ambit_date_time_t build_date;
};

struct ambit_log_sample_unknown_s {
    size_t datalen;
    uint8_t *data;
};

struct ambit_log_sample_s {
    ambit_log_sample_type_t type;  /* type of sample */
    uint32_t time;                 /* time from zero */
    ambit_date_time_t utc_time;
    union {
        ambit_log_sample_periodic_t periodic;
        ambit_log_sample_ibi_t ibi;
        ambit_log_sample_ttff_t ttff;
        ambit_log_sample_distance_source_t distance_source;
        ambit_log_sample_lapinfo_t lapinfo;
        ambit_log_sample_altitude_source_t altitude_source;
        ambit_log_sample_gps_base_t gps_base;
        ambit_log_sample_gps_small_t gps_small;
        ambit_log_sample_gps_tiny_t gps_tiny;
        ambit_log_sample_time_t time;
        ambit_log_sample_swimming_turn_t swimming_turn;
        ambit_log_sample_activity_t activity;
        ambit_log_sample_cadence_source_t cadence_source;
        ambit_log_sample_position_t position;
        ambit_log_sample_fwinfo_t fwinfo;
        ambit_log_sample_unknown_t unknown;
    } u;
};

struct ambit_log_header_s {
    ambit_date_time_t date_time;
    uint32_t duration;              /* ms */
    uint16_t ascent;                /* m */
    uint16_t descent;               /* m */
    uint32_t ascent_time;           /* ms */
    uint32_t descent_time;          /* ms */
    uint32_t recovery_time;         /* ms */
    uint16_t speed_avg;             /* m/h */
    uint16_t speed_max;             /* m/h */
    uint32_t speed_max_time;        /* ms */
    int16_t  altitude_max;          /* m */
    int16_t  altitude_min;          /* m */
    uint32_t altitude_max_time;     /* ms */
    uint32_t altitude_min_time;     /* ms */
    uint8_t  heartrate_avg;         /* bpm */
    uint8_t  heartrate_max;         /* bpm */
    uint8_t  heartrate_min;         /* bpm */
    uint32_t heartrate_max_time;    /* ms */
    uint32_t heartrate_min_time;    /* ms */
    uint8_t  peak_training_effect;  /* effect scale 0.1 */
    uint8_t  activity_type;
    char    *activity_name;         /* name of activity in UTF-8 */
    int16_t  temperature_max;       /* degree celsius scale 0.1 */
    int16_t  temperature_min;       /* degree celsius scale 0.1 */
    uint32_t temperature_max_time;  /* ms */
    uint32_t temperature_min_time;  /* ms */
    uint32_t distance;              /* m */
    uint32_t samples_count;         /* number of samples in log */
    uint16_t energy_consumption;    /* kcal */
    uint32_t first_fix_time;        /* ms */
    uint8_t  battery_start;         /* percent */
    uint8_t  battery_end;           /* percent */
    uint32_t distance_before_calib; /* m */

    uint8_t  unknown1[5];
    uint8_t  unknown2;
    uint8_t  cadence_max;           /* rpm */
    uint8_t  cadence_avg;           /* rpm */
    uint8_t  unknown3[2];
    uint16_t swimming_pool_lengths;
    uint32_t cadence_max_time;      /* ms */
    uint32_t swimming_pool_length;  /* m */
    uint8_t  unknown5[4];
    uint8_t  unknown6[24];
};

struct ambit_log_entry_s {
    ambit_log_header_t header;
    uint32_t samples_count;
    ambit_log_sample_t *samples;
};

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif /* __LIBAMBIT_STRUCTS_H__ */
