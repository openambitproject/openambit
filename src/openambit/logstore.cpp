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
#include "logstore.h"

#include <QFile>
#include <QStringList>
#include <QDir>
#include <QRegExp>

#include <QDebug>

typedef struct sample_type_names_s {
    ambit_log_sample_type_t id;
    QString XMLName;
} sample_type_names_t;

static sample_type_names_t sampleTypeNames[] = {
    { ambit_log_sample_type_periodic, "periodic" },
    { ambit_log_sample_type_logpause, "log-pause" },
    { ambit_log_sample_type_logrestart, "log-restart" },
    { ambit_log_sample_type_ibi, "ibi" },
    { ambit_log_sample_type_ttff, "ttff" },
    { ambit_log_sample_type_distance_source, "distance-source" },
    { ambit_log_sample_type_lapinfo, "lap-info" },
    { ambit_log_sample_type_altitude_source, "altitude-source" },
    { ambit_log_sample_type_gps_base, "gps-base" },
    { ambit_log_sample_type_gps_small, "gps-small" },
    { ambit_log_sample_type_gps_tiny, "gps-tiny" },
    { ambit_log_sample_type_time, "time" },
    { ambit_log_sample_type_activity, "activity" },
    { ambit_log_sample_type_position, "position" },
    { ambit_log_sample_type_unknown, "unknown" },
    { (ambit_log_sample_type_t)0, "" }
};

typedef struct sample_distance_source_name_s {
    u_int8_t source_id;
    QString XMLName;
} sample_distance_source_name_t;

static sample_distance_source_name_t sampleDistanceSourceNames[] = {
    { 0x02, "GPS" },
    { 0x03, "Wrist" },
    { 0, "" }
};

typedef struct sample_altitude_source_name_s {
    u_int8_t source_id;
    QString XMLName;
} sample_altitude_source_name_t;

static sample_altitude_source_name_t sampleAltitudeSourceNames[] = {
    { 0x04, "Pressure" },
    { 0, "" }
};

typedef struct sample_lap_event_type_s {
    u_int8_t event_type;
    QString XMLName;
} sample_lap_event_type_t;

static sample_lap_event_type_t sampleLapEventTypeNames[] = {
    { 0x01, "Manual" },
    { 0x14, "High Interval" },
    { 0x15, "Low Interval" },
    { 0x16, "Interval" },
    { 0x1e, "Pause" },
    { 0x1f, "Start" },
    { 0, "" }
};

LogStore::LogStore(QObject *parent) :
    QObject(parent)
{
    storagePath = QString(getenv("HOME")) + "/.openambit";
}

LogEntry *LogStore::store(ambit_device_info_t *deviceInfo, ambit_personal_settings_t *personalSettings, ambit_log_entry_t *logEntry)
{
    QDateTime dateTime(QDate(logEntry->header.date_time.year, logEntry->header.date_time.month, logEntry->header.date_time.day),
                       QTime(logEntry->header.date_time.hour, logEntry->header.date_time.minute, logEntry->header.date_time.msec/1000));

    return storeInternal(QString(deviceInfo->serial), dateTime, deviceInfo, personalSettings, logEntry);
}

LogEntry *LogStore::store(LogEntry *entry)
{
    return storeInternal(entry->device, entry->time, entry->deviceInfo, entry->personalSettings, entry->logEntry, entry->movescountId);
}

void LogStore::storeMovescountId(QString device, QDateTime time, QString movescountId)
{
    LogEntry *entry, *retEntry;

    if ((entry = read(device, time)) != NULL) {
        entry->movescountId = movescountId;

        retEntry = store(entry);
        delete retEntry;
        delete entry;
    }
}

bool LogStore::logExists(QString device, ambit_log_header_t *logHeader)
{
    QDateTime dateTime(QDate(logHeader->date_time.year, logHeader->date_time.month, logHeader->date_time.day),
                       QTime(logHeader->date_time.hour, logHeader->date_time.minute, logHeader->date_time.msec/1000));

    return QFile::exists(logEntryPath(device, dateTime));
}

LogEntry *LogStore::read(QString device, QDateTime time)
{
    return readInternal(logEntryPath(device, time));
}

LogEntry *LogStore::read(LogDirEntry dirEntry)
{
    return readInternal(storagePath + "/" + dirEntry.filename);
}

LogEntry *LogStore::read(QString filename)
{
    return readInternal(storagePath + "/" + filename);
}

QList<LogStore::LogDirEntry> LogStore::dir(QString device)
{
    QList<LogDirEntry> dirList;
    QRegExp rx("log_([0-9a-zA-Z]+)_([0-9]{4})_([0-9]{2})_([0-9]{2})_([0-9]{2})_([0-9]{2})_([0-9]{2}).log");

    QStringList nameFilter;
    if (device == "") {
        nameFilter.append("log_*.log");
    }
    else {
        nameFilter.append("log_" + device + "_*.log");
    }

    QDir directory(storagePath);
    QStringList matches = directory.entryList(nameFilter, QDir::Files, QDir::Name);
    foreach (QString match, matches) {
        if (rx.indexIn(match) >= 0) {
            LogDirEntry dirEntry;
            dirEntry.filename = rx.cap(0);
            dirEntry.device = rx.cap(1);
            dirEntry.time = QDateTime(QDate(rx.cap(2).toInt(), rx.cap(3).toInt(), rx.cap(4).toInt()),
                                      QTime(rx.cap(5).toInt(), rx.cap(6).toInt(), rx.cap(7).toInt()));
            dirList.append(dirEntry);
        }
    }

    return dirList;
}

QString LogStore::logEntryPath(QString device, QDateTime time)
{
    return storagePath + "/log_" + device + "_" + time.toString("yyyy_MM_dd_hh_mm_ss") + ".log";
}

LogEntry *LogStore::storeInternal(QString serial, QDateTime dateTime, ambit_device_info_t *deviceInfo, ambit_personal_settings_t *personalSettings, ambit_log_entry_t *logEntry, QString movescountId)
{
    LogEntry *retEntry = new LogEntry();

    XMLWriter writer(deviceInfo, dateTime, movescountId, personalSettings, logEntry);
    QFile logfile(logEntryPath(serial, dateTime));
    logfile.open(QIODevice::WriteOnly);
    writer.write(&logfile);
    logfile.close();
    logfile.open(QIODevice::ReadOnly);
    XMLReader reader(retEntry);
    if (!reader.read(&logfile)) {
        delete retEntry;
        retEntry = NULL;
    }

    return retEntry;
}

LogEntry *LogStore::readInternal(QString path)
{
    LogEntry *retEntry = NULL;

    if (QFile::exists(path)) {
        retEntry = new LogEntry();
        QFile logfile(path);
        logfile.open(QIODevice::ReadOnly);
        XMLReader reader(retEntry);
        if (!reader.read(&logfile)) {
            QString error = reader.errorString();
            qDebug() << error;
            delete retEntry;
            retEntry = NULL;
        }
    }

    return retEntry;
}



LogStore::XMLReader::XMLReader(LogEntry *logEntry) : logEntry(logEntry)
{
}

bool LogStore::XMLReader::read(QIODevice *device)
{
    xml.setDevice(device);

    if (xml.readNextStartElement()) {
        if (xml.name() == "openambitlog" && xml.attributes().value("version") == "1.0") {
            readRoot();
        }
        else {
            xml.raiseError(QObject::tr("The file is not an openambit version 1.0 file."));
        }
    }

    return !xml.error();
}

QString LogStore::XMLReader::errorString() const
{
    return QObject::tr("%1\nLine %2, column %3")
            .arg(xml.errorString())
            .arg(xml.lineNumber())
            .arg(xml.columnNumber());
}

void LogStore::XMLReader::readRoot()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "openambitlog");

    while (xml.readNextStartElement()) {
        if (xml.name() == "SerialNumber") {
            readSerial();
        }
        else if (xml.name() == "Time") {
            readTime();
        }
        else if (xml.name() == "MovescountId") {
            readMovescountId();
        }
        else if (xml.name() == "DeviceInfo") {
            readDeviceInfo();
        }
        else if (xml.name() == "PersonalSettings") {
            readPersonalSettings();
        }
        else if (xml.name() == "Log") {
            readLog();
        }
        else {
            xml.skipCurrentElement();
        }
    }
}

void LogStore::XMLReader::readSerial()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "SerialNumber");

    logEntry->device = xml.readElementText();
}

void LogStore::XMLReader::readTime()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "Time");

    QString datestring = xml.readElementText();
    logEntry->time = QDateTime::fromString(datestring, Qt::ISODate);
}

void LogStore::XMLReader::readMovescountId()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "MovescountId");

    logEntry->movescountId = xml.readElementText();
}

void LogStore::XMLReader::readDeviceInfo()
{
    QRegExp versionRX("([0-9]+)\\.([0-9]+)\\.([0-9]+)");

    Q_ASSERT(xml.isStartElement() && xml.name() == "DeviceInfo");

    if (logEntry->deviceInfo == NULL) {
        logEntry->deviceInfo = (ambit_device_info_t*)malloc(sizeof(ambit_device_info_t));
        memset(logEntry->deviceInfo, 0, sizeof(ambit_device_info_t));
    }

    while (xml.readNextStartElement()) {
        if (xml.name() == "Serial") {
            strcpy(logEntry->deviceInfo->serial, xml.readElementText().toLatin1().data());
        }
        else if (xml.name() == "Model") {
            strcpy(logEntry->deviceInfo->model, xml.readElementText().toLatin1().data());
        }
        else if (xml.name() == "Name") {
            strcpy(logEntry->deviceInfo->name, xml.readElementText().toLatin1().data());
        }
        else if (xml.name() == "FWVersion") {
            if (versionRX.indexIn(xml.readElementText()) >= 0) {
                logEntry->deviceInfo->fw_version[0] = versionRX.cap(1).toInt();
                logEntry->deviceInfo->fw_version[1] = versionRX.cap(2).toInt();
                logEntry->deviceInfo->fw_version[2] = versionRX.cap(3).toInt() & 0xff;
                logEntry->deviceInfo->fw_version[3] = (versionRX.cap(3).toInt() >> 8) & 0xff;
            }
        }
        else if (xml.name() == "HWVersion") {
            if (versionRX.indexIn(xml.readElementText()) >= 0) {
                logEntry->deviceInfo->hw_version[0] = versionRX.cap(1).toInt();
                logEntry->deviceInfo->hw_version[1] = versionRX.cap(2).toInt();
                logEntry->deviceInfo->hw_version[2] = versionRX.cap(3).toInt() & 0xff;
                logEntry->deviceInfo->hw_version[3] = (versionRX.cap(3).toInt() >> 8) & 0xff;
            }
        }
        else {
            xml.skipCurrentElement();
        }
    }
}

void LogStore::XMLReader::readPersonalSettings()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "PersonalSettings");

    if (logEntry->personalSettings == NULL) {
        logEntry->personalSettings = (ambit_personal_settings_t*)malloc(sizeof(ambit_personal_settings_t));
        memset(logEntry->personalSettings, 0, sizeof(ambit_personal_settings_t));
    }

    while (xml.readNextStartElement()) {
        if (xml.name() == "SportModeButtonLock") {
            logEntry->personalSettings->sportmode_button_lock = xml.readElementText().toUInt();
        }
        else if (xml.name() == "TimeModeButtonLock") {
            logEntry->personalSettings->timemode_button_lock = xml.readElementText().toUInt();
        }
        else if (xml.name() == "CompassDeclination") {
            logEntry->personalSettings->compass_declination = xml.readElementText().toInt();
        }
        else if (xml.name() == "UnitsMode") {
            logEntry->personalSettings->units_mode = xml.readElementText().toUInt();
        }
        else if (xml.name() == "Units") {
            while (xml.readNextStartElement()) {
                if (xml.name() == "AirPressureUnit") {
                    logEntry->personalSettings->units.pressure = xml.readElementText().toUInt();
                }
                else if (xml.name() == "AltitudeUnit") {
                    logEntry->personalSettings->units.altitude = xml.readElementText().toUInt();
                }
                else if (xml.name() == "DistanceUnit") {
                    logEntry->personalSettings->units.distance = xml.readElementText().toUInt();
                }
                else if (xml.name() == "HeightUnit") {
                    logEntry->personalSettings->units.height = xml.readElementText().toUInt();
                }
                else if (xml.name() == "TemperatureUnit") {
                    logEntry->personalSettings->units.temperature = xml.readElementText().toUInt();
                }
                else if (xml.name() == "VerticalSpeedUnit") {
                    logEntry->personalSettings->units.verticalspeed = xml.readElementText().toUInt();
                }
                else if (xml.name() == "WeightUnit") {
                    logEntry->personalSettings->units.weight = xml.readElementText().toUInt();
                }
                else if (xml.name() == "CompassUnit") {
                    logEntry->personalSettings->units.compass = xml.readElementText().toUInt();
                }
                else if (xml.name() == "HRUnit") {
                    logEntry->personalSettings->units.heartrate = xml.readElementText().toUInt();
                }
                else if (xml.name() == "SpeedUnit") {
                    logEntry->personalSettings->units.speed = xml.readElementText().toUInt();
                }
                else {
                    xml.skipCurrentElement();
                }
            }
        }
        else if (xml.name() == "GPSPositionFormat") {
            logEntry->personalSettings->gps_position_format = xml.readElementText().toUInt();
        }
        else if (xml.name() == "Language") {
            logEntry->personalSettings->language = xml.readElementText().toUInt();
        }
        else if (xml.name() == "NavigationStyle") {
            logEntry->personalSettings->navigation_style = xml.readElementText().toUInt();
        }
        else if (xml.name() == "GPSTimeKeeping") {
            logEntry->personalSettings->sync_time_w_gps = xml.readElementText().toUInt();
        }
        else if (xml.name() == "Use24hClock") {
            logEntry->personalSettings->time_format = xml.readElementText().toUInt();
        }
        else if (xml.name() == "Alarm") {
            QTime time = QTime::fromString(xml.readElementText(), "hh:mm");
            logEntry->personalSettings->alarm.hour = time.hour();
            logEntry->personalSettings->alarm.minute = time.minute();
        }
        else if (xml.name() == "AlarmEnable") {
            logEntry->personalSettings->alarm_enable = xml.readElementText().toUInt();
        }
        else if (xml.name() == "DualTime") {
            QTime time = QTime::fromString(xml.readElementText(), "hh:mm");
            logEntry->personalSettings->dual_time.hour = time.hour();
            logEntry->personalSettings->dual_time.minute = time.minute();
        }
        else if (xml.name() == "DisplayDateMode") {
            logEntry->personalSettings->date_format = xml.readElementText().toUInt();
        }
        else if (xml.name() == "TonesMode") {
            logEntry->personalSettings->tones_mode = xml.readElementText().toUInt();
        }
        else if (xml.name() == "BacklightMode") {
            logEntry->personalSettings->backlight_mode = xml.readElementText().toUInt();
        }
        else if (xml.name() == "BacklightBrightness") {
            logEntry->personalSettings->backlight_brightness = xml.readElementText().toUInt();
        }
        else if (xml.name() == "DisplayBrightness") {
            logEntry->personalSettings->display_brightness = xml.readElementText().toUInt();
        }
        else if (xml.name() == "DisplayIsNegative") {
            logEntry->personalSettings->display_is_negative = xml.readElementText().toUInt();
        }
        else if (xml.name() == "Weight") {
            logEntry->personalSettings->weight = xml.readElementText().toUInt();
        }
        else if (xml.name() == "BirthYear") {
            logEntry->personalSettings->birthyear = xml.readElementText().toUInt();
        }
        else if (xml.name() == "MaxHR") {
            logEntry->personalSettings->max_hr = xml.readElementText().toUInt();
        }
        else if (xml.name() == "RestHR") {
            logEntry->personalSettings->rest_hr = xml.readElementText().toUInt();
        }
        else if (xml.name() == "FitnessLevel") {
            logEntry->personalSettings->fitness_level = xml.readElementText().toUInt();
        }
        else if (xml.name() == "IsMale") {
            logEntry->personalSettings->is_male = xml.readElementText().toUInt();
        }
        else if (xml.name() == "Length") {
            logEntry->personalSettings->length = xml.readElementText().toUInt();
        }
        else if (xml.name() == "AltiBaroMode") {
            logEntry->personalSettings->alti_baro_mode = xml.readElementText().toUInt();
        }
        else if (xml.name() == "FusedAltiDisabled") {
            logEntry->personalSettings->fused_alti_disabled = xml.readElementText().toUInt();
        }
        else if (xml.name() == "BikePODCalibration") {
            logEntry->personalSettings->bikepod_calibration = xml.readElementText().toUInt();
        }
        else if (xml.name() == "BikePODCalibration2") {
            logEntry->personalSettings->bikepod_calibration2 = xml.readElementText().toUInt();
        }
        else if (xml.name() == "BikePODCalibration3") {
            logEntry->personalSettings->bikepod_calibration3 = xml.readElementText().toUInt();
        }
        else if (xml.name() == "FootPODCalibration") {
            logEntry->personalSettings->footpod_calibration = xml.readElementText().toUInt();
        }
        else if (xml.name() == "AutomaticBikePowerCalibration") {
            logEntry->personalSettings->automatic_bikepower_calib = xml.readElementText().toUInt();
        }
        else {
            xml.skipCurrentElement();
        }
    }
}

void LogStore::XMLReader::readLog()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "Log");

    if (logEntry->logEntry == NULL) {
        logEntry->logEntry = (ambit_log_entry_t*)calloc(1, sizeof(ambit_log_entry_t));
    }

    while (xml.readNextStartElement()) {
        if (xml.name() == "Header") {
            readLogHeader();
        }
        else if (xml.name() == "Samples") {
            readLogSamples();
        }
        else {
            xml.skipCurrentElement();
        }
    }
}

void LogStore::XMLReader::readLogHeader()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "Header");

    while (xml.readNextStartElement()) {
        if (xml.name() == "DateTime") {
            QDateTime datetime = QDateTime::fromString(xml.readElementText(), Qt::ISODate);
            logEntry->logEntry->header.date_time.year = datetime.date().year();
            logEntry->logEntry->header.date_time.month = datetime.date().month();
            logEntry->logEntry->header.date_time.day = datetime.date().day();
            logEntry->logEntry->header.date_time.hour = datetime.time().hour();
            logEntry->logEntry->header.date_time.minute = datetime.time().minute();
            logEntry->logEntry->header.date_time.msec = datetime.time().second()*1000 + datetime.time().msec();
        }
        else if (xml.name() == "Duration") {
            logEntry->logEntry->header.duration = xml.readElementText().toUInt();
        }
        else if (xml.name() == "Ascent") {
            logEntry->logEntry->header.ascent = xml.readElementText().toUInt();
        }
        else if (xml.name() == "Descent") {
            logEntry->logEntry->header.descent = xml.readElementText().toUInt();
        }
        else if (xml.name() == "AscentTime") {
            logEntry->logEntry->header.ascent_time = xml.readElementText().toUInt();
        }
        else if (xml.name() == "DescentTime") {
            logEntry->logEntry->header.descent_time = xml.readElementText().toUInt();
        }
        else if (xml.name() == "RecoveryTime") {
            logEntry->logEntry->header.recovery_time = xml.readElementText().toUInt();
        }
        else if (xml.name() == "Speed") {
            while (xml.readNextStartElement()) {
                if (xml.name() == "Avg") {
                    logEntry->logEntry->header.speed_avg = xml.readElementText().toUInt();
                }
                else if (xml.name() == "Max") {
                    logEntry->logEntry->header.speed_max = xml.readElementText().toUInt();
                }
                else if (xml.name() == "MaxTime") {
                    logEntry->logEntry->header.speed_max_time = xml.readElementText().toUInt();
                }
                else {
                    xml.skipCurrentElement();
                }
            }
        }
        else if (xml.name() == "Cadence") {
            while (xml.readNextStartElement()) {
                if (xml.name() == "Avg") {
                    logEntry->logEntry->header.cadence_avg = xml.readElementText().toUInt();
                }
                else if (xml.name() == "Max") {
                    logEntry->logEntry->header.cadence_max = xml.readElementText().toUInt();
                }
                else if (xml.name() == "MaxTime") {
                    logEntry->logEntry->header.cadence_max_time = xml.readElementText().toUInt();
                }
                else {
                    xml.skipCurrentElement();
                }
            }
        }
        else if (xml.name() == "Altitude") {
            while (xml.readNextStartElement()) {
                if (xml.name() == "Max") {
                    logEntry->logEntry->header.altitude_max = xml.readElementText().toInt();
                }
                else if (xml.name() == "Min") {
                    logEntry->logEntry->header.altitude_min = xml.readElementText().toInt();
                }
                else if (xml.name() == "MaxTime") {
                    logEntry->logEntry->header.altitude_max_time = xml.readElementText().toUInt();
                }
                else if (xml.name() == "MinTime") {
                    logEntry->logEntry->header.altitude_min_time = xml.readElementText().toUInt();
                }
                else {
                    xml.skipCurrentElement();
                }
            }
        }
        else if (xml.name() == "HR") {
            while (xml.readNextStartElement()) {
                if (xml.name() == "Avg") {
                    logEntry->logEntry->header.heartrate_avg = xml.readElementText().toUInt();
                }
                else if (xml.name() == "Max") {
                    logEntry->logEntry->header.heartrate_max = xml.readElementText().toUInt();
                }
                else if (xml.name() == "Min") {
                    logEntry->logEntry->header.heartrate_min = xml.readElementText().toUInt();
                }
                else if (xml.name() == "MaxTime") {
                    logEntry->logEntry->header.heartrate_max_time = xml.readElementText().toUInt();
                }
                else if (xml.name() == "MinTime") {
                    logEntry->logEntry->header.heartrate_min_time = xml.readElementText().toUInt();
                }
                else {
                    xml.skipCurrentElement();
                }
            }
        }
        else if (xml.name() == "PeakTrainingEffect") {
            logEntry->logEntry->header.peak_training_effect = xml.readElementText().toUInt();
        }
        else if (xml.name() == "ActivityType") {
            logEntry->logEntry->header.activity_type = xml.readElementText().toUInt();
        }
        else if (xml.name() == "Activity") {
            QByteArray ba = xml.readElementText().toLocal8Bit();
            const char *c_str = ba.data();
            strncpy(logEntry->logEntry->header.activity_name, c_str, 16);
            logEntry->logEntry->header.activity_name[16] = 0;
        }
        else if (xml.name() == "Temperature") {
            while (xml.readNextStartElement()) {
                if (xml.name() == "Max") {
                    logEntry->logEntry->header.temperature_max = xml.readElementText().toInt();
                }
                else if (xml.name() == "Min") {
                    logEntry->logEntry->header.temperature_min = xml.readElementText().toInt();
                }
                else if (xml.name() == "MaxTime") {
                    logEntry->logEntry->header.temperature_max_time = xml.readElementText().toUInt();
                }
                else if (xml.name() == "MinTime") {
                    logEntry->logEntry->header.temperature_min_time = xml.readElementText().toUInt();
                }
                else {
                    xml.skipCurrentElement();
                }
            }
        }
        else if (xml.name() == "Distance") {
            logEntry->logEntry->header.distance = xml.readElementText().toUInt();
        }
        else if (xml.name() == "LogItemCount") {
            logEntry->logEntry->header.samples_count = xml.readElementText().toUInt();
        }
        else if (xml.name() == "Energy") {
            logEntry->logEntry->header.energy_consumption = xml.readElementText().toUInt();
        }
        else if (xml.name() == "TimeToFirstFix") {
            logEntry->logEntry->header.first_fix_time = xml.readElementText().toUInt();
        }
        else if (xml.name() == "BatteryChargeAtStart") {
            logEntry->logEntry->header.battery_start = xml.readElementText().toUInt();
        }
        else if (xml.name() == "BatteryCharge") {
            logEntry->logEntry->header.battery_end = xml.readElementText().toUInt();
        }
        else if (xml.name() == "DistanceBeforeCalibrationChange") {
            logEntry->logEntry->header.distance_before_calib = xml.readElementText().toUInt();
        }
        else if (xml.name() == "Unknown1") {
            QByteArray val = xml.readElementText().toLocal8Bit();
            const char *c_str = val.data();
            for (int i=0; i<5 && i<val.length()/2; i++) {
                sscanf(c_str, "%2hhx", &logEntry->logEntry->header.unknown1[i]);
                c_str += 2 * sizeof(char);
            }
        }
        else if (xml.name() == "Unknown2") {
            QByteArray val = xml.readElementText().toLocal8Bit();
            const char *c_str = val.data();
            sscanf(c_str, "%2hhx", &logEntry->logEntry->header.unknown2);
        }
        else if (xml.name() == "Unknown3") {
            QByteArray val = xml.readElementText().toLocal8Bit();
            const char *c_str = val.data();
            // Handle previously unknown cadence data
            if (val.length() == 12) {
                sscanf(c_str, "%2hhx", &logEntry->logEntry->header.cadence_max);
                c_str += 2 * sizeof(char);
                sscanf(c_str, "%2hhx", &logEntry->logEntry->header.cadence_avg);
                c_str += 2 * sizeof(char);
            }
            for (int i=0; i<4 && i<val.length()/2; i++) {
                sscanf(c_str, "%2hhx", &logEntry->logEntry->header.unknown3[i]);
                c_str += 2 * sizeof(char);
            }
        }
        else if (xml.name() == "Unknown4") {
            QByteArray val = xml.readElementText().toLocal8Bit();
            const char *c_str = val.data();
            // Handle previously unknown cadence data
            if (val.length() == 16) {
                uint32_t cadence_max_time = 0;
                uint8_t tmpval;
                for (int i=0; i<4; i++) {
                    sscanf(c_str, "%2hhx", &tmpval);
                    cadence_max_time |= (tmpval << (8*(3-i)));
                    c_str += 2 * sizeof(char);
                }
                logEntry->logEntry->header.cadence_max_time = cadence_max_time;
            }
            for (int i=0; i<4 && i<val.length()/2; i++) {
                sscanf(c_str, "%2hhx", &logEntry->logEntry->header.unknown4[i]);
                c_str += 2 * sizeof(char);
            }
        }
        else if (xml.name() == "Unknown5") {
            QByteArray val = xml.readElementText().toLocal8Bit();
            const char *c_str = val.data();
            for (int i=0; i<4 && i<val.length()/2; i++) {
                sscanf(c_str, "%2hhx", &logEntry->logEntry->header.unknown5[i]);
                c_str += 2 * sizeof(char);
            }
        }
        else if (xml.name() == "Unknown6") {
            QByteArray val = xml.readElementText().toLocal8Bit();
            const char *c_str = val.data();
            for (int i=0; i<24 && i<val.length()/2; i++) {
                sscanf(c_str, "%2hhx", &logEntry->logEntry->header.unknown6[i]);
                c_str += 2 * sizeof(char);
            }
        }
        else {
            xml.skipCurrentElement();
        }
    }
}

void LogStore::XMLReader::readLogSamples()
{
    int sampleCount = 0;
    ambit_log_sample_type_t type;

    Q_ASSERT(xml.isStartElement() && xml.name() == "Samples");

    if (logEntry->logEntry->samples == NULL) {
        logEntry->logEntry->samples = (ambit_log_sample_t *)calloc(logEntry->logEntry->header.samples_count, sizeof(ambit_log_sample_t));
        logEntry->logEntry->samples_count = logEntry->logEntry->header.samples_count;
    }

    while (xml.readNextStartElement()) {
        if (xml.name() == "Sample") {
            QList<ambit_log_sample_periodic_value_t> periodicValues;
            int ibiCount = 0;
            type = ambit_log_sample_type_unknown;
            while (xml.readNextStartElement()) {
                if (xml.name() == "Type") {
                    type = (ambit_log_sample_type_t)xml.attributes().value("id").toString().toUInt();
                    logEntry->logEntry->samples[sampleCount].type = type;
                    xml.skipCurrentElement();
                }
                else if (xml.name() == "UTC") {
                    QDateTime datetime = QDateTime::fromString(xml.readElementText(), "yyyy-MM-ddThh:mm:ss.zzzZ");
                    logEntry->logEntry->samples[sampleCount].utc_time.year = datetime.date().year();
                    logEntry->logEntry->samples[sampleCount].utc_time.month = datetime.date().month();
                    logEntry->logEntry->samples[sampleCount].utc_time.day = datetime.date().day();
                    logEntry->logEntry->samples[sampleCount].utc_time.hour = datetime.time().hour();
                    logEntry->logEntry->samples[sampleCount].utc_time.minute = datetime.time().minute();
                    logEntry->logEntry->samples[sampleCount].utc_time.msec = datetime.time().second()*1000 + datetime.time().msec();
                }
                else if (xml.name() == "Time") {
                    logEntry->logEntry->samples[sampleCount].time = xml.readElementText().toUInt();
                }
                else {
                    switch(type) {
                    case ambit_log_sample_type_periodic:
                        readPeriodicSample(&periodicValues);
                        break;
                    case ambit_log_sample_type_logpause:
                        /* Should not get here! */
                        xml.skipCurrentElement();
                        break;
                    case ambit_log_sample_type_logrestart:
                        /* Should not get here! */
                        xml.skipCurrentElement();
                        break;
                    case ambit_log_sample_type_ibi:
                        if (xml.name() == "IBI") {
                            logEntry->logEntry->samples[sampleCount].u.ibi.ibi[ibiCount] = xml.readElementText().toUInt();
                            ibiCount++;
                            logEntry->logEntry->samples[sampleCount].u.ibi.ibi_count = ibiCount;
                        }
                        else {
                            /* Should not get here! */
                            xml.skipCurrentElement();
                        }
                        break;
                    case ambit_log_sample_type_ttff:
                        logEntry->logEntry->samples[sampleCount].u.ttff = xml.readElementText().toUInt();
                        break;
                    case ambit_log_sample_type_distance_source:
                        if (xml.name() == "DistanceSource") {
                            int distanceId = xml.attributes().value("id").toString().toUInt();
                            logEntry->logEntry->samples[sampleCount].u.distance_source = distanceId;
                            xml.skipCurrentElement();
                        }
                        else {
                            /* Should not get here! */
                            xml.skipCurrentElement();
                        }
                        break;
                    case ambit_log_sample_type_lapinfo:
                        if (xml.name() == "Lap") {
                            while(xml.readNextStartElement()) {
                                if (xml.name() == "Type") {
                                    int lapTypeId = xml.attributes().value("id").toString().toUInt();
                                    logEntry->logEntry->samples[sampleCount].u.lapinfo.event_type = lapTypeId;
                                    xml.skipCurrentElement();
                                }
                                else if (xml.name() == "DateTime") {
                                    QDateTime datetime = QDateTime::fromString(xml.readElementText(), Qt::ISODate);
                                    logEntry->logEntry->samples[sampleCount].u.lapinfo.date_time.year = datetime.date().year();
                                    logEntry->logEntry->samples[sampleCount].u.lapinfo.date_time.month = datetime.date().month();
                                    logEntry->logEntry->samples[sampleCount].u.lapinfo.date_time.day = datetime.date().day();
                                    logEntry->logEntry->samples[sampleCount].u.lapinfo.date_time.hour = datetime.time().hour();
                                    logEntry->logEntry->samples[sampleCount].u.lapinfo.date_time.minute = datetime.time().minute();
                                    logEntry->logEntry->samples[sampleCount].u.lapinfo.date_time.msec = datetime.time().second()*1000 + datetime.time().msec();
                                }
                                else if (xml.name() == "Duration") {
                                    logEntry->logEntry->samples[sampleCount].u.lapinfo.duration = xml.readElementText().toUInt();
                                }
                                else if (xml.name() == "Distance") {
                                    logEntry->logEntry->samples[sampleCount].u.lapinfo.distance = xml.readElementText().toUInt();
                                }
                                else {
                                    /* Should not get here! */
                                    xml.skipCurrentElement();
                                }
                            }
                        }
                        else {
                            /* Should not get here! */
                            xml.skipCurrentElement();
                        }
                        break;
                    case ambit_log_sample_type_altitude_source:
                        if (xml.name() == "AltitudeSource") {
                            int altitudeSourceId = xml.attributes().value("id").toString().toUInt();
                            logEntry->logEntry->samples[sampleCount].u.altitude_source.source_type = altitudeSourceId;
                            xml.skipCurrentElement();
                        }
                        else if (xml.name() == "AltitudeOffset") {
                            logEntry->logEntry->samples[sampleCount].u.altitude_source.altitude_offset = xml.readElementText().toInt();
                        }
                        else if (xml.name() == "PressureOffset") {
                            logEntry->logEntry->samples[sampleCount].u.altitude_source.pressure_offset = xml.readElementText().toInt();
                        }
                        else {
                            /* Should not get here! */
                            xml.skipCurrentElement();
                        }
                        break;
                    case ambit_log_sample_type_gps_base:
                        if (xml.name() == "NavValid") {
                            logEntry->logEntry->samples[sampleCount].u.gps_base.navvalid = xml.readElementText().toUInt();
                        }
                        else if (xml.name() == "NavType") {
                            logEntry->logEntry->samples[sampleCount].u.gps_base.navtype = xml.readElementText().toUInt();
                        }
                        else if (xml.name() == "UTCReference") {
                            QDateTime datetime = QDateTime::fromString(xml.readElementText(), "yyyy-MM-ddThh:mm:ss.zzzZ");
                            logEntry->logEntry->samples[sampleCount].u.gps_base.utc_base_time.year = datetime.date().year();
                            logEntry->logEntry->samples[sampleCount].u.gps_base.utc_base_time.month = datetime.date().month();
                            logEntry->logEntry->samples[sampleCount].u.gps_base.utc_base_time.day = datetime.date().day();
                            logEntry->logEntry->samples[sampleCount].u.gps_base.utc_base_time.hour = datetime.time().hour();
                            logEntry->logEntry->samples[sampleCount].u.gps_base.utc_base_time.minute = datetime.time().minute();
                            logEntry->logEntry->samples[sampleCount].u.gps_base.utc_base_time.msec = datetime.time().second()*1000 + datetime.time().msec();
                        }
                        else if (xml.name() == "Latitude") {
                            logEntry->logEntry->samples[sampleCount].u.gps_base.latitude = xml.readElementText().toInt();
                        }
                        else if (xml.name() == "Longitude") {
                            logEntry->logEntry->samples[sampleCount].u.gps_base.longitude = xml.readElementText().toInt();
                        }
                        else if (xml.name() == "GPSAltitude") {
                            logEntry->logEntry->samples[sampleCount].u.gps_base.altitude = xml.readElementText().toInt();
                        }
                        else if (xml.name() == "GPSSpeed") {
                            logEntry->logEntry->samples[sampleCount].u.gps_base.speed = xml.readElementText().toUInt();
                        }
                        else if (xml.name() == "GPSHeading") {
                            logEntry->logEntry->samples[sampleCount].u.gps_base.heading = xml.readElementText().toInt();
                        }
                        else if (xml.name() == "EHPE") {
                            logEntry->logEntry->samples[sampleCount].u.gps_base.ehpe = xml.readElementText().toUInt();
                        }
                        else if (xml.name() == "NumberOfSatellites") {
                            logEntry->logEntry->samples[sampleCount].u.gps_base.noofsatellites = xml.readElementText().toUInt();
                        }
                        else if (xml.name() == "GpsHDOP") {
                            logEntry->logEntry->samples[sampleCount].u.gps_base.hdop = xml.readElementText().toUInt();
                        }
                        else if (xml.name() == "Satellites") {
                            QList<ambit_log_gps_satellite_t> satellites;
                            while(xml.readNextStartElement()) {
                                if (xml.name() == "Satellite") {
                                    ambit_log_gps_satellite_t satellite;
                                    while(xml.readNextStartElement()) {
                                        if (xml.name() == "SV") {
                                            satellite.sv = xml.readElementText().toUInt();
                                        }
                                        else if (xml.name() == "SNR") {
                                            satellite.snr = xml.readElementText().toUInt();
                                        }
                                        else if (xml.name() == "State") {
                                            satellite.state = xml.readElementText().toUInt();
                                        }
                                        else {
                                            /* Should not get here! */
                                            xml.skipCurrentElement();
                                        }
                                    }
                                    satellites.append(satellite);
                                }
                                else {
                                    /* Should not get here! */
                                    xml.skipCurrentElement();
                                }
                            }
                            if (satellites.count() > 0) {
                                logEntry->logEntry->samples[sampleCount].u.gps_base.satellites_count = satellites.count();
                                logEntry->logEntry->samples[sampleCount].u.gps_base.satellites = (ambit_log_gps_satellite_t*)calloc(satellites.count(), sizeof(ambit_log_gps_satellite_t));
                                for (int i=0; i<satellites.count(); i++) {
                                    logEntry->logEntry->samples[sampleCount].u.gps_base.satellites[i] = satellites.at(i);
                                }
                            }
                        }
                        else {
                            /* Should not get here! */
                            xml.skipCurrentElement();
                        }
                        break;
                    case ambit_log_sample_type_gps_small:
                        if (xml.name() == "Latitude") {
                            logEntry->logEntry->samples[sampleCount].u.gps_small.latitude = xml.readElementText().toInt();
                        }
                        else if (xml.name() == "Longitude") {
                            logEntry->logEntry->samples[sampleCount].u.gps_small.longitude = xml.readElementText().toInt();
                        }
                        else if (xml.name() == "EHPE") {
                            logEntry->logEntry->samples[sampleCount].u.gps_small.ehpe = xml.readElementText().toUInt();
                        }
                        else if (xml.name() == "NumberOfSatellites") {
                            logEntry->logEntry->samples[sampleCount].u.gps_small.noofsatellites = xml.readElementText().toUInt();
                        }
                        else {
                            /* Should not get here! */
                            xml.skipCurrentElement();
                        }
                        break;
                    case ambit_log_sample_type_gps_tiny:
                        if (xml.name() == "Latitude") {
                            logEntry->logEntry->samples[sampleCount].u.gps_tiny.latitude = xml.readElementText().toInt();
                        }
                        else if (xml.name() == "Longitude") {
                            logEntry->logEntry->samples[sampleCount].u.gps_tiny.longitude = xml.readElementText().toInt();
                        }
                        else if (xml.name() == "EHPE") {
                            logEntry->logEntry->samples[sampleCount].u.gps_tiny.ehpe = xml.readElementText().toUInt();
                        }
                        else if (xml.name() == "Unknown") {
                            logEntry->logEntry->samples[sampleCount].u.gps_tiny.unknown = xml.readElementText().toUInt();
                        }
                        else {
                            /* Should not get here! */
                            xml.skipCurrentElement();
                        }
                        break;
                    case ambit_log_sample_type_time:
                        if (xml.name() == "TimeRef") {
                            QTime timeref = QTime::fromString(xml.readElementText(), Qt::ISODate);
                            logEntry->logEntry->samples[sampleCount].u.time.hour = timeref.hour();
                            logEntry->logEntry->samples[sampleCount].u.time.minute = timeref.minute();
                            logEntry->logEntry->samples[sampleCount].u.time.second = timeref.second();
                        }
                        else {
                            /* Should not get here! */
                            xml.skipCurrentElement();
                        }
                        break;
                    case ambit_log_sample_type_activity:
                        if (xml.name() == "ActivityType") {
                            logEntry->logEntry->samples[sampleCount].u.activity.activitytype = xml.readElementText().toUInt();
                        }
                        else if (xml.name() == "CustomModeId") {
                            logEntry->logEntry->samples[sampleCount].u.activity.custommode = xml.readElementText().toUInt();
                        }
                        else {
                            /* Should not get here! */
                            xml.skipCurrentElement();
                        }
                        break;
                    case ambit_log_sample_type_position:
                        if (xml.name() == "Latitude") {
                            logEntry->logEntry->samples[sampleCount].u.position.latitude = xml.readElementText().toInt();
                        }
                        else if (xml.name() == "Longitude") {
                            logEntry->logEntry->samples[sampleCount].u.position.longitude = xml.readElementText().toInt();
                        }
                        else {
                            /* Should not get here! */
                            xml.skipCurrentElement();
                        }
                        break;
                    case ambit_log_sample_type_unknown:
                        if (xml.name() == "Data") {
                            QByteArray val = xml.readElementText().toLocal8Bit();
                            const char *c_str = val.data();
                            if (val.length() >= 2) {
                                logEntry->logEntry->samples[sampleCount].u.unknown.data = (uint8_t*)malloc(val.length()/2);
                                for (int i=0; i<val.length()/2; i++) {
                                    sscanf(c_str, "%2hhx", &logEntry->logEntry->samples[sampleCount].u.unknown.data[i]);
                                    c_str += 2 * sizeof(char);
                                }
                            }
                        }
                        else {
                            /* Should not get here! */
                            xml.skipCurrentElement();
                        }
                        break;
                    default:
                        xml.skipCurrentElement();
                    }
                }
            }
            if (type == ambit_log_sample_type_periodic && periodicValues.count() > 0) {
                logEntry->logEntry->samples[sampleCount].u.periodic.value_count = periodicValues.count();
                logEntry->logEntry->samples[sampleCount].u.periodic.values = (ambit_log_sample_periodic_value_t*)calloc(periodicValues.count(), sizeof(ambit_log_sample_periodic_value_t));
                for (int i=0; i<periodicValues.count(); i++) {
                    logEntry->logEntry->samples[sampleCount].u.periodic.values[i] = periodicValues.at(i);
                }
            }
            sampleCount++;
        }
        else {
            xml.skipCurrentElement();
        }
    }
}

void LogStore::XMLReader::readPeriodicSample(QList<ambit_log_sample_periodic_value_t> *valueContent)
{
    ambit_log_sample_periodic_value_t value;

    if (xml.name() == "Latitude") {
        value.type = ambit_log_sample_periodic_type_latitude;
        value.u.latitude = xml.readElementText().toInt();
        valueContent->append(value);
    }
    else if (xml.name() == "Longitude") {
        value.type = ambit_log_sample_periodic_type_longitude;
        value.u.latitude = xml.readElementText().toInt();
        valueContent->append(value);
    }
    else if (xml.name() == "Distance") {
        value.type = ambit_log_sample_periodic_type_distance;
        value.u.latitude = xml.readElementText().toUInt();
        valueContent->append(value);
    }
    else if (xml.name() == "Speed") {
        value.type = ambit_log_sample_periodic_type_speed;
        value.u.latitude = xml.readElementText().toUInt();
        valueContent->append(value);
    }
    else if (xml.name() == "HR") {
        value.type = ambit_log_sample_periodic_type_hr;
        value.u.latitude = xml.readElementText().toUInt();
        valueContent->append(value);
    }
    else if (xml.name() == "Time") {
        value.type = ambit_log_sample_periodic_type_time;
        value.u.latitude = xml.readElementText().toUInt();
        valueContent->append(value);
    }
    else if (xml.name() == "GPSSpeed") {
        value.type = ambit_log_sample_periodic_type_gpsspeed;
        value.u.latitude = xml.readElementText().toUInt();
        valueContent->append(value);
    }
    else if (xml.name() == "WristAccSpeed") {
        value.type = ambit_log_sample_periodic_type_wristaccspeed;
        value.u.latitude = xml.readElementText().toUInt();
        valueContent->append(value);
    }
    else if (xml.name() == "BikePodSpeed") {
        value.type = ambit_log_sample_periodic_type_bikepodspeed;
        value.u.latitude = xml.readElementText().toUInt();
        valueContent->append(value);
    }
    else if (xml.name() == "EHPE") {
        value.type = ambit_log_sample_periodic_type_ehpe;
        value.u.latitude = xml.readElementText().toUInt();
        valueContent->append(value);
    }
    else if (xml.name() == "EVPE") {
        value.type = ambit_log_sample_periodic_type_evpe;
        value.u.latitude = xml.readElementText().toUInt();
        valueContent->append(value);
    }
    else if (xml.name() == "Altitude") {
        value.type = ambit_log_sample_periodic_type_altitude;
        value.u.latitude = xml.readElementText().toInt();
        valueContent->append(value);
    }
    else if (xml.name() == "AbsPressure") {
        value.type = ambit_log_sample_periodic_type_abspressure;
        value.u.latitude = xml.readElementText().toUInt();
        valueContent->append(value);
    }
    else if (xml.name() == "EnergyConsumption") {
        value.type = ambit_log_sample_periodic_type_energy;
        value.u.latitude = xml.readElementText().toUInt();
        valueContent->append(value);
    }
    else if (xml.name() == "Temperature") {
        value.type = ambit_log_sample_periodic_type_temperature;
        value.u.latitude = xml.readElementText().toInt();
        valueContent->append(value);
    }
    else if (xml.name() == "BatteryCharge") {
        value.type = ambit_log_sample_periodic_type_charge;
        value.u.latitude = xml.readElementText().toUInt();
        valueContent->append(value);
    }
    else if (xml.name() == "GPSAltitude") {
        value.type = ambit_log_sample_periodic_type_gpsaltitude;
        value.u.latitude = xml.readElementText().toInt();
        valueContent->append(value);
    }
    else if (xml.name() == "GPSHeading") {
        value.type = ambit_log_sample_periodic_type_gpsheading;
        value.u.latitude = xml.readElementText().toInt();
        valueContent->append(value);
    }
    else if (xml.name() == "GpsHDOP") {
        value.type = ambit_log_sample_periodic_type_gpshdop;
        value.u.latitude = xml.readElementText().toUInt();
        valueContent->append(value);
    }
    else if (xml.name() == "GpsVDOP") {
        value.type = ambit_log_sample_periodic_type_gpsvdop;
        value.u.latitude = xml.readElementText().toUInt();
        valueContent->append(value);
    }
    else if (xml.name() == "WristCadence") {
        value.type = ambit_log_sample_periodic_type_wristcadence;
        value.u.latitude = xml.readElementText().toUInt();
        valueContent->append(value);
    }
    else if (xml.name() == "SNR") {
        value.type = ambit_log_sample_periodic_type_snr;
        QByteArray val = xml.readElementText().toLocal8Bit();
        const char *c_str = val.data();
        for (int i=0; i<16 && i<val.length()/2; i++) {
            sscanf(c_str, "%2hhx", &value.u.snr[i]);
            c_str += 2 * sizeof(char);
        }
        valueContent->append(value);
    }
    else if (xml.name() == "NumberOfSatellites") {
        value.type = ambit_log_sample_periodic_type_noofsatellites;
        value.u.latitude = xml.readElementText().toUInt();
        valueContent->append(value);
    }
    else if (xml.name() == "SeaLevelPressure") {
        value.type = ambit_log_sample_periodic_type_sealevelpressure;
        value.u.latitude = xml.readElementText().toUInt();
        valueContent->append(value);
    }
    else if (xml.name() == "VerticalSpeed") {
        value.type = ambit_log_sample_periodic_type_verticalspeed;
        value.u.latitude = xml.readElementText().toInt();
        valueContent->append(value);
    }
    else if (xml.name() == "Cadence") {
        value.type = ambit_log_sample_periodic_type_cadence;
        value.u.latitude = xml.readElementText().toUInt();
        valueContent->append(value);
    }
    else if (xml.name() == "BikePower") {
        value.type = ambit_log_sample_periodic_type_bikepower;
        value.u.latitude = xml.readElementText().toUInt();
        valueContent->append(value);
    }
    else if (xml.name() == "SwimmingStrokeCount") {
        value.type = ambit_log_sample_periodic_type_swimingstrokecnt;
        value.u.latitude = xml.readElementText().toUInt();
        valueContent->append(value);
    }
    else if (xml.name() == "RuleOutput1") {
        value.type = ambit_log_sample_periodic_type_ruleoutput1;
        value.u.latitude = xml.readElementText().toInt();
        valueContent->append(value);
    }
    else if (xml.name() == "RuleOutput2") {
        value.type = ambit_log_sample_periodic_type_ruleoutput2;
        value.u.latitude = xml.readElementText().toInt();
        valueContent->append(value);
    }
    else if (xml.name() == "RuleOutput3") {
        value.type = ambit_log_sample_periodic_type_ruleoutput3;
        value.u.latitude = xml.readElementText().toInt();
        valueContent->append(value);
    }
    else if (xml.name() == "RuleOutput4") {
        value.type = ambit_log_sample_periodic_type_ruleoutput4;
        value.u.latitude = xml.readElementText().toInt();
        valueContent->append(value);
    }
    else if (xml.name() == "RuleOutput5") {
        value.type = ambit_log_sample_periodic_type_ruleoutput5;
        value.u.latitude = xml.readElementText().toInt();
        valueContent->append(value);
    }
    else {
        xml.skipCurrentElement();
    }
}


LogStore::XMLWriter::XMLWriter(ambit_device_info_t *deviceInfo, QDateTime time, QString movescountId, ambit_personal_settings_t *personalSettings, ambit_log_entry_t *logEntry) :
    deviceInfo(deviceInfo), time(time), movescountId(movescountId), personalSettings(personalSettings), logEntry(logEntry)
{
    xml.setAutoFormatting(true);
}

bool LogStore::XMLWriter::write(QIODevice *device)
{
    bool ret = true;

    xml.setDevice(device);

    xml.writeStartDocument();
    xml.writeDTD("<!DOCTYPE openambitlog>");
    xml.writeStartElement("openambitlog");
    xml.writeAttribute("version", "1.0");

    xml.writeTextElement("SerialNumber", QString("%1").arg(deviceInfo->serial));
    xml.writeTextElement("Time", time.toString(Qt::ISODate));
    xml.writeTextElement("MovescountId", QString("%1").arg(movescountId));
    ret = writeDeviceInfo();
    ret = writePersonalSettings();
    if (ret) {
        ret = writeLogEntry();
    }

    xml.writeEndDocument();

    return ret;
}

bool LogStore::XMLWriter::writeDeviceInfo()
{
    xml.writeStartElement("DeviceInfo");
    xml.writeTextElement("Serial", QString("%1").arg(deviceInfo->serial));
    xml.writeTextElement("Model", QString("%1").arg(deviceInfo->model));
    xml.writeTextElement("Name", QString("%1").arg(deviceInfo->name));
    xml.writeTextElement("FWVersion", QString("%1.%2.%3").arg((int)deviceInfo->fw_version[0]).arg((int)deviceInfo->fw_version[1]).arg((int)deviceInfo->fw_version[2] | ((int)deviceInfo->fw_version[3] << 8)));
    xml.writeTextElement("HWVersion", QString("%1.%2.%3").arg((int)deviceInfo->hw_version[0]).arg((int)deviceInfo->hw_version[1]).arg((int)deviceInfo->hw_version[2] | ((int)deviceInfo->hw_version[3] << 8)));
    xml.writeEndElement();

    return true;
}

bool LogStore::XMLWriter::writePersonalSettings()
{
    xml.writeStartElement("PersonalSettings");
    xml.writeTextElement("SportModeButtonLock", QString("%1").arg(personalSettings->sportmode_button_lock));
    xml.writeTextElement("TimeModeButtonLock", QString("%1").arg(personalSettings->timemode_button_lock));
    xml.writeTextElement("CompassDeclination", QString("%1").arg(personalSettings->compass_declination));
    xml.writeTextElement("UnitsMode", QString("%1").arg(personalSettings->units_mode));
    xml.writeStartElement("Units");
        xml.writeTextElement("AirPressureUnit", QString("%1").arg(personalSettings->units.pressure));
        xml.writeTextElement("AltitudeUnit", QString("%1").arg(personalSettings->units.altitude));
        xml.writeTextElement("DistanceUnit", QString("%1").arg(personalSettings->units.distance));
        xml.writeTextElement("HeightUnit", QString("%1").arg(personalSettings->units.height));
        xml.writeTextElement("TemperatureUnit", QString("%1").arg(personalSettings->units.temperature));
        xml.writeTextElement("VerticalSpeedUnit", QString("%1").arg(personalSettings->units.verticalspeed));
        xml.writeTextElement("WeightUnit", QString("%1").arg(personalSettings->units.weight));
        xml.writeTextElement("CompassUnit", QString("%1").arg(personalSettings->units.compass));
        xml.writeTextElement("HRUnit", QString("%1").arg(personalSettings->units.heartrate));
        xml.writeTextElement("SpeedUnit", QString("%1").arg(personalSettings->units.speed));
    xml.writeEndElement();
    xml.writeTextElement("GPSPositionFormat", QString("%1").arg(personalSettings->gps_position_format));
    xml.writeTextElement("Language", QString("%1").arg(personalSettings->language));
    xml.writeTextElement("NavigationStyle", QString("%1").arg(personalSettings->navigation_style));
    xml.writeTextElement("GPSTimeKeeping", QString("%1").arg(personalSettings->sync_time_w_gps));
    xml.writeTextElement("Use24hClock", QString("%1").arg(personalSettings->time_format));
    QString timeFormat;
    timeFormat.sprintf("%02u:%02u", personalSettings->alarm.hour, personalSettings->alarm.minute);
    xml.writeTextElement("Alarm", timeFormat.sprintf("%02u:%02u", personalSettings->alarm.hour, personalSettings->alarm.minute));
    xml.writeTextElement("AlarmEnable", QString("%1").arg(personalSettings->alarm_enable));
    xml.writeTextElement("DualTime", timeFormat.sprintf("%02u:%02u", personalSettings->dual_time.hour, personalSettings->dual_time.minute));
    xml.writeTextElement("DisplayDateMode", QString("%1").arg(personalSettings->date_format));
    xml.writeTextElement("TonesMode", QString("%1").arg(personalSettings->tones_mode));
    xml.writeTextElement("BacklightMode", QString("%1").arg(personalSettings->backlight_mode));
    xml.writeTextElement("BacklightBrightness", QString("%1").arg(personalSettings->backlight_brightness));
    xml.writeTextElement("DisplayBrightness", QString("%1").arg(personalSettings->display_brightness));
    xml.writeTextElement("DisplayIsNegative", QString("%1").arg(personalSettings->display_is_negative));
    xml.writeTextElement("Weight", QString("%1").arg(personalSettings->weight));
    xml.writeTextElement("BirthYear", QString("%1").arg(personalSettings->birthyear));
    xml.writeTextElement("MaxHR", QString("%1").arg(personalSettings->max_hr));
    xml.writeTextElement("RestHR", QString("%1").arg(personalSettings->rest_hr));
    xml.writeTextElement("FitnessLevel", QString("%1").arg(personalSettings->fitness_level));
    xml.writeTextElement("IsMale", QString("%1").arg(personalSettings->is_male));
    xml.writeTextElement("Length", QString("%1").arg(personalSettings->length));
    xml.writeTextElement("AltiBaroMode", QString("%1").arg(personalSettings->alti_baro_mode));
    xml.writeTextElement("FusedAltiDisabled", QString("%1").arg(personalSettings->fused_alti_disabled));
    xml.writeTextElement("BikePODCalibration", QString("%1").arg(personalSettings->bikepod_calibration));
    xml.writeTextElement("BikePODCalibration2", QString("%1").arg(personalSettings->bikepod_calibration2));
    xml.writeTextElement("BikePODCalibration3", QString("%1").arg(personalSettings->bikepod_calibration3));
    xml.writeTextElement("FootPODCalibration", QString("%1").arg(personalSettings->footpod_calibration));
    xml.writeTextElement("AutomaticBikePowerCalibration", QString("%1").arg(personalSettings->automatic_bikepower_calib));

    xml.writeEndElement();

    return true;
}

bool LogStore::XMLWriter::writeLogEntry()
{
    u_int32_t i;

    xml.writeStartElement("Log");
    xml.writeStartElement("Header");
    QDateTime dateTime(QDate(logEntry->header.date_time.year, logEntry->header.date_time.month, logEntry->header.date_time.day), QTime(logEntry->header.date_time.hour, logEntry->header.date_time.minute, 0).addMSecs(logEntry->header.date_time.msec));
    xml.writeTextElement("DateTime", dateTime.toString(Qt::ISODate));
    xml.writeTextElement("Duration", QString("%1").arg(logEntry->header.duration));
    xml.writeTextElement("Ascent", QString("%1").arg(logEntry->header.ascent));
    xml.writeTextElement("Descent", QString("%1").arg(logEntry->header.descent));
    xml.writeTextElement("AscentTime", QString("%1").arg(logEntry->header.ascent_time));
    xml.writeTextElement("DescentTime", QString("%1").arg(logEntry->header.descent_time));
    xml.writeTextElement("RecoveryTime", QString("%1").arg(logEntry->header.recovery_time));
    xml.writeStartElement("Speed");
    xml.writeTextElement("Avg", QString("%1").arg(logEntry->header.speed_avg));
    xml.writeTextElement("Max", QString("%1").arg(logEntry->header.speed_max));
    xml.writeTextElement("MaxTime", QString("%1").arg(logEntry->header.speed_max_time));
    xml.writeEndElement();
    xml.writeStartElement("Cadence");
    xml.writeTextElement("Avg", QString("%1").arg(logEntry->header.cadence_avg));
    xml.writeTextElement("Max", QString("%1").arg(logEntry->header.cadence_max));
    xml.writeTextElement("MaxTime", QString("%1").arg(logEntry->header.cadence_max_time));
    xml.writeEndElement();
    xml.writeStartElement("Altitude");
    xml.writeTextElement("Max", QString("%1").arg(logEntry->header.altitude_max));
    xml.writeTextElement("Min", QString("%1").arg(logEntry->header.altitude_min));
    xml.writeTextElement("MaxTime", QString("%1").arg(logEntry->header.altitude_max_time));
    xml.writeTextElement("MinTime", QString("%1").arg(logEntry->header.altitude_min_time));
    xml.writeEndElement();
    xml.writeStartElement("HR");
    xml.writeTextElement("Avg", QString("%1").arg(logEntry->header.heartrate_avg));
    xml.writeTextElement("Max", QString("%1").arg(logEntry->header.heartrate_max));
    xml.writeTextElement("Min", QString("%1").arg(logEntry->header.heartrate_min));
    xml.writeTextElement("MaxTime", QString("%1").arg(logEntry->header.heartrate_max_time));
    xml.writeTextElement("MinTime", QString("%1").arg(logEntry->header.heartrate_min_time));
    xml.writeEndElement();
    xml.writeTextElement("PeakTrainingEffect", QString("%1").arg(logEntry->header.peak_training_effect));
    xml.writeTextElement("ActivityType", QString("%1").arg(logEntry->header.activity_type));
    xml.writeTextElement("Activity", QString(logEntry->header.activity_name));
    xml.writeStartElement("Temperature");
    xml.writeTextElement("Max", QString("%1").arg(logEntry->header.temperature_max));
    xml.writeTextElement("Min", QString("%1").arg(logEntry->header.temperature_min));
    xml.writeTextElement("MaxTime", QString("%1").arg(logEntry->header.temperature_max_time));
    xml.writeTextElement("MinTime", QString("%1").arg(logEntry->header.temperature_min_time));
    xml.writeEndElement();
    xml.writeTextElement("Distance", QString("%1").arg(logEntry->header.distance));
    xml.writeTextElement("LogItemCount", QString("%1").arg(logEntry->header.samples_count));
    xml.writeTextElement("Energy", QString("%1").arg(logEntry->header.energy_consumption));
    xml.writeTextElement("TimeToFirstFix", QString("%1").arg(logEntry->header.first_fix_time));
    xml.writeTextElement("BatteryChargeAtStart", QString("%1").arg(logEntry->header.battery_start));
    xml.writeTextElement("BatteryCharge", QString("%1").arg(logEntry->header.battery_end));
    xml.writeTextElement("DistanceBeforeCalibrationChange", QString("%1").arg(logEntry->header.distance_before_calib));

    QString hexstring;
    hexstring = hexstring.sprintf("%02x%02x%02x%02x%02x", logEntry->header.unknown1[0],
                                                          logEntry->header.unknown1[1],
                                                          logEntry->header.unknown1[2],
                                                          logEntry->header.unknown1[3],
                                                          logEntry->header.unknown1[4]);
    xml.writeTextElement("Unknown1", hexstring);
    hexstring = hexstring.sprintf("%02x", logEntry->header.unknown2);
    xml.writeTextElement("Unknown2", hexstring);
    hexstring = hexstring.sprintf("%02x%02x%02x%02x", logEntry->header.unknown3[0],
                                                      logEntry->header.unknown3[1],
                                                      logEntry->header.unknown3[2],
                                                      logEntry->header.unknown3[3]);
    xml.writeTextElement("Unknown3", hexstring);
    hexstring = hexstring.sprintf("%02x%02x%02x%02x", logEntry->header.unknown4[0],
                                                      logEntry->header.unknown4[1],
                                                      logEntry->header.unknown4[2],
                                                      logEntry->header.unknown4[3]);
    xml.writeTextElement("Unknown4", hexstring);
    hexstring = hexstring.sprintf("%02x%02x%02x%02x", logEntry->header.unknown5[0],
                                                      logEntry->header.unknown5[1],
                                                      logEntry->header.unknown5[2],
                                                      logEntry->header.unknown5[3]);
    xml.writeTextElement("Unknown5", hexstring);
    hexstring = hexstring.sprintf("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                                  logEntry->header.unknown6[0],
                                  logEntry->header.unknown6[1],
                                  logEntry->header.unknown6[2],
                                  logEntry->header.unknown6[3],
                                  logEntry->header.unknown6[4],
                                  logEntry->header.unknown6[5],
                                  logEntry->header.unknown6[6],
                                  logEntry->header.unknown6[7],
                                  logEntry->header.unknown6[8],
                                  logEntry->header.unknown6[9],
                                  logEntry->header.unknown6[10],
                                  logEntry->header.unknown6[11],
                                  logEntry->header.unknown6[12],
                                  logEntry->header.unknown6[13],
                                  logEntry->header.unknown6[14],
                                  logEntry->header.unknown6[15],
                                  logEntry->header.unknown6[16],
                                  logEntry->header.unknown6[17],
                                  logEntry->header.unknown6[18],
                                  logEntry->header.unknown6[19],
                                  logEntry->header.unknown6[20],
                                  logEntry->header.unknown6[21],
                                  logEntry->header.unknown6[22],
                                  logEntry->header.unknown6[23]);
    xml.writeTextElement("Unknown6", hexstring);

    xml.writeEndElement();
    xml.writeStartElement("Samples");
    for (i=0; i<logEntry->samples_count; i++) {
        writeLogSample(&logEntry->samples[i]);
    }
    xml.writeEndElement();
    xml.writeEndElement();

    return true;
}

bool LogStore::XMLWriter::writeLogSample(ambit_log_sample_t *sample)
{
    sample_type_names_t *sample_type_name;
    sample_distance_source_name_t *distance_source_name;
    sample_altitude_source_name_t *altitude_source_name;
    sample_lap_event_type_t *lap_type_name;
    int i;

    xml.writeStartElement("Sample");
    xml.writeStartElement("Type");
    xml.writeAttribute("id", QString("%1").arg(sample->type));
    for (sample_type_name = &sampleTypeNames[0]; sample_type_name->XMLName != NULL; sample_type_name++) {
        if (sample_type_name->id == sample->type) {
            xml.writeCharacters(QString(sample_type_name->XMLName));
            break;
        }
    }
    xml.writeEndElement();
    QDateTime dateTime(QDate(sample->utc_time.year, sample->utc_time.month, sample->utc_time.day), QTime(sample->utc_time.hour, sample->utc_time.minute, 0).addMSecs(sample->utc_time.msec));
    dateTime.setTimeSpec(Qt::UTC);
    xml.writeTextElement("UTC", dateTime.toString("yyyy-MM-ddThh:mm:ss.zzzZ"));
    xml.writeTextElement("Time", QString("%1").arg(sample->time));
    switch(sample->type) {
    case ambit_log_sample_type_periodic:
        writePeriodicSample(sample);
        break;
    case ambit_log_sample_type_logpause:
        break;
    case ambit_log_sample_type_logrestart:
        break;
    case ambit_log_sample_type_ibi:
        for (i=0; i<sample->u.ibi.ibi_count; i++) {
            xml.writeTextElement("IBI", QString("%1").arg(sample->u.ibi.ibi[i]));
        }
        break;
    case ambit_log_sample_type_ttff:
        xml.writeTextElement("ttff", QString("%1").arg(sample->u.ttff));
        break;
    case ambit_log_sample_type_distance_source:
    {
        xml.writeStartElement("DistanceSource");
        xml.writeAttribute("id", QString("%1").arg(sample->u.distance_source));
        for (distance_source_name = &sampleDistanceSourceNames[0]; distance_source_name->XMLName != ""; distance_source_name++) {
            if (distance_source_name->source_id == sample->u.distance_source) {
                xml.writeCharacters(QString(distance_source_name->XMLName));
                break;
            }
        }
        xml.writeEndElement();
        break;
    }
    case ambit_log_sample_type_lapinfo:
    {
        xml.writeStartElement("Lap");
        xml.writeStartElement("Type");
        xml.writeAttribute("id", QString("%1").arg(sample->u.lapinfo.event_type));
        for (lap_type_name = &sampleLapEventTypeNames[0]; lap_type_name->XMLName != ""; lap_type_name++) {
            if (lap_type_name->event_type == sample->u.lapinfo.event_type) {
                xml.writeCharacters(QString(lap_type_name->XMLName));
            }
        }
        xml.writeEndElement();
        QDateTime dateTime(QDate(sample->u.lapinfo.date_time.year, sample->u.lapinfo.date_time.month, sample->u.lapinfo.date_time.day), QTime(sample->u.lapinfo.date_time.hour, sample->u.lapinfo.date_time.minute, 0).addMSecs(sample->u.lapinfo.date_time.msec));
        xml.writeTextElement("DateTime", dateTime.toString(Qt::ISODate));
        xml.writeTextElement("Duration", QString("%1").arg(sample->u.lapinfo.duration));
        xml.writeTextElement("Distance", QString("%1").arg(sample->u.lapinfo.distance));
        xml.writeEndElement();
        break;
    }
    case ambit_log_sample_type_altitude_source:
    {
        xml.writeStartElement("AltitudeSource");
        xml.writeAttribute("id", QString("%1").arg(sample->u.altitude_source.source_type));
        for (altitude_source_name = &sampleAltitudeSourceNames[0]; altitude_source_name->XMLName != ""; altitude_source_name++) {
            if (altitude_source_name->source_id == sample->u.altitude_source.source_type) {
                xml.writeCharacters(QString(altitude_source_name->XMLName));
                break;
            }
        }
        xml.writeEndElement();
        xml.writeTextElement("AltitudeOffset", QString("%1").arg(sample->u.altitude_source.altitude_offset));
        xml.writeTextElement("PressureOffset", QString("%1").arg(sample->u.altitude_source.pressure_offset));
        break;
    }
    case ambit_log_sample_type_gps_base:
    {
        xml.writeTextElement("NavValid", QString("%1").arg(sample->u.gps_base.navvalid));
        xml.writeTextElement("NavType", QString("%1").arg(sample->u.gps_base.navtype));
        QDateTime dateTime(QDate(sample->u.gps_base.utc_base_time.year, sample->u.gps_base.utc_base_time.month, sample->u.gps_base.utc_base_time.day), QTime(sample->u.gps_base.utc_base_time.hour, sample->u.gps_base.utc_base_time.minute, 0).addMSecs(sample->u.gps_base.utc_base_time.msec));
        dateTime.setTimeSpec(Qt::UTC);
        xml.writeTextElement("UTCReference", dateTime.toString("yyyy-MM-ddThh:mm:ss.zzzZ"));
        xml.writeTextElement("Latitude", QString("%1").arg(sample->u.gps_base.latitude));
        xml.writeTextElement("Longitude", QString("%1").arg(sample->u.gps_base.longitude));
        xml.writeTextElement("GPSAltitude", QString("%1").arg(sample->u.gps_base.altitude));
        xml.writeTextElement("GPSSpeed", QString("%1").arg(sample->u.gps_base.speed));
        xml.writeTextElement("GPSHeading", QString("%1").arg(sample->u.gps_base.heading));
        xml.writeTextElement("EHPE", QString("%1").arg(sample->u.gps_base.ehpe));
        xml.writeTextElement("NumberOfSatellites", QString("%1").arg(sample->u.gps_base.noofsatellites));
        xml.writeTextElement("GpsHDOP", QString("%1").arg(sample->u.gps_base.hdop));
        xml.writeStartElement("Satellites");
        ambit_log_gps_satellite_t *satellite;
        for (i=0; i<sample->u.gps_base.satellites_count; i++) {
            xml.writeStartElement("Satellite");
            satellite = &sample->u.gps_base.satellites[i];
            xml.writeTextElement("SV", QString("%1").arg(satellite->sv));
            xml.writeTextElement("SNR", QString("%1").arg(satellite->snr));
            xml.writeTextElement("State", QString("%1").arg(satellite->state));
            xml.writeEndElement();
        }
        xml.writeEndElement();
        break;
    }
    case ambit_log_sample_type_gps_small:
    {
        xml.writeTextElement("Latitude", QString("%1").arg(sample->u.gps_small.latitude));
        xml.writeTextElement("Longitude", QString("%1").arg(sample->u.gps_small.longitude));
        xml.writeTextElement("EHPE", QString("%1").arg(sample->u.gps_small.ehpe));
        xml.writeTextElement("NumberOfSatellites", QString("%1").arg(sample->u.gps_small.noofsatellites));
        break;
    }
    case ambit_log_sample_type_gps_tiny:
    {
        xml.writeTextElement("Latitude", QString("%1").arg(sample->u.gps_tiny.latitude));
        xml.writeTextElement("Longitude", QString("%1").arg(sample->u.gps_tiny.longitude));
        xml.writeTextElement("EHPE", QString("%1").arg(sample->u.gps_tiny.ehpe));
        xml.writeTextElement("Unknown", QString("%1").arg(sample->u.gps_tiny.unknown));
        break;
    }
    case ambit_log_sample_type_time:
    {
        QTime timeref(sample->u.time.hour, sample->u.time.minute, sample->u.time.second);
        xml.writeTextElement("TimeRef", timeref.toString(Qt::ISODate));
        break;
    }
    case ambit_log_sample_type_activity:
        xml.writeTextElement("ActivityType", QString("%1").arg(sample->u.activity.activitytype));
        xml.writeTextElement("CustomModeId", QString("%1").arg(sample->u.activity.custommode));
        break;
    case ambit_log_sample_type_position:
        xml.writeTextElement("Latitude", QString("%1").arg(sample->u.position.latitude));
        xml.writeTextElement("Longitude", QString("%1").arg(sample->u.position.longitude));
        break;
    case ambit_log_sample_type_unknown:
    {
        QString data = "";
        for (size_t i=0; i<sample->u.unknown.datalen; i++) {
            data += data.sprintf("%02x", sample->u.unknown.data[i]);
        }
        xml.writeTextElement("Data", data);
        break;
    }
    }

    xml.writeEndElement();

    return true;
}

bool LogStore::XMLWriter::writePeriodicSample(ambit_log_sample_t *sample)
{
    int i;
    ambit_log_sample_periodic_value_t *value;

    for (i=0; i<sample->u.periodic.value_count; i++) {
        value = &sample->u.periodic.values[i];

        switch(value->type) {
        case ambit_log_sample_periodic_type_latitude:
            xml.writeTextElement("Latitude", QString("%1").arg(value->u.latitude));
            break;
        case ambit_log_sample_periodic_type_longitude:
            xml.writeTextElement("Longitude", QString("%1").arg(value->u.longitude));
            break;
        case ambit_log_sample_periodic_type_distance:
            xml.writeTextElement("Distance", QString("%1").arg(value->u.distance));
            break;
        case ambit_log_sample_periodic_type_speed:
            xml.writeTextElement("Speed", QString("%1").arg(value->u.speed));
            break;
        case ambit_log_sample_periodic_type_hr:
            xml.writeTextElement("HR", QString("%1").arg(value->u.hr));
            break;
        case ambit_log_sample_periodic_type_time:
            xml.writeTextElement("Time", QString("%1").arg(value->u.time));
            break;
        case ambit_log_sample_periodic_type_gpsspeed:
            xml.writeTextElement("GPSSpeed", QString("%1").arg(value->u.gpsspeed));
            break;
        case ambit_log_sample_periodic_type_wristaccspeed:
            xml.writeTextElement("WristAccSpeed", QString("%1").arg(value->u.wristaccspeed));
            break;
        case ambit_log_sample_periodic_type_bikepodspeed:
            xml.writeTextElement("BikePodSpeed", QString("%1").arg(value->u.bikepodspeed));
            break;
        case ambit_log_sample_periodic_type_ehpe:
            xml.writeTextElement("EHPE", QString("%1").arg(value->u.ehpe));
            break;
        case ambit_log_sample_periodic_type_evpe:
            xml.writeTextElement("EVPE", QString("%1").arg(value->u.evpe));
            break;
        case ambit_log_sample_periodic_type_altitude:
            xml.writeTextElement("Altitude", QString("%1").arg(value->u.altitude));
            break;
        case ambit_log_sample_periodic_type_abspressure:
            xml.writeTextElement("AbsPressure", QString("%1").arg(value->u.abspressure));
            break;
        case ambit_log_sample_periodic_type_energy:
            xml.writeTextElement("EnergyConsumption", QString("%1").arg(value->u.energy));
            break;
        case ambit_log_sample_periodic_type_temperature:
            xml.writeTextElement("Temperature", QString("%1").arg(value->u.temperature));
            break;
        case ambit_log_sample_periodic_type_charge:
            xml.writeTextElement("BatteryCharge", QString("%1").arg(value->u.charge));
            break;
        case ambit_log_sample_periodic_type_gpsaltitude:
            xml.writeTextElement("GPSAltitude", QString("%1").arg(value->u.gpsaltitude));
            break;
        case ambit_log_sample_periodic_type_gpsheading:
            xml.writeTextElement("GPSHeading", QString("%1").arg(value->u.gpsheading));
            break;
        case ambit_log_sample_periodic_type_gpshdop:
            xml.writeTextElement("GpsHDOP", QString("%1").arg(value->u.gpshdop));
            break;
        case ambit_log_sample_periodic_type_gpsvdop:
            xml.writeTextElement("GpsVDOP", QString("%1").arg(value->u.gpsvdop));
            break;
        case ambit_log_sample_periodic_type_wristcadence:
            xml.writeTextElement("WristCadence", QString("%1").arg(value->u.wristcadence));
            break;
        case ambit_log_sample_periodic_type_snr:
        {
            QString format;
            QString snr = format.sprintf("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                                         value->u.snr[0],
                                         value->u.snr[1],
                                         value->u.snr[2],
                                         value->u.snr[3],
                                         value->u.snr[4],
                                         value->u.snr[5],
                                         value->u.snr[6],
                                         value->u.snr[7],
                                         value->u.snr[8],
                                         value->u.snr[9],
                                         value->u.snr[10],
                                         value->u.snr[11],
                                         value->u.snr[12],
                                         value->u.snr[13],
                                         value->u.snr[14],
                                         value->u.snr[15]);
            xml.writeTextElement("SNR", snr);
            break;
        }
        case ambit_log_sample_periodic_type_noofsatellites:
            xml.writeTextElement("NumberOfSatellites", QString("%1").arg(value->u.noofsatellites));
            break;
        case ambit_log_sample_periodic_type_sealevelpressure:
            xml.writeTextElement("SeaLevelPressure", QString("%1").arg(value->u.sealevelpressure));
            break;
        case ambit_log_sample_periodic_type_verticalspeed:
            xml.writeTextElement("VerticalSpeed", QString("%1").arg(value->u.verticalspeed));
            break;
        case ambit_log_sample_periodic_type_cadence:
            xml.writeTextElement("Cadence", QString("%1").arg(value->u.cadence));
            break;
        case ambit_log_sample_periodic_type_bikepower:
            xml.writeTextElement("BikePower", QString("%1").arg(value->u.bikepower));
            break;
        case ambit_log_sample_periodic_type_swimingstrokecnt:
            xml.writeTextElement("SwimmingStrokeCount", QString("%1").arg(value->u.swimingstrokecnt));
            break;
        case ambit_log_sample_periodic_type_ruleoutput1:
            xml.writeTextElement("RuleOutput1", QString("%1").arg(value->u.ruleoutput1));
            break;
        case ambit_log_sample_periodic_type_ruleoutput2:
            xml.writeTextElement("RuleOutput2", QString("%1").arg(value->u.ruleoutput2));
            break;
        case ambit_log_sample_periodic_type_ruleoutput3:
            xml.writeTextElement("RuleOutput3", QString("%1").arg(value->u.ruleoutput3));
            break;
        case ambit_log_sample_periodic_type_ruleoutput4:
            xml.writeTextElement("RuleOutput4", QString("%1").arg(value->u.ruleoutput4));
            break;
        case ambit_log_sample_periodic_type_ruleoutput5:
            xml.writeTextElement("RuleOutput5", QString("%1").arg(value->u.ruleoutput5));
            break;
        }
    }

    return true;
}
