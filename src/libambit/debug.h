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
#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stddef.h>
#include <stdint.h>

typedef enum debug_level_e {
    debug_level_err,
    debug_level_warn,
    debug_level_info
} debug_level_t;

void debug_printf(debug_level_t level, const char *file, int line, const char *func, const char *fmt, ...);

#ifdef DEBUG_PRINT_ERROR
#define LOG_ERROR(fmt, ...) debug_printf(debug_level_err, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#else
#define LOG_ERROR(fmt, ...)
#endif
#ifdef DEBUG_PRINT_WARNING
#define LOG_WARNING(fmt, ...) debug_printf(debug_level_warn, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#else
#define LOG_WARNING(fmt, ...)
#endif
#ifdef DEBUG_PRINT_INFO
#define LOG_INFO(fmt, ...) debug_printf(debug_level_info, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#else
#define LOG_INFO(fmt, ...)
#endif
#define ONLYDEBUGVAR(x) (void)(x)

#endif /* __DEBUG_H__ */
