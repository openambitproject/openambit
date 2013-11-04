#ifndef __LIBAMBIT_H__
#define __LIBAMBIT_H__

#include <stdint.h>
#include <time.h>

typedef struct ambit_object_s ambit_object_t;

typedef struct ambit_device_info_s {
    char model[17];
    char serial[17];
    uint8_t fw_version[4];
    uint8_t hw_version[4];
} ambit_device_info_t;

typedef struct ambit_device_data_time_s {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint16_t ms;
} ambit_device_data_time_t;

typedef struct ambit_device_status_s {
    uint8_t  charge;
} ambit_device_status_t;

typedef struct ambit_personal_settings_s {
    uint16_t weight;
    uint16_t birthyear;
    uint8_t  max_hr;
    uint8_t  rest_hr;
    uint8_t  length;
} ambit_personal_settings_t;

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

int libambit_device_info_get(ambit_object_t *object, ambit_device_info_t *status);

int libambit_date_time_set(ambit_object_t *object, struct tm *date_time);

int libambit_device_status_get(ambit_object_t *object, ambit_device_status_t *status);

int libambit_personal_settings_get(ambit_object_t *object, ambit_personal_settings_t *settings);

int libambit_log_read_get_next(ambit_object_t *object);

#endif /* __LIBAMBIT_H__ */
