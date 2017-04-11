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
#ifndef __PMEM20_H__
#define __PMEM20_H__

#include <stddef.h>
#include <stdint.h>
#include "libambit.h"

typedef struct libambit_pmem20_s {
    uint16_t chunk_size;
    struct {
        bool initialized;
        uint32_t mem_start;
        uint32_t mem_size;
        uint32_t first_entry;
        uint32_t last_entry;
        uint32_t entries;
        uint32_t next_free_address;
        struct {
            uint32_t current;
            uint32_t next;
            uint32_t prev;
        } current;
        uint8_t *buffer;
        uint8_t *chunks_read;
    } log;
    ambit_object_t *ambit_object;
} libambit_pmem20_t;

int libambit_pmem20_init(libambit_pmem20_t *object, ambit_object_t *ambit_object, uint16_t chunk_size);
int libambit_pmem20_deinit(libambit_pmem20_t *object);
int libambit_pmem20_log_init(libambit_pmem20_t *object, uint32_t mem_start, uint32_t mem_size);
int libambit_pmem20_log_deinit(libambit_pmem20_t *object);
int libambit_pmem20_log_next_header(libambit_pmem20_t *object, ambit_log_header_t *log_header);
ambit_log_entry_t *libambit_pmem20_log_read_entry(libambit_pmem20_t *object);
ambit_log_entry_t *libambit_pmem20_log_read_entry_address(libambit_pmem20_t *object, uint32_t address, uint32_t length);
int libambit_pmem20_log_parse_header(uint8_t *data, size_t datalen, ambit_log_header_t *log_header);
int libambit_pmem20_gps_orbit_write(libambit_pmem20_t *object, const uint8_t *data, size_t datalen, bool include_sha256_hash);
int libambit_pmem20_sport_mode_write(libambit_pmem20_t *object, const uint8_t *data, size_t datalen, bool include_sha256_hash);
int libambit_pmem20_app_data_write(libambit_pmem20_t *object, const uint8_t *data, size_t datalen, bool include_sha256_hash);

#endif /* __PMEM20_H__ */
