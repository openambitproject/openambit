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
#ifndef MOVESCOUNT_H
#define MOVESCOUNT_H

#include <QObject>
#include <QList>
#include <QThread>
#include <QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

#include <libambit.h>

#include "deviceinfo.h"
#include "logentry.h"
#include "logstore.h"
#include "movescountjson.h"
#include "movescountlogdirentry.h"
#include "movescountlogchecker.h"

class MovesCount : public QObject
{
    Q_OBJECT
public:
    static MovesCount* instance();
    void exit();

    void setBaseAddress(QString baseAddress);
    void setAppkey(QString appkey);
    void setUsername(QString username);
    void setUserkey(QString userkey);
    QString generateUserkey();
    void setDevice(const DeviceInfo& device_info);

    bool isAuthorized();
    int getOrbitalData(u_int8_t **data);
    int getPersonalSettings(ambit_personal_settings_t *settings, bool onlychangedsettings);
    int getRoute(ambit_route_t *routes, ambit_personal_settings_t *ps, QString url);
    int getRoutePoints(ambit_route_t *routes, ambit_personal_settings_t *ps, QString url);
    int applyPersonalSettingsFromDevice(ambit_personal_settings_t *movesPersonalSettings, ambit_personal_settings_t *devicePersonalSettings);
    void getDeviceSettings();
    int getCustomModeData(ambit_sport_mode_device_settings_t *ambitCustomModes);
    int getAppsData(ambit_app_rules_t *ambitApps);
    QList<MovesCountLogDirEntry> getMovescountEntries(QDate startTime, QDate endTime);

    void checkAuthorization();
    void checkLatestFirmwareVersion();
    void writePersonalSettings(ambit_personal_settings_t *settings);
    void writeLog(LogEntry *logEntry);

signals:
    void newerFirmwareExists(QByteArray fw_version);
    void movesCountAuth(bool authorized);
    void logMoveID(QString device, QDateTime time, QString moveID);

private slots:
    void authCheckFinished();
    void firmwareReplyFinished();
    void recheckAuthorization();
    void handleAuthorizationSignal(bool authorized);

    int getOrbitalDataInThread(u_int8_t **data);
    int getPersonalSettingsInThread(ambit_personal_settings_t *settings, bool onlychangedsettings);
    int getRouteInThread(ambit_route_t *routes, ambit_personal_settings_t *ps, QString url);
    int getRoutePointsInThread(ambit_route_t *routes, ambit_personal_settings_t *ps, QString url);
    void getDeviceSettingsInThread();
    int getCustomModeDataInThread(ambit_sport_mode_device_settings_t *ambitSettings);
    int getAppsDataInThread(ambit_app_rules_t *ambitApps);
    QList<MovesCountLogDirEntry> getMovescountEntriesInThread(QDate startTime, QDate endTime);

    void checkAuthorizationInThread();
    void checkLatestFirmwareVersionInThread();
    void writePersonalSettingsInThread(ambit_personal_settings_t *settings);
    void writeLogInThread(LogEntry *logEntry);

private:
    MovesCount();
    ~MovesCount();

    bool checkReplyAuthorization(QNetworkReply *reply);

    QNetworkReply *asyncGET(QString path, QString additionalHeaders, bool auth);
    QNetworkReply *syncGET(QString path, QString additionalHeaders, bool auth);

    QNetworkReply *asyncPOST(QString path, QString additionalHeaders, QByteArray &postData, bool auth);
    QNetworkReply *syncPOST(QString path, QString additionalHeaders, QByteArray &postData, bool auth);

    QNetworkReply *asyncPUT(QString path, QString additionalHeaders, QByteArray &postData, bool auth);
    QNetworkReply *syncPUT(QString path, QString additionalHeaders, QByteArray &postData, bool auth);

#ifdef QT_DEBUG
    void writeJsonToStorage(QString filename, QByteArray &data);
#endif

    bool exiting;
    bool authorized;

    QString baseAddress;
    QString appkey;
    QString username;
    QString userkey;
    QString model;
    QString serial;
    DeviceInfo device_info;

    QNetworkAccessManager *manager;
    QNetworkReply *firmwareCheckReply;
    QNetworkReply *authCheckReply;

    MovesCountJSON jsonParser;

    LogStore logStore;

    MovesCountLogChecker *logChecker;

    QThread workerThread;
};

#endif // MOVESCOUNT_H
