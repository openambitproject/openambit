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
#ifndef MOVESCOUNTXML_H
#define MOVESCOUNTXML_H

#include <QObject>
#include <QList>
#include <QIODevice>
#include <QXmlStreamWriter>
#include "logentry.h"

class MovesCountXML : public QObject
{
    Q_OBJECT
public:
    explicit MovesCountXML(QObject *parent = 0);
    
signals:
    
public slots:
    void writeLog(LogEntry *logEntry);

private:
    QString logEntryPath(LogEntry *logEntry);

    QString storagePath;

    class XMLWriter
    {
    public:
        XMLWriter(LogEntry *logEntry);
        bool write(QIODevice *device);

    private:
        bool writeLogEntry();
        bool writeLogSample(ambit_log_sample_t *sample, QList<quint16> *ibis);
        bool writePeriodicSample(ambit_log_sample_t *sample);

        QString dateTimeString(QDateTime &dateTime);
        QList<int> rearrangeSamples();

        LogEntry *logEntry = NULL;
        QXmlStreamWriter xml;
    };
};

#endif // MOVESCOUNTXML_H
