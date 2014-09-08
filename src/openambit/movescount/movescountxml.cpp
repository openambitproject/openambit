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
#include "movescountxml.h"
#include <QFile>

#define _USE_MATH_DEFINES
#include <math.h>

typedef struct typename_lookup_entry_s {
    u_int8_t id;
    QString XMLName;
} typename_lookup_entry_t;

static typename_lookup_entry_t sampleDistanceSourceNames[] = {
    { 0x02, "Gps" },
    { 0x03, "Wrist" },
    { 0, "" }
};

static typename_lookup_entry_t sampleAltitudeSourceNames[] = {
    { 0x04, "Pressure" },
    { 0, "" }
};

static typename_lookup_entry_t sampleLapEventTypeNames[] = {
    { 0x00, "Distance" },
    { 0x01, "Manual" },
    { 0x14, "High Interval" },
    { 0x15, "Low Interval" },
    { 0x16, "Interval" },
    { 0x1e, "Pause" },
    { 0x1f, "Start" },
    { 0, "" }
};

static typename_lookup_entry_t sampleActivityNames[] = {
    { 0x01, "Not Specified Sport" },
    { 0x02, "Multisport" },
    { 0x03, "Run" },
    { 0x04, "Cycling" },
    { 0x05, "Mountain Biking" },
    { 0x06, "UNDEFINED7" },
    { 0x07, "Skating" },
    { 0x08, "Aerobics" },
    { 0x09, "Yoga Pilates" },
    { 0x0a, "Trekking" },
    { 0x0b, "Walking" },
    { 0x0c, "Sailing" },
    { 0x0d, "Kayaking" },
    { 0x0e, "Rowing" },
    { 0x0f, "Climbing" },
    { 0x10, "Indoor Cycling" },
    { 0x11, "Circuit Training" },
    { 0x12, "Triathlon" },
    { 0x13, "Alpine Skiing" },
    { 0x14, "Snowboarding" },
    { 0x15, "Crosscountry Skiing" },
    { 0x16, "Weight Training" },
    { 0x17, "Basketball" },
    { 0x18, "Soccer" },
    { 0x19, "Ice Hockey" },
    { 0x1a, "Volleyball" },
    { 0x1b, "Football" },
    { 0x1c, "Softball" },
    { 0x1d, "Cheerleading" },
    { 0x1e, "Baseball" },
    { 0x1f, "UNDEFINED32" },
    { 0x20, "Tennis" },
    { 0x21, "Badminton" },
    { 0x22, "Table Tenni" },
    { 0x23, "Racquet Ball" },
    { 0x24, "Squash" },
    { 0x25, "Combat Sport" },
    { 0x26, "Boxing" },
    { 0x27, "Floorball" },
    { 0x28, "UNDEFINED41" },
    { 0x29, "UNDEFINED42" },
    { 0x2a, "UNDEFINED43" },
    { 0x2b, "UNDEFINED44" },
    { 0x2c, "UNDEFINED45" },
    { 0x2d, "UNDEFINED46" },
    { 0x2e, "UNDEFINED47" },
    { 0x2f, "UNDEFINED48" },
    { 0x30, "UNDEFINED49" },
    { 0x31, "UNDEFINED50" },
    { 0x32, "Scuba Diving" },
    { 0x33, "Free Diving" },
    { 0x34, "UNDEFINED53" },
    { 0x35, "UNDEFINED54" },
    { 0x36, "UNDEFINED55" },
    { 0x37, "UNDEFINED56" },
    { 0x38, "UNDEFINED57" },
    { 0x39, "UNDEFINED58" },
    { 0x3a, "UNDEFINED59" },
    { 0x3b, "UNDEFINED60" },
    { 0x3c, "Adventure Racing" },
    { 0x3d, "Bowling" },
    { 0x3e, "Cricket" },
    { 0x3f, "Crosstrainer" },
    { 0x40, "Dancing" },
    { 0x41, "Golf" },
    { 0x42, "Gymnastics" },
    { 0x43, "Handball" },
    { 0x44, "Horseback Riding" },
    { 0x45, "Ice Skating" },
    { 0x46, "Indoor Rowing" },
    { 0x47, "Canoeing" },
    { 0x48, "Motorsports" },
    { 0x49, "Mountaineering" },
    { 0x4a, "Orienteering" },
    { 0x4b, "Rugby" },
    { 0x4c, "UNDEFINED77" },
    { 0x4d, "Ski Touring" },
    { 0x4e, "Stretching" },
    { 0x4f, "Telemark Skiing" },
    { 0x50, "Track And Field" },
    { 0x51, "Trail Running" },
    { 0x52, "Openwater Swimming" },
    { 0x53, "UNDEFINED84" },
    { 0x54, "UNDEFINED85" },
    { 0x55, "UNDEFINED86" },
    { 0x56, "UNDEFINED87" },
    { 0x57, "UNDEFINED88" },
    { 0x58, "UNDEFINED89" },
    { 0x59, "UNDEFINED90" },
    { 0x5a, "UNDEFINED91" },
    { 0x5b, "UNDEFINED92" },
    { 0x5c, "UNDEFINED93" },
    { 0x5d, "UNDEFINED94" },
    { 0x5e, "UNDEFINED95" },
    { 0x5f, "UNDEFINED96" },
    { 0x60, "UNDEFINED97" },
    { 0x61, "UNDEFINED98" },
    { 0x62, "UNDEFINED99" },
    { 0x63, "Other" },
    { 0x64, "Butterfly" },
    { 0x65, "Backstroke" },
    { 0x66, "Breaststroke" },
    { 0x67, "Freestyle" },
    { 0x68, "Drill" },
    { 0, "" }
};

MovesCountXML::MovesCountXML(QObject *parent) :
    QObject(parent)
{
    storagePath = QString(getenv("HOME")) + "/.openambit/movescount";
}

void MovesCountXML::writeLog(LogEntry *logEntry)
{
    XMLWriter writer(logEntry);
    QFile logfile(logEntryPath(logEntry));
    logfile.open(QIODevice::WriteOnly | QIODevice::Truncate);

    writer.write(&logfile);

    logfile.close();
}

QString MovesCountXML::logEntryPath(LogEntry *logEntry)
{
    return storagePath + "/log-" + logEntry->device + "-" + logEntry->time.toString("yyyy-MM-ddThh_mm_ss") + "-0.log";
}

MovesCountXML::XMLWriter::XMLWriter(LogEntry *logEntry) :
    logEntry(logEntry)
{
    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(-1);
}

bool MovesCountXML::XMLWriter::write(QIODevice *device)
{
    bool ret = true;

    xml.setDevice(device);

    xml.writeStartDocument();
    ret = writeLogEntry();
    xml.writeEndDocument();

    return ret;
}

bool MovesCountXML::XMLWriter::writeLogEntry()
{
    u_int32_t i;

    xml.writeStartElement("header");
    xml.writeTextElement("Duration", QString::number((double)logEntry->logEntry->header.duration/1000.0, 'g', 16));
    xml.writeTextElement("Ascent", QString("%1").arg(logEntry->logEntry->header.ascent));
    xml.writeTextElement("Descent", QString("%1").arg(logEntry->logEntry->header.descent));
    xml.writeTextElement("AscentTime", QString::number((double)logEntry->logEntry->header.ascent_time/1000.0, 'g', 16));
    xml.writeTextElement("DescentTime", QString::number((double)logEntry->logEntry->header.descent_time/1000.0, 'g', 16));
    xml.writeTextElement("RecoveryTime", QString::number((double)logEntry->logEntry->header.recovery_time/1000.0, 'g', 16));
    xml.writeStartElement("Speed");
    xml.writeTextElement("Avg", QString::number((double)logEntry->logEntry->header.speed_avg/3600.0, 'g', 16));
    xml.writeTextElement("Max", QString::number((double)logEntry->logEntry->header.speed_max/3600.0, 'g', 16));
    xml.writeTextElement("MaxTime", QString::number((double)logEntry->logEntry->header.speed_max_time/1000.0, 'g', 16));
    xml.writeEndElement();
    xml.writeStartElement("Cadence");
    xml.writeTextElement("Avg", QString::number((double)logEntry->logEntry->header.cadence_avg/60.0, 'g', 16));
    xml.writeTextElement("Max", QString::number((double)logEntry->logEntry->header.cadence_max/60.0, 'g', 16));
    xml.writeTextElement("MaxTime", QString::number((double)logEntry->logEntry->header.cadence_max_time/1000.0, 'g', 16));
    xml.writeEndElement();
    xml.writeStartElement("Altitude");
    xml.writeTextElement("Max", QString("%1").arg(logEntry->logEntry->header.altitude_max));
    xml.writeTextElement("Min", QString("%1").arg(logEntry->logEntry->header.altitude_min));
    xml.writeTextElement("MaxTime", QString::number((double)logEntry->logEntry->header.altitude_max_time/1000.0, 'g', 16));
    xml.writeTextElement("MinTime", QString::number((double)logEntry->logEntry->header.altitude_min_time/1000.0, 'g', 16));
    xml.writeEndElement();
    // Only include HR if heartrate seems to be present
    if (logEntry->logEntry->header.heartrate_avg != 0) {
        xml.writeStartElement("HR");
        xml.writeTextElement("Avg", QString::number((double)logEntry->logEntry->header.heartrate_avg/logEntry->personalSettings->rest_hr, 'g', 16));
        xml.writeTextElement("Max", QString::number((double)logEntry->logEntry->header.heartrate_max/logEntry->personalSettings->rest_hr, 'g', 16));
        xml.writeTextElement("Min", QString::number((double)logEntry->logEntry->header.heartrate_min/logEntry->personalSettings->rest_hr, 'g', 16));
        xml.writeTextElement("MaxTime", QString::number((double)logEntry->logEntry->header.heartrate_max_time/1000.0, 'g', 16));
        xml.writeTextElement("MinTime", QString::number((double)logEntry->logEntry->header.heartrate_min_time/1000.0, 'g', 16));
        xml.writeEndElement();
        xml.writeTextElement("PeakTrainingEffect", QString::number((double)logEntry->logEntry->header.peak_training_effect/10.0, 'g', 16));
    }
    xml.writeTextElement("ActivityType", QString("%1").arg(logEntry->logEntry->header.activity_type));
    xml.writeTextElement("Activity", QString::fromLatin1(logEntry->logEntry->header.activity_name));
    xml.writeStartElement("Temperature");
    xml.writeTextElement("Max", QString::number((double)logEntry->logEntry->header.temperature_max/10.0 + 273.15, 'g', 16));
    xml.writeTextElement("Min", QString::number((double)logEntry->logEntry->header.temperature_min/10.0 + 273.15, 'g', 16));
    xml.writeTextElement("MaxTime", QString::number((double)logEntry->logEntry->header.temperature_max_time/1000.0, 'g', 16));
    xml.writeTextElement("MinTime", QString::number((double)logEntry->logEntry->header.temperature_min_time/1000.0, 'g', 16));
    xml.writeEndElement();
    xml.writeTextElement("Distance", QString("%1").arg(logEntry->logEntry->header.distance));
    xml.writeTextElement("LogItemCount", QString("%1").arg(logEntry->logEntry->header.samples_count));
    // Only include energy if heartrate seems to be present
    if (logEntry->logEntry->header.heartrate_avg != 0) {
        xml.writeTextElement("Energy", QString::number((double)logEntry->logEntry->header.energy_consumption*4186.8, 'g', 16)); // kcal to Joule
    }
    xml.writeTextElement("TimeToFirstFix", QString::number((double)logEntry->logEntry->header.first_fix_time/1000.0, 'g', 16));
    xml.writeTextElement("BatteryChargeAtStart", QString::number((double)logEntry->logEntry->header.battery_start/100.0, 'g', 16));
    xml.writeTextElement("BatteryCharge", QString::number((double)logEntry->logEntry->header.battery_end/100.0, 'g', 16));
    xml.writeTextElement("DistanceBeforeCalibrationChange", QString("%1").arg(logEntry->logEntry->header.distance_before_calib));
    QDateTime dateTime(QDate(logEntry->logEntry->header.date_time.year,
                             logEntry->logEntry->header.date_time.month,
                             logEntry->logEntry->header.date_time.day),
                       QTime(logEntry->logEntry->header.date_time.hour,
                             logEntry->logEntry->header.date_time.minute, 0).addMSecs(logEntry->logEntry->header.date_time.msec));
    xml.writeTextElement("DateTime", dateTime.toString(Qt::ISODate));
    xml.writeEndElement();
    QList<quint16> ibis;
    xml.writeStartElement("Samples");
    QList<int> order = rearrangeSamples();
    foreach(int index, order) {
        writeLogSample(&logEntry->logEntry->samples[index], &ibis);
    }

    xml.writeEndElement();
    if (ibis.count() > 0) {
        xml.writeStartElement("IBI");
        for (i=0; i<(u_int32_t)ibis.count(); i++) {
            if (i > 0) {
                xml.writeCharacters(" ");
            }
            xml.writeCharacters(QString("%1").arg(ibis.at(i)));
        }
        xml.writeEndElement();
    }

    return true;
}

bool MovesCountXML::XMLWriter::writeLogSample(ambit_log_sample_t *sample, QList<quint16> *ibis)
{
    typename_lookup_entry_t *name_lookup;
    int i;

    switch(sample->type) {
    case ambit_log_sample_type_periodic:
        xml.writeStartElement("Sample");
        writePeriodicSample(sample);
        xml.writeEndElement();
        break;
    case ambit_log_sample_type_logpause:
    {
        //! TODO: Unknown representation!
        break;
    }
    case ambit_log_sample_type_logrestart:
    {
        //! TODO: Unknown representation!
        break;
    }
    case ambit_log_sample_type_ibi:
        for (i=0; i<sample->u.ibi.ibi_count; i++) {
            ibis->append(sample->u.ibi.ibi[i]);
        }
        break;
    case ambit_log_sample_type_ttff:
        /* Unknown! */
        break;
    case ambit_log_sample_type_distance_source:
    {
        xml.writeStartElement("Sample");
        xml.writeTextElement("Time", QString::number((double)sample->time/1000.0, 'g', 16));
        xml.writeStartElement("Events");
        xml.writeStartElement("Distance");
        xml.writeStartElement("Source");
        for (name_lookup = &sampleDistanceSourceNames[0]; name_lookup->XMLName != ""; name_lookup++) {
            if (name_lookup->id == sample->u.distance_source) {
                xml.writeCharacters(QString(name_lookup->XMLName));
                break;
            }
        }
        xml.writeEndElement();
        QDateTime dateTime(QDate(sample->utc_time.year, sample->utc_time.month, sample->utc_time.day), QTime(sample->utc_time.hour, sample->utc_time.minute, 0).addMSecs(sample->utc_time.msec));
        dateTime.setTimeSpec(Qt::UTC);
        if (sample->u.distance_source == 0x02) {
            // Only include UTC time for GPS source
            xml.writeTextElement("UTC", dateTimeString(dateTime));
        }
        xml.writeEndElement();
        xml.writeEndElement();
        xml.writeTextElement("UTC", dateTimeString(dateTime));
        xml.writeEndElement();
        break;
    }
    case ambit_log_sample_type_lapinfo:
    {
        xml.writeStartElement("Sample");
        QDateTime dateTime(QDate(sample->utc_time.year, sample->utc_time.month, sample->utc_time.day), QTime(sample->utc_time.hour, sample->utc_time.minute, 0).addMSecs(sample->utc_time.msec));
        dateTime.setTimeSpec(Qt::UTC);
        xml.writeTextElement("UTC", dateTimeString(dateTime));
        xml.writeTextElement("Time", QString::number((double)sample->time/1000.0, 'g', 16));
        xml.writeStartElement("Events");
        if (sample->u.lapinfo.event_type == 0x1e || sample->u.lapinfo.event_type == 0x1f) {
            xml.writeStartElement("Pause");
            xml.writeTextElement("Type", QString("%1").arg(sample->u.lapinfo.event_type));
            xml.writeTextElement("Duration", QString::number((double)sample->u.lapinfo.duration/1000.0, 'g', 16));
            xml.writeTextElement("Distance", QString("%1").arg(sample->u.lapinfo.distance));
            xml.writeTextElement("State", sample->u.lapinfo.event_type == 0x1e ? "True" : "False");
            xml.writeEndElement();
        }
        else {
            xml.writeStartElement("Lap");
            xml.writeStartElement("Type");
            for (name_lookup = &sampleLapEventTypeNames[0]; name_lookup->XMLName != ""; name_lookup++) {
                if (name_lookup->id == sample->u.lapinfo.event_type) {
                    xml.writeCharacters(QString(name_lookup->XMLName));
                }
            }
            xml.writeEndElement();
            xml.writeTextElement("Duration", QString::number((double)sample->u.lapinfo.duration/1000.0, 'g', 16));
            xml.writeTextElement("Distance", QString("%1").arg(sample->u.lapinfo.distance));
            xml.writeEndElement();
        }
        xml.writeEndElement();
        xml.writeEndElement();
        break;
    }
    case ambit_log_sample_type_altitude_source:
    {
        xml.writeStartElement("Sample");
        xml.writeTextElement("Time", QString::number((double)sample->time/1000.0, 'g', 16));
        xml.writeStartElement("Events");
        xml.writeStartElement("Altitude");
        xml.writeStartElement("Source");
        for (name_lookup = &sampleAltitudeSourceNames[0]; name_lookup->XMLName != ""; name_lookup++) {
            if (name_lookup->id == sample->u.altitude_source.source_type) {
                xml.writeCharacters(QString(name_lookup->XMLName));
                break;
            }
        }
        xml.writeEndElement();
        xml.writeTextElement("AltitudeOffset", QString("%1").arg(sample->u.altitude_source.altitude_offset));
        xml.writeTextElement("PressureOffset", QString("%1").arg(sample->u.altitude_source.pressure_offset*10));
        QDateTime dateTime(QDate(sample->utc_time.year, sample->utc_time.month, sample->utc_time.day), QTime(sample->utc_time.hour, sample->utc_time.minute, 0).addMSecs(sample->utc_time.msec));
        dateTime.setTimeSpec(Qt::UTC);
        xml.writeTextElement("UTC", dateTimeString(dateTime));
        xml.writeEndElement();
        xml.writeEndElement();
        xml.writeTextElement("UTC", dateTimeString(dateTime));
        xml.writeEndElement();
        break;
    }
    case ambit_log_sample_type_gps_base:
    {
        xml.writeStartElement("Sample");
        QString sprintfer;
        QString navtype = sprintfer.sprintf("0x%04x", sample->u.gps_base.navtype);
        xml.writeTextElement("NavType", navtype);
        if (sample->u.gps_base.navvalid == 0x0000) {
            xml.writeTextElement("NavValid", "validated");
        }
        else {
            xml.writeTextElement("NavValid", "not validated");
        }
        QString navTypeExplanation;

        if ((sample->u.gps_base.navtype & 0x0007) == 0) {
            navTypeExplanation = "no fix";
        }
        else {
            switch ((sample->u.gps_base.navtype & 0x0007)) {
            case 0x01:
                navTypeExplanation = "1-SV KF solution";
                break;
            case 0x02:
                navTypeExplanation = "2-SV KF solution";
                break;
            case 0x03:
                navTypeExplanation = "3-SV KF solution";
                break;
            case 0x04:
                navTypeExplanation = "4 or more SV KF solution";
                break;
            case 0x05:
                navTypeExplanation = "2-D least-squares solution";
                break;
            case 0x06:
                navTypeExplanation = "3-D least-squares solution";
                break;
            case 0x07:
                navTypeExplanation = "DR solution (see bits 8, 14-15)";
                break;
            }
            if ((sample->u.gps_base.navtype & 0x0008)) {
                navTypeExplanation += ", trickle";
            }
            switch ((sample->u.gps_base.navtype & 0x0030) >> 4) {
            case 0x01:
                navTypeExplanation += ", KF alti hold";
                break;
            case 0x02:
                navTypeExplanation += ", user alti hold";
                break;
            case 0x03:
                navTypeExplanation += ", always alti hold";
                break;
            }
            if ((sample->u.gps_base.navtype & 0x0040)) {
                navTypeExplanation += ", DOP exceeded";
            }
            if ((sample->u.gps_base.navtype & 0x0080)) {
                navTypeExplanation += ", DGPS corr";
            }
            if ((sample->u.gps_base.navtype & 0x0200)) {
                navTypeExplanation += ", overdetermined";
            }
            if ((sample->u.gps_base.navtype & 0x0400)) {
                navTypeExplanation += ", velo DR timeout";
            }
            if ((sample->u.gps_base.navtype & 0x1000)) {
                navTypeExplanation += ", invalid velocity";
            }
            if ((sample->u.gps_base.navtype & 0x2000)) {
                navTypeExplanation += ", alti hold disabled";
            }
        }
        xml.writeTextElement("NavTypeExplanation", navTypeExplanation);
        xml.writeStartElement("Satellites");
        ambit_log_gps_satellite_t *satellite;
        for (i=0; i<sample->u.gps_base.satellites_count; i++) {
            xml.writeStartElement("Satellite");
            satellite = &sample->u.gps_base.satellites[i];
            xml.writeTextElement("SV", QString("%1").arg(satellite->sv));
            xml.writeTextElement("SNR", QString("%1").arg(satellite->snr));
            //! TODO: What parts of state should be used to determine this!?
            if (satellite->state & 0x20) {
                xml.writeTextElement("CodeLocked", "true");
            }
            else {
                xml.writeTextElement("CodeLocked", "false");
            }
            if (satellite->state & 0x80) {
                xml.writeTextElement("Ephemeris", "true");
            }
            else {
                xml.writeTextElement("Ephemeris", "false");
            }
            QString stateText;
            if (satellite->state == 0) {
                stateText = "State 0";
            }
            else {
                stateText = sprintfer.sprintf("State 0x%02x", satellite->state);
            }
            xml.writeTextElement("Custom", stateText);
            xml.writeEndElement();
        }
        xml.writeEndElement();
        xml.writeTextElement("GPSAltitude", QString::number((double)sample->u.gps_base.altitude/100.0, 'g', 16));
        xml.writeTextElement("GPSHeading", QString::number((double)sample->u.gps_base.heading*M_PI/180/100, 'g', 16));
        xml.writeTextElement("GPSSpeed", QString::number((double)sample->u.gps_base.speed/100.0, 'g', 16));
        xml.writeTextElement("GpsHDOP", QString::number((double)sample->u.gps_base.hdop/5.0, 'g', 16));
        xml.writeTextElement("NumberOfSatellites", QString("%1").arg(sample->u.gps_base.noofsatellites));
        xml.writeTextElement("Latitude", QString::number((double)sample->u.gps_base.latitude*M_PI/180/10000000, 'g', 16)); // degrees -> radians
        xml.writeTextElement("Longitude", QString::number((double)sample->u.gps_base.longitude*M_PI/180/10000000, 'g', 16)); // degrees -> radians
        xml.writeTextElement("EHPE", QString::number((double)sample->u.gps_base.ehpe/100.0, 'g', 16));
        xml.writeTextElement("Time", QString::number((double)sample->time/1000.0, 'g', 16));
        QDateTime dateTime(QDate(sample->u.gps_base.utc_base_time.year, sample->u.gps_base.utc_base_time.month, sample->u.gps_base.utc_base_time.day), QTime(sample->u.gps_base.utc_base_time.hour, sample->u.gps_base.utc_base_time.minute, 0).addMSecs(sample->u.gps_base.utc_base_time.msec));
        dateTime.setTimeSpec(Qt::UTC);
        xml.writeTextElement("UTC", dateTimeString(dateTime));
        xml.writeTextElement("SampleType", "gps-base");
        xml.writeEndElement();
        break;
    }
    case ambit_log_sample_type_gps_small:
    {
        xml.writeStartElement("Sample");
        xml.writeTextElement("NumberOfSatellites", QString("%1").arg(sample->u.gps_small.noofsatellites));
        xml.writeTextElement("Latitude", QString::number((double)sample->u.gps_small.latitude*M_PI/180/10000000, 'g', 16));
        xml.writeTextElement("Longitude", QString::number((double)sample->u.gps_small.longitude*M_PI/180/10000000, 'g', 16));
        xml.writeTextElement("EHPE", QString::number((double)sample->u.gps_small.ehpe/100.0, 'g', 16));
        xml.writeTextElement("Time", QString::number((double)sample->time/1000.0, 'g', 16));
        // "Mimic" moveslinks strange second handling
        int seconds = sample->utc_time.msec/1000;
        if ((sample->utc_time.msec % 1000) >= 100) {
            seconds++;
        }
        QDateTime dateTime(QDate(sample->utc_time.year, sample->utc_time.month, sample->utc_time.day), QTime(sample->utc_time.hour, sample->utc_time.minute, 0).addSecs(seconds));
        dateTime.setTimeSpec(Qt::UTC);
        xml.writeTextElement("UTC", dateTimeString(dateTime));
        xml.writeTextElement("SampleType", "gps-small");
        xml.writeEndElement();
        break;
    }
    case ambit_log_sample_type_gps_tiny:
    {
        xml.writeStartElement("Sample");
        xml.writeTextElement("Latitude", QString::number((double)sample->u.gps_tiny.latitude*M_PI/180/10000000, 'g', 16));
        xml.writeTextElement("Longitude", QString::number((double)sample->u.gps_tiny.longitude*M_PI/180/10000000, 'g', 16));
        xml.writeTextElement("EHPE", QString::number((double)sample->u.gps_tiny.ehpe/100.0, 'g', 16));
        xml.writeTextElement("Time", QString::number((double)sample->time/1000.0, 'g', 16));
        // "Mimic" moveslinks strange second handling
        int seconds = sample->utc_time.msec/1000;
        if ((sample->utc_time.msec % 1000) >= 100) {
            seconds++;
        }
        QDateTime dateTime(QDate(sample->utc_time.year, sample->utc_time.month, sample->utc_time.day), QTime(sample->utc_time.hour, sample->utc_time.minute, 0).addSecs(seconds));
        dateTime.setTimeSpec(Qt::UTC);
        xml.writeTextElement("UTC", dateTimeString(dateTime));
        xml.writeTextElement("SampleType", "gps-tiny");
        xml.writeEndElement();
        break;
    }
    case ambit_log_sample_type_time:
    {
        //! TODO: Unknown representation!
        break;
    }
    case ambit_log_sample_type_activity:
    {
        xml.writeStartElement("Sample");
        xml.writeTextElement("Time", QString::number((double)sample->time/1000.0, 'g', 16));
        xml.writeStartElement("Events");
        xml.writeStartElement("Activity");
        xml.writeTextElement("CustomModeId", QString("%1").arg(sample->u.activity.custommode));
        xml.writeStartElement("Type");
        typename_lookup_entry_t *name_lookup;
        for (name_lookup = &sampleActivityNames[0]; name_lookup->XMLName != ""; name_lookup++) {
            if (name_lookup->id == sample->u.activity.activitytype) {
                xml.writeCharacters(QString(name_lookup->XMLName));
                break;
            }
        }
        xml.writeEndElement();
        xml.writeEndElement();
        xml.writeEndElement();
        QDateTime dateTime(QDate(sample->utc_time.year, sample->utc_time.month, sample->utc_time.day), QTime(sample->utc_time.hour, sample->utc_time.minute, 0).addMSecs(sample->utc_time.msec));
        dateTime.setTimeSpec(Qt::UTC);
        xml.writeTextElement("UTC", dateTimeString(dateTime));
        xml.writeEndElement();
        break;
    }
    case ambit_log_sample_type_position:
        //! TODO: Unknown representation!
        break;
    default:
        break;
    }

    return true;
}

bool MovesCountXML::XMLWriter::writePeriodicSample(ambit_log_sample_t *sample)
{
    int i;
    ambit_log_sample_periodic_value_t *value;

    for (i=0; i<sample->u.periodic.value_count; i++) {
        value = &sample->u.periodic.values[i];

        switch(value->type) {
        case ambit_log_sample_periodic_type_latitude:
            xml.writeTextElement("Latitude", QString::number((double)value->u.latitude*M_PI/180/10000000, 'g', 16));
            break;
        case ambit_log_sample_periodic_type_longitude:
            xml.writeTextElement("Longitude", QString::number((double)value->u.longitude*M_PI/180/10000000, 'g', 16));
            break;
        case ambit_log_sample_periodic_type_distance:
            xml.writeTextElement("Distance", QString("%1").arg(value->u.distance));
            break;
        case ambit_log_sample_periodic_type_speed:
            xml.writeTextElement("Speed", QString::number((double)value->u.speed/100.0, 'g', 16));
            break;
        case ambit_log_sample_periodic_type_hr:
            xml.writeTextElement("HR", QString::number((double)value->u.hr/logEntry->personalSettings->rest_hr, 'g', 16));
            break;
        case ambit_log_sample_periodic_type_time:
            xml.writeTextElement("Time", QString::number((double)value->u.time/1000.0, 'g', 16));
            break;
        case ambit_log_sample_periodic_type_gpsspeed:
            xml.writeTextElement("GPSSpeed", QString::number((double)value->u.gpsspeed/100.0, 'g', 16));
            break;
        case ambit_log_sample_periodic_type_wristaccspeed:
            xml.writeTextElement("WristAccSpeed", QString::number((double)value->u.wristaccspeed/100.0, 'g', 16));
            break;
        case ambit_log_sample_periodic_type_bikepodspeed:
            xml.writeTextElement("BikePodSpeed", QString::number((double)value->u.bikepodspeed/100.0, 'g', 16));
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
            xml.writeTextElement("AbsPressure", QString("%1").arg(value->u.abspressure*10));
            break;
        case ambit_log_sample_periodic_type_energy:
            xml.writeTextElement("EnergyConsumption", QString::number((double)value->u.energy*6.978, 'g', 16)); // WTF is this unit!?
            break;
        case ambit_log_sample_periodic_type_temperature:
            xml.writeTextElement("Temperature", QString::number((double)value->u.temperature/10.0 + 273.15, 'g', 16));
            break;
        case ambit_log_sample_periodic_type_charge:
            xml.writeTextElement("BatteryCharge", QString::number((double)value->u.charge/100.0, 'g', 16));
            break;
        case ambit_log_sample_periodic_type_gpsaltitude:
            xml.writeTextElement("GPSAltitude", QString("%1").arg(value->u.gpsaltitude));
            break;
        case ambit_log_sample_periodic_type_gpsheading:
            xml.writeTextElement("GPSHeading", QString::number((double)value->u.gpsheading*M_PI/180/10000000, 'g', 16));
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
            QString snr = QString("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x")
                                           .arg(value->u.snr[0])
                                           .arg(value->u.snr[1])
                                           .arg(value->u.snr[2])
                                           .arg(value->u.snr[3])
                                           .arg(value->u.snr[4])
                                           .arg(value->u.snr[5])
                                           .arg(value->u.snr[6])
                                           .arg(value->u.snr[7])
                                           .arg(value->u.snr[8])
                                           .arg(value->u.snr[9])
                                           .arg(value->u.snr[10])
                                           .arg(value->u.snr[11])
                                           .arg(value->u.snr[12])
                                           .arg(value->u.snr[13])
                                           .arg(value->u.snr[14])
                                           .arg(value->u.snr[15]);
            xml.writeTextElement("SNR", snr);
            break;
        }
        case ambit_log_sample_periodic_type_noofsatellites:
            xml.writeTextElement("NumberOfSatellites", QString("%1").arg(value->u.noofsatellites));
            break;
        case ambit_log_sample_periodic_type_sealevelpressure:
            xml.writeTextElement("SeaLevelPressure", QString("%1").arg(value->u.sealevelpressure*10));
            break;
        case ambit_log_sample_periodic_type_verticalspeed:
            xml.writeTextElement("VerticalSpeed", QString::number((double)value->u.verticalspeed/100.0, 'g', 16));
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

    xml.writeTextElement("Time", QString::number((double)sample->time/1000.0, 'g', 16));
    xml.writeTextElement("SampleType", "periodic");
    QDateTime dateTime(QDate(sample->utc_time.year, sample->utc_time.month, sample->utc_time.day), QTime(sample->utc_time.hour, sample->utc_time.minute, 0).addMSecs(sample->utc_time.msec));
    dateTime.setTimeSpec(Qt::UTC);
    xml.writeTextElement("UTC", dateTimeString(dateTime));

    return true;
}

QString MovesCountXML::XMLWriter::dateTimeString(QDateTime &dateTime)
{
    if (dateTime.time().msec() != 0) {
        return dateTime.toString("yyyy-MM-ddThh:mm:ss.zzzZ");
    }
    else {
        return dateTime.toString("yyyy-MM-ddThh:mm:ssZ");
    }
}

QList<int> MovesCountXML::XMLWriter::rearrangeSamples()
{
    QList<int> sampleList;
    u_int32_t index = 0;
    u_int32_t lastMatching = 0;

    while (index < logEntry->logEntry->samples_count) {
        // First find all entries with the same time
        lastMatching = index;
        while (lastMatching + 1 < logEntry->logEntry->samples_count &&
               logEntry->logEntry->samples[index].time == logEntry->logEntry->samples[lastMatching+1].time) {
            lastMatching++;
        }

        // Put start/stop at top
        for (u_int32_t i=index; i<=lastMatching; i++) {
            if (logEntry->logEntry->samples[i].type == ambit_log_sample_type_lapinfo &&
                (logEntry->logEntry->samples[i].u.lapinfo.event_type == 0x1e ||
                 logEntry->logEntry->samples[i].u.lapinfo.event_type == 0x1f)) {
                sampleList.append(i);
            }
        }

        // Then any gps-entries
        for (u_int32_t i=index; i<=lastMatching; i++) {
            if (logEntry->logEntry->samples[i].type == ambit_log_sample_type_gps_base ||
                logEntry->logEntry->samples[i].type == ambit_log_sample_type_gps_small ||
                logEntry->logEntry->samples[i].type == ambit_log_sample_type_gps_tiny) {
                sampleList.append(i);
            }
        }

        // Then periodic samples
        for (u_int32_t i=index; i<=lastMatching; i++) {
            if (logEntry->logEntry->samples[i].type == ambit_log_sample_type_periodic) {
                sampleList.append(i);
            }
        }

        // Then any other!
        for (u_int32_t i=index; i<=lastMatching; i++) {
            if (logEntry->logEntry->samples[i].type != ambit_log_sample_type_gps_base &&
                logEntry->logEntry->samples[i].type != ambit_log_sample_type_gps_small &&
                logEntry->logEntry->samples[i].type != ambit_log_sample_type_gps_tiny &&
                logEntry->logEntry->samples[i].type != ambit_log_sample_type_periodic &&
                !(logEntry->logEntry->samples[i].type == ambit_log_sample_type_lapinfo &&
                    (logEntry->logEntry->samples[i].u.lapinfo.event_type == 0x1e ||
                     logEntry->logEntry->samples[i].u.lapinfo.event_type == 0x1f))) {
                sampleList.append(i);
            }
        }

        index = lastMatching + 1;
    }

    return sampleList;
}
