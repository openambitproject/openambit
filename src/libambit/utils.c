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
#include "utils.h"
#include "debug.h"

#include <ctype.h>
#include <errno.h>
#include <iconv.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int date_get_num(const char **pp, int n_min, int n_max, int len_max)
{
    int i, val, c;
    const char *p;
    p = *pp;
    val = 0;
    for(i = 0; i < len_max; i++) {
        c = *p;
        if (!isdigit(c))
            break;
        val = (val * 10) + c - '0';
        p++;
    }
    /* no number read ? */
    if (p == *pp)
        return -1;
    if (val < n_min || val > n_max)
        return -1;
    *pp = p;
    return val;
}
char *libambit_strptime(const char *p, const char *fmt, struct tm *dt)
{
    int c, val;
    for(;;) {
        /* consume time string until a non whitespace char is found */
        while (isspace(*fmt)) {
            while (isspace(*p)) {
                p++;
            }
            fmt++;
        }
        c = *fmt++;
        if (c == '\0') {
            return (char *)p;
        } else if (c == '%') {
            c = *fmt++;
            switch(c) {
              case 'H':
              case 'J':
                val = date_get_num(&p, 0, c == 'H' ? 23 : INT_MAX, 2);
                if (val == -1)
                    return NULL;
                dt->tm_hour = val;
                break;
              case 'M':
                val = date_get_num(&p, 0, 59, 2);
                if (val == -1)
                    return NULL;
                dt->tm_min = val;
                break;
              case 'S':
                val = date_get_num(&p, 0, 59, 2);
                if (val == -1)
                    return NULL;
                dt->tm_sec = val;
                break;
              case 'Y':
                val = date_get_num(&p, 0, 9999, 4);
                if (val == -1)
                    return NULL;
                dt->tm_year = val - 1900;
                break;
              case 'm':
                val = date_get_num(&p, 1, 12, 2);
                if (val == -1)
                    return NULL;
                dt->tm_mon = val - 1;
                break;
              case 'd':
                val = date_get_num(&p, 1, 31, 2);
                if (val == -1)
                    return NULL;
                dt->tm_mday = val;
                break;
              case '%':
                goto match;
              default:
                return NULL;
            }
        } else {
          match:
            if (c != *p)
                return NULL;
            p++;
        }
    }
}

/* return number representation of hex, or on error 0xff */
static uint8_t hextob(char ch)
{
    if (ch >= '0' && ch <= '9')
        return (ch - '0');
    else if (ch >= 'A' && ch <= 'F')
        return (ch - 'A' + 10);
    else if (ch >= 'a' && ch <= 'f')
        return (ch - 'a' + 10);
    else
        return 0xff;
}
int libambit_htob(const char *hex_string, uint8_t *binary, size_t binary_size)
{
    int i = 0;
    uint8_t ch;
    size_t bytes_written = 0;

    if (hex_string[0] == '\0' || strlen(hex_string) % 2 != 0) {
        return -1;
    }

    while (bytes_written < binary_size && *hex_string != '\0') {
        if ((ch = hextob(*(hex_string++))) == 0xff)
            return -1;
        binary[i] = ch << 4;
        if ((ch = hextob(*(hex_string++))) == 0xff)
            return -1;
        binary[i++] |= ch;
    }

    return i;
}

char * utf8memconv(const char *src, size_t n, const char *encoding)
{
    char *rv = NULL;
    iconv_t cd = (iconv_t) -1;

    if (src) {
        cd = iconv_open("UTF-8", (encoding ? encoding : "ASCII"));
        if ((iconv_t) -1 == cd) {
            LOG_ERROR("iconv_open: %s", strerror(errno));
        }
        else {
            size_t ilen = n;
            size_t olen = n * 4 + 1;
            char  *ibuf = (char *)src;
            char  *obuf = (char *)malloc(olen * sizeof(char));

            if (obuf) {
                size_t n = olen;
                size_t sz;

                rv = obuf;
                sz = iconv(cd, &ibuf, &ilen, &obuf, &olen);

                if ((size_t) -1 == sz) {
                    LOG_ERROR("iconv: %s", strerror(errno));
                    free(rv);
                    rv = NULL;
                }
                else {      /* we're good, terminate string */
                    rv[n - olen] = '\0';
                    rv = realloc(rv, strlen(rv) + 1);
                }
            }
        }
    }

    if ((iconv_t) -1 != cd) {
        iconv_close(cd);
    }

    return rv;
}

char * utf8wcsconv(const wchar_t *src)
{
    size_t len = wcslen(src) * sizeof(wchar_t);

    return utf8memconv((char *)src, len, "WCHAR_T");
}
