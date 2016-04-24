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
#ifndef __UTILS_H__
#define __UTILS_H__

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <wchar.h>

/**
 * Reduced implementation of the Unix specific strptime
 */
char *libambit_strptime(const char *p, const char *fmt, struct tm *dt);

/**
 * Hex string to binary conversion
 * \param Hex string to parse
 * \param binary Buffer to store parsed hex data
 * \param binary_size Length of buffer
 * \return Number of converted bytes, or -1 if error occured
 */
int libambit_htob(const char *hex_string, uint8_t *binary, size_t binary_size);

/**
 * Converts \a n octets to a UTF-8 encoded string.
 *
 * The caller gets to manage the memory associated with the returned
 * string.  In case \a encoding is \c NULL, ASCII will be assumed.
 */
char * utf8memconv(const char *src, size_t n, const char *encoding);

/**
 * Converts a wide character string to a UTF-8 encoded one.
 *
 * The caller get s to manage the memory associated with the returned
 * string.
 */
char * utf8wcsconv(const wchar_t *src);

// static helpers
static inline uint8_t read8(const uint8_t *buf, size_t offset)
{
    return buf[offset];
}

static inline uint16_t read16(const uint8_t *buf, size_t offset)
{
    return (buf[offset] | (buf[offset+1] << 8));
}

static inline uint32_t read32(const uint8_t *buf, size_t offset)
{
    return (buf[offset] | (buf[offset+1] << 8) | (buf[offset+2] << 16) | (buf[offset+3] << 24));
}

static inline uint8_t read8inc(const uint8_t *buf, size_t *offset)
{
    *offset += 1;
    return buf[(*offset)-1];
}

static inline uint16_t read16inc(const uint8_t *buf, size_t *offset)
{
    *offset += 2;
    return (buf[(*offset)-2] | (buf[(*offset)-1] << 8));
}

static inline uint32_t read32inc(const uint8_t *buf, size_t *offset)
{
    *offset += 4;
    return (buf[(*offset)-4] | (buf[(*offset)-3] << 8) | (buf[(*offset)-2] << 16) | (buf[(*offset)-1] << 24));
}

static inline uint8_t *find_sequence(uint8_t *buf, size_t size, const uint8_t *seq, size_t seq_len)
{
    size_t i = 0;

    while (1) {
        if (i - size < seq_len)
           return NULL;

        if (memcmp((char *) buf + i, (const char *) seq, seq_len) == 0)
            return buf + i;

        i++;
    }

    return NULL;
}

#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])
#define ARRAY_FOR_EACH(_arr, _elem) \
    for (size_t _i = 0; _i < ARRAY_LENGTH(_arr) && (_elem = &_arr[_i]); _i++)

#endif /* __UTILS_H__ */
