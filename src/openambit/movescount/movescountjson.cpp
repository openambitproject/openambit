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
#include "movescountjson.h"

#include <QRegExp>
#include <QVariantMap>
#include <QVariantList>
#include <qjson/parser.h>
#include <qjson/serializer.h>
#include <zlib.h>
#include <math.h>

MovesCountJSON::MovesCountJSON(QObject *parent) :
    QObject(parent)
{
}

int MovesCountJSON::parseFirmwareVersionReply(QByteArray &input, u_int8_t fw_version[4])
{
    QJson::Parser parser;
    bool ok;
    QRegExp rx("([0-9]+)\\.([0-9]+)\\.([0-9]+)");

    if (input.length() <= 0) {
        return -1;
    }

    QVariantMap result = parser.parse(input, &ok).toMap();

    if (ok && result["LatestFirmwareVersion"].toString().length() > 0) {
        if (rx.indexIn(result["LatestFirmwareVersion"].toString()) >= 0) {
            fw_version[0] = rx.cap(1).toInt();
            fw_version[1] = rx.cap(2).toInt();
            fw_version[2] = rx.cap(3).toInt() & 0xff;
            fw_version[3] = (rx.cap(3).toInt() >> 8) & 0xff;
            return 0;
        }
    }

    return -1;
}

int MovesCountJSON::parseLogReply(QByteArray &input, QString &moveId)
{
    QJson::Parser parser;
    bool ok;

    if (input.length() <= 0) {
        return -1;
    }

    QVariantMap result = parser.parse(input, &ok).toMap();

    if (ok && result["MoveID"].toString().length() > 0 && result["MoveID"].toString() != "0") {
        moveId = result["MoveID"].toString();
        return 0;
    }

    return -1;
}

int MovesCountJSON::parseLogDirReply(QByteArray &input, QList<MovesCountLogDirEntry> &entries)
{
    QJson::Parser parser;
    QVariantList logList;

    bool ok;

    if (input.length() <= 0) {
        return -1;
    }

    logList = parser.parse(input, &ok).toList();

    if (ok) {
        foreach(QVariant entryVar, logList) {
            QVariantMap entry = entryVar.toMap();
            entries.append(MovesCountLogDirEntry(entry["MoveID"].toString(), QDateTime::fromString(entry["LocalStartTime"].toString(), "yyyy-MM-ddThh:mm:ss.zzz"), entry["ActivityID"].toUInt()));
        }

        return 0;
    }

    return -1;
}

int MovesCountJSON::generateLogData(LogEntry *logEntry, QByteArray &output)
{
    QJson::Serializer serializer;
    bool ok;
    QVariantMap content;
    QVariantList IBIContent;
    QVariantList marksContent;
    QVariantList periodicSamplesContent;
    QVariantList GPSSamplesContent;
    QByteArray uncompressedData, compressedData;
    ambit_log_sample_t *sample;

    QDateTime localBaseTime(QDate(logEntry->logEntry->header.date_time.year,
                                  logEntry->logEntry->header.date_time.month,
                                  logEntry->logEntry->header.date_time.day),
                            QTime(logEntry->logEntry->header.date_time.hour,
                                  logEntry->logEntry->header.date_time.minute, 0).addMSecs(logEntry->logEntry->header.date_time.msec));

    // Loop through content
    QList<int> order = rearrangeSamples(logEntry);
    foreach(int index, order) {
        sample = &logEntry->logEntry->samples[index];

        switch(sample->type) {
        case ambit_log_sample_type_periodic:
        {
            QVariantMap tmpMap;
            tmpMap.insert("LocalTime", dateTimeString(localBaseTime.addMSecs(sample->time)));
            writePeriodicSample(sample, tmpMap);
            periodicSamplesContent.append(tmpMap);
            break;
        }
        case ambit_log_sample_type_ibi:
            for (int j=0; j<sample->u.ibi.ibi_count; j++) {
                IBIContent.append(sample->u.ibi.ibi[j]);
            }
            break;
        case ambit_log_sample_type_gps_base:
        {
            QVariantMap tmpMap;
            tmpMap.insert("Altitude", (double)sample->u.gps_base.altitude/100.0);
            tmpMap.insert("EHPE", (double)sample->u.gps_base.ehpe/100.0);
            tmpMap.insert("Latitude", (double)sample->u.gps_base.latitude/10000000);
            tmpMap.insert("LocalTime", dateTimeString(localBaseTime.addMSecs(sample->time)));
            tmpMap.insert("Longitude", (double)sample->u.gps_base.longitude/10000000);
            GPSSamplesContent.append(tmpMap);
            break;
        }
        case ambit_log_sample_type_gps_small:
        {
            QVariantMap tmpMap;
            tmpMap.insert("Altitude", (double)0);
            tmpMap.insert("EHPE", (double)sample->u.gps_small.ehpe/100.0);
            tmpMap.insert("Latitude", (double)sample->u.gps_small.latitude/10000000);
            tmpMap.insert("LocalTime", dateTimeString(localBaseTime.addMSecs(sample->time)));
            tmpMap.insert("Longitude", (double)sample->u.gps_small.longitude/10000000);
            GPSSamplesContent.append(tmpMap);
            break;
        }
        case ambit_log_sample_type_gps_tiny:
        {
            QVariantMap tmpMap;
            tmpMap.insert("Altitude", (double)0);
            tmpMap.insert("EHPE", (double)sample->u.gps_tiny.ehpe/100.0);
            tmpMap.insert("Latitude", (double)sample->u.gps_tiny.latitude/10000000);
            tmpMap.insert("LocalTime", dateTimeString(localBaseTime.addMSecs(sample->time)));
            tmpMap.insert("Longitude", (double)sample->u.gps_tiny.longitude/10000000);
            GPSSamplesContent.append(tmpMap);
            break;
        }
        case ambit_log_sample_type_lapinfo:
            switch (sample->u.lapinfo.event_type) {
            case 0x00: /* autolap = 5 */
            {
                QVariantMap tmpMap;
                tmpMap.insert("LocalTime", dateTimeString(localBaseTime.addMSecs(sample->time)));
                tmpMap.insert("Type", 5);
                marksContent.append(tmpMap);
                break;
            }
            case 0x01: /* manual = 0 */
            case 0x16: /* interval = 0 */
            {
                QVariantMap tmpMap;
                tmpMap.insert("LocalTime", dateTimeString(localBaseTime.addMSecs(sample->time)));
                tmpMap.insert("Type", 0);
                marksContent.append(tmpMap);
                break;
            }
            case 0x1f: /* start = 1 */
            {
                QVariantMap tmpMap;
                tmpMap.insert("LocalTime", dateTimeString(localBaseTime.addMSecs(sample->time)));
                if (sample->time > 0) {
                    tmpMap.insert("Type", 1);
                }
                marksContent.append(tmpMap);
                break;
            }
            case 0x1e: /* pause = 2 */
            {
                QVariantMap tmpMap;
                tmpMap.insert("LocalTime", dateTimeString(localBaseTime.addMSecs(sample->time)));
                tmpMap.insert("Type", 2);
                marksContent.append(tmpMap);
                break;
            }
            case 0x14: /* high interval = 3 */
            case 0x15: /* low interval = 3 */
            {
                QVariantMap tmpMap;
                tmpMap.insert("LocalTime", dateTimeString(localBaseTime.addMSecs(sample->time)));
                tmpMap.insert("Type", 3);
                marksContent.append(tmpMap);
                break;
            }
            };
            break;
        case ambit_log_sample_type_activity:
        {
            QVariantMap tmpMap;
            tmpMap.insert("LocalTime", dateTimeString(localBaseTime.addMSecs(sample->time)));
            tmpMap.insert("NextActivityID", sample->u.activity.activitytype);
            tmpMap.insert("Type", 8);
            marksContent.append(tmpMap);
            break;
        }
        default:
            break;
        }
    }

    serializer.setDoublePrecision(16);
    serializer.setIndentMode(QJson::IndentCompact);

    content.insert("ActivityID", logEntry->logEntry->header.activity_type);
    content.insert("AscentAltitude", (double)logEntry->logEntry->header.ascent);
    content.insert("AscentTime", (double)logEntry->logEntry->header.ascent_time/1000.0);
    content.insert("AvgCadence", logEntry->logEntry->header.cadence_avg);
    content.insert("AvgHR", logEntry->logEntry->header.heartrate_avg);
    content.insert("AvgSpeed", (double)logEntry->logEntry->header.speed_avg/3600.0);
    content.insert("DescentAltitude", (double)logEntry->logEntry->header.descent);
    content.insert("DescentTime", (double)logEntry->logEntry->header.descent_time/1000.0);
    content.insert("DeviceName", logEntry->deviceInfo->model);
    content.insert("DeviceSerialNumber", logEntry->deviceInfo->serial);
    content.insert("Distance", logEntry->logEntry->header.distance);
    content.insert("Duration", (double)logEntry->logEntry->header.duration/1000.0);
    content.insert("Energy", logEntry->logEntry->header.energy_consumption);
    content.insert("FlatTime", QVariant::Invalid);
    if (logEntry->logEntry->header.altitude_max >= -1000 && logEntry->logEntry->header.altitude_max <= 10000) {
        content.insert("HighAltitude", (double)logEntry->logEntry->header.altitude_max);
    }
    else {
        content.insert("HighAltitude", QVariant::Invalid);
    }
    if (IBIContent.count() > 0) {
        uncompressedData = serializer.serialize(IBIContent);
        compressData(uncompressedData, compressedData);
        QVariantMap IBIDataMap;
        IBIDataMap.insert("CompressedValues", compressedData.toBase64());
        content.insert("IBIData", IBIDataMap);                        /* compressed */
    }
    content.insert("LocalStartTime", dateTimeString(localBaseTime));
    if (logEntry->logEntry->header.altitude_min >= -1000 && logEntry->logEntry->header.altitude_min <= 10000)
        content.insert("LowAltitude", (double)logEntry->logEntry->header.altitude_min);
    content.insert("Marks", marksContent);
    content.insert("MaxCadence", logEntry->logEntry->header.cadence_max);
    content.insert("MaxSpeed", (double)logEntry->logEntry->header.speed_max/3600.0);
    if (logEntry->logEntry->header.temperature_max >= -1000 && logEntry->logEntry->header.temperature_max <= 1000) {
        content.insert("MaxTemp", (double)logEntry->logEntry->header.temperature_max/10.0);
    }
    content.insert("MinHR", logEntry->logEntry->header.heartrate_min);
    if (logEntry->logEntry->header.temperature_min >= -1000 && logEntry->logEntry->header.temperature_min <= 1000) {
        content.insert("MinTemp", (double)logEntry->logEntry->header.temperature_min/10.0);
    }
    content.insert("PeakHR", logEntry->logEntry->header.heartrate_max);
    content.insert("PeakTrainingEffect", (double)logEntry->logEntry->header.peak_training_effect/10.0);
    content.insert("RecoveryTime", (double)logEntry->logEntry->header.recovery_time/1000.0);
    if (periodicSamplesContent.count() > 0) {
        uncompressedData = serializer.serialize(periodicSamplesContent);
        compressData(uncompressedData, compressedData);
        QVariantMap periodicSamplesDataMap;
        periodicSamplesDataMap.insert("CompressedSampleSets", compressedData.toBase64());
        content.insert("Samples", periodicSamplesDataMap);            /* compressed */
    }
    content.insert("SerialNumber", QVariant::Invalid);
    content.insert("StartLatitude", QVariant::Invalid);
    content.insert("StartLongitude", QVariant::Invalid);
    if (GPSSamplesContent.count() > 0) {
        uncompressedData = serializer.serialize(GPSSamplesContent);
        compressData(uncompressedData, compressedData);
        QVariantMap GPSSamplesDataMap;
        GPSSamplesDataMap.insert("CompressedTrackPoints", compressedData.toBase64());
        content.insert("Track", GPSSamplesDataMap);                   /* compressed */
    }

    output = serializer.serialize(content, &ok);

    return (ok ? 0 : -1);
}

bool MovesCountJSON::writePeriodicSample(ambit_log_sample_t *sample, QVariantMap &output)
{
    int i;
    ambit_log_sample_periodic_value_t *value;

    for (i=0; i<sample->u.periodic.value_count; i++) {
        value = &sample->u.periodic.values[i];

        switch(value->type) {
        case ambit_log_sample_periodic_type_latitude:
            output.insert("Latitude", (double)value->u.latitude/10000000);
            break;
        case ambit_log_sample_periodic_type_longitude:
            output.insert("Longitude", (double)value->u.longitude/10000000);
            break;
        case ambit_log_sample_periodic_type_distance:
            if (value->u.distance != 0xffffffff) {
                output.insert("Distance", value->u.distance);
            }
            break;
        case ambit_log_sample_periodic_type_speed:
            if (value->u.speed != 0xffff) {
                output.insert("Speed", (double)value->u.speed/100.0);
            }
            break;
        case ambit_log_sample_periodic_type_hr:
            if (value->u.hr != 0xff) {
                output.insert("HeartRate", value->u.hr);
            }
            break;
        case ambit_log_sample_periodic_type_time:
            output.insert("Time", (double)value->u.time/1000.0);
            break;
        case ambit_log_sample_periodic_type_gpsspeed:
            if (value->u.gpsspeed != 0xffff) {
                output.insert("GPSSpeed", (double)value->u.gpsspeed/100.0);
            }
            break;
        case ambit_log_sample_periodic_type_wristaccspeed:
            if (value->u.wristaccspeed != 0xffff) {
                output.insert("WristAccSpeed", (double)value->u.wristaccspeed/100.0);
            }
            break;
        case ambit_log_sample_periodic_type_bikepodspeed:
            if (value->u.bikepodspeed != 0xffff) {
                output.insert("BikePodSpeed", (double)value->u.bikepodspeed/100.0);
            }
            break;
        case ambit_log_sample_periodic_type_ehpe:
            output.insert("EHPE", value->u.ehpe);
            break;
        case ambit_log_sample_periodic_type_evpe:
            output.insert("EVPE", value->u.evpe);
            break;
        case ambit_log_sample_periodic_type_altitude:
            if (value->u.altitude >= -1000 && value->u.altitude <= 10000) {
                output.insert("Altitude", (double)value->u.altitude);
            }
            break;
        case ambit_log_sample_periodic_type_abspressure:
            output.insert("AbsPressure", (int)round((double)value->u.abspressure/10.0));
            break;
        case ambit_log_sample_periodic_type_energy:
            if (value->u.energy) {
                output.insert("EnergyConsumption", (double)value->u.energy/10.0);
            }
            break;
        case ambit_log_sample_periodic_type_temperature:
            if (value->u.temperature >= -1000 && value->u.temperature <= 1000) {
                output.insert("Temperature", (double)value->u.temperature/10.0);
            }
            break;
        case ambit_log_sample_periodic_type_charge:
            if (value->u.charge <= 100) {
                output.insert("BatteryCharge", (double)value->u.charge/100.0);
            }
            break;
        case ambit_log_sample_periodic_type_gpsaltitude:
            if (value->u.gpsaltitude >= -1000 && value->u.gpsaltitude <= 10000) {
                output.insert("GPSAltitude", value->u.gpsaltitude);
            }
            break;
        case ambit_log_sample_periodic_type_gpsheading:
            if (value->u.gpsheading != 0xffff) {
                output.insert("GPSHeading", (double)value->u.gpsheading/10000000);
            }
            break;
        case ambit_log_sample_periodic_type_gpshdop:
            if (value->u.gpshdop != 0xff) {
                output.insert("GpsHDOP", value->u.gpshdop);
            }
            break;
        case ambit_log_sample_periodic_type_gpsvdop:
            if (value->u.gpsvdop != 0xff) {
                output.insert("GpsVDOP", value->u.gpsvdop);
            }
            break;
        case ambit_log_sample_periodic_type_wristcadence:
            if (value->u.wristcadence != 0xffff) {
                output.insert("WristCadence", value->u.wristcadence);
            }
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
            output.insert("SNR", snr);
            break;
        }
        case ambit_log_sample_periodic_type_noofsatellites:
            if (value->u.noofsatellites != 0xff) {
                output.insert("NumberOfSatellites", value->u.noofsatellites);
            }
            break;
        case ambit_log_sample_periodic_type_sealevelpressure:
            output.insert("SeaLevelPressure", (int)round((double)value->u.sealevelpressure/10.0));
            break;
        case ambit_log_sample_periodic_type_verticalspeed:
            output.insert("VerticalSpeed", (double)value->u.verticalspeed/100.0);
            break;
        case ambit_log_sample_periodic_type_cadence:
            if (value->u.cadence != 0xff) {
                output.insert("Cadence", value->u.cadence);
            }
            break;
        case ambit_log_sample_periodic_type_bikepower:
            if (value->u.bikepower != 0xffff) {
                output.insert("BikePower", value->u.bikepower);
            }
            break;
        case ambit_log_sample_periodic_type_swimingstrokecnt:
            output.insert("SwimmingStrokeCount", value->u.swimingstrokecnt);
            break;
        case ambit_log_sample_periodic_type_ruleoutput1:
            if (value->u.ruleoutput1 != -2147483648) { /* 0xffffffff */
                output.insert("RuleOutput1", value->u.ruleoutput1);
            }
            break;
        case ambit_log_sample_periodic_type_ruleoutput2:
            if (value->u.ruleoutput2 != -2147483648) { /* 0xffffffff */
                output.insert("RuleOutput2", value->u.ruleoutput2);
            }
            break;
        case ambit_log_sample_periodic_type_ruleoutput3:
            if (value->u.ruleoutput3 != -2147483648) { /* 0xffffffff */
                output.insert("RuleOutput3", value->u.ruleoutput3);
            }
            break;
        case ambit_log_sample_periodic_type_ruleoutput4:
            if (value->u.ruleoutput4 != -2147483648) { /* 0xffffffff */
                output.insert("RuleOutput4", value->u.ruleoutput4);
            }
            break;
        case ambit_log_sample_periodic_type_ruleoutput5:
            if (value->u.ruleoutput5 != -2147483648) { /* 0xffffffff */
                output.insert("RuleOutput5", value->u.ruleoutput5);
            }
            break;
        }
    }

    return true;
}

int MovesCountJSON::compressData(QByteArray &content, QByteArray &output)
{
    int ret = -1, res, deflate_res;
    z_stream strm;
    gz_header header;
    size_t destLen = compressBound(content.length());

    memset(&strm, 0, sizeof(z_stream));
    memset(&header, 0, sizeof(gz_header));

    if (destLen > 0) {
        u_int8_t *buf = (u_int8_t*)malloc(destLen);
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        header.os = 0x00; // FAT

        res = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY);
        res = deflateSetHeader(&strm, &header);

        strm.next_in = reinterpret_cast<uint8_t *>(content.data());
        strm.avail_in = content.length();
        strm.next_out = buf;
        strm.avail_out = destLen;

        res = deflate(&strm, Z_NO_FLUSH);

        if (res == Z_OK) {
            deflate_res = Z_OK;
            while (deflate_res == Z_OK) {
                deflate_res = deflate(&strm, Z_FINISH);
            }

            if (deflate_res == Z_STREAM_END) {
                output.setRawData((char*)buf, destLen - strm.avail_out);
                ret = 0;
            }
        }

        deflateEnd(&strm);
    }

    return ret;
}

QList<int> MovesCountJSON::rearrangeSamples(LogEntry *logEntry)
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

QString MovesCountJSON::dateTimeString(QDateTime dateTime)
{
    if (dateTime.time().msec() != 0) {
        return dateTime.toString("yyyy-MM-ddThh:mm:ss.zzz");
    }
    else {
        return dateTime.toString("yyyy-MM-ddThh:mm:ss");
    }
}
