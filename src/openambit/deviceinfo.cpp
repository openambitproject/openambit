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
#include "deviceinfo.h"

DeviceInfo&
DeviceInfo::operator=(const ambit_device_info_t& devinfo)
{
    this->name   = QString::fromUtf8(devinfo.name);
    this->model  = QString::fromUtf8(devinfo.model);
    this->serial = QString::fromUtf8(devinfo.serial);

    memcpy(this->fw_version, devinfo.fw_version, 4);
    memcpy(this->hw_version, devinfo.hw_version, 4);

    this->access_status = devinfo.access_status;

    return *this;
}
