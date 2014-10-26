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

#endif /* __UTILS_H__ */
