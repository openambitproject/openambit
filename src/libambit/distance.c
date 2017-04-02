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


#include "distance.h"

#define EARTH_RADIUS 6367

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

double deg2rad(double deg)
{
    return deg * 4.0 * atan(1.0) / 180.0;
}

double distance_calc(double lat_a, double long_a, double lat_b, double long_b)
{
    //return distance in km
    lat_a = deg2rad(lat_a);
    lat_b = deg2rad(lat_b);
    long_a = deg2rad(long_a);
    long_b = deg2rad(long_b);

    double lat_dist = lat_b - lat_a;
    double long_dist = long_b - long_a;

    double tmp = pow(sin(lat_dist/2),2) +
        cos(lat_a) * cos(lat_b) * pow(sin(long_dist/2),2);
    tmp = 2 * atan2(sqrt(tmp), sqrt(1-tmp));
    tmp = (EARTH_RADIUS * tmp);
    return tmp;
}

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

