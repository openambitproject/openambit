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
#include "movescountlogdirentry.h"

class MovesCountJSON : public QObject
{
    Q_OBJECT
public:
    explicit MovesCountJSON(QObject *parent = 0);

    int parseFirmwareVersionReply(QByteArray &input, u_int8_t fw_version[4]);
    int parseLogReply(QByteArray &input, QString &moveId);
    int parseLogDirReply(QByteArray &input, QList<MovesCountLogDirEntry> &entries);

    int generateLogData(LogEntry *logEntry, QByteArray &output);
    
signals:
    
public slots:

private:
    bool writePeriodicSample(ambit_log_sample_t *sample, QVariantMap &output);

    int compressData(QByteArray &content, QByteArray &output);
    QList<int> rearrangeSamples(LogEntry *logEntry);
    QString dateTimeString(QDateTime dateTime);
};

#endif // MOVESCOUNTJSON_H
