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
#ifndef __LIBAMBIT_H__
#define __LIBAMBIT_H__

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

typedef struct ambit_object_s ambit_object_t;

typedef struct ambit_device_info_s {
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
} ambit_device_info_t;

typedef struct ambit_device_status_s {
    uint8_t  charge;
} ambit_device_status_t;

typedef struct ambit_waypoint_s {
    uint16_t      index;
    char          name[50];
    char          route_name[50];
    uint8_t       ctime_second;
    uint8_t       ctime_minute;
    uint8_t       ctime_hour;
    uint8_t       ctime_day;
    uint8_t       ctime_month;
    uint16_t      ctime_year;
    int32_t       latitude;
    int32_t       longitude;
    uint16_t      altitude;
    uint8_t       type;
    uint8_t       status;
} ambit_waypoint_t;

typedef struct ambit_routepoint_s {
    int32_t      lat;      //devide value by 10000000
    int32_t      lon;      //devide value by 10000000
    int32_t      altitude; //meters
    uint32_t     distance; //relative distance from 0 - 1.000.000
} ambit_routepoint_t;

typedef struct ambit_route_s {
    uint32_t      id;
    char          name[50];
    uint16_t      waypoint_count;
    uint16_t      activity_id;
    uint16_t      altitude_asc; //meters
    uint16_t      altitude_dec; //meters
    uint16_t      points_count;
    uint32_t      distance;
    int32_t       start_lat;  //devide value by 10000000
    int32_t       start_lon;  //devide value by 10000000
    int32_t       end_lat;  //devide value by 10000000
    int32_t       end_lon;  //devide value by 10000000
    int32_t       max_lat;  //devide value by 10000000
    int32_t       min_lat;  //devide value by 10000000
    int32_t       max_lon;  //devide value by 10000000
    int32_t       min_lon;  //devide value by 10000000
    int32_t       mid_lat;  //devide value by 10000000
    int32_t       mid_lon;  //devide value by 10000000
    ambit_routepoint_t *points;
} ambit_route_t;

typedef struct ambit_personal_settings_s {
    uint8_t  sportmode_button_lock;
    uint8_t  timemode_button_lock;
    uint16_t compass_declination;
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
    struct {
        ambit_route_t *data;
        uint8_t  count;
    } routes;
    struct {
        ambit_waypoint_t *data;
        uint16_t count;
    } waypoints;
} ambit_personal_settings_t;

typedef struct ambit_log_date_time_s {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint16_t msec;
} ambit_date_time_t;

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

typedef enum movescount_waypoint_type_e {
    movescount_waypoint_type_building = 0,
    movescount_waypoint_type_home = 1,
    movescount_waypoint_type_car = 2,
    movescount_waypoint_type_parking = 3,
    movescount_waypoint_type_camp = 4,
    movescount_waypoint_type_camping = 5,
    movescount_waypoint_type_food = 6,
    movescount_waypoint_type_restaurant = 7,
    movescount_waypoint_type_cafe = 8,
    movescount_waypoint_type_lodging = 9,
    movescount_waypoint_type_hostel = 10,
    movescount_waypoint_type_hotel = 11,
    movescount_waypoint_type_water = 12,
    movescount_waypoint_type_river = 13,
    movescount_waypoint_type_lake = 14,
    movescount_waypoint_type_coast = 15,
    movescount_waypoint_type_mouantain = 16,
    movescount_waypoint_type_hill = 17,
    movescount_waypoint_type_valley = 18,
    movescount_waypoint_type_cliff = 19,
    movescount_waypoint_type_forest = 20,
    movescount_waypoint_type_crossroad = 21,
    movescount_waypoint_type_sight = 22,
    movescount_waypoint_type_beginning = 23,
    movescount_waypoint_type_end = 24,
    movescount_waypoint_type_geocache = 25,
    movescount_waypoint_type_poi = 26,
    movescount_waypoint_type_road = 27,
    movescount_waypoint_type_trail = 28,
    movescount_waypoint_type_rock = 29,
    movescount_waypoint_type_meadow = 30,
    movescount_waypoint_type_cave = 31,
    movescount_waypoint_type_internal_wp_start = 32,
    movescount_waypoint_type_internal_wp_end = 33
} movescount_waypoint_type_t;

typedef struct ambit_log_sample_periodic_value_s {
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
} ambit_log_sample_periodic_value_t;

typedef struct ambit_log_gps_satellite_s {
    uint8_t sv;
    uint8_t snr;
    uint8_t state;
} ambit_log_gps_satellite_t;

typedef struct ambit_log_sample_s {
    ambit_log_sample_type_t type;  /* type of sample */
    uint32_t time;                 /* time from zero */
    ambit_date_time_t utc_time;
    union {
        struct {
            uint8_t value_count;
            ambit_log_sample_periodic_value_t *values;
        } periodic;
        struct {
            uint8_t ibi_count;
            uint16_t ibi[32];
        } ibi;
        uint16_t ttff;
        uint8_t  distance_source;               /* 0x00 = Bikepod,
                                                   0x01 = Footpod,
                                                   0x02 = GPS,
                                                   0x03 = Wrist,
                                                   0x04 = Indoorswimming,
                                                   0x05 = Outdoorswimming */
        struct {
            uint8_t event_type;                 /* 0x01 = manual lap,
                                                   0x14 = high interval end,
                                                   0x15 = low interval end,
                                                   0x16 = interval start,
                                                   0x1e = pause,
                                                   0x1f = start */
            ambit_date_time_t date_time;
            uint32_t duration;                  /* ms */
            uint32_t distance;                  /* meters */
        } lapinfo;
        struct {
            uint8_t source_type;                /* 0x04 = pressure */
            int16_t altitude_offset;
            int16_t pressure_offset;
        } altitude_source;
        struct {
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
        } gps_base;
        struct {
            uint8_t  noofsatellites;            /* */
            int32_t  latitude;                  /* degree, scale: 0.0000001, -90 <= latitude <= 90 */
            int32_t  longitude;                 /* degree, scale: 0.0000001, -180 <= latitude <= 180 */
            uint32_t ehpe;                      /* meters scale: 0.01 */
        } gps_small;
        struct {
            int32_t  latitude;                  /* degree, scale: 0.0000001, -90 <= latitude <= 90 */
            int32_t  longitude;                 /* degree, scale: 0.0000001, -180 <= latitude <= 180 */
            uint32_t ehpe;                      /* meters scale: 0.01 */
            uint8_t  unknown;                   /* Unknown data */
        } gps_tiny;
        struct {
            uint8_t  hour;
            uint8_t  minute;
            uint8_t  second;
        } time;
        struct {
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
        } swimming_turn;
        struct {
            uint16_t activitytype;
            uint32_t sportmode;
        } activity;
        uint8_t cadence_source;                 /* 0x40 = Wrist */
        struct {
            int32_t  latitude;                  /* degree, scale: 0.0000001, -90 <= latitude <= 90 */
            int32_t  longitude;                 /* degree, scale: 0.0000001, -180 <= latitude <= 180 */
        } position;
        struct {
            uint8_t version[4];
            ambit_date_time_t build_date;
        } fwinfo;
        struct {
            size_t datalen;
            uint8_t *data;
        } unknown;
    } u;
} ambit_log_sample_t;

typedef struct ambit_log_header_s {
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
} ambit_log_header_t;

typedef struct ambit_log_entry_s {
    ambit_log_header_t header;
    uint32_t samples_count;
    ambit_log_sample_t *samples;
} ambit_log_entry_t;


typedef struct ambit_sport_mode_settings_s {
    char     activity_name[16];
    uint16_t activity_id;
    uint16_t sport_mode_id;
    uint8_t  unknown1[2];
    uint16_t hrbelt_and_pods;       /* bit pattern representing usage of hr belt or pods */
    uint16_t alti_baro_mode;
    uint16_t gps_interval;
    uint16_t recording_interval;
    uint16_t autolap;               /* m */
    uint16_t heartrate_max;         /* bps */
    uint16_t heartrate_min;         /* bps */
    uint16_t use_heartrate_limits;
    uint8_t  unknown2[2];
    uint16_t auto_pause;
    uint16_t auto_scroll;           /* s */
    uint16_t use_interval_timer;
    uint16_t interval_repetitions;
    uint16_t interval_timer_max_unit;   /* m or s */
    uint8_t  unknown3[6];
    uint16_t interval_timer_max;    /* s or m */ /*Maybe 2 bytes from unknown3 should be included in this field? */
    uint8_t  unknown4[2];
    uint16_t interval_timer_min_unit;   /* m or s */
    uint8_t  unknown5[6];
    uint16_t interval_timer_min;    /* s or m */ /*Maybe 2 bytes from unknown3 should be included in this field? */
    uint8_t  unknown6[14];
    uint16_t backlight_mode;
    uint16_t display_mode;
    uint16_t quick_navigation;
} ambit_sport_mode_settings_t;

typedef struct ambit_sport_mode_display_layout_s {
    uint16_t header;
    uint16_t length;
    uint16_t display_layout;
    uint8_t unknown[2];
} ambit_sport_mode_display_layout_t;

typedef struct ambit_sport_mode_row_s {
    uint16_t header;
    uint16_t length;
    uint16_t row_nbr;
    uint16_t item;
} ambit_sport_mode_row_t;

typedef struct ambit_sport_mode_view_s {
    uint16_t header;
    uint16_t length;
    uint16_t item;
} ambit_sport_mode_view_t;

typedef struct ambit_write_header_s {
    uint16_t header;
    uint16_t length;
} ambit_write_header_t;

typedef struct ambit_sport_mode_display_s {
    uint16_t requiresHRBelt;
    uint16_t type;
    uint16_t row1;
    uint16_t row2;
    uint16_t row3;
    uint32_t views_count;
    uint16_t *view;
} ambit_sport_mode_display_t;

typedef struct ambit_app_index_s {
    uint16_t index;
    uint16_t logging;
} ambit_apps_list_t;

typedef struct ambit_sport_mode_group_s {
    uint16_t activity_id;
    uint16_t sport_mode_group_id;
    bool is_visible;
    char activity_name[24];
    uint32_t sport_mode_index_count;
    uint16_t *sport_mode_index;
} ambit_sport_mode_group_t;

typedef struct ambit_sport_mode_s {
    ambit_sport_mode_settings_t settings;
    uint32_t displays_count;
    ambit_sport_mode_display_t *display;
    uint16_t apps_list_count;
    ambit_apps_list_t *apps_list;
} ambit_sport_mode_t;

typedef struct ambit_sport_mode_device_settings_s {
    uint32_t sport_modes_count;
    ambit_sport_mode_t *sport_modes;
    uint32_t sport_mode_groups_count;
    ambit_sport_mode_group_t *sport_mode_groups;
    uint32_t app_ids_count;
    uint32_t app_ids[40];
} ambit_sport_mode_device_settings_t;

typedef struct ambit_app_rule_s {
    uint32_t app_rule_data_length;
    uint32_t app_id;
    uint8_t *app_rule_data;
} ambit_app_rule_t;

typedef struct ambit_app_rules_s {
    uint32_t app_rules_count;
    ambit_app_rule_t *app_rules;
} ambit_app_rules_t;

/** \brief Create a list of all known Ambit clocks on the system
 *
 *  The list may include clocks that are not supported or cannot be
 *  accessed.
 */
ambit_device_info_t * libambit_enumerate(void);

/** \brief Release resources acquired by libambit_enumerate()
 */
void libambit_free_enumeration(ambit_device_info_t *devices);

/** \brief Create an Ambit object for a clock
 *
 *  The pointer returned corresponds to a known, accessible and
 *  supported clock.  In case no such clock is found \c NULL is
 *  returned.
 */
ambit_object_t * libambit_new(const ambit_device_info_t *device);

/** \brief Create an Ambit object from a \a pathname
 *
 *  Convenience function for when the path name for a clock is known.
 *  These path names are platform dependent.
 */
ambit_object_t * libambit_new_from_pathname(const char *pathname);

/**
 * Close open Ambit object
 * \param object Object to close
 */
void libambit_close(ambit_object_t *object);

/**
 * Set sync message to device display
 * \param object Object to set display on
 */
void libambit_sync_display_show(ambit_object_t *object);

/**
 * Clear sync message from device display
 * \param object Object to clear display on
 */
void libambit_sync_display_clear(ambit_object_t *object);

/**
 * Set current date and time to clock
 * \param object Object to write to
 * \param date_time Date and time to set
 * \return 0 on success, else -1
 */
int libambit_date_time_set(ambit_object_t *object, struct tm *date_time);

/**
 * Get status info from device
 * \param object Object to get status from
 * \param status Status object to be filled
 * \return 0 on success, else -1
 */
int libambit_device_status_get(ambit_object_t *object, ambit_device_status_t *status);

/**
 * Get settings from device
 * \param object Object to get settings from
 * \param settings Settings object to be filled
 * \return 0 on success, else -1
 */
int libambit_personal_settings_get(ambit_object_t *object, ambit_personal_settings_t *settings);

/**
 * Read GPS orbit header, useful to check if new write is needed.
 * NOTE! Reversed byte order compared to the data that is written.
 * \param object Object to get settings from
 * \param data Header data (date + !?)
 * \return 0 on success, else -1
 */
int libambit_gps_orbit_header_read(ambit_object_t *object, uint8_t data[8]);

/**
 * Write GPS orbit data
 * \param object Object to get settings from
 * \param data Data to be written
 * \param datalen Length of data
 * \return 0 on success, else -1
 */
int libambit_gps_orbit_write(ambit_object_t *object, uint8_t *data, size_t datalen);

/**
 * Write Custom mode displays
 * \param object Object to get settings from
 * \param ambit_sport_modes settings object to be written
 * \return 0 on success, else -1
 */
int libambit_sport_mode_write(ambit_object_t *object, ambit_sport_mode_device_settings_t *ambit_sport_modes);


int libambit_app_data_write(ambit_object_t *object, ambit_sport_mode_device_settings_t *ambit_sport_modes, ambit_app_rules_t* ambit_apps);

/**
 * Callback function for checking if a specific log entry should be read out or
 * skipped during log readouts
 * \param object Object reference
 * \param log_header Pointer to log header that is about to be read out
 * \return Should return 0 to skip entry, else -1
 */
typedef int (*ambit_log_skip_cb)(void *userref, ambit_log_header_t *log_header);

/**
 * Callback function to push log entry to calling application
 * \param object Object reference
 * \param log_entry Complete log entry. NOTE! Caller is responsible of freeing
 * allocated memory with lib_ambit_log_entry_free()
 */
typedef void (*ambit_log_push_cb)(void *userref, ambit_log_entry_t *log_entry);

/**
 * Callback function to notify about progress
 * \param object Object reference
 * \param log_count Number of available logs
 * \param log_current Current log number in progress
 * \param progress_percent Total progress in percent
 */
typedef void (*ambit_log_progress_cb)(void *userref, uint16_t log_count, uint16_t log_current, uint8_t progress_percent);

/**
 * Read complete log of all excercises from device (filtered by skip_cb
 * \param object Object reference
 * \param skip_cb Callback to be used to check if a specific entry should read
 * or skipped. Use NULL to get all entries.
 * \param push_cb Callback to use for pushing read out entry to caller.
 * \return Number of entries read, or -1 on error
 * \note Caller is responsible of freeing log entries with
 * libambit_log_entry_free()
 */
int libambit_log_read(ambit_object_t *object, ambit_log_skip_cb skip_cb, ambit_log_push_cb push_cb, ambit_log_progress_cb progress_cb, void *userref);
/**
 * Free log entry allocated by libambit_log_read
 * \param log_entry Log entry to free
 */
void libambit_log_entry_free(ambit_log_entry_t *log_entry);
/**
 * Init ambit_route_t struct
 */
ambit_route_t* libambit_route_alloc(uint16_t route_count);
/**
 * Free struct allocated by libambit_route_alloc
 * \param personal_settings Struct to free
 */
void libambit_route_free(ambit_route_t *routes, uint16_t route_count);
/**
 * Append to a ambit_personal_settings_t.waypoints
 */
void libambit_waypoint_append(ambit_personal_settings_t *ps, ambit_waypoint_t *waypoints, uint8_t num_to_append);
/**
 * Init personal_settings struct
 */
ambit_personal_settings_t* libambit_personal_settings_alloc();
/**
 * Free struct allocated by libambit_personal_settings_alloc
 * \param personal_settings Struct to free
 */
void libambit_personal_settings_free(ambit_personal_settings_t *personal_settings);

/**
 * Read Waypoint entries
 */
int libambit_navigation_read(ambit_object_t *object, ambit_personal_settings_t *personal_settings);

/**
 * Write Waypoint entries
 */
int libambit_navigation_write(ambit_object_t *object, ambit_personal_settings_t *personal_settings);

/**
 * Allocates memmory for device settings structure and
 * initiate pointer in the structure to NULL and sport_modes_count and sport_mode_groups_count to 0.
 * \note Caller is responsible of freeing the struct with libambit_sport_mode_device_settings_free()
 * \return pointer to allocated data struct.
 */
ambit_sport_mode_device_settings_t *libambit_malloc_sport_mode_device_settings(void);

/**
 * Allocates memmory for a number of custom mode structures and
 * initiate display pointer in the structures to NULL and displays_count to 0.
 * \param count number of custom modes that will be allocated.
 * \param ambit_settings structure where these custom modes belongs to.
 * The ambit_settings will be updated to point at the allocated data and the custom modes count will be set to count.
 * \return true if allocation was succesfull.
 */
bool libambit_malloc_sport_modes(uint16_t count, ambit_sport_mode_device_settings_t *ambit_settings);

/**
 * Allocates memmory for a number of custom mode group structures and
 * initiate structure pointers to NULL and index_count to 0.
 * \param count number of custom mode groups that will be allocated.
 * \param ambit_settings structure where these custom mode groups belongs to.
 * The ambit_settings will be updated to point at the allocated data and the custom mode groups count will be set to count.
 * \return true if allocation was succesfull.
 */
bool libambit_malloc_sport_mode_groups(uint16_t count, ambit_sport_mode_device_settings_t *ambit_settings);

/**
 * Allocates memmory for a number of display structures and
 * initiate structure pointers to NULL and view_count to 0.
 * \param count number of displays that will be allocated.
 * \param ambit_sport_mode structure where these displays belongs to.
 * The ambit_sport_mode will be updated to point at the allocated data and the display count will be set to count.
 * \return true if allocation was succesfull.
 */
bool libambit_malloc_sport_mode_displays(uint16_t count, ambit_sport_mode_t *ambit_sport_mode);

/**
 * Allocates memmory for a number of views.
 * \param count number of views that will be allocated.
 * \param ambit_displays structure where these views belongs to.
 * The ambit_displays will be updated to point at the allocated data and the views count will be set to count.
 * \return true if allocation was succesfull.
 */
bool libambit_malloc_sport_mode_view(uint16_t count, ambit_sport_mode_display_t *ambit_displays);

/**
 * Allocates memmory for a number of app ids.
 * \param count number of app_ids (uint32_t) that will be allocated.
 * \param ambit_sport_mode structure where these app_ids belongs to.
 * The ambit_sport_mode will be updated to point at the allocated data and the app_ids_count will be set to count.
 * \return true if allocation was succesfull.
 */
bool libambit_malloc_sport_mode_app_ids(uint16_t count, ambit_sport_mode_t *ambit_sport_mode);

/**
 * Allocates memmory for a number of custom mode index.
 * \param count number of custom mode index that will be allocated.
 * \param ambit_sport_mode_group structure where these custom mode index belongs to.
 * The ambit_sport_mode_group will be updated to point at the allocated data and the custom mode index count will be set to count.
 * \return true if allocation was succesfull.
 */
bool libambit_malloc_sport_mode_index(uint16_t count, ambit_sport_mode_group_t *ambit_sport_mode_group);

/**
 * Free device setting and under laying data structures,
 * allocated by ambit_maloc_*
 * \param settings Device settings to free
 */
void libambit_sport_mode_device_settings_free(ambit_sport_mode_device_settings_t *settings);

/**
 * Free structures for app rules and under laying data,
 * allocated by liblibambit_malloc_app_rules and libambit_malloc_app_rule
 * \param app_rules structure to be freed
 */
void libambit_app_rules_free(ambit_app_rules_t *app_rules);

/**
 * Allocates memmory for ambit_app_rules_t structure and
 * initiate pointer in the structure to NULL and app_rules_count to 0.
 * \note Caller is responsible of freeing the struct with libambit_app_rules_free()
 * \return pointer to allocated data struct.
 */
ambit_app_rules_t *liblibambit_malloc_app_rules(void);

/**
 * Allocates memmory for a number of app rules.
 * \param count number of app rules that will be allocated.
 * \param ambit_app_rules structure from where the app rules will be referenced from.
 * The app_rule pointer in ambit_app_rules will be updated to point to the allocated data and the app_rules_count will be set to count.
 * \return true if allocation was succesfull.
 */
bool libambit_malloc_app_rule(uint16_t count, ambit_app_rules_t *ambit_app_rules);

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif /* __LIBAMBIT_H__ */
