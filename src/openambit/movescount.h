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
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QTimer>
#include <libambit.h>
#include "logentry.h"
#include "movescountjson.h"

class MovesCount : public QObject
{
    Q_OBJECT
public:
    static MovesCount* instance();

    void setBaseAddress(QString baseAddress);
    void setAppkey(QString appkey);
    void setUsername(QString username);
    void setUserkey(QString userkey);
    QString generateUserkey();
    void setDevice(ambit_device_info_t *device_info);

    bool isAuthorized();
    int getOrbitalData(u_int8_t **data);
    int getPersonalSettings(ambit_personal_settings_t *settings);
    int getDeviceSettings();
signals:
    void newerFirmwareExists(QByteArray fw_version);
    void movesCountAuth(bool authorized);
    void logMoveID(QString device, QDateTime time, QString moveID);
    
public slots:
    void checkLatestFirmwareVersion();
    void writePersonalSettings(ambit_personal_settings_t *settings);
    void writeLog(LogEntry *logEntry);

private slots:
    void firmwareReplyFinished();
    void recheckAuthorization();

private:
    MovesCount();
    MovesCount(const MovesCount &);
    MovesCount& operator=(const MovesCount &);

    QNetworkReply *asyncGET(QString path, QString additionalHeaders, bool auth);
    QNetworkReply *syncGET(QString path, QString additionalHeaders, bool auth);

    QNetworkReply *asyncPOST(QString path, QString additionalHeaders, QByteArray &postData, bool auth);
    QNetworkReply *syncPOST(QString path, QString additionalHeaders, QByteArray &postData, bool auth);

    bool checkReplyAuthorization(QNetworkReply *reply);

    bool authorized = false;

    QString baseAddress;
    QString appkey;
    QString username;
    QString userkey;
    QString model;
    QString serial;
    ambit_device_info_t device_info;

    QNetworkAccessManager *manager;
    QNetworkReply *firmwareCheckReply = NULL;

    MovesCountJSON jsonParser;
};

#endif // MOVESCOUNT_H
