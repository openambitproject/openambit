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
#include "devicemanager.h"

#include <QTimer>
#include <QDebug>
#include <stdio.h>
#include <libambit.h>

DeviceManager::DeviceManager(QObject *parent) :
    QObject(parent), deviceObject(NULL), udevListener(NULL)
{
    movesCount = MovesCount::instance();
    currentPersonalSettings = libambit_personal_settings_alloc();
}

DeviceManager::~DeviceManager()
{
    mutex.lock();
    delete udevListener;
    chargeTimer.stop();

    if(currentPersonalSettings != NULL) {
        libambit_personal_settings_free(currentPersonalSettings);
    }
    if(deviceObject != NULL) {
        libambit_close(deviceObject);
    }

    mutex.unlock();
}

void DeviceManager::start()
{
    connect(&chargeTimer, SIGNAL(timeout()), this, SLOT(chargeTimerHit()));
    chargeTimer.setInterval(10000);
    chargeTimer.start();

    // Connect udev listener, fire a chargeTimerHit (implicit device detect), if hit
    udevListener = new UdevListener();
    connect(udevListener, SIGNAL(deviceEvent()), this, SLOT(chargeTimerHit()));

    // Connect movescount Id feedback to local handler
    connect(movesCount, SIGNAL(logMoveID(QString,QDateTime,QString)), this, SLOT(logMovescountID(QString,QDateTime,QString)));
}

void DeviceManager::detect()
{
    int res = -1;

    mutex.lock();
    if (this->deviceObject != NULL) {
        libambit_close(this->deviceObject);
        this->deviceObject = NULL;
        emit deviceRemoved();
    }

    ambit_device_info_t *devinfo = libambit_enumerate();
    if (devinfo) {
        this->currentDeviceInfo = *devinfo;
        emit deviceDetected(this->currentDeviceInfo);
        this->deviceObject = libambit_new(devinfo);
    }
    libambit_free_enumeration(devinfo);

    mutex.unlock();

    if (res == 0) {
        chargeTimerHit();
    }
}

void DeviceManager::startSync(bool readAllLogs = false)
{
    Settings settings;
    int res = -1;
    int waypoint_sync_res = -1;
    time_t current_time;
    struct tm *local_time;
    uint8_t *orbitData = NULL;
    int orbitDataLen;
    ambit_personal_settings_t *movecountPersonalSettings = libambit_personal_settings_alloc();

    bool syncTime = settings.value("syncSettings/syncTime", true).toBool();
    bool syncOrbit = settings.value("syncSettings/syncOrbit", true).toBool();
    bool syncSportMode = settings.value("syncSettings/syncSportMode", false).toBool();
    bool syncNavigation = settings.value("syncSettings/syncNavigation", false).toBool();
    bool syncMovescount = settings.value("movescountSettings/movescountEnable", false).toBool();

    mutex.lock();
    this->syncMovescount = syncMovescount;
    currentSyncPart = 0;
    syncParts = 2;
    if (syncTime) syncParts++;
    if (syncOrbit) syncParts+=2;
    if (syncSportMode) syncParts++;
    if (syncMovescount) syncParts++;

    if (this->deviceObject != NULL) {
        emit this->syncProgressInform(QString(tr("Reading personal settings")), false, true, 0);

        // Reading personal settings + waypoints
        res = libambit_personal_settings_get(this->deviceObject, currentPersonalSettings);
        waypoint_sync_res = libambit_navigation_read(this->deviceObject, currentPersonalSettings);
        currentSyncPart++;

        libambit_sync_display_show(this->deviceObject);

        if (syncTime && res != -1) {
            emit this->syncProgressInform(QString(tr("Setting date/time")), false, true, 100*currentSyncPart/syncParts);
            current_time = time(NULL);
            local_time = localtime(&current_time);
            res = libambit_date_time_set(this->deviceObject, local_time);
            currentSyncPart++;
        }

        if (res != -1) {
            qDebug() << "Start reading log...";
            emit this->syncProgressInform(QString(tr("Reading log files")), false, true, 100*currentSyncPart/syncParts);
            res = libambit_log_read(this->deviceObject, readAllLogs ? NULL : &log_skip_cb, &log_push_cb, &log_progress_cb, this);
            currentSyncPart++;
            qDebug() << "End reading log...";
        }

        if (waypoint_sync_res != -1 && syncNavigation) {
            qDebug() << "Start reading navigation...";
            emit this->syncProgressInform(QString(tr("Synchronizing navigation")), false, true, 100*currentSyncPart/syncParts);
            currentSyncPart++;

            if((movesCount->getPersonalSettings(movecountPersonalSettings, true)) != -1) {
                 movesCount->applyPersonalSettingsFromDevice(movecountPersonalSettings, currentPersonalSettings);
                 movesCount->writePersonalSettings(movecountPersonalSettings);
                 emit this->syncProgressInform(QString(tr("Write navigation")), false, false, 100*currentSyncPart/syncParts);
                 libambit_navigation_write(this->deviceObject, movecountPersonalSettings);
                 emit this->syncProgressInform(QString(tr("Synchronized navigation")), false, false, 100*currentSyncPart/syncParts);
            }
            qDebug() << "End reading navigation...";
        }

        if (syncSportMode && res != -1) {
            qDebug() << "Start sport mode";
            emit this->syncProgressInform(QString(tr("Fetching sport modes")), false, true, 100*currentSyncPart/syncParts);

            ambit_app_rules_t* ambitApps = liblibambit_malloc_app_rules();
            movesCount->getAppsData(ambitApps);

            ambit_sport_mode_device_settings_t *ambitDeviceSettings = libambit_malloc_sport_mode_device_settings();
            if (movesCount->getCustomModeData(ambitDeviceSettings) != -1) {
                emit this->syncProgressInform(QString(tr("Write sport modes")), false, false, 100*currentSyncPart/syncParts);
                res = libambit_sport_mode_write(this->deviceObject, ambitDeviceSettings);

                emit this->syncProgressInform(QString(tr("Write apps")), false, true, 100*currentSyncPart/syncParts);
                res = libambit_app_data_write(this->deviceObject, ambitDeviceSettings, ambitApps);
            }
            libambit_sport_mode_device_settings_free(ambitDeviceSettings);
            libambit_app_rules_free(ambitApps);

            currentSyncPart++;
            qDebug() << "End reading sport mode";
        }

        qDebug() << "Outer space debug message";

        if (syncOrbit && res != -1) {
            qDebug() << "Start sync Orbit";
            emit this->syncProgressInform(QString(tr("Fetching orbital data")), false, true, 100*currentSyncPart/syncParts);
            if ((orbitDataLen = movesCount->getOrbitalData(&orbitData)) != -1) {
                currentSyncPart++;
                emit this->syncProgressInform(QString(tr("Writing orbital data")), false, false, 100*currentSyncPart/syncParts);
                res = libambit_gps_orbit_write(this->deviceObject, orbitData, orbitDataLen);
                free(orbitData);
            }
            else {
                currentSyncPart++;
                emit this->syncProgressInform(QString(tr("Failed to get orbital data")), true, false, 100*currentSyncPart/syncParts);
                res = -1;
            }

            qDebug() << "End Orbit sync";

            currentSyncPart++;
        }

        libambit_sync_display_clear(this->deviceObject);
    }
    mutex.unlock();

    libambit_personal_settings_free(movecountPersonalSettings);
    movecountPersonalSettings = NULL;

    emit syncFinished(res >= 0);

    if (res == -1) {
        // Failed to read! We better try another detect
        detect();
    }
}

void DeviceManager::chargeTimerHit()
{
    int res = -1;
    ambit_device_status_t status;

    if (mutex.tryLock()) {
        if (this->deviceObject != NULL) {
            if ((res = libambit_device_status_get(this->deviceObject, &status)) == 0) {
                emit deviceCharge(status.charge);
            }
        }
        mutex.unlock();
    }
    else {
        res = 0;
    }

    if (res != 0) {
        // Failed to read! We better try another detect
        detect();
    }
}

void DeviceManager::logMovescountID(QString device, QDateTime time, QString moveID)
{
    logStore.storeMovescountId(device, time, moveID);
}

int DeviceManager::log_skip_cb(void *ref, ambit_log_header_t *log_header)
{
    DeviceManager *manager = static_cast<DeviceManager*> (ref);
    if (manager->logStore.logExists(manager->currentDeviceInfo.serial, log_header)) {
        return 0;
    }
    return 1;
}

void DeviceManager::log_push_cb(void *ref, ambit_log_entry_t *log_entry)
{
    DeviceManager *manager = static_cast<DeviceManager*> (ref);
    LogEntry *entry = manager->logStore.store(manager->currentDeviceInfo, manager->currentPersonalSettings, log_entry);
    if (entry != NULL) {
        //! TODO: make this optional, only used for debugging
        manager->movesCountXML.writeLog(entry);

        if (manager->syncMovescount) {
            manager->movesCount->writeLog(entry);
        }

        delete entry;
    }
}

void DeviceManager::log_progress_cb(void *ref, uint16_t log_count, uint16_t log_current, uint8_t progress_percent)
{
    DeviceManager *manager = static_cast<DeviceManager*> (ref);
    progress_percent = 100*manager->currentSyncPart/manager->syncParts + progress_percent*1/manager->syncParts;
    emit manager->syncProgressInform(QString(tr("Downloading log %1 of %2")).arg(log_current).arg(log_count), false, false, progress_percent);
}
