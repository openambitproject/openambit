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

        if (!m_Instance)
            m_Instance = new MovesCount;

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

        delete logChecker;

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

void MovesCount::setDevice(ambit_device_info_t *device_info)
{
    memcpy(&this->device_info, device_info, sizeof(ambit_device_info_t));
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

int MovesCount::getPersonalSettings(ambit_personal_settings_t *settings)
{
    int ret = -1;

    if (&workerThread == QThread::currentThread()) {
        ret = getPersonalSettingsInThread(settings);
    }
    else {
        QMetaObject::invokeMethod(this, "getPersonalSettingsInThread", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(int, ret),
                                  Q_ARG(ambit_personal_settings_t *, settings));
    }

    return ret;
}

void MovesCount::getDeviceSettings()
{
    QMetaObject::invokeMethod(this, "getDeviceSettingsInThread", Qt::AutoConnection);
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
    u_int8_t fw_version[4];

    if (firmwareCheckReply != NULL) {
        if (firmwareCheckReply->error() == QNetworkReply::NoError) {
            QByteArray data = firmwareCheckReply->readAll();
            if (jsonParser.parseFirmwareVersionReply(data, fw_version) == 0) {
                if (fw_version[0] > device_info.fw_version[0] ||
                    (fw_version[0] == device_info.fw_version[0] && (fw_version[1] > device_info.fw_version[1] ||
                     (fw_version[1] == device_info.fw_version[1] && ((fw_version[2] | (fw_version[3] << 8)) > (device_info.fw_version[2] | (device_info.fw_version[3] << 8))))))) {
                    emit newerFirmwareExists(QByteArray((const char*)fw_version, 4));
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
    if (authorized) {
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

int MovesCount::getPersonalSettingsInThread(ambit_personal_settings_t *settings)
{
    Q_UNUSED(settings);
    return 0;
}

void MovesCount::getDeviceSettingsInThread()
{
    QNetworkReply *reply;

    reply = syncGET("/userdevices/" + QString("%1").arg(device_info.serial), "", true);

    if (checkReplyAuthorization(reply)) {
        QByteArray _data = reply->readAll();
    }
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
                                      .arg(device_info.hw_version[3] << 8 | device_info.hw_version[2]), "", false);
        connect(firmwareCheckReply, SIGNAL(finished()), this, SLOT(firmwareReplyFinished()));
    }
}

void MovesCount::writePersonalSettingsInThread(ambit_personal_settings_t *settings)
{
    Q_UNUSED(settings);
}

void MovesCount::writeLogInThread(LogEntry *logEntry)
{
    QByteArray output;
    QNetworkReply *reply;
    QString moveId;

    jsonParser.generateLogData(logEntry, output);

    reply = syncPOST("/moves/", "", output, true);

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        if (jsonParser.parseLogReply(data, moveId) == 0) {
            emit logMoveID(logEntry->device, logEntry->time, moveId);
        }
    }
    else {
        qDebug() << "Failed to upload log, movescount.com replied with \"" << reply->readAll() << "\"";
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
