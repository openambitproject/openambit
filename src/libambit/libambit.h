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

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "libambit-types.h"

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

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
