/*
 * (C) Copyright 2017 Lars Andre Landås
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

#ifndef __DISTANCE_H__
#define __DISTANCE_H__

#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include "libambit.h"

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

double deg2rad(double deg);
double distance_calc(double lat_a, double long_a, double lat_b, double long_b);

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif


#endif /* __DISTANCE_H__ */

