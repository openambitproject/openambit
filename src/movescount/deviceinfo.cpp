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

    this->fw_version[0] = devinfo.fw_version[0];
    this->fw_version[1] = devinfo.fw_version[1];
    this->fw_version[2] = devinfo.fw_version[2] | (devinfo.fw_version[3] << 8);
    this->hw_version[0] = devinfo.hw_version[0];
    this->hw_version[1] = devinfo.hw_version[1];
    this->hw_version[2] = devinfo.hw_version[2] | (devinfo.hw_version[3] << 8);

    this->access_status = devinfo.access_status;
    this->is_supported  = devinfo.is_supported;

    return *this;
}
