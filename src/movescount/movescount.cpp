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

#include "logstore.h"

#define AUTH_CHECK_TIMEOUT 5000 /* ms */
#define GPS_ORBIT_DATA_MIN_SIZE 30000 /* byte */

static MovesCount *m_Instance;

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
    QNetworkReply *reply;

    reply = syncGET("/devices/gpsorbit/binary", "", false);

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
    QNetworkReply *reply;

    reply = syncGET("/userdevices/" + QString("%1").arg(device_info.serial), QString("onlychangedsettings=%1&includeallsportmodes=false&model=%2&eswverrsion=%3.%4.%5").arg((onlychangedsettings?"true":"fasle")).arg(device_info.model).arg(device_info.fw_version[0]).arg(device_info.fw_version[1]).arg(device_info.fw_version[2]), true);

    if(reply->error() == QNetworkReply::NoError) {
        QByteArray _data = reply->readAll();

        if (_data.length() > 0) {
            jsonParser.parsePersonalSettings(_data, settings, this);
            ret = _data.length();
        }
    }

    delete reply;

    return ret;
}

int MovesCount::getRouteInThread(ambit_route_t *route, ambit_personal_settings_t *ps, QString url)
{

    int ret = -1;
    QNetworkReply *reply;

    reply = syncGET("/" + url, "", true);

    if(reply->error() == QNetworkReply::NoError) {
        QByteArray _data = reply->readAll();

        if (_data.length() > 0) {
            jsonParser.parseRoute(_data, route, ps, this);
            ret = _data.length();
        }
    }

    delete reply;

    return ret;
}

int MovesCount::getRoutePointsInThread(ambit_route_t *route, ambit_personal_settings_t *ps, QString url)
{

    int ret = -1;
    QNetworkReply *reply;

    reply = syncGET("/" + url, "type=routepoints&maxpoints=1000", true);

    if(reply->error() == QNetworkReply::NoError) {
        QByteArray _data = reply->readAll();

        if (_data.length() > 0) {
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
    QNetworkReply *reply;

    reply = syncGET("/userdevices/" + device_info.serial, "", true);

    if (checkReplyAuthorization(reply)) {
        QByteArray _data = reply->readAll();
    }
}

int MovesCount::getCustomModeDataInThread(ambit_sport_mode_device_settings_t *ambitSettings)
{
    int ret = -1;
    QNetworkReply *reply;

    reply = syncGET("/userdevices/" + device_info.serial, "", true);

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

int MovesCount::getAppsDataInThread(ambit_app_rules_t* ambitApps)
{
    int ret = -1;
    QNetworkReply *reply;

    reply = syncGET("/rules/private", "", true);

    if (checkReplyAuthorization(reply)) {
        QByteArray _data = reply->readAll();

        if (jsonParser.parseAppRulesReply(_data, ambitApps) == 0) {
            ret = 0;
        }
    }

    return ret;
}

QList<MovesCountLogDirEntry> MovesCount::getMovescountEntriesInThread(QDate startTime, QDate endTime)
{
    QNetworkReply *reply;
    QList<MovesCountLogDirEntry> retList;

    reply = syncGET("/moves/private", "startdate=" + startTime.toString("yyyy-MM-dd") + "&enddate=" + endTime.toString("yyyy-MM-dd"), true);

    if (checkReplyAuthorization(reply)) {
        QByteArray _data = reply->readAll();

        if (jsonParser.parseLogDirReply(_data, retList) != 0) {
            // empty list if parse failed
            retList.clear();
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
    QNetworkReply *reply;

    jsonParser.generateNewPersonalSettings(settings, device_info, json_settings);
    reply = syncPUT("/userdevices/" + QString("%1").arg(device_info.serial), "resetchangedsettings=true", json_settings, true);

    if(reply->error() == QNetworkReply::NoError) {
        QByteArray _data = reply->readAll();
    } else {
        qDebug() << QString("writePersonalSettingsInThread error: ") << reply->error();
    }
}

void MovesCount::writeLogInThread(LogEntry *logEntry)
{
    QByteArray output;
    QNetworkReply *reply;
    QString moveId;
    QByteArray data;

    jsonParser.generateLogData(logEntry, output);

#ifdef QT_DEBUG
    // Write json data to storage
    writeJsonToStorage("log-" + logEntry->device + "-" + logEntry->time.toString("yyyy-MM-ddThh_mm_ss") + ".json", output);
#endif

    reply = syncPOST("/moves/", "", output, true);

    if (reply->error() == QNetworkReply::NoError){
        data = reply->readAll();
        if (jsonParser.parseLogReply(data, moveId) == 0) {
            emit logMoveID(logEntry->device, logEntry->time, moveId);
        } else {
            qDebug() << "Failed to upload log, movescount.com replied with \"" << reply->readAll() << "\"";
            emit uploadError(data);
        }
    } 
    else if(reply->error() == QNetworkReply::ContentConflictError){
        // this is not useful currently as it seems that we re-visit some logs every time
        //emit uploadError("Move already uploaded");
        qDebug() << "Movescount replied with ContentConflictError for move '" << logEntry->logEntry->header.activity_name <<
            "' from " << logEntry->logEntry->header.date_time.year << "-" << logEntry->logEntry->header.date_time.month << "-" << logEntry->logEntry->header.date_time.day;
    }
    else {
        data = reply->readAll();
        qDebug() << "Failed to upload log (err code:" << reply->error() << "), movescount.com replied with \"" << reply->readAll() << "\"";
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

QNetworkReply *MovesCount::asyncGET(QString path, QString additionalHeaders, bool auth)
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

    return this->manager->get(req);
}

QNetworkReply *MovesCount::syncGET(QString path, QString additionalHeaders, bool auth)
{
    QNetworkReply *reply;

    reply = asyncGET(path, additionalHeaders, auth);
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    return reply;
}

QNetworkReply *MovesCount::asyncPOST(QString path, QString additionalHeaders, QByteArray &postData, bool auth)
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

    return this->manager->post(req, postData);
}

QNetworkReply *MovesCount::syncPOST(QString path, QString additionalHeaders, QByteArray &postData, bool auth)
{
    QNetworkReply *reply;

    reply = asyncPOST(path, additionalHeaders, postData, auth);
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    return reply;
}

QNetworkReply *MovesCount::asyncPUT(QString path, QString additionalHeaders, QByteArray &postData, bool auth)
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

    return this->manager->put(req, postData);
}

QNetworkReply *MovesCount::syncPUT(QString path, QString additionalHeaders, QByteArray &postData, bool auth)
{
    QNetworkReply *reply;

    reply = asyncPUT(path, additionalHeaders, postData, auth);
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    return reply;
}

#ifdef QT_DEBUG
#include <QDir>
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
