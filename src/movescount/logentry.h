/*
 * (C) Copyright 2013 Emil Ljungdahl
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
#ifndef LOGENTRY_H
#define LOGENTRY_H

#include <QDateTime>
#include <libambit.h>

#include "deviceinfo.h"

class LogEntry
{
public:
    explicit LogEntry();
    LogEntry(const LogEntry &other);
    ~LogEntry();

    LogEntry& operator=(const LogEntry &rhs);

    bool isUploaded();

    QString device;
    QDateTime time;
    QString movescountId;
    DeviceInfo deviceInfo;
    ambit_personal_settings_t *personalSettings = NULL;
    ambit_log_entry_t *logEntry = NULL;
};

#endif // LOGENTRY_H
