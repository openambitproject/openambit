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
#ifndef MOVESCOUNTJSON_H
#define MOVESCOUNTJSON_H

#include <QObject>
#include <QList>
#include <QVariantMap>
#include <libambit.h>

#include "logentry.h"
#include "movescount.h"
#include "movescountlogdirentry.h"
#include "movescountsettings.h"

class MovesCount;

class MovesCountJSON : public QObject
{
    Q_OBJECT
public:
    explicit MovesCountJSON(QObject *parent = 0);

    int parseFirmwareVersionReply(QByteArray &input, u_int8_t fw_version[3]);
    int parseLogReply(QByteArray &input, QString &moveId);
    int parseLogDirReply(QByteArray &input, QList<MovesCountLogDirEntry> &entries);
    int parsePersonalSettings(QByteArray &input, ambit_personal_settings_t *ps, MovesCount *movescount);
    int parseRoute(QByteArray &input, ambit_route_t *routes, ambit_personal_settings_t *ps, MovesCount *movescount);
    int parseRoutePoints(QByteArray &input, ambit_route_t *routes, ambit_personal_settings_t *ps);
    bool appendWaypoint(uint16_t count, ambit_personal_settings_t *ps, QString route_name, QString waypoint_name, int32_t lat, int32_t lon, uint16_t altitude, uint8_t type);
    int parseDeviceSettingsReply(QByteArray &input, MovescountSettings &movescountSettings);
    int parseAppRulesReply(QByteArray &input, ambit_app_rules_t* ambitApps); // QList<uint> &appRulesId, QList<QByteArray> &appRulesData);

    int generateNewPersonalSettings(ambit_personal_settings_t *settings, DeviceInfo &device_info, QByteArray &output);
    int generateLogData(LogEntry *logEntry, QByteArray &output);

signals:

public slots:

private:
    bool writePeriodicSample(ambit_log_sample_t *sample, QVariantMap &output);
    bool copyDataString(QVariant entry, char *data, size_t maxlength);
    bool appendRoutePoint(ambit_route_t *route, int point_number, int32_t lat, int32_t lon, int32_t altitude, uint32_t distance);

    int compressData(QByteArray &content, QByteArray &output);
    QList<int> rearrangeSamples(LogEntry *logEntry);
    QString dateTimeString(QDateTime dateTime);
    QDateTime dateTimeRound(QDateTime dateTime, int msecRoundFactor);
    QDateTime dateTimeCompensate(QDateTime dateTime, QDateTime prevDateTime, int minOffset);

    QVariantMap parseJsonMap(const QByteArray& input, bool& ok) const;
    QVariantList parseJsonList(const QByteArray& input, bool& ok) const;
};

#endif // MOVESCOUNTJSON_H
