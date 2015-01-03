/*
 * (C) Copyright 2014 Olaf Meeuwissen
 *
 * This file is part of Openambit.
 *
 * Openambit is free software: you can redistribute it and/or modify
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
#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include <QString>

#include <libambit.h>

struct DeviceInfo
{
    QString name;
    QString model;
    QString serial;

    int fw_version[3];
    int hw_version[3];

    int access_status;
    bool is_supported;

    DeviceInfo& operator= (const ambit_device_info_t& devinfo);
};

#endif // DEVICEINFO_H
