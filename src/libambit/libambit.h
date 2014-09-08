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

#define LIBAMBIT_MODEL_NAME_LENGTH    16
#define LIBAMBIT_SERIAL_LENGTH        16
#define LIBAMBIT_PRODUCT_NAME_LENGTH  32

typedef struct ambit_object_s ambit_object_t;

typedef struct ambit_device_info_s {
    char name[LIBAMBIT_PRODUCT_NAME_LENGTH+1];
    char model[LIBAMBIT_MODEL_NAME_LENGTH+1];
    char serial[LIBAMBIT_SERIAL_LENGTH+1];
    uint8_t fw_version[4];
    uint8_t hw_version[4];
    bool is_supported;
} ambit_device_info_t;

typedef struct ambit_device_status_s {
    uint8_t  charge;
} ambit_device_status_t;

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
    uint8_t  fused_alti_disabled;
    uint16_t bikepod_calibration;     /* scale 0.0001 */
    uint16_t bikepod_calibration2;    /* scale 0.0001 */
    uint16_t bikepod_calibration3;    /* scale 0.0001 */
    uint16_t footpod_calibration;     /* scale 0.0001 */
    uint8_t  automatic_bikepower_calib;
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
    ambit_log_sample_type_activity = 0x0318,
    ambit_log_sample_type_position = 0x031b,
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
        uint8_t  distance_source;               /* 2 = GPS, 3 = Wrist */
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
            uint16_t activitytype;
            uint32_t custommode;
        } activity;
        struct {
            int32_t  latitude;                  /* degree, scale: 0.0000001, -90 <= latitude <= 90 */
            int32_t  longitude;                 /* degree, scale: 0.0000001, -180 <= latitude <= 180 */
        } position;
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
    char     activity_name[16+1];   /* name of activity in ISO 8859-1 */
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
    uint8_t  unknown3[4];
    uint32_t cadence_max_time;      /* ms */
    uint8_t  unknown4[4];
    uint8_t  unknown5[4];
    uint8_t  unknown6[24];
} ambit_log_header_t;

typedef struct ambit_log_entry_s {
    ambit_log_header_t header;
    uint32_t samples_count;
    ambit_log_sample_t *samples;
} ambit_log_entry_t;

/**
 * Try to detect clock
 * If clock detected, object handle is returned
 * \return object handle if clock found, else NULL
 */
ambit_object_t *libambit_detect(void);

/**
 * Close open Ambit object
 * \param object Object to close
 */
void libambit_close(ambit_object_t *object);

/**
 * Check if detected device is currently supported
 * \param object Object to check
 * \return true if device supported, else false
 */
bool libambit_device_supported(ambit_object_t *object);

/**
 * Get device info on connected dev
 * \param object Object to get info from
 * \param status Status object to be filled
 * \return 0 on success, else -1
 */
int libambit_device_info_get(ambit_object_t *object, ambit_device_info_t *status);

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

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif /* __LIBAMBIT_H__ */
