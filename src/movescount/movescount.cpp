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
#include <QtNetwork/QNetworkRequest>
#ifdef QT_DEBUG
#include <QtNetwork/QSslConfiguration>
#endif
#include <QEventLoop>
#include <QMutex>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>

#include "logstore.h"

#define AUTH_CHECK_TIMEOUT 5000 /* ms */
#define GPS_ORBIT_DATA_MIN_SIZE 30000 /* byte */

void logReply(QNetworkReply *reply);

static MovesCount *m_Instance;
static void writeJson(QByteArray _data, const char* name);

MovesCount* MovesCount::instance()
{
    static QMutex mutex;
    if (!m_Instance) {
        mutex.lock();

        if (!m_Instance) {
            m_Instance = new MovesCount;
        }

        mutex.unlock();
    }

    return m_Instance;
}

void MovesCount::exit()
{
    static QMutex mutex;

    exiting = true;

    mutex.lock();
    if (m_Instance) {
        workerThread.quit();
        workerThread.wait();

        delete m_Instance;
        m_Instance = NULL;
    }
    mutex.unlock();
}

void MovesCount::setBaseAddress(QString baseAddress)
{
    this->baseAddress = baseAddress;
    if (this->baseAddress[this->baseAddress.length()-1] == '/') {
        this->baseAddress = this->baseAddress.remove(this->baseAddress.length()-1, 1);
    }
}

void MovesCount::setAppkey(QString appkey)
{
    this->appkey = appkey;
}

void MovesCount::setUsername(QString username)
{
    this->username = username;

    checkAuthorization();
}

void MovesCount::setUserkey(QString userkey)
{
    this->userkey = userkey;
}

QString MovesCount::generateUserkey()
{
    char randString[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    QString retString = "";

    for (int i=0; i<14; i++) {
        retString += QString(randString[qrand() % sizeof(randString)]);
    }

    return retString;
}

void MovesCount::setDevice(const DeviceInfo& device_info)
{
    this->device_info = device_info;
}

void MovesCount::setUploadLogs(bool uploadLogs) {
    this->uploadLogs = uploadLogs;
}

bool MovesCount::isAuthorized()
{
    return authorized;
}

int MovesCount::getOrbitalData(u_int8_t **data)
{
    int ret = -1;

    if (&workerThread == QThread::currentThread()) {
        ret = getOrbitalDataInThread(data);
    }
    else {
        QMetaObject::invokeMethod(this, "getOrbitalDataInThread", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(int, ret),
                                  Q_ARG(u_int8_t **, data));
    }

    return ret;
}

int MovesCount::getPersonalSettings(ambit_personal_settings_t *settings, bool onlychangedsettings)
{
    int ret = -1;

    if (&workerThread == QThread::currentThread()) {
        ret = getPersonalSettingsInThread(settings, onlychangedsettings);
    }
    else {
        QMetaObject::invokeMethod(this, "getPersonalSettingsInThread", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(int, ret),
                                  Q_ARG(ambit_personal_settings_t *, settings), Q_ARG(bool, onlychangedsettings));
    }

    return ret;
}

int MovesCount::getRoute(ambit_route_t *route, ambit_personal_settings_t *ps, QString url)
{
    int ret = -1;

    if (&workerThread == QThread::currentThread()) {
        ret = getRouteInThread(route, ps, url);
    }
    else {
        QMetaObject::invokeMethod(this, "getRouteInThread", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(int, ret),
                                  Q_ARG(ambit_route_t *, route),Q_ARG(ambit_personal_settings_t *, ps) , Q_ARG(QString, url));
    }

    return ret;
}

int MovesCount::getRouteFromFile(ambit_route_t *route, ambit_personal_settings_t *ps, QString url, QString directory)
{
    // find filename based on directory and URL
    // routes/310212
    QString routeId = url.remove("routes/");

    QStringList filters;
    // routes_573247115_SMD22.v1.d1.Sexten-Innichen-Toblach-Marchkinkele-Winnebach-Sexten.55km.1550hm.json
    filters << QString("routes_").append(routeId).append("_*");

    QDir dir = QDir(directory);
    QStringList dirs = dir.entryList(filters);
    if (dirs.empty())
    {
        qDebug() << "Did not find " << filters << " at " << directory;
        return -1;
    }

    QString fileName = dirs.at(0);

    // both file-names start similar, so we need to make sure we use
    // the non-points-one which we want to read here
    if (fileName.contains("_points_")) {
        fileName = dirs.at(1);
    }

    QFile routeFile(QString(directory).append("/").append(fileName));
    qDebug() << "Reading route from " << routeFile;

    if (!routeFile.open(QIODevice::ReadOnly)){
        qDebug() << "Route file " << routeFile << " not available for reading!";
        return -1;
    }

    QByteArray _data = routeFile.readAll();

    int ret = -1;
    if (_data.length() > 0) {
        jsonParser.parseRoute(_data, route, ps, NULL, directory);

        ret = _data.length();
    }

    return ret;
}

int MovesCount::getRoutePoints(ambit_route_t *route, ambit_personal_settings_t *ps, QString url)
{
    int ret = -1;

    if (&workerThread == QThread::currentThread()) {
        ret = getRoutePointsInThread(route, ps, url);
    }
    else {
        QMetaObject::invokeMethod(this, "getRoutePointsInThread", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(int, ret),
                                  Q_ARG(ambit_route_t *, route),Q_ARG(ambit_personal_settings_t *, ps), Q_ARG(QString, url));
    }

    return ret;
}

int MovesCount::getRoutePointsFromFile(ambit_route_t *route, ambit_personal_settings_t *ps, QString url, QString directory)
{
    // routes/310212
    QString routeId = url.remove("routes/").remove("/points");

    QStringList filters;
    // routes_573247115_points_SMD22.v1.d1.Sexten-Innichen-Toblach-Marchkinkele-Winnebach-Sexten.55km.1550hm.json
    filters << QString("routes_").append(routeId).append("_points_*");

    QDir dir = QDir(directory);
    QStringList dirs = dir.entryList(filters);

    if (dirs.empty())
    {
        qDebug() << "Did not find " << filters << " at " << directory;
        return -1;
    }

    const QString &fileName = dirs.at(0);
    QFile routePointsFile(QString(directory).append("/").append(fileName));

    qDebug() << "Reading points from " << routePointsFile;

    if (!routePointsFile.open(QIODevice::ReadOnly)){
        qDebug() << "Points file " << routePointsFile << " not available for reading!";
        return -1;
    }

    QByteArray _data = routePointsFile.readAll();

    int ret = -1;
    if (_data.length() > 0) {
        ret = jsonParser.parseRoutePoints(_data, route, ps);
    }

    return ret;
}

void MovesCount::getDeviceSettings()
{
    QMetaObject::invokeMethod(this, "getDeviceSettingsInThread", Qt::AutoConnection);
}

int MovesCount::getCustomModeData(ambit_sport_mode_device_settings_t* ambitCustomModes)
{
    int ret = -1;

    if (&workerThread == QThread::currentThread()) {
        ret = getCustomModeDataInThread(ambitCustomModes);
    }
    else {
        QMetaObject::invokeMethod(this, "getCustomModeDataInThread", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(int, ret),
                                  Q_ARG(ambit_sport_mode_device_settings_t*, ambitCustomModes));
    }

    return ret;
}

int MovesCount::getWatchModeConfig(ambit_sport_mode_device_settings_t* ambitCustomModes)
{
    int ret = -1;

    if (&workerThread == QThread::currentThread()) {
        ret = getWatchModeDataThread(ambitCustomModes);
    }
    else {
        QMetaObject::invokeMethod(this, "getWatchModeDataThread", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(int, ret),
                                  Q_ARG(ambit_sport_mode_device_settings_t*, ambitCustomModes));
    }

    return ret;
}


int MovesCount::getAppsData(ambit_app_rules_t* ambitApps)
{
    int ret = -1;

    if (&workerThread == QThread::currentThread()) {
        ret = getAppsDataInThread(ambitApps);
    }
    else {
        QMetaObject::invokeMethod(this, "getAppsDataInThread", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(int, ret),
                                  Q_ARG(ambit_app_rules_t*, ambitApps));
    }

    return ret;
}

int MovesCount::getWatchAppConfig(ambit_app_rules_t* ambitApps)
{
    int ret = -1;

    if (&workerThread == QThread::currentThread()) {
        ret = getWatchAppConfigThread(ambitApps);
    }
    else {
        QMetaObject::invokeMethod(this, "getWatchAppConfigThread", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(int, ret),
                                  Q_ARG(ambit_app_rules_t*, ambitApps));
    }

    return ret;
}

QList<MovesCountLogDirEntry> MovesCount::getMovescountEntries(QDate startTime, QDate endTime)
{
    QList<MovesCountLogDirEntry> retList;

    if (&workerThread == QThread::currentThread()) {
        retList = getMovescountEntriesInThread(startTime, endTime);
    }
    else {
        QMetaObject::invokeMethod(this, "getMovescountEntriesInThread", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(QList<MovesCountLogDirEntry>, retList),
                                  Q_ARG(QDate, startTime),
                                  Q_ARG(QDate, endTime));
    }

    return retList;
}

void MovesCount::checkAuthorization()
{
    QMetaObject::invokeMethod(this, "checkAuthorizationInThread", Qt::AutoConnection);
}

void MovesCount::checkLatestFirmwareVersion()
{
    QMetaObject::invokeMethod(this, "checkLatestFirmwareVersionInThread", Qt::AutoConnection);
}

void MovesCount::writePersonalSettings(ambit_personal_settings_t *settings)
{
    if (&workerThread == QThread::currentThread()) {
        writePersonalSettingsInThread(settings);
    }
    else {
        QMetaObject::invokeMethod(this, "writePersonalSettingsInThread", Qt::BlockingQueuedConnection,
                                  Q_ARG(ambit_personal_settings_t *, settings));
    }
}

void MovesCount::writeLog(LogEntry *logEntry)
{
    if (&workerThread == QThread::currentThread()) {
        writeLogInThread(logEntry);
    }
    else {
        QMetaObject::invokeMethod(this, "writeLogInThread", Qt::BlockingQueuedConnection,
                                  Q_ARG(LogEntry *, logEntry));
    }
}

void MovesCount::authCheckFinished()
{
    if (authCheckReply != NULL) {
        checkReplyAuthorization(authCheckReply);
        authCheckReply->deleteLater();
        authCheckReply = NULL;
    }
}

void MovesCount::firmwareReplyFinished()
{
    u_int8_t fw_version[3];

    if (firmwareCheckReply != NULL) {
        if (firmwareCheckReply->error() == QNetworkReply::NoError) {
            QByteArray data = firmwareCheckReply->readAll();
            if (jsonParser.parseFirmwareVersionReply(data, fw_version) == 0) {
                if (fw_version[0] > device_info.fw_version[0] ||
                    (fw_version[0] == device_info.fw_version[0] && (fw_version[1] > device_info.fw_version[1] ||
                     (fw_version[1] == device_info.fw_version[1] && (fw_version[2] > device_info.fw_version[2]))))) {
                    emit newerFirmwareExists(QByteArray((const char*)fw_version, 3));
                }
            }
        }

        firmwareCheckReply->deleteLater();
        firmwareCheckReply = NULL;
    }
}

void MovesCount::recheckAuthorization()
{
    getDeviceSettings();
}

void MovesCount::handleAuthorizationSignal(bool authorized)
{
    if (authorized && uploadLogs) {
        logChecker->run();
    }
}

int MovesCount::getOrbitalDataInThread(u_int8_t **data)
{
    int ret = -1;
    QNetworkReply *reply = syncGET("/devices/gpsorbit/binary", "", false);

    if(reply->error() == QNetworkReply::NoError) {
        QByteArray _data = reply->readAll();

        if (_data.length() >= GPS_ORBIT_DATA_MIN_SIZE) {
            *data = (u_int8_t*)malloc(_data.length());

            memcpy(*data, _data.data(), _data.length());

            ret = _data.length();
        }
    }

    delete reply;

    return ret;
}

int MovesCount::getPersonalSettingsInThread(ambit_personal_settings_t *settings, bool onlychangedsettings)
{
    int ret = -1;
    QNetworkReply *reply = syncGET("/userdevices/" + QString("%1").arg(device_info.serial), QString("onlychangedsettings=%1&includeallsportmodes=false&model=%2&eswverrsion=%3.%4.%5").arg((onlychangedsettings?"true":"fasle")).arg(device_info.model).arg(device_info.fw_version[0]).arg(device_info.fw_version[1]).arg(device_info.fw_version[2]), true);

    if(reply->error() == QNetworkReply::NoError) {
        QByteArray _data = reply->readAll();

        if (_data.length() > 0) {
            writeJson(_data, QString(getenv("HOME")).toUtf8() + "/.openambit/personal_settings.json");

            jsonParser.parsePersonalSettings(_data, settings, this, NULL);
            ret = _data.length();
        }
    }

    delete reply;

    return ret;
}

int MovesCount::getRouteInThread(ambit_route_t *route, ambit_personal_settings_t *ps, QString url)
{

    int ret = -1;
    QNetworkReply *reply = syncGET("/" + url, "", true);

    if(reply->error() == QNetworkReply::NoError) {
        QByteArray _data = reply->readAll();

        if (_data.length() > 0) {
            jsonParser.parseRoute(_data, route, ps, this, NULL);

            QString file = QString(getenv("HOME")).toUtf8() + QString("/.openambit/") + url.replace("/", "_") + "_" + QString::fromLatin1(route->name) + ".json";
            writeJson(_data, file.toStdString().c_str());

            ret = _data.length();
        }
    }

    delete reply;

    return ret;
}

int MovesCount::getRoutePointsInThread(ambit_route_t *route, ambit_personal_settings_t *ps, QString url)
{

    int ret = -1;
    QNetworkReply *reply = syncGET("/" + url, "type=routepoints&maxpoints=1000", true);

    if(reply->error() == QNetworkReply::NoError) {
        QByteArray _data = reply->readAll();

        if (_data.length() > 0) {
            QString file = QString(getenv("HOME")).toUtf8() + QString("/.openambit/") + url.replace("/", "_") + "_" + QString::fromLatin1(route->name) + "_points.json";
            writeJson(_data, file.toStdString().c_str());

            ret = jsonParser.parseRoutePoints(_data, route, ps);
        }
    }

    delete reply;

    return ret;
}

int MovesCount::applyPersonalSettingsFromDevice(ambit_personal_settings_t *movesPersonalSettings, ambit_personal_settings_t *devicePersonalSettings)
{

    //TODO: resolve name conflict, rename device waypoint?
    bool device_waypoint_has_changes = false;
    int  device_waypoint_num_changes = 0; //this value is only a control value for allocating memory if device_waypoint_has_changes
    int  device_waypoint_count = devicePersonalSettings->waypoints.count;
    int  moves_waypoint_count = movesPersonalSettings->waypoints.count;

    for(int x=0; x<device_waypoint_count; x++) {
        if(devicePersonalSettings->waypoints.data[x].status == 2) { //Waypoint marked for removal
            for(int y=0; y<moves_waypoint_count; y++) {
                if(strncmp(devicePersonalSettings->waypoints.data[x].name, movesPersonalSettings->waypoints.data[y].name,49) == 0 && strncmp(devicePersonalSettings->waypoints.data[x].route_name, movesPersonalSettings->waypoints.data[y].route_name,49) == 0) {
                    devicePersonalSettings->waypoints.data[x].status = 0;
                    movesPersonalSettings->waypoints.data[x].status = 2; //Mark for actual removal
                    device_waypoint_has_changes = true;
                    device_waypoint_num_changes--;
                }
            }
        } else if(devicePersonalSettings->waypoints.data[x].status == 1) { //Waypoint marked for addition;
            for(int y=0; y<moves_waypoint_count; y++) {
                if(strncmp(devicePersonalSettings->waypoints.data[x].name, movesPersonalSettings->waypoints.data[y].name,49) == 0 && strncmp(devicePersonalSettings->waypoints.data[x].route_name, movesPersonalSettings->waypoints.data[y].route_name,49) == 0) {
                    movesPersonalSettings->waypoints.data[x].status = 2; //Marked for removal due to name conflict (device win)
                }

            }
            device_waypoint_has_changes = true;
            device_waypoint_num_changes++;
        }
    }

    if(device_waypoint_has_changes) {
        int new_waypoint_count = moves_waypoint_count+device_waypoint_num_changes;
        int new_added = 0;
        ambit_waypoint_t *new_waypoints = (ambit_waypoint_t *)malloc(sizeof(ambit_waypoint_t)*new_waypoint_count);

        for(int x=0; x<moves_waypoint_count && new_added<new_waypoint_count; x++) {
            if(movesPersonalSettings->waypoints.data[x].status == 0) {
                new_waypoints[new_added] = movesPersonalSettings->waypoints.data[x];
                ++new_added;
            }
        }

        for(int x=0; x<device_waypoint_count && new_added<new_waypoint_count; x++) {
            if(devicePersonalSettings->waypoints.data[x].status == 1) {
                new_waypoints[new_added] = devicePersonalSettings->waypoints.data[x];
                ++new_added;
            }
        }

        if(movesPersonalSettings->waypoints.data != NULL) {
            free(movesPersonalSettings->waypoints.data);
        }

        movesPersonalSettings->waypoints.data = new_waypoints;
        movesPersonalSettings->waypoints.count = new_waypoint_count;
    }

    return 0;
}

void MovesCount::getDeviceSettingsInThread()
{
    QNetworkReply *reply = syncGET("/userdevices/" + device_info.serial, "", true);

    if (checkReplyAuthorization(reply)) {
        QByteArray _data = reply->readAll();
    }
}

void writeJson(QByteArray _data, const char* name) {
    QFile logfile(name);
    logfile.open(QIODevice::WriteOnly | QIODevice::Truncate);

    // pretty print JSON
    QJsonDocument doc = QJsonDocument::fromJson(_data);
    QString formattedJsonString = doc.toJson(QJsonDocument::Indented);

    logfile.write(formattedJsonString.toUtf8());
    logfile.close();
}

int MovesCount::getCustomModeDataInThread(ambit_sport_mode_device_settings_t *ambitSettings)
{
    int ret = -1;
    QNetworkReply *reply = syncGET("/userdevices/" + device_info.serial, "", true);

    if (checkReplyAuthorization(reply)) {
        QByteArray _data = reply->readAll();
        MovescountSettings settings = MovescountSettings();

        if (jsonParser.parseDeviceSettingsReply(_data, settings) == 0) {
            settings.toAmbitData(ambitSettings);
            ret = 0;
        }
    }

    return ret;
}

int MovesCount::getWatchModeDataThread(ambit_sport_mode_device_settings_t *ambitSettings)
{
    int ret = -1;
    QNetworkReply *reply = syncGET("/userdevices/" + device_info.serial, "", true);

    if (checkReplyAuthorization(reply)) {
        QByteArray _data = reply->readAll();
        MovescountSettings settings = MovescountSettings();

        writeJson(_data, QString(getenv("HOME")).toUtf8() + "/.openambit/settings.json");

        if (jsonParser.parseDeviceSettingsReply(_data, settings) == 0) {
            settings.toAmbitData(ambitSettings);
            ret = 0;
        }
    }

    return ret;
}

int MovesCount::getAppsDataInThread(ambit_app_rules_t* ambitApps)
{
    int ret = -1;
    QNetworkReply *reply = syncGET("/rules/private", "", true);

    if (checkReplyAuthorization(reply)) {
        QByteArray _data = reply->readAll();

        if (jsonParser.parseAppRulesReply(_data, ambitApps) == 0) {
            ret = 0;
        }
    }

    return ret;
}

int MovesCount::getWatchAppConfigThread(ambit_app_rules_t* ambitApps)
{
    int ret = -1;
    QNetworkReply *reply = syncGET("/rules/private", "", true);

    if (checkReplyAuthorization(reply)) {
        QByteArray _data = reply->readAll();

        writeJson(_data, QString(getenv("HOME")).toUtf8() + "/.openambit/apprules.json");

        if (jsonParser.parseAppRulesReply(_data, ambitApps) == 0) {
            ret = 0;
        }
    }

    return ret;
}

QList<MovesCountLogDirEntry> MovesCount::getMovescountEntriesInThread(QDate startTime, QDate endTime)
{
    QList<MovesCountLogDirEntry> retList;

    QNetworkReply *reply = syncGET("/moves/private", "startdate=" + startTime.toString("yyyy-MM-dd") + "&enddate=" + endTime.toString("yyyy-MM-dd"), true);

    if (checkReplyAuthorization(reply)) {
        QByteArray _data = reply->readAll();

        //qDebug() << "Movescount replied with \"" << _data << "\"";

        if (reply->error() == QNetworkReply::NoError){
            if (jsonParser.parseLogDirReply(_data, retList) != 0) {
                qDebug() << "Parsing reply for movescount-entries failed";

                // empty list if parse failed
                retList.clear();
            } else {
                qDebug() << "Movescount returned" << retList.count() << "moves for startdate:" <<
                    startTime.toString("yyyy-MM-dd") << ", enddate:" << endTime.toString("yyyy-MM-dd");
            }
        } else {
            qDebug() << "Failed to fetch logs (err code:" << reply->error() << "), movescount.com replied with \"" << _data << "\"";
        }
    }

    return retList;
}

void MovesCount::checkAuthorizationInThread()
{
    if (authCheckReply == NULL) {
        authCheckReply = asyncGET("/members/private", "", true);
        connect(authCheckReply, SIGNAL(finished()), this, SLOT(authCheckFinished()));
    }
}

void MovesCount::checkLatestFirmwareVersionInThread()
{
    if (firmwareCheckReply == NULL) {
        firmwareCheckReply = asyncGET("/devices/" + QString("%1/%2.%3.%4")
                                      .arg(device_info.model)
                                      .arg(device_info.hw_version[0])
                                      .arg(device_info.hw_version[1])
                                      .arg(device_info.hw_version[2]), "", false);
        connect(firmwareCheckReply, SIGNAL(finished()), this, SLOT(firmwareReplyFinished()));
    }
}

void MovesCount::writePersonalSettingsInThread(ambit_personal_settings_t *settings)
{
    QByteArray json_settings;

    jsonParser.generateNewPersonalSettings(settings, device_info, json_settings);
    QNetworkReply *reply = syncPUT("/userdevices/" + QString("%1").arg(device_info.serial), "resetchangedsettings=true", json_settings, true);

    if(reply->error() == QNetworkReply::NoError) {
        QByteArray _data = reply->readAll();
    }
}

void MovesCount::writeLogInThread(LogEntry *logEntry)
{
    QByteArray output;

    jsonParser.generateLogData(logEntry, output);

#ifdef QT_DEBUG
    // Write json data to storage
    writeJsonToStorage("log-" + logEntry->device + "-" + logEntry->time.toString("yyyy-MM-ddThh_mm_ss") + ".json", output);
#endif

    QNetworkReply *reply = syncPOST("/moves/", "", output, true);

    QByteArray data = reply->readAll();
    if (reply->error() == QNetworkReply::NoError){
        QString moveId;
        if (jsonParser.parseLogReply(data, moveId) == 0) {
            qDebug() << "Movescount reported move-id " << moveId << " for move '" << logEntry->logEntry->header.activity_name <<
            "' from " << logEntry->logEntry->header.date_time.year << "-" << logEntry->logEntry->header.date_time.month << "-" << logEntry->logEntry->header.date_time.day;
            emit logMoveID(logEntry->device, logEntry->time, moveId);
        } else {
            qDebug() << "Failed to upload log for move '" << logEntry->logEntry->header.activity_name <<
                     "' from " << logEntry->logEntry->header.date_time.year << "-" << logEntry->logEntry->header.date_time.month << "-" << logEntry->logEntry->header.date_time.day <<
                     ", movescount.com replied with \"" << data << "\"";
            emit uploadError(data);
        }
    } 
    else if(reply->error() == QNetworkReply::ContentConflictError){
        // this is not useful currently as it seems that we re-visit some logs every time
        //emit uploadError("Move already uploaded");
        qDebug() << "Movescount replied with ContentConflictError for move '" << logEntry->logEntry->header.activity_name <<
            "' from " << logEntry->logEntry->header.date_time.year << "-" << logEntry->logEntry->header.date_time.month << "-" << logEntry->logEntry->header.date_time.day <<
            ": " << data;
    } else {
        qDebug() << "Failed to upload log for move '" << logEntry->logEntry->header.activity_name <<
                 "' from " << logEntry->logEntry->header.date_time.year << "-" << logEntry->logEntry->header.date_time.month << "-" << logEntry->logEntry->header.date_time.day <<
                 ": (err code:" << reply->error() << "), movescount.com replied with \"" << data << "\"";
        emit uploadError(data);
    }
}

MovesCount::MovesCount() :
    exiting(false), authorized(false), firmwareCheckReply(NULL), authCheckReply(NULL)
{
    this->manager = new QNetworkAccessManager(this);

    this->logChecker = new MovesCountLogChecker();

    this->moveToThread(&workerThread);
    workerThread.start();

    connect(this, SIGNAL(movesCountAuth(bool)), this, SLOT(handleAuthorizationSignal(bool)));
}

MovesCount::~MovesCount()
{
    workerThread.exit();
    workerThread.wait();

    delete this->logChecker;
    delete this->manager;
}

bool MovesCount::checkReplyAuthorization(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
        authorized = false;
        emit movesCountAuth(false);
        QTimer::singleShot(AUTH_CHECK_TIMEOUT, this, SLOT(recheckAuthorization()));
    }
    else if(reply->error() == QNetworkReply::NoError) {
        authorized = true;
        emit movesCountAuth(true);
    }

    return authorized;
}

QNetworkReply *MovesCount::asyncGET(const QString &path, const QString &additionalHeaders, bool auth)
{
    QNetworkRequest req;
    QString url = this->baseAddress + path + "?appkey=" + this->appkey;

    if (auth) {
        url += "&userkey=" + this->userkey + "&email=" + this->username;
    }
    if (additionalHeaders.length() > 0) {
        url += "&" + additionalHeaders;
    }

    #ifdef QT_DEBUG
    qDebug() << "asyncGet: " << url;
    QSslConfiguration ssl_config = QSslConfiguration::defaultConfiguration();
    ssl_config.setPeerVerifyMode(QSslSocket::VerifyNone);
    QSslConfiguration::setDefaultConfiguration(ssl_config);
    #endif
    req.setRawHeader("User-Agent", "ArREST v1.0");
    req.setUrl(QUrl(url));

    QNetworkReply *reply = this->manager->get(req);

    logReply(reply);

    return reply;
}

QNetworkReply *MovesCount::syncGET(const QString &path, const QString &additionalHeaders, bool auth)
{
    QNetworkReply *reply = asyncGET(path, additionalHeaders, auth);
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    logReply(reply);

    return reply;
}

QNetworkReply *MovesCount::asyncPOST(const QString &path, const QString &additionalHeaders, QByteArray &postData, bool auth)
{
    QNetworkRequest req;
    QString url = this->baseAddress + path + "?appkey=" + this->appkey;

    if (auth) {
        url += "&userkey=" + this->userkey + "&email=" + this->username;
    }
    if (additionalHeaders.length() > 0) {
        url += "&" + additionalHeaders;
    }

    #ifdef QT_DEBUG
    qDebug() << "asyncPost: " << url;
    QSslConfiguration ssl_config = QSslConfiguration::defaultConfiguration();
    ssl_config.setPeerVerifyMode(QSslSocket::VerifyNone);
    QSslConfiguration::setDefaultConfiguration(ssl_config);
    #endif
    req.setRawHeader("User-Agent", "ArREST v1.0");
    req.setRawHeader("Content-Type", "application/json");
    req.setUrl(QUrl(url));

    QNetworkReply *reply = this->manager->post(req, postData);

    logReply(reply);

    return reply;
}

QNetworkReply *MovesCount::syncPOST(const QString &path, const QString &additionalHeaders, QByteArray &postData, bool auth)
{
    QNetworkReply *reply = asyncPOST(path, additionalHeaders, postData, auth);
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    return reply;
}

QNetworkReply *MovesCount::asyncPUT(const QString &path, const QString &additionalHeaders, QByteArray &postData, bool auth)
{
    QNetworkRequest req;
    QString url = this->baseAddress + path + "?appkey=" + this->appkey;

    if (auth) {
        url += "&userkey=" + this->userkey + "&email=" + this->username;
    }
    if (additionalHeaders.length() > 0) {
        url += "&" + additionalHeaders;
    }

    #ifdef QT_DEBUG
    qDebug() << "asyncPut: " << url;
    QSslConfiguration ssl_config = QSslConfiguration::defaultConfiguration();
    ssl_config.setPeerVerifyMode(QSslSocket::VerifyNone);
    QSslConfiguration::setDefaultConfiguration(ssl_config);
    #endif
    req.setRawHeader("User-Agent", "ArREST v1.0");
    req.setRawHeader("Content-Type", "application/json");
    req.setUrl(QUrl(url));

    QNetworkReply *reply = this->manager->put(req, postData);

    logReply(reply);

    return reply;
}

QNetworkReply *MovesCount::syncPUT(const QString &path, const QString &additionalHeaders, QByteArray &postData, bool auth)
{
    QNetworkReply *reply = asyncPUT(path, additionalHeaders, postData, auth);
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    logReply(reply);

    return reply;
}

void logReply(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Error: " << reply->error() << ": " << reply->errorString() << ", movescount.com replied with \"" << reply->readAll() << "\"";
    }
}

#ifdef QT_DEBUG
void MovesCount::writeJsonToStorage(QString filename, QByteArray &data)
{
    QString storagePath = QString(getenv("HOME")) + "/.openambit/movescount";
    if (QDir().mkpath(storagePath)) {
        QFile logfile(storagePath + "/" + filename);
        logfile.open(QIODevice::WriteOnly | QIODevice::Truncate);
        logfile.write(data);
        logfile.close();
    }
}
#endif
