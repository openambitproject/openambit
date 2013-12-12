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
#include <libambit.h>

void DeviceManager::start()
{
    connect(&chargeTimer, SIGNAL(timeout()), this, SLOT(chargeTimerHit()));
    chargeTimer.setInterval(10000);
    chargeTimer.start();
}

DeviceManager::~DeviceManager()
{
    chargeTimer.stop();
}

void DeviceManager::detect()
{
    int res = -1;

    mutex.lock();
    if (this->deviceObject != NULL) {
        libambit_close(this->deviceObject);
    }
    this->deviceObject = libambit_detect();

    if (this->deviceObject != NULL && (res = libambit_device_info_get(this->deviceObject, &this->currentDeviceInfo)) == 0) {
        emit deviceDetected(this->currentDeviceInfo, libambit_device_supported(this->deviceObject));
    }
    else {
        emit deviceRemoved();
    }
    mutex.unlock();

    if (res == 0) {
        chargeTimerHit();
    }
}

void DeviceManager::startSync(bool readAllLogs = false)
{
    int res = -1;

    mutex.lock();
    if (this->deviceObject != NULL) {
        emit this->syncProgressInform(QString(tr("Reading personal settings")), true, 0);
        res = libambit_personal_settings_get(this->deviceObject, &currentPersonalSettings);

        if (res != -1) {
            emit this->syncProgressInform(QString(tr("Reading log files")), true, 0);
            res = libambit_log_read(this->deviceObject, readAllLogs ? NULL : &log_skip_cb, &log_push_cb, &log_progress_cb, this);
        }
    }
    mutex.unlock();

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

int DeviceManager::log_skip_cb(void *ref, ambit_log_header_t *log_header)
{
    DeviceManager *manager = static_cast<DeviceManager*> (ref);
    if (manager->logStore.logExists(QString(manager->currentDeviceInfo.serial), log_header)) {
        return 0;
    }
    return 1;
}

void DeviceManager::log_push_cb(void *ref, ambit_log_entry_t *log_entry)
{
    DeviceManager *manager = static_cast<DeviceManager*> (ref);
    LogEntry *entry = manager->logStore.store(QString(manager->currentDeviceInfo.serial), &manager->currentPersonalSettings, log_entry);
    if (entry != NULL) {
        manager->movesCount.sendLog(entry);
    }

    delete entry;
}

void DeviceManager::log_progress_cb(void *ref, uint16_t log_count, uint16_t log_current, uint8_t progress_percent)
{
    DeviceManager *manager = static_cast<DeviceManager*> (ref);
    emit manager->syncProgressInform(QString(tr("Downloading message %1 of %2")).arg(log_current).arg(log_count), false, progress_percent);
}
