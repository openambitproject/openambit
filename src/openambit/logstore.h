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
#ifndef LOGSTORE_H
#define LOGSTORE_H

#include <QObject>
#include <QDateTime>
#include <QList>
#include <QIODevice>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <libambit.h>

#include "logentry.h"

class LogStore : public QObject
{
    Q_OBJECT
public:
    class LogDirEntry
    {
    public:
        QString device;
        QDateTime time;
        QString filename;
    };

    explicit LogStore(QObject *parent = 0);
    LogEntry *store(QString device, ambit_personal_settings_t *personalSettings, ambit_log_entry_t *logEntry);
    bool logExists(QString device, ambit_log_header_t *logHeader);
    LogEntry *read(QString device, QDateTime time);
    LogEntry *read(LogDirEntry dirEntry);
    LogEntry *read(QString filename);
    QList<LogDirEntry> dir(QString device = "");
signals:
    
public slots:

private:
    QString logEntryPath(QString device, QDateTime time);

    QString storagePath;

    class XMLReader
    {
    public:
        XMLReader(LogEntry *logEntry);
        bool read(QIODevice *device);

        QString errorString() const;
    private:
        void readRoot();
        void readSerial();
        void readTime();
        void readPersonalSettings();
        void readLog();
        void readLogHeader();
        void readLogSamples();
        void readPeriodicSample(QList<ambit_log_sample_periodic_value_t> *valueContent);
        QXmlStreamReader xml;
        LogEntry *logEntry;
    };

    class XMLWriter
    {
    public:
        XMLWriter(QString serial, QDateTime time, ambit_personal_settings_t *personalSettings, ambit_log_entry_t *logEntry);
        bool write(QIODevice *device);

    private:
        bool writePersonalSettings();
        bool writeLogEntry();
        bool writeLogSample(ambit_log_sample_t *sample);
        bool writePeriodicSample(ambit_log_sample_t *sample);

        QString serial;
        QDateTime time;
        QXmlStreamWriter xml;
        ambit_personal_settings_t *personalSettings;
        ambit_log_entry_t *logEntry;
    };
};

#endif // LOGSTORE_H
