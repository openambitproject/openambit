//
// Created by dstadler on 19.10.19.
//

#include "Task.h"
#include <thread>
#include <movescount/movescount.h>
#include <movescount/movescountjson.h>
#include <libambit_int.h>

void startSync(ambit_object_t *deviceObject, ambit_personal_settings_t *currentPersonalSettings, const char *directory,
               Task *task);

void Task::run() {
    printf("Running openambit-routes\n");

    ambit_device_info_t *info = libambit_enumerate();
    ambit_device_status_t status;
    ambit_personal_settings_t settings;
    memset(&settings, 0, sizeof(ambit_personal_settings_t));

    if (info) {
        printf("Device: %s, serial: %s\n", info->name, info->serial);
        if (0 == info->access_status) {
            printf("F/W version: %d.%d.%d\n", info->fw_version[0], info->fw_version[1], (info->fw_version[2] << 0) | (info->fw_version[3] << 8));
            if (!info->is_supported) {
                printf("ERROR: Device is not supported yet!\n");

                hasError();
            }
        }
        else {
            printf("%s: %s\n", info->path, strerror(info->access_status));
        }

        ambit_object_t *ambit_object = libambit_new(info);
        if (ambit_object) {
            if (libambit_device_status_get(ambit_object, &status) == 0) {
                printf("Current charge: %d%%\n", status.charge);
            } else {
                printf("ERROR: Failed to read status\n");

                hasError();
            }

            if (libambit_personal_settings_get(ambit_object, &settings) == 0) {
                printf("Personal settings: \n");
                printf("weight: %d\n", settings.weight);
                printf("birthyear: %d\n", settings.birthyear);
                printf("max_hr: %d\n", settings.max_hr);
                printf("rest_hr: %d\n", settings.rest_hr);
                printf("fitness_level: %d\n", settings.fitness_level);
                printf("is_male: %d\n", settings.is_male);
                printf("length: %d\n", settings.length);
            } else {
                printf("ERROR: Failed to read personal settings\n");

                hasError();
            }

            if (0 != info->access_status || !info->is_supported) {
                printf("ERROR: Device not supported\n");

                hasError();
            } else {
                printf("Storing routes from %s on the watch\n", directory);

                startSync(ambit_object, &settings, directory, this);

            }

            libambit_close(ambit_object);
        }
    }
    else {
        printf("ERROR: No clock found, exiting\n");

        hasError();

        emit error(NULL);

        return;
    }

    libambit_free_enumeration(info);

    if (isError) {
        emit error(NULL);
    } else {
        emit finished();
    }
}

void Task::hasError() {
    isError = true;
}

void Task::error(QByteArray data) {
    printf("ERROR: Exiting\n");
    ((QCoreApplication*)parent())->exit(1);
}

void startSync(ambit_object_t *deviceObject, ambit_personal_settings_t *currentPersonalSettings, const char* directory,
               Task *task)
{
    if (deviceObject != NULL) {
        // Reading personal settings + waypoints
        int res = libambit_personal_settings_get(deviceObject, currentPersonalSettings);
        if (res == -1) {
            qDebug() << "Error reading personal settings from watch";

            task->hasError();

            return;
        }

        int waypoint_sync_res = libambit_navigation_read(deviceObject, currentPersonalSettings);

        libambit_sync_display_show(deviceObject);

        if (waypoint_sync_res != -1) {
            qDebug() << "Start reading navigation...";

            ambit_personal_settings_t *newPersonalSettings = libambit_personal_settings_alloc();
            memset(newPersonalSettings, 0, sizeof(ambit_personal_settings_t));

            qDebug() << "Get Personal Settings";

            QFile settingsFile(QString(directory).append("/personal_settings.json"));
            qDebug() << "Reading settings from " << settingsFile;

            if (!settingsFile.exists()) {
                qDebug() << "Settings file " << settingsFile << " does not exist!";
            }
            if (!settingsFile.open(QIODevice::ReadOnly)){
                qDebug() << "Settings file " << settingsFile << " not available for reading!";
            }

            QByteArray _data = settingsFile.readAll();

            if (_data.length() == 0) {
                qDebug() << "Reading settings from " << settingsFile << " failed";

                task->hasError();
            } else {
                MovesCountJSON jsonParser;
                QString dir(directory);
                if ((jsonParser.parsePersonalSettings(_data, newPersonalSettings, NULL, dir)) != -1) {
                    MovesCount *movesCount = MovesCount::instance();
                    movesCount->applyPersonalSettingsFromDevice(newPersonalSettings, currentPersonalSettings);
                    //movesCount->writePersonalSettings(newPersonalSettings);
                    libambit_navigation_write(deviceObject, newPersonalSettings);

                    qDebug() << "Written routes from " << directory;

                    movesCount->exit();
                } else {
                    qDebug() << "ERROR: Failed to read navigation-data from directory " << directory;

                    task->hasError();
                }
            }
            qDebug() << "End reading navigation";

            libambit_personal_settings_free(newPersonalSettings);
        }

        if(currentPersonalSettings->waypoints.data != NULL) {
            free(currentPersonalSettings->waypoints.data);
            currentPersonalSettings->waypoints.data = NULL;
        }

        libambit_sync_display_clear(deviceObject);
    }
}
