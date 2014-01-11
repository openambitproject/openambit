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

#include <stdarg.h>
#include <stdio.h>

/*
 * Local variables
 */
static char debug_err_text[] = "ERROR";
static char debug_warn_text[] = "WARNING";
static char debug_info_text[] = "INFO";

void debug_printf(debug_level_t level, const char *file, int line, const char *func, const char *fmt, ...)
{
    FILE *output;
    const char *leveltxt;
    va_list ap;

    if (level == debug_level_err) {
        output = stderr;
        leveltxt = debug_err_text;
    }
    else if (level == debug_level_warn) {
        output = stderr;
        leveltxt = debug_warn_text;
    }
    else {
        output = stdout;
        leveltxt = debug_info_text;
    }

    fprintf(output, "libambit %s: ", leveltxt);
#ifdef DEBUG_PRINT_FILE_LINE
    fprintf(output, "%s:%d ", file, line);
#else
    // Remove compiler warning
    file = NULL;
    line = 0;
#endif
    fprintf(output, "%s(): ", func);

    va_start(ap, fmt);
    vfprintf(output, fmt, ap);
    va_end(ap);

    fprintf(output, "\n");

    fflush(output);
}
