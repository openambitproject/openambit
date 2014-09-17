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
#define PMEM20_LOG_START                  0x000f4240
#define PMEM20_LOG_SIZE                   0x0029f630 /* 2 750 000 */
#define PMEM20_LOG_WRAP_START_OFFSET      0x00000012
#define PMEM20_LOG_WRAP_BUFFER_MARGIN     0x00010000 /* Max theoretical size of sample */
#define PMEM20_LOG_HEADER_MIN_LEN                512 /* Header actually longer, but not interesting*/

#define PMEM20_GPS_ORBIT_START            0x000704e0

typedef struct __attribute__((__packed__)) periodic_sample_spec_s {
    uint16_t type;
    uint16_t offset;
    uint16_t length;
} periodic_sample_spec_t;

/*
 * Static functions
 */
static int parse_sample(uint8_t *buf, size_t offset, uint8_t **spec, ambit_log_entry_t *log_entry, size_t *sample_count);
static int read_upto(ambit_object_t *object, uint32_t address, uint32_t length);
static int read_log_chunk(ambit_object_t *object, uint32_t address);
static int write_data_chunk(ambit_object_t *object, uint32_t address, size_t buffer_count, uint8_t **buffers, size_t *buffer_sizes);
static void add_time(ambit_date_time_t *intime, int32_t offset, ambit_date_time_t *outtime);
static int is_leap(unsigned int y);
static void to_timeval(ambit_date_time_t *ambit_time, struct timeval *timeval);

/*
 * Static variables
 */



/*
 * Public functions
 */
int libambit_pmem20_init(ambit_object_t *object, uint16_t chunk_size)
{
    object->pmem20.chunk_size = chunk_size;

    return 0;
}

int libambit_pmem20_log_init(ambit_object_t *object)
{
    int ret = -1;
    size_t offset;

    // Allocate buffer for complete memory
    if (object->pmem20.log.buffer != NULL) {
        free(object->pmem20.log.buffer);
    }
    if (object->pmem20.log.chunks_read != NULL) {
        free(object->pmem20.log.chunks_read);
    }
    memset(&object->pmem20.log, 0, sizeof(object->pmem20.log));

    object->pmem20.log.buffer = malloc(PMEM20_LOG_SIZE + PMEM20_LOG_WRAP_BUFFER_MARGIN);
    object->pmem20.log.chunks_read = malloc((PMEM20_LOG_SIZE/object->pmem20.chunk_size)+1);

    if (object->pmem20.log.buffer != NULL && object->pmem20.log.chunks_read != NULL) {
        // Set all chunks to NOT read
        memset(object->pmem20.log.chunks_read, 0, (PMEM20_LOG_SIZE/object->pmem20.chunk_size)+1);

        // Read initial log header
        LOG_INFO("Reading first log data chunk");
        ret = read_log_chunk(object, PMEM20_LOG_START);
        
        if (ret == 0) {
            // Parse PMEM header
            offset = 0;
            object->pmem20.log.last_entry = read32inc(object->pmem20.log.buffer, &offset);
            object->pmem20.log.first_entry = read32inc(object->pmem20.log.buffer, &offset);
            object->pmem20.log.entries = read32inc(object->pmem20.log.buffer, &offset);
            object->pmem20.log.next_free_address = read32inc(object->pmem20.log.buffer, &offset);
            object->pmem20.log.current.current = PMEM20_LOG_START;
            object->pmem20.log.current.next = object->pmem20.log.first_entry;
            object->pmem20.log.current.prev = PMEM20_LOG_START;

            LOG_INFO("log data header read, entries=%d, first_entry=%08x, last_entry=%08x, next_free_address=%08x", object->pmem20.log.entries, object->pmem20.log.first_entry, object->pmem20.log.last_entry, object->pmem20.log.next_free_address);

            // Set initialized
            object->pmem20.log.initialized = true;
        }
        else {
            LOG_WARNING("Failed to read first data chunk");
        }
    }

    return ret;
}

int libambit_pmem20_deinit(ambit_object_t *object)
{
    if (object->pmem20.log.buffer != NULL) {
        free(object->pmem20.log.buffer);
    }
    if (object->pmem20.log.chunks_read != NULL) {
        free(object->pmem20.log.chunks_read);
    }
    memset(&object->pmem20.log, 0, sizeof(object->pmem20.log));

    return 0;
}

int libambit_pmem20_log_next_header(ambit_object_t *object, ambit_log_header_t *log_header)
{
    int ret = -1;
    size_t buffer_offset;
    uint16_t tmp_len;

    LOG_INFO("Reading header of next log entry");

    if (!object->pmem20.log.initialized) {
        LOG_ERROR("Trying to get next log without initialization");
        return -1;
    }

    // Check if we reached end of entries
    if (object->pmem20.log.current.current == object->pmem20.log.current.next) {
        LOG_INFO("No more entries to read");
        return 0;
    }

    if (read_upto(object, object->pmem20.log.current.next, PMEM20_LOG_HEADER_MIN_LEN) == 0) {
        buffer_offset = (object->pmem20.log.current.next - PMEM20_LOG_START);
        // First check that header seems to be correctly present
        if (strncmp((char*)object->pmem20.log.buffer + buffer_offset, "PMEM", 4) == 0) {
            object->pmem20.log.current.current = object->pmem20.log.current.next;
            buffer_offset += 4;
            object->pmem20.log.current.next = read32inc(object->pmem20.log.buffer, &buffer_offset);
            object->pmem20.log.current.prev = read32inc(object->pmem20.log.buffer, &buffer_offset);
            tmp_len = read16inc(object->pmem20.log.buffer, &buffer_offset);
            buffer_offset += tmp_len;
            tmp_len = read16inc(object->pmem20.log.buffer, &buffer_offset);
            if (libambit_pmem20_log_parse_header(object->pmem20.log.buffer + buffer_offset, tmp_len, log_header) == 0) {
                LOG_INFO("Log entry header parsed");
                ret = 1;
            }
            else {
                LOG_ERROR("Failed to parse log entry header correctly");
            }
        }
        else {
            LOG_ERROR("Failed to find valid log entry header start");
        }
    }
    else {
        LOG_WARNING("Failed to read log entry header");
    }

    // Unset initialized of something went wrong
    if (ret < 0) {
        object->pmem20.log.initialized = false;
    }

    return ret;
}

ambit_log_entry_t *libambit_pmem20_log_read_entry(ambit_object_t *object)
{
    // Note! We assume that the caller has called libambit_pmem20_log_next_header just before
    uint8_t *periodic_sample_spec;
    uint16_t tmp_len, sample_len;
    size_t buffer_offset, sample_count = 0, i;
    ambit_log_entry_t *log_entry;
    ambit_log_sample_t *last_periodic = NULL, *utcsource = NULL, *altisource = NULL;
    ambit_date_time_t utcbase;
    uint32_t altisource_index = 0;
    uint32_t last_base_lat = 0, last_base_long = 0;
    uint32_t last_small_lat = 0, last_small_long = 0;
    uint32_t last_ehpe = 0;

    if (!object->pmem20.log.initialized) {
        LOG_ERROR("Trying to get log entry without initialization");
        return NULL;
    }

    // Allocate log entry
    if ((log_entry = calloc(1, sizeof(ambit_log_entry_t))) == NULL) {
        object->pmem20.log.initialized = false;
        return NULL;
    }

    LOG_INFO("Reading log entry from address=%08x", object->pmem20.log.current.current);

    buffer_offset = (object->pmem20.log.current.current - PMEM20_LOG_START);
    buffer_offset += 12;
    // Read samples content definition
    tmp_len = read16inc(object->pmem20.log.buffer, &buffer_offset);
    periodic_sample_spec = object->pmem20.log.buffer + buffer_offset;
    buffer_offset += tmp_len;
    // Parse header
    tmp_len = read16inc(object->pmem20.log.buffer, &buffer_offset);
    if (libambit_pmem20_log_parse_header(object->pmem20.log.buffer + buffer_offset, tmp_len, &log_entry->header) != 0) {
        LOG_ERROR("Failed to parse log entry header correctly");
        free(log_entry);
        object->pmem20.log.initialized = false;
        return NULL;
    }
    buffer_offset += tmp_len;
    // Now that we know number of samples, allocate space for them!
    if ((log_entry->samples = calloc(log_entry->header.samples_count, sizeof(ambit_log_sample_t))) == NULL) {
        free(log_entry);
        object->pmem20.log.initialized = false;
        return NULL;
    }
    log_entry->samples_count = log_entry->header.samples_count;

    LOG_INFO("Log entry got %d samples, reading", log_entry->samples_count);

    // OK, so we are at start of samples, get them all!
    while (sample_count < log_entry->samples_count) {
        /* NOTE! The double reads below seems a bit unoptimized,
           but if we need optimization, we should optimize read_upto
           instead...
           To ease the pain on wraparound we simply duplicate the sample
           to the end of the buffer. */

        // First check for log area wrap
        if (buffer_offset >= PMEM20_LOG_SIZE - 1) {
            read_upto(object, PMEM20_LOG_START + PMEM20_LOG_WRAP_START_OFFSET, 2);
            sample_len = read16(object->pmem20.log.buffer, PMEM20_LOG_WRAP_START_OFFSET);
        }
        else if (buffer_offset == PMEM20_LOG_SIZE - 2) {
            read_upto(object, PMEM20_LOG_START + PMEM20_LOG_WRAP_START_OFFSET, 1);
            sample_len = object->pmem20.log.buffer[buffer_offset] | (object->pmem20.log.buffer[PMEM20_LOG_WRAP_START_OFFSET] << 8);
        }
        else {
            read_upto(object, PMEM20_LOG_START + buffer_offset, 2);
            sample_len = read16(object->pmem20.log.buffer, buffer_offset);
        }

        // Read all data
        if (buffer_offset + 2 < (PMEM20_LOG_SIZE-1)) {
            read_upto(object, PMEM20_LOG_START + buffer_offset + 2, sample_len);
        }
        if (buffer_offset + 2 + sample_len > PMEM20_LOG_SIZE) {
            read_upto(object, PMEM20_LOG_START + PMEM20_LOG_WRAP_START_OFFSET, (buffer_offset + 2 + sample_len) - PMEM20_LOG_SIZE);
            memcpy(object->pmem20.log.buffer + PMEM20_LOG_SIZE, object->pmem20.log.buffer + PMEM20_LOG_WRAP_START_OFFSET, (buffer_offset + 2 + sample_len) - PMEM20_LOG_SIZE);
        }

        if (parse_sample(object->pmem20.log.buffer, buffer_offset, &periodic_sample_spec, log_entry, &sample_count) == 1) {
            // Calculate times
            if (log_entry->samples[sample_count-1].type == ambit_log_sample_type_periodic) {
                last_periodic = &log_entry->samples[sample_count-1];
            }
            else if (last_periodic != NULL) {
                log_entry->samples[sample_count-1].time += last_periodic->time;
            }
            else {
                log_entry->samples[sample_count-1].time = 0;
            }

            if (utcsource == NULL && log_entry->samples[sample_count-1].type == ambit_log_sample_type_gps_base) {
                utcsource = &log_entry->samples[sample_count-1];
                // Calculate UTC base time
                add_time(&utcsource->u.gps_base.utc_base_time, 0-utcsource->time, &utcbase);
            }

            // Calculate positions
            if (log_entry->samples[sample_count-1].type == ambit_log_sample_type_gps_base) {
                last_base_lat = log_entry->samples[sample_count-1].u.gps_base.latitude;
                last_base_long = log_entry->samples[sample_count-1].u.gps_base.longitude;
                last_small_lat = log_entry->samples[sample_count-1].u.gps_base.latitude;
                last_small_long = log_entry->samples[sample_count-1].u.gps_base.longitude;
                last_ehpe = log_entry->samples[sample_count-1].u.gps_base.ehpe;
            }
            else if (log_entry->samples[sample_count-1].type == ambit_log_sample_type_gps_small) {
                log_entry->samples[sample_count-1].u.gps_small.latitude = last_base_lat + log_entry->samples[sample_count-1].u.gps_small.latitude*10;
                log_entry->samples[sample_count-1].u.gps_small.longitude = last_base_long + log_entry->samples[sample_count-1].u.gps_small.longitude*10;
                last_small_lat = log_entry->samples[sample_count-1].u.gps_small.latitude;
                last_small_long = log_entry->samples[sample_count-1].u.gps_small.longitude;
                last_ehpe = log_entry->samples[sample_count-1].u.gps_small.ehpe;
            }
            else if (log_entry->samples[sample_count-1].type == ambit_log_sample_type_gps_tiny) {
                log_entry->samples[sample_count-1].u.gps_tiny.latitude = last_small_lat + log_entry->samples[sample_count-1].u.gps_tiny.latitude*10;
                log_entry->samples[sample_count-1].u.gps_tiny.longitude = last_small_long + log_entry->samples[sample_count-1].u.gps_tiny.longitude*10;
                log_entry->samples[sample_count-1].u.gps_tiny.ehpe = (last_ehpe > 700 ? 700 : last_ehpe);
                last_small_lat = log_entry->samples[sample_count-1].u.gps_tiny.latitude;
                last_small_long = log_entry->samples[sample_count-1].u.gps_tiny.longitude;
            }

            if (altisource == NULL && log_entry->samples[sample_count-1].type == ambit_log_sample_type_altitude_source) {
                altisource = &log_entry->samples[sample_count-1];
                altisource_index = sample_count-1;
            }
        }
        buffer_offset += 2 + sample_len;
        // Wrap
        if (buffer_offset >= PMEM20_LOG_SIZE) {
            buffer_offset = PMEM20_LOG_WRAP_START_OFFSET + (buffer_offset - PMEM20_LOG_SIZE);
        }
    }

    // Loop through samples again and correct times etc
    for (sample_count = 0; sample_count < log_entry->header.samples_count; sample_count++) {
        // Set UTC times (if UTC source found)
        if (utcsource != NULL) {
            add_time(&utcbase, log_entry->samples[sample_count].time, &log_entry->samples[sample_count].utc_time);
        }
        // Correct altitude based on altitude offset in altitude source
        if (altisource != NULL && log_entry->samples[sample_count].type == ambit_log_sample_type_periodic && sample_count < altisource_index) {
            for (i=0; i<log_entry->samples[sample_count].u.periodic.value_count; i++) {
                if (log_entry->samples[sample_count].u.periodic.values[i].type == ambit_log_sample_periodic_type_sealevelpressure) {
                    log_entry->samples[sample_count].u.periodic.values[i].u.sealevelpressure += altisource->u.altitude_source.pressure_offset;
                }
                if (log_entry->samples[sample_count].u.periodic.values[i].type == ambit_log_sample_periodic_type_altitude) {
                    log_entry->samples[sample_count].u.periodic.values[i].u.altitude += altisource->u.altitude_source.altitude_offset;
                }
            }
        }
    }

    return log_entry;
}

int libambit_pmem20_log_parse_header(uint8_t *data, size_t datalen, ambit_log_header_t *log_header)
{
    size_t offset = 0;

    // Check that header is long enough to be parsed correctly
    if (datalen < 129) {
        return -1;
    }

    offset = 1;
    log_header->date_time.year = read16inc(data, &offset);
    log_header->date_time.month = read8inc(data, &offset);
    log_header->date_time.day = read8inc(data, &offset);
    log_header->date_time.hour = read8inc(data, &offset);
    log_header->date_time.minute = read8inc(data, &offset);
    log_header->date_time.msec = read8inc(data, &offset)*1000;

    memcpy(log_header->unknown1, data+offset, 5);
    offset += 5;

    log_header->duration = read32inc(data, &offset)*100; // seconds 0.1
    log_header->ascent = read16inc(data, &offset);
    log_header->descent = read16inc(data, &offset);
    log_header->ascent_time = read32inc(data, &offset)*1000;
    log_header->descent_time = read32inc(data, &offset)*1000;
    log_header->recovery_time = read16inc(data, &offset)*60*1000;
    log_header->speed_avg = read16inc(data, &offset)*10; // 10 m/h
    log_header->speed_max = read16inc(data, &offset)*10; // 10 m/h
    log_header->altitude_max = read16inc(data, &offset);
    log_header->altitude_min = read16inc(data, &offset);
    log_header->heartrate_avg = read8inc(data, &offset);
    log_header->heartrate_max = read8inc(data, &offset);
    log_header->peak_training_effect = read8inc(data, &offset);
    log_header->activity_type = read8inc(data, &offset);
    memcpy(log_header->activity_name, data + offset, 16);
    log_header->activity_name[16] = 0;
    offset += 16;
    log_header->heartrate_min = read8inc(data, &offset);

    log_header->unknown2 = read8inc(data, &offset);

    log_header->temperature_max = read16inc(data, &offset);
    log_header->temperature_min = read16inc(data, &offset);
    log_header->distance = read32inc(data, &offset);
    log_header->samples_count = read32inc(data, &offset);
    log_header->energy_consumption = read16inc(data, &offset);

    log_header->cadence_max = read8inc(data, &offset);
    log_header->cadence_avg = read8inc(data, &offset);

    memcpy(log_header->unknown3, data+offset, 4);
    offset += 4;

    log_header->speed_max_time = read32inc(data, &offset);
    log_header->altitude_max_time = read32inc(data, &offset);
    log_header->altitude_min_time = read32inc(data, &offset);
    log_header->heartrate_max_time = read32inc(data, &offset);
    log_header->heartrate_min_time = read32inc(data, &offset);
    log_header->temperature_max_time = read32inc(data, &offset);
    log_header->temperature_min_time = read32inc(data, &offset);
    log_header->cadence_max_time = read32inc(data, &offset);

    memcpy(log_header->unknown4, data+offset, 4);
    offset += 4;

    log_header->first_fix_time = read16inc(data, &offset)*1000;
    log_header->battery_start = read8inc(data, &offset);
    log_header->battery_end = read8inc(data, &offset);

    memcpy(log_header->unknown5, data+offset, 4);
    offset += 4;

    log_header->distance_before_calib = read32inc(data, &offset);

    if (datalen >= offset + 24) {
        memcpy(log_header->unknown6, data+offset, 24);
        offset += 24;
    }

    return 0;
}

int libambit_pmem20_gps_orbit_write(ambit_object_t *object, uint8_t *data, size_t datalen)
{
    int ret = -1;
    uint8_t *bufptrs[2];
    size_t bufsizes[2];
    uint8_t startheader[4];
    uint8_t tailbuf[8];
    uint32_t *_sizeptr = (uint32_t*)&startheader[0];
    uint32_t address = PMEM20_GPS_ORBIT_START;
    size_t offset = 0;

    *_sizeptr = htole32(datalen);
    bufptrs[0] = startheader;
    bufsizes[0] = 4;
    bufptrs[1] = data;
    bufsizes[1] = object->pmem20.chunk_size - 4; // We assume that data is
                                                 // always > chunk_size

    // Write first chunk (including length)
    ret = write_data_chunk(object, address, 2, bufptrs, bufsizes);
    offset += bufsizes[1];
    address += object->pmem20.chunk_size;

    // Write rest of the chunks
    while (ret == 0 && offset < datalen) {
        bufptrs[0] = data + offset;
        bufsizes[0] = (datalen - offset > object->pmem20.chunk_size ? object->pmem20.chunk_size : datalen - offset);

        ret = write_data_chunk(object, address, 1, bufptrs, bufsizes);
        offset += bufsizes[0];
        address += bufsizes[0];
    }

    // Write tail length (or what is really!?)
    if (ret == 0) {
        *((uint32_t*)(&tailbuf[0])) = htole32(PMEM20_GPS_ORBIT_START);
        *((uint32_t*)(&tailbuf[4])) = htole32(bufsizes[0]);
        ret = libambit_protocol_command(object, ambit_command_data_tail_len, tailbuf, sizeof(tailbuf), NULL, NULL, 0);
    }

    return ret;
}

/**
 * Parse the given sample
 * \return number of samples added (1 or 0)
 */
static int parse_sample(uint8_t *buf, size_t offset, uint8_t **spec, ambit_log_entry_t *log_entry, size_t *sample_count)
{
    int ret = 0;
    size_t int_offset = offset;
    uint16_t sample_len = read16inc(buf, &int_offset);
    uint8_t  sample_type = read8inc(buf, &int_offset);
    uint8_t  episodic_type;
    uint16_t spec_count, spec_type, spec_offset;
    periodic_sample_spec_t *spec_entry;
    int i;

    switch (sample_type) {
      case 0:   /* periodic sample specifier */
        // Update specifier on input
        *spec = buf + offset + 2;
        break;
      case 2:   /* periodic sample */
        log_entry->samples[*sample_count].type = ambit_log_sample_type_periodic;
        log_entry->samples[*sample_count].time = read32(buf, offset + sample_len - 2);
        
        // Loop through specifier and set corresponding fields
        spec_count = read16(*spec, 1);
        log_entry->samples[*sample_count].u.periodic.value_count = spec_count;
        log_entry->samples[*sample_count].u.periodic.values = calloc(spec_count, sizeof(ambit_log_sample_periodic_value_t));
        for (i=0, spec_entry = (periodic_sample_spec_t*)(*spec + 3); i<spec_count; i++, spec_entry++) {
            spec_type = le16toh(spec_entry->type);
            spec_offset = le16toh(spec_entry->offset);
            switch(spec_type) {
              case ambit_log_sample_periodic_type_latitude:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_latitude;
                log_entry->samples[*sample_count].u.periodic.values[i].u.latitude = read32(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_longitude:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_longitude;
                log_entry->samples[*sample_count].u.periodic.values[i].u.longitude = read32(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_distance:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_distance;
                log_entry->samples[*sample_count].u.periodic.values[i].u.distance = read32(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_speed:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_speed;
                log_entry->samples[*sample_count].u.periodic.values[i].u.speed = read16(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_hr:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_hr;
                log_entry->samples[*sample_count].u.periodic.values[i].u.hr = read8(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_time:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_time;
                log_entry->samples[*sample_count].u.periodic.values[i].u.time = read32(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_gpsspeed:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_gpsspeed;
                log_entry->samples[*sample_count].u.periodic.values[i].u.gpsspeed = read16(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_wristaccspeed:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_wristaccspeed;
                log_entry->samples[*sample_count].u.periodic.values[i].u.wristaccspeed = read16(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_bikepodspeed:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_bikepodspeed;
                log_entry->samples[*sample_count].u.periodic.values[i].u.bikepodspeed = read16(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_ehpe:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_ehpe;
                log_entry->samples[*sample_count].u.periodic.values[i].u.ehpe = read32(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_evpe:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_evpe;
                log_entry->samples[*sample_count].u.periodic.values[i].u.evpe = read32(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_altitude:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_altitude;
                log_entry->samples[*sample_count].u.periodic.values[i].u.altitude = read16(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_abspressure:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_abspressure;
                log_entry->samples[*sample_count].u.periodic.values[i].u.abspressure = read32(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_energy:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_energy;
                log_entry->samples[*sample_count].u.periodic.values[i].u.energy = read16(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_temperature:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_temperature;
                log_entry->samples[*sample_count].u.periodic.values[i].u.temperature = read16(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_charge:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_charge;
                log_entry->samples[*sample_count].u.periodic.values[i].u.charge = read8(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_gpsaltitude:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_gpsaltitude;
                log_entry->samples[*sample_count].u.periodic.values[i].u.gpsaltitude = read32(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_gpsheading:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_gpsheading;
                log_entry->samples[*sample_count].u.periodic.values[i].u.gpsheading = read16(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_gpshdop:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_gpshdop;
                log_entry->samples[*sample_count].u.periodic.values[i].u.gpshdop = read8(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_gpsvdop:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_gpsvdop;
                log_entry->samples[*sample_count].u.periodic.values[i].u.gpsvdop = read8(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_wristcadence:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_wristcadence;
                log_entry->samples[*sample_count].u.periodic.values[i].u.wristcadence = read16(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_snr:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_snr;
                memcpy(log_entry->samples[*sample_count].u.periodic.values[i].u.snr, buf + int_offset + spec_offset, 16);
                break;
              case ambit_log_sample_periodic_type_noofsatellites:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_noofsatellites;
                log_entry->samples[*sample_count].u.periodic.values[i].u.noofsatellites = read8(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_sealevelpressure:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_sealevelpressure;
                log_entry->samples[*sample_count].u.periodic.values[i].u.sealevelpressure = read16(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_verticalspeed:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_verticalspeed;
                log_entry->samples[*sample_count].u.periodic.values[i].u.verticalspeed = read16(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_cadence:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_cadence;
                log_entry->samples[*sample_count].u.periodic.values[i].u.cadence = read8(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_bikepower:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_bikepower;
                log_entry->samples[*sample_count].u.periodic.values[i].u.bikepower = read16(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_swimingstrokecnt:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_swimingstrokecnt;
                log_entry->samples[*sample_count].u.periodic.values[i].u.swimingstrokecnt = read32(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_ruleoutput1:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_ruleoutput1;
                log_entry->samples[*sample_count].u.periodic.values[i].u.ruleoutput1 = read32(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_ruleoutput2:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_ruleoutput2;
                log_entry->samples[*sample_count].u.periodic.values[i].u.ruleoutput2 = read32(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_ruleoutput3:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_ruleoutput3;
                log_entry->samples[*sample_count].u.periodic.values[i].u.ruleoutput3 = read32(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_ruleoutput4:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_ruleoutput4;
                log_entry->samples[*sample_count].u.periodic.values[i].u.ruleoutput4 = read32(buf, int_offset + spec_offset);
                break;
              case ambit_log_sample_periodic_type_ruleoutput5:
                log_entry->samples[*sample_count].u.periodic.values[i].type = ambit_log_sample_periodic_type_ruleoutput5;
                log_entry->samples[*sample_count].u.periodic.values[i].u.ruleoutput5 = read32(buf, int_offset + spec_offset);
                break;
            }
        }
        ret = 1;
        break;
      case 3:
        // First parameter is relative time
        log_entry->samples[*sample_count].time = read32inc(buf, &int_offset);
        episodic_type = read8inc(buf, &int_offset);
        switch (episodic_type) {
          case 0x04:
            log_entry->samples[*sample_count].type = ambit_log_sample_type_logpause;
            break;
          case 0x05:
            log_entry->samples[*sample_count].type = ambit_log_sample_type_logrestart;
            break;
          case 0x06:
            log_entry->samples[*sample_count].type = ambit_log_sample_type_ibi;
            for (i=0; i<(sample_len - 6)/2; i++) {
                log_entry->samples[*sample_count].u.ibi.ibi[i] = read16inc(buf, &int_offset);
            }
            log_entry->samples[*sample_count].u.ibi.ibi_count = (sample_len - 6)/2;
            break;
          case 0x07:
            log_entry->samples[*sample_count].type = ambit_log_sample_type_ttff;
            log_entry->samples[*sample_count].u.ttff = read16inc(buf, &int_offset);
            break;
          case 0x08:
            log_entry->samples[*sample_count].type = ambit_log_sample_type_distance_source;
            log_entry->samples[*sample_count].u.distance_source = read8inc(buf, &int_offset);
            break;
          case 0x09:
            log_entry->samples[*sample_count].type = ambit_log_sample_type_lapinfo;
            log_entry->samples[*sample_count].u.lapinfo.event_type = read8inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.lapinfo.date_time.year = read16inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.lapinfo.date_time.month = read8inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.lapinfo.date_time.day = read8inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.lapinfo.date_time.hour = read8inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.lapinfo.date_time.minute = read8inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.lapinfo.date_time.msec = read8inc(buf, &int_offset)*1000;
            log_entry->samples[*sample_count].u.lapinfo.duration = read32inc(buf, &int_offset)*100;
            log_entry->samples[*sample_count].u.lapinfo.distance = read32inc(buf, &int_offset);
            break;
          case 0x0d:
            log_entry->samples[*sample_count].type = ambit_log_sample_type_altitude_source;
            log_entry->samples[*sample_count].u.altitude_source.source_type = read8inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.altitude_source.altitude_offset = read16inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.altitude_source.pressure_offset = read16inc(buf, &int_offset);
            break;
          case 0x0f:
            log_entry->samples[*sample_count].type = ambit_log_sample_type_gps_base;
            log_entry->samples[*sample_count].u.gps_base.navvalid = read16inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.gps_base.navtype = read16inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.gps_base.utc_base_time.year = read16inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.gps_base.utc_base_time.month = read8inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.gps_base.utc_base_time.day = read8inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.gps_base.utc_base_time.hour = read8inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.gps_base.utc_base_time.minute = read8inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.gps_base.utc_base_time.msec = read16inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.gps_base.latitude = read32inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.gps_base.longitude = read32inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.gps_base.altitude = read32inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.gps_base.speed = read16inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.gps_base.heading = read16inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.gps_base.ehpe = read32inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.gps_base.noofsatellites = read8inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.gps_base.hdop = read8inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.gps_base.satellites = calloc((sample_len - 40)/4, sizeof(ambit_log_gps_satellite_t));
            for (i=0; i<(sample_len - 40)/4; i++) {
                log_entry->samples[*sample_count].u.gps_base.satellites[i].sv = read8inc(buf, &int_offset);
                log_entry->samples[*sample_count].u.gps_base.satellites[i].state = read8inc(buf, &int_offset);
                int_offset += 1;
                log_entry->samples[*sample_count].u.gps_base.satellites[i].snr = read8inc(buf, &int_offset);
            }
            log_entry->samples[*sample_count].u.gps_base.satellites_count = (sample_len - 40)/4;
            break;
          case 0x10:
            log_entry->samples[*sample_count].type = ambit_log_sample_type_gps_small;
            log_entry->samples[*sample_count].u.gps_small.latitude = (int16_t)read16inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.gps_small.longitude = (int16_t)read16inc(buf, &int_offset);
            int_offset += 2; // Time (seconds)
            log_entry->samples[*sample_count].u.gps_small.ehpe = read8inc(buf, &int_offset)*100;
            log_entry->samples[*sample_count].u.gps_small.noofsatellites = read8inc(buf, &int_offset);
            break;
          case 0x11:
            log_entry->samples[*sample_count].type = ambit_log_sample_type_gps_tiny;
            log_entry->samples[*sample_count].u.gps_tiny.latitude = (int8_t)read8inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.gps_tiny.longitude = (int8_t)read8inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.gps_tiny.unknown = read8inc(buf, &int_offset);
            break;
          case 0x12:
            log_entry->samples[*sample_count].type = ambit_log_sample_type_time;
            log_entry->samples[*sample_count].u.time.hour = read8inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.time.minute = read8inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.time.second = read8inc(buf, &int_offset);
            break;
          case 0x18:
            log_entry->samples[*sample_count].type = ambit_log_sample_type_activity;
            log_entry->samples[*sample_count].u.activity.activitytype = read16inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.activity.custommode = read32inc(buf, &int_offset);
            break;
          case 0x1b:
            log_entry->samples[*sample_count].type = ambit_log_sample_type_position;
            log_entry->samples[*sample_count].u.position.latitude = read32inc(buf, &int_offset);
            log_entry->samples[*sample_count].u.position.longitude = read32inc(buf, &int_offset);
            break;
          default:
            LOG_WARNING("Found unknown episodic sample type (0x%02x)", episodic_type);
            log_entry->samples[*sample_count].type = ambit_log_sample_type_unknown;
            log_entry->samples[*sample_count].u.unknown.datalen = sample_len;
            log_entry->samples[*sample_count].u.unknown.data = malloc(sample_len);
            memcpy(log_entry->samples[*sample_count].u.unknown.data, buf + offset + 2, sample_len);
            break;
        }
        ret = 1;
        break;
      default:
        LOG_WARNING("Found unknown sample type (0x%02x)", sample_type);
        log_entry->samples[*sample_count].type = ambit_log_sample_type_unknown;
        log_entry->samples[*sample_count].u.unknown.datalen = sample_len;
        log_entry->samples[*sample_count].u.unknown.data = malloc(sample_len);
        memcpy(log_entry->samples[*sample_count].u.unknown.data, buf + offset + 2, sample_len);
        ret = 1;
        break;
    }

    *sample_count += ret;

    return ret;
}

static int read_upto(ambit_object_t *object, uint32_t address, uint32_t length)
{
    uint32_t start_address = address - ((address - PMEM20_LOG_START) % object->pmem20.chunk_size);

    while (start_address < address + length) {
        if (object->pmem20.log.chunks_read[(start_address - PMEM20_LOG_START)/object->pmem20.chunk_size] == 0) {
            if (read_log_chunk(object, start_address) != 0) {
                return -1;
            }
            object->pmem20.log.chunks_read[(start_address - PMEM20_LOG_START)/object->pmem20.chunk_size] = 1;
        }
        start_address += object->pmem20.chunk_size;
    }

    return 0;
}

static int read_log_chunk(ambit_object_t *object, uint32_t address)
{
    int ret = -1;

    uint8_t *buffer = object->pmem20.log.buffer + (address - PMEM20_LOG_START);
    uint32_t length = object->pmem20.chunk_size;

    uint8_t *reply = NULL;
    size_t replylen = 0;

    uint8_t send_data[8];
    uint32_t *_address = (uint32_t*)&send_data[0];
    uint32_t *_length = (uint32_t*)&send_data[4];

    if ((address + object->pmem20.chunk_size) > (PMEM20_LOG_START + PMEM20_LOG_SIZE)) {
        length = PMEM20_LOG_START + PMEM20_LOG_SIZE - address;
    }

    *_address = htole32(address);
    *_length = htole32(length);

    if (libambit_protocol_command(object, ambit_command_log_read, send_data, sizeof(send_data), &reply, &replylen, 0) == 0 &&
        replylen == length + 8) {
        memcpy(buffer, reply + 8, length);
        ret = 0;
    }

    libambit_protocol_free(reply);

    return ret;
}

static int write_data_chunk(ambit_object_t *object, uint32_t address, size_t buffer_count, uint8_t **buffers, size_t *buffer_sizes)
{
    int ret = -1;

    uint8_t *reply = NULL;
    size_t replylen = 0;

    uint8_t *send_data;
    size_t send_data_len = 8;

    int i;
    for (i=0; i<buffer_count; i++) {
        send_data_len += buffer_sizes[i];
    }
    send_data = malloc(send_data_len);

    if (send_data != NULL) {
        uint32_t *_address = (uint32_t*)&send_data[0];
        uint32_t *_length = (uint32_t*)&send_data[4];
        uint8_t *send_data_ptr = &send_data[8];

        *_address = htole32(address);
        *_length = htole32(send_data_len - 8);

        for (i=0; i<buffer_count; i++) {
            memcpy(send_data_ptr, buffers[i], buffer_sizes[i]);
            send_data_ptr += buffer_sizes[i];
        }

        if (libambit_protocol_command(object, ambit_command_data_write, send_data, send_data_len, &reply, &replylen, 0) == 0) {
            ret = 0;
        }

        free(send_data);
        libambit_protocol_free(reply);
    }

    return ret;
}

static void add_time(ambit_date_time_t *intime, int32_t offset, ambit_date_time_t *outtime)
{
    struct timeval timeval;
    struct tm *tm;

    to_timeval(intime, &timeval);

    if (offset >= 0) {
        timeval.tv_sec += offset / 1000;
        timeval.tv_usec += (offset % 1000)*1000;
    }
    else {
        timeval.tv_sec += offset / 1000;
        timeval.tv_usec -= (abs(offset) % 1000)*1000;
    }
    if (timeval.tv_usec >= 1000000) {
        timeval.tv_sec++;
        timeval.tv_usec -= 1000000;
    }
    if(timeval.tv_usec < 0) {
        timeval.tv_sec--;
        timeval.tv_usec += 1000000;
    }

    tm = gmtime(&timeval.tv_sec);
    outtime->msec = tm->tm_sec * 1000 + (timeval.tv_usec / 1000);
    outtime->minute = tm->tm_min;
    outtime->hour = tm->tm_hour;
    outtime->day = tm->tm_mday;
    outtime->month = tm->tm_mon;
    outtime->year = tm->tm_year;
}

static int is_leap(unsigned int y) {
    y += 1900;
    return (y % 4) == 0 && ((y % 100) != 0 || (y % 400) == 0);
}

static void to_timeval(ambit_date_time_t *ambit_time, struct timeval *timeval) {
    static const unsigned ndays[2][12] = {
        {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
        {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
    };
    timeval->tv_sec = 0;
    timeval->tv_usec = 0;
    int i;

    for (i = 70; i < ambit_time->year; ++i) {
        timeval->tv_sec += is_leap(i) ? 366 : 365;
    }

    for (i = 0; i < ambit_time->month; ++i) {
        timeval->tv_sec += ndays[is_leap(ambit_time->year)][i];
    }
    timeval->tv_sec += ambit_time->day - 1;
    timeval->tv_sec *= 24;
    timeval->tv_sec += ambit_time->hour;
    timeval->tv_sec *= 60;
    timeval->tv_sec += ambit_time->minute;
    timeval->tv_sec *= 60;
    timeval->tv_sec += ambit_time->msec / 1000;
    timeval->tv_usec = (ambit_time->msec % 1000)*1000;
}
