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
#include "movescount.h"
#include "movescountjson.h"

#include <QRegExp>
#include <QVariantMap>
#include <QStringList>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <zlib.h>
#include <cmath>
#include <cstdio>

MovesCountJSON::MovesCountJSON(QObject *parent) :
    QObject(parent)
{
}

int MovesCountJSON::parseFirmwareVersionReply(QByteArray &input, u_int8_t fw_version[3])
{
    QRegExp rx("([0-9]+)\\.([0-9]+)\\.([0-9]+)");

    if (input.length() <= 0) {
        return -1;
    }

    bool ok = false;
    QVariantMap result = parseJsonMap(input, ok);

    if (ok && result["LatestFirmwareVersion"].toString().length() > 0) {
        if (rx.indexIn(result["LatestFirmwareVersion"].toString()) >= 0) {
            fw_version[0] = rx.cap(1).toInt();
            fw_version[1] = rx.cap(2).toInt();
            fw_version[2] = rx.cap(3).toInt();
            return 0;
        }
    }

    return -1;
}

int MovesCountJSON::parseLogReply(QByteArray &input, QString &moveId)
{
    if (input.length() <= 0) {
        return -1;
    }

    bool ok = false;
    QVariantMap result = parseJsonMap(input, ok);

    if (ok && result["MoveID"].toString().length() > 0 && result["MoveID"].toString() != "0") {
        moveId = result["MoveID"].toString();
        return 0;
    }

    return -1;
}

int MovesCountJSON::parsePersonalSettings(QByteArray &input, ambit_personal_settings_t *ps, MovesCount *movescount)
{

    if (input.length() <= 0) {
        return -1;
    }

    bool ok = false;
    QVariantMap result = parseJsonMap(input, ok);

	 if(!ok) {
        return -1;
    }

    //Empty waypoints in personal_settings (ps)
    ps->waypoints.count = 0;
    if(ps->waypoints.data != NULL) free(ps->waypoints.data);

    if(ps->routes.count == 0 && result["RouteURIs"].type() == QVariant::String) {
        QStringList routes = result["RouteURIs"].toString().split(',');
        ps->routes.count = routes.size();
        ps->routes.data  = libambit_route_alloc(ps->routes.count);
        for(int x=0; x<routes.size(); x++) {
            movescount->getRoute(&(ps->routes.data[x]), ps, routes.value(x));
        }

        //Sort routes on name
        QStringList qslist_route_name;
        uint16_t sort_offset = 0;
        ambit_route_t *routes_sorted = (ambit_route_t*)calloc(ps->routes.count,sizeof(ambit_route_t));

        for(int x=0; x < ps->routes.count;++x) {
            qslist_route_name.append(QString(ps->routes.data[x].name));
        }

        qslist_route_name.sort();
        for(int k=0, s=qslist_route_name.size(), max=(s/2); k<max; k++) qslist_route_name.swap(k,s-(1+k));

        for(int x=0; x < qslist_route_name.size(); ++x) {
            for(int y = 0; y < ps->routes.count;++y) {
                if(qslist_route_name.at(x).compare(QString(ps->routes.data[y].name)) == 0) {
                    routes_sorted[sort_offset++] = ps->routes.data[y];
                    break;
                }
            }
        }

        free(ps->routes.data);
        ps->routes.data = routes_sorted;

    }

    if(result["Waypoints"].type() == QVariant::List && result["Waypoints"].toList().count()>0) {
        ambit_waypoint_t *waypoints_to_append = NULL;
        uint8_t num_to_append = (uint8_t)result["Waypoints"].toList().count();


        waypoints_to_append = (ambit_waypoint_t*)malloc(sizeof(ambit_waypoint_t)*num_to_append);

        for(int x=0; x<num_to_append; x++) {
            QDateTime dt;
            QVariantMap jsonWp = result["Waypoints"].toList().at(x).toMap();
            waypoints_to_append[x].altitude = (uint16_t)jsonWp["Altitude"].toInt();
            waypoints_to_append[x].longitude = (uint32_t)(jsonWp["Longitude"].toFloat()*10000000);
            waypoints_to_append[x].latitude = (uint32_t)(jsonWp["Latitude"].toFloat()*10000000);
            waypoints_to_append[x].type = (uint8_t)(jsonWp["Type"].toInt());
            this->copyDataString(jsonWp["Name"],waypoints_to_append[x].name, 49);
            strncpy(waypoints_to_append[x].route_name, "", 49);
            dt = QDateTime::fromString(jsonWp["CreationLocalTime"].toString(), Qt::ISODate);
            waypoints_to_append[x].ctime_second = (uint8_t)dt.toString("s").toInt();
            waypoints_to_append[x].ctime_minute = (uint8_t)dt.toString("m").toInt();
            waypoints_to_append[x].ctime_hour = (uint8_t)dt.toString("h").toInt();
            waypoints_to_append[x].ctime_day = (uint8_t)dt.toString("d").toInt();
            waypoints_to_append[x].ctime_month = (uint8_t)dt.toString("M").toInt();
            waypoints_to_append[x].ctime_year = (uint16_t)dt.toString("yyyy").toInt();
        }

        libambit_waypoint_append(ps, waypoints_to_append, num_to_append);
        free(waypoints_to_append);
    }

    //Sort waypoints on route_name
    QStringList qslist_route_name;
    QString last = "";
    uint16_t sort_offset = 0;
    ambit_waypoint_t *wp_sorted = (ambit_waypoint_t*)calloc(ps->waypoints.count,sizeof(ambit_waypoint_t));

    for(int x=0; x < ps->waypoints.count;++x) {
        if(x == 0 || last.compare(QString(ps->waypoints.data[x].route_name)) !=0 ) {
            last = QString(ps->waypoints.data[x].route_name);
            qslist_route_name.append(last);
        }
    }

    qslist_route_name.sort();

    for(int x=0; x < qslist_route_name.size(); ++x) {
        for(int y = 0; y < ps->waypoints.count;++y) {
            if(qslist_route_name.at(x).compare(QString(ps->waypoints.data[y].route_name)) == 0) {
                wp_sorted[sort_offset++] = ps->waypoints.data[y];
            }
        }
    }

    free(ps->waypoints.data);
    ps->waypoints.data = wp_sorted;

    return 0;
}

bool MovesCountJSON::copyDataString(const QVariant& entry, char *data, size_t maxlength)
{
    QByteArray ba=entry.toString().toLatin1();
    strncpy(data, ba.data(), maxlength);
    return true;
}

int MovesCountJSON::parseRoute(QByteArray &input, ambit_route_t *route, ambit_personal_settings_t *ps, MovesCount *movescount)
{
    if (input.length() <= 0) {
        return -1;
    }

    bool ok = false;
    QVariantMap result = parseJsonMap(input, ok);
    QString name = "";

	 if(!ok) {
        return -1;
    }

    if(result["RoutePointsCount"].type() == QVariant::ULongLong) {
        route->points_count = (uint16_t)result["RoutePointsCount"].toInt();
    }

    route->id = result["RouteID"].toUInt();
    strncpy(route->name, result["Name"].toString().toLatin1().data(),49);
    route->waypoint_count = result["WaypointCount"].toUInt();

    int ret;

    if(result["RoutePointsURI"].type() == QVariant::String) {
        if((ret = movescount->getRoutePoints(route, ps, result["RoutePointsURI"].toString())) < 2) {
            if(route->points != NULL) {
                free(route->points);
                route->points = NULL;
            }
            route->points_count = 0;
            return -1;
        }

        route->points_count = ret;
    } else {
        return -1;
    }

    route->activity_id = result["ActivityID"].toUInt();
    route->altitude_dec = result["DescentAltitude"].toUInt();
    route->altitude_asc= result["AscentAltitude"].toUInt();
    route->distance = result["Distance"].toUInt();

    //TODO: Remove
    //libambit_debug_route_print(route);

    return (int)route->points_count;
}

int MovesCountJSON::parseRoutePoints(QByteArray &input, ambit_route_t *route, ambit_personal_settings_t *ps)
{
    //Todo: Implement compressed and json routepoints. It looks like yo have to tell the uiservices-api to use
    //      the other format specific, so no hurry to implement.
    int ret = -1;

    if (input.length() <= 0) {
        return -1;
    }

    bool ok = false;
    QVariantMap result = parseJsonMap(input, ok);
    QVariantList jsonRoutePoints;
    int32_t clat = 0, clon=0;
    uint16_t cur_waypoint_count = 0;

    if(!ok) {
        return -1;
    }

    if(result["CompressedRoutePoints"].type() == QVariant::String && result["CompressedRoutePoints"].toString() != "") {
        qDebug() << "Is Compressed data";
        //gunzip data
        return -2; //Should not be possible
    }

    if(result["RoutePoints"].type() == QVariant::List && result["RoutePoints"].toList().size() > 1) {
        jsonRoutePoints = result["RoutePoints"].toList();
    }

    if(jsonRoutePoints.size()>1) {

        route->points = (ambit_routepoint_t*)malloc(jsonRoutePoints.size()*sizeof(ambit_routepoint_t));
        for(int x=0; x<jsonRoutePoints.size(); ++x) {
            QVariantMap jCurrent = jsonRoutePoints.at(x).toMap();
            clat = (int32_t)(jCurrent["Latitude"].toDouble()*10000000);
            clon = (int32_t)(jCurrent["Longitude"].toDouble()*10000000);
            appendRoutePoint(route, x, clat, clon, jCurrent["Altitude"].toInt(), (uint32_t)(jCurrent["RelativeDistance"].toDouble()*100000));

            if(jCurrent["Name"].toString() != "") {
                QByteArray ba1 = jCurrent["Name"].toString().toLatin1();
                appendWaypoint(cur_waypoint_count, ps, route->name, ba1.data(), clat, clon, jCurrent["Altitude"].toInt(), jCurrent["Type"].toInt());
                ++cur_waypoint_count;
            }
        }

        ret = jsonRoutePoints.size();

    } else if (result["Points"].type() == QVariant::String && result["Points"].toString() != "") {
        /* This format does not support waypoints */
        QStringList strPointsList = result["Points"].toString().split(';');
        route->points = (ambit_routepoint_t*)malloc(strPointsList.size()*sizeof(ambit_routepoint_t));
        for(int x = 0; x<strPointsList.size();++x) {
            QStringList strPointArray = strPointsList.at(x).split(',');
            clat = (int32_t)(strPointArray.at(0).toDouble()*10000000);
            clon = (int32_t)(strPointArray.at(1).toDouble()*10000000);
            appendRoutePoint(route, x, clat, clon, strPointArray.at(2).toInt(), (uint32_t)strPointArray.at(3).toDouble()*100000);
        }
        ret = strPointsList.size();
    } else {
        return -1;
    }

    route->end_lat = route->points[(ret-1)].lat;
    route->end_lon = route->points[(ret-1)].lon;

    route->mid_lat = route->max_lat - (route->max_lat - route->min_lat)/2;
    route->mid_lon = route->max_lon - (route->max_lon - route->min_lon)/2;

    if(cur_waypoint_count == 0) {
        appendWaypoint(cur_waypoint_count++, ps, route->name, (char*)"A", route->start_lat, route->start_lon, route->points[0].altitude, movescount_waypoint_type_internal_wp_start);
        appendWaypoint(cur_waypoint_count, ps, route->name, (char*)"B", route->end_lat, route->end_lon, route->points[(ret-1)].altitude, movescount_waypoint_type_internal_wp_end);
    }

    return ret;
}

bool MovesCountJSON::appendRoutePoint(ambit_route_t *route, int point_number, int32_t lat, int32_t lon, int32_t altitude, uint32_t distance)
{
    if(point_number == 0) {
        route->start_lat = lat;
        route->start_lon = lon;
        route->max_lat = route->start_lat;
        route->min_lat = route->start_lat;
        route->max_lon = route->start_lon;
        route->min_lon = route->start_lon;
    }

    //This is needed to calculate route->mid_lat and route->mid_lon
    if(lat > route->max_lat) route->max_lat = lat;
    if(lat < route->min_lat) route->min_lat = lat;
    if(lon > route->max_lon) route->max_lon = lon;
    if(lon < route->min_lon) route->min_lon = lon;

    route->points[point_number].lat = lat;
    route->points[point_number].lon = lon;
    route->points[point_number].altitude = altitude;
    route->points[point_number].distance = distance;

    return true;
}

bool MovesCountJSON::appendWaypoint(uint16_t count, ambit_personal_settings_t *ps, char *route_name, char *waypoint_name, int32_t lat, int32_t lon, uint16_t altitude, uint8_t type) {

    ambit_waypoint_t wp;
    wp.index = 0;
    strncpy(wp.name, waypoint_name, 49);
    strncpy(wp.route_name, route_name, 49);
    wp.ctime_second = count;
    wp.ctime_minute = wp.ctime_hour = 0;
    wp.ctime_day = wp.ctime_month = 1;
    wp.ctime_year = 1980;
    wp.latitude = lat;
    wp.longitude = lon;
    wp.altitude = altitude;
    wp.type = type;
    wp.status = 0;
    libambit_waypoint_append(ps, &wp, 1);
    return true;
}

int MovesCountJSON::parseLogDirReply(QByteArray &input, QList<MovesCountLogDirEntry> &entries)
{
    if (input.length() <= 0) {
        return -1;
    }

    bool ok = false;
    QVariantMap logList = parseJsonMap(input, ok);

    if (ok) {
        foreach(QVariant entryVar, logList) {
            QVariantMap entry = entryVar.toMap();
            entries.append(MovesCountLogDirEntry(entry["MoveID"].toString(), QDateTime::fromString(entry["LocalStartTime"].toString(), "yyyy-MM-ddThh:mm:ss.zzz"), entry["ActivityID"].toUInt()));
        }

        return 0;
    }

    return -1;
}

int MovesCountJSON::generateNewPersonalSettings(ambit_personal_settings_t *settings, DeviceInfo &device_info, QByteArray &output)
{
    bool ok;
    QVariantMap content;

    content.insert("DeviceName", device_info.model);
    content.insert("FirmwareVersion", QString("%1.%2.%3")
            .arg(device_info.fw_version[0]).arg(device_info.fw_version[1])
            .arg(device_info.fw_version[2]));
    content.insert("HardwareVersion", QString("%1.%2.%3")
            .arg(device_info.hw_version[0]).arg(device_info.hw_version[1])
            .arg(device_info.hw_version[2]));
    content.insert("SerialNumber", device_info.serial);

    QVariantList waypoints;

    if(settings->waypoints.count>0) {

        QVariantMap c_waypoint;

        for(int x=0; x < settings->waypoints.count; x++) {
            if(settings->waypoints.data[x].route_name[0] == '\0') {
                c_waypoint.clear();
                c_waypoint.insert("Altitude", settings->waypoints.data[x].altitude);
                c_waypoint.insert("CreationLocalTime", QString("%1-%2-%3T%4:%5:%6.0")
                        .arg(settings->waypoints.data[x].ctime_year, 4, 10, QLatin1Char('0'))
                        .arg(settings->waypoints.data[x].ctime_month, 2, 10, QLatin1Char('0'))
                        .arg(settings->waypoints.data[x].ctime_day, 2, 10, QLatin1Char('0'))
                        .arg(settings->waypoints.data[x].ctime_hour, 2, 10, QLatin1Char('0'))
                        .arg(settings->waypoints.data[x].ctime_minute, 2, 10, QLatin1Char('0'))
                        .arg(settings->waypoints.data[x].ctime_second, 2, 10, QLatin1Char('0'))
                        );
                c_waypoint.insert("Latitude",  (double)settings->waypoints.data[x].latitude/10000000);
                c_waypoint.insert("Longitude",  (double)settings->waypoints.data[x].longitude/10000000);
                c_waypoint.insert("Name", QString::fromLatin1(settings->waypoints.data[x].name));
                c_waypoint.insert("Type", settings->waypoints.data[x].type);
                waypoints.append(c_waypoint);

            }
        }
    }

    content.insert("Waypoints", waypoints);

    output = QJsonDocument(QJsonObject::fromVariantMap(content)).toJson(QJsonDocument::Compact);
    ok = !output.isEmpty();

    return (ok ? 0 : -1);
}

int MovesCountJSON::parseDeviceSettingsReply(QByteArray &input, MovescountSettings &movescountSettings)
{
    bool ok;
    QVariantMap entry = parseJsonMap(input, ok);

    if (ok) {
        QVariantMap settingsMap = entry["Settings"].toMap();
        movescountSettings.parse(settingsMap);

        return 0;
    }

    return -1;
}

int MovesCountJSON::parseAppRulesReply(QByteArray &input, ambit_app_rules_t* ambitApps)
{
    bool ok;
    QVariantList appRulesList = parseJsonList(input, ok);

    if (ok) {

        QList<uint> appRulesId;
        QList<QByteArray> appRulesData;
        foreach(QVariant entryVar, appRulesList) {
            QVariantMap entry = entryVar.toMap();
            appRulesId.append(entry["RuleID"].toUInt());
            QByteArray binary;
            foreach (QVariant binaryVar, entry["Binary"].toList()) {
                binary.append(binaryVar.toInt(&ok));
                if(!ok) {
                    return -1;
                }
            }
            appRulesData.append(binary);
        }

        if (libambit_malloc_app_rule(appRulesId.count(), ambitApps)) {
            int i = 0;
            for (i=0; i<appRulesId.count(); i++) {
                ambitApps->app_rules[i].app_id = appRulesId.at(i);
                ambitApps->app_rules[i].app_rule_data_length = appRulesData.at(i).count();

                ambitApps->app_rules[i].app_rule_data = (uint8_t*)malloc(appRulesData.at(i).count());
                if (ambitApps->app_rules[i].app_rule_data != NULL) {
                    memcpy(ambitApps->app_rules[i].app_rule_data, appRulesData.at(i).data(), appRulesData.at(i).count());
                }
            }
            return 0;
        }
    }

    return -1;
}

/**
 * @brief MovesCountJSON::generateLogData
 * @param logEntry
 * @param output
 * @return
 * @note Fucked up facts about movescount:
 *  - The periodic samples timestamps are truncated to 10th of milliseconds by movescount
 *  - That would be fine, if it wasn't for the swimming logs where the periodic entries
 *    are matched to the time of entries in the marksContent list (which has ms precision).
 *  - Some of the entries that should match in marksContent are virtual created entries,
 *    generated here. This can lead to time collisions.
 *  - To compensate for the collisions, samples might need to be shifted in time,
 *    hence the fuzz with dateTimeCompensate
 */
int MovesCountJSON::generateLogData(LogEntry *logEntry, QByteArray &output)
{
    bool ok, inPause = false;
    QVariantMap content;
    QVariantList IBIContent;
    QVariantList marksContent;
    QVariantList periodicSamplesContent;
    QVariantList GPSSamplesContent;
    QByteArray uncompressedData, compressedData;
    ambit_log_sample_t *sample;
    QDateTime prevMarksDateTime;
    QDateTime prevPeriodicSamplesDateTime;

    QDateTime localBaseTime(QDate(logEntry->logEntry->header.date_time.year,
                                  logEntry->logEntry->header.date_time.month,
                                  logEntry->logEntry->header.date_time.day),
                            QTime(logEntry->logEntry->header.date_time.hour,
                                  logEntry->logEntry->header.date_time.minute, 0).addMSecs(logEntry->logEntry->header.date_time.msec));

    // Loop through content
    QList<int> order = rearrangeSamples(logEntry);
    for (int i=0; i<order.length(); i++) {
        sample = &logEntry->logEntry->samples[order[i]];

        switch(sample->type) {
        case ambit_log_sample_type_periodic:
        {
            QVariantMap tmpMap;
            prevPeriodicSamplesDateTime = dateTimeRound(dateTimeCompensate(dateTimeRound(localBaseTime.addMSecs(sample->time), 10), prevPeriodicSamplesDateTime, 0), 10);
            tmpMap.insert("LocalTime", dateTimeString(prevPeriodicSamplesDateTime));
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
                if (sample->time > 0) {
                    prevMarksDateTime = dateTimeCompensate(localBaseTime.addMSecs(sample->time), prevMarksDateTime, 1);
                }
                else {
                    prevMarksDateTime = localBaseTime.addMSecs(sample->time);
                }
                tmpMap.insert("LocalTime", dateTimeString(prevMarksDateTime));
                tmpMap.insert("Type", 5);
                marksContent.append(tmpMap);
                break;
            }
            case 0x01: /* manual = 0 */
            case 0x16: /* interval = 0 */
            {
                // Try to remove strange manual lap events at end after pause/stop
                // A lap during a pause doesn't make sense anyway?
                if (!inPause) {
                    QVariantMap tmpMap;
                    if (sample->time > 0) {
                        prevMarksDateTime = dateTimeCompensate(localBaseTime.addMSecs(sample->time), prevMarksDateTime, 1);
                    }
                    else {
                        prevMarksDateTime = localBaseTime.addMSecs(sample->time);
                    }
                    tmpMap.insert("LocalTime", dateTimeString(prevMarksDateTime));
                    tmpMap.insert("Type", 0);
                    marksContent.append(tmpMap);
                }
                break;
            }
            case 0x1f: /* start = 1 */
            {
                QVariantMap tmpMap;
                if (sample->time > 0) {
                    prevMarksDateTime = dateTimeCompensate(localBaseTime.addMSecs(sample->time), prevMarksDateTime, 1);
                    tmpMap.insert("LocalTime", dateTimeString(prevMarksDateTime));
                    tmpMap.insert("Type", 1);
                }
                else {
                    prevMarksDateTime = localBaseTime.addMSecs(sample->time);
                    tmpMap.insert("LocalTime", dateTimeString(prevMarksDateTime));
                }
                marksContent.append(tmpMap);

                inPause = false;
                break;
            }
            case 0x1e: /* pause = 2 */
            {
                QVariantMap tmpMap;
                if (sample->time > 0) {
                    prevMarksDateTime = dateTimeCompensate(localBaseTime.addMSecs(sample->time), prevMarksDateTime, 1);
                }
                else {
                    prevMarksDateTime = localBaseTime.addMSecs(sample->time);
                }
                tmpMap.insert("LocalTime", dateTimeString(prevMarksDateTime));
                tmpMap.insert("Type", 2);
                marksContent.append(tmpMap);

                inPause = true;
                break;
            }
            case 0x14: /* high interval = 3 */
            case 0x15: /* low interval = 3 */
            {
                QVariantMap tmpMap;
                if (sample->time > 0) {
                    prevMarksDateTime = dateTimeCompensate(localBaseTime.addMSecs(sample->time), prevMarksDateTime, 1);
                }
                else {
                    prevMarksDateTime = localBaseTime.addMSecs(sample->time);
                }
                tmpMap.insert("LocalTime", dateTimeString(prevMarksDateTime));
                tmpMap.insert("Type", 3);
                marksContent.append(tmpMap);
                break;
            }
            }
            break;
        case ambit_log_sample_type_swimming_turn:
        {
            int nextIndex;
            ambit_log_sample_t *next_swimming_turn = NULL;
            uint8_t style = 0;
            QDateTime sampleDateTime;
            if (sample->time > 0) {
                sampleDateTime = dateTimeCompensate(localBaseTime.addMSecs(sample->time), prevMarksDateTime, 1);
            }
            else {
                sampleDateTime = localBaseTime.addMSecs(sample->time);
            }

            // Find next swimming turn, to check what marks to generate
            for (nextIndex=i+1; nextIndex<order.length(); nextIndex++) {
                next_swimming_turn = &logEntry->logEntry->samples[order[nextIndex]];
                if (next_swimming_turn->type == ambit_log_sample_type_swimming_turn) {
                    break;
                }
            }
            if (nextIndex == order.length()) {
                next_swimming_turn = NULL;
            }
            if (next_swimming_turn == NULL || sample->u.swimming_turn.style != next_swimming_turn->u.swimming_turn.style) {
                QVariantMap tmpMap;
                QVariantList calibration;
                tmpMap.insert("LocalTime", dateTimeString(sampleDateTime));
                tmpMap.insert("Type", 7);
                tmpMap.insert("SwimmingStyle", sample->u.swimming_turn.style);
                for (size_t k=0; k<sizeof(sample->u.swimming_turn.classification)/sizeof(sample->u.swimming_turn.classification[0]); k++) {
                    calibration.append(sample->u.swimming_turn.classification[k]);
                }
                tmpMap.insert("SwimmingStyleCalibration", calibration);
                marksContent.append(tmpMap);

                if (next_swimming_turn != NULL) {
                    style = next_swimming_turn->u.swimming_turn.style;
                }
                else {
                    style = 0;
                }

                // Add some time to timestamp
                sampleDateTime = sampleDateTime.addMSecs(5);
            }
            else {
                style = sample->u.swimming_turn.style;
            }

            sampleDateTime = dateTimeRound(dateTimeCompensate(dateTimeCompensate(dateTimeRound(sampleDateTime, 10), prevMarksDateTime, 0), prevPeriodicSamplesDateTime, 0), 10);

            QVariantMap tmpMap, attribMap;
            QVariantList attributes;
            tmpMap.insert("LocalTime", dateTimeString(sampleDateTime));
            tmpMap.insert("Type", 5);
            tmpMap.insert("SwimmingStyle", style);
            attribMap.insert("Name", "type");
            attribMap.insert("Value", "swimmingturn");
            attributes.append(attribMap);
            tmpMap.insert("Attributes", attributes);
            marksContent.append(tmpMap);

            QVariantMap periodicMap;
            periodicMap.insert("Distance", sample->u.swimming_turn.distance / 100);
            periodicMap.insert("LocalTime", dateTimeString(sampleDateTime));
            periodicSamplesContent.append(periodicMap);

            prevPeriodicSamplesDateTime = prevMarksDateTime = sampleDateTime;

            break;
        }
        case ambit_log_sample_type_swimming_stroke:
        {
            QVariantMap tmpMap;
            prevPeriodicSamplesDateTime = dateTimeRound(dateTimeCompensate(dateTimeRound(localBaseTime.addMSecs(sample->time), 10), prevPeriodicSamplesDateTime, 0), 10);
            tmpMap.insert("LocalTime", dateTimeString(prevPeriodicSamplesDateTime));
            tmpMap.insert("SwimmingStrokeType", 0);
            periodicSamplesContent.append(tmpMap);
            break;
        }
        case ambit_log_sample_type_activity:
        {
            QVariantMap tmpMap;
            if (sample->time > 0) {
                prevMarksDateTime = dateTimeCompensate(localBaseTime.addMSecs(sample->time), prevMarksDateTime, 1);
            }
            else {
                prevMarksDateTime = localBaseTime.addMSecs(sample->time);
            }
            tmpMap.insert("LocalTime", dateTimeString(prevMarksDateTime));
            tmpMap.insert("NextActivityID", sample->u.activity.activitytype);
            tmpMap.insert("Type", 8);
            marksContent.append(tmpMap);
            break;
        }
        default:
            break;
        }
    }

    content.insert("ActivityID", logEntry->logEntry->header.activity_type);
    content.insert("AscentAltitude", (double)logEntry->logEntry->header.ascent);
    content.insert("AscentTime", (double)logEntry->logEntry->header.ascent_time/1000.0);
    content.insert("AvgCadence", logEntry->logEntry->header.cadence_avg);
    content.insert("AvgHR", logEntry->logEntry->header.heartrate_avg);
    content.insert("AvgSpeed", (double)logEntry->logEntry->header.speed_avg/3600.0);
    content.insert("DescentAltitude", (double)logEntry->logEntry->header.descent);
    content.insert("DescentTime", (double)logEntry->logEntry->header.descent_time/1000.0);
    content.insert("DeviceName", logEntry->deviceInfo.model);
    content.insert("DeviceSerialNumber", logEntry->deviceInfo.serial);
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
        uncompressedData = QJsonDocument(QJsonArray::fromVariantList(IBIContent)).toJson(QJsonDocument::Compact);
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
        uncompressedData = QJsonDocument(QJsonArray::fromVariantList(periodicSamplesContent)).toJson(QJsonDocument::Compact);
        compressData(uncompressedData, compressedData);
        QVariantMap periodicSamplesDataMap;
        periodicSamplesDataMap.insert("CompressedSampleSets", compressedData.toBase64());
        content.insert("Samples", periodicSamplesDataMap);            /* compressed */
    }
    content.insert("SerialNumber", QVariant::Invalid);
    content.insert("StartLatitude", QVariant::Invalid);
    content.insert("StartLongitude", QVariant::Invalid);
    if (GPSSamplesContent.count() > 0) {
        uncompressedData = QJsonDocument(QJsonArray::fromVariantList(GPSSamplesContent)).toJson(QJsonDocument::Compact);
        compressData(uncompressedData, compressedData);
        QVariantMap GPSSamplesDataMap;
        GPSSamplesDataMap.insert("CompressedTrackPoints", compressedData.toBase64());
        content.insert("Track", GPSSamplesDataMap);                   /* compressed */
    }

    output = QJsonDocument(QJsonObject::fromVariantMap(content)).toJson(QJsonDocument::Compact);
    ok = !output.isEmpty();

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
            if (value->u.latitude <= 90 && value->u.latitude >= -90){
                output.insert("Latitude", (double)value->u.latitude/10000000);
            }
            break;
        case ambit_log_sample_periodic_type_longitude:
            if (value->u.longitude <= 180 && value->u.longitude >= -180){
                output.insert("Longitude", (double)value->u.longitude/10000000);
            }
            break;
        case ambit_log_sample_periodic_type_distance:
            if (value->u.distance != 0xffffffff && value->u.distance != 0xb400000) {
                output.insert("Distance", value->u.distance);
            }
            break;
        case ambit_log_sample_periodic_type_speed:
            if (value->u.speed != 0xffff && (value->u.speed/100) <= 556) {
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
            if (value->u.altitude >= -1000 && value->u.altitude <= 15000) {
                output.insert("Altitude", (double)value->u.altitude);
            }
            break;
        case ambit_log_sample_periodic_type_abspressure:
            output.insert("AbsPressure", (int)round((double)value->u.abspressure/10.0));
            break;
        case ambit_log_sample_periodic_type_energy:
            if (value->u.energy && value->u.energy <= 1000) {
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
            if (value->u.gpsaltitude >= -1000 && value->u.gpsaltitude <= 15000) {
                output.insert("GPSAltitude", value->u.gpsaltitude);
            }
            break;
        case ambit_log_sample_periodic_type_gpsheading:
            if (value->u.gpsheading != 0xffff && value->u.gpsheading >= 0 && value->u.gpsheading <= 360) {
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
            if (value->u.sealevelpressure >= 8500 && value->u.sealevelpressure <= 11000) {
                output.insert("SeaLevelPressure", (int)round((double)value->u.sealevelpressure/10.0));
            }
            break;
        case ambit_log_sample_periodic_type_verticalspeed:
            if ((value->u.verticalspeed/100.0) >= -59 && (value->u.verticalspeed/100.0) <= 59){
                output.insert("VerticalSpeed", (double)value->u.verticalspeed/100.0);
            }
            break;
        case ambit_log_sample_periodic_type_cadence:
            if (value->u.cadence != 0xff) {
                output.insert("Cadence", value->u.cadence);
            }
            break;
        case ambit_log_sample_periodic_type_bikepower:
            if (value->u.bikepower <= 2000) {
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
    size_t destLen = compressBound(content.length());

    if (destLen > 0) {
        u_int8_t *buf = (u_int8_t*)malloc(destLen);

        z_stream strm;
        gz_header header;
        memset(&strm, 0, sizeof(z_stream));
        memset(&header, 0, sizeof(gz_header));

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
                // make sure to copy the data and free buf to not cause a memory leak
                output.clear();
                output.append((char*)buf, destLen - strm.avail_out);
                ret = 0;
            }
        }

        free(buf);
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

QString MovesCountJSON::dateTimeString(const QDateTime& dateTime)
{
    if (dateTime.time().msec() != 0) {
        return dateTime.toString("yyyy-MM-ddThh:mm:ss.zzz");
    }
    else {
        return dateTime.toString("yyyy-MM-ddThh:mm:ss");
    }
}

QDateTime MovesCountJSON::dateTimeRound(QDateTime dateTime, int msecRoundFactor)
{
    if (msecRoundFactor != 1) {
        return dateTime.addMSecs(qRound(1.0*dateTime.time().msec()/msecRoundFactor)*msecRoundFactor - dateTime.time().msec());
    }
    else {
        return dateTime;
    }
}

QDateTime MovesCountJSON::dateTimeCompensate(QDateTime dateTime, const QDateTime& prevDateTime, int minOffset)
{
    if (dateTime <= prevDateTime) {
        return prevDateTime.addMSecs(minOffset);
    }
    return dateTime;
}

QVariantMap MovesCountJSON::parseJsonMap(const QByteArray& input, bool& ok) const
{
    if (input.length() <= 0) {
      ok = false;
      return QVariantMap();
    }

    QJsonParseError err;
    QJsonDocument json = QJsonDocument::fromJson(input, &err);
    ok = !json.isNull();
    return json.object().toVariantMap();
}

QVariantList MovesCountJSON::parseJsonList(const QByteArray& input, bool& ok) const
{
    if (input.length() <= 0) {
      ok = false;
      return QVariantList();
    }

    QJsonParseError err;
    QJsonDocument json = QJsonDocument::fromJson(input, &err);
    ok = !json.isNull();
    return json.array().toVariantList();
}

