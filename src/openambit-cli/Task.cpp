//
// Created by dstadler on 19.10.19.
//

#include "Task.h"
#include <movescount/movescount.h>
#include <movescount/logstore.h>
#include <movescount/movescountxml.h>
#include <libambit_int.h>

#define APPKEY                 "DbCBVqja20NKdrimBHQxtYIdczUJ56WHIWlC6A7vp6NPC0D0a8wA5d0ODyywFKe6"
#define MOVESCOUNT_DEFAULT_URL "https://uiservices.movescount.com/"

MovesCount *movesCountSetup(const char *username, const char *userkey);
void startSync(ambit_object_t *deviceObject, ambit_personal_settings_t *currentPersonalSettings, MovesCount *movesCount,
               bool readAllLogs, bool syncTime, bool syncOrbit, bool syncSportMode, bool syncNavigation, bool writeLogs,
               bool writeSettingsJSON, const char *settingsInputFile, const char* appInputFile, Task *task);
static int log_skip_cb(void *ambit_object, ambit_log_header_t *log_header);
static void log_data_cb(void *object, ambit_log_entry_t *log_entry);

int readSportModesFromFile(const char *file, ambit_sport_mode_device_settings_t *ambitDeviceSettings);
int readAppsFromFile(const char *file, ambit_app_rules_t* ambitApps);

LogStore logStore;
MovesCountXML movesCountXML;

typedef struct syncData_s {
    ambit_object_t *deviceObject;
    ambit_personal_settings_t *currentPersonalSettings;
    bool syncMovescount;
    MovesCount *movesCount;
} syncData_t;

void Task::run() {
    printf("Running openambit-cli\n");

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
                printf("Connecting to movescount\n");
                MovesCount *movesCount = movesCountSetup(username, userkey);
                if (movesCount != NULL) {
                    printf("Connected\n");

                    //connect(movesCount, SIGNAL(movesCountAuth(bool)), this, SLOT(movesCountAuth(bool)), Qt::QueuedConnection);

                    DeviceInfo deviceInfo = DeviceInfo();

                    deviceInfo = *info;

                    movesCount->setDevice(deviceInfo);
                    //movesCount->getDeviceSettings();

                    printf("Having device: %s/%s/%s/%d.%d.%d/%d.%d.%d/%d/%d\n",
                           deviceInfo.name.toStdString().c_str(),
                           deviceInfo.model.toStdString().c_str(),
                           deviceInfo.serial.toStdString().c_str(),
                           deviceInfo.fw_version[0],
                           deviceInfo.fw_version[1],
                           deviceInfo.fw_version[2],
                           deviceInfo.hw_version[0],
                           deviceInfo.hw_version[1],
                           deviceInfo.hw_version[2],
                           deviceInfo.access_status,
                           deviceInfo.is_supported);

                    startSync(ambit_object, &settings, movesCount, readAllLogs, syncTime, syncOrbit, syncSportMode,
                              syncNavigation, writeLogs, writeSettingsJSON, settingsInputFile, appInputFile, this);

                    if(settings.waypoints.data != NULL) {
                        free(settings.waypoints.data);
                    }

                    movesCount->exit();
                }
            }

            libambit_close(ambit_object);
        }
    }
    else {
        printf("ERROR: No clock found, exiting\n");

        hasError();

        emit error();

        return;
    }

    libambit_free_enumeration(info);

    if (isError) {
        emit error();
    } else {
        emit finished();
    }
}

void Task::hasError() {
    isError = true;
}

void Task::error() {
    ((QCoreApplication*)parent())->exit(1);
}

MovesCount *movesCountSetup(const char *username, const char *userkey)
{
    printf("movesCountSetup\n");
    MovesCount *movesCount = MovesCount::instance();
    movesCount->setAppkey(APPKEY);
    movesCount->setBaseAddress(MOVESCOUNT_DEFAULT_URL);

    if(userkey != NULL) {
        movesCount->setUserkey(/*movesCount->generateUserkey()*/ userkey);
    }

    //connect(movesCount, SIGNAL(newerFirmwareExists(QByteArray)), this, SLOT(newerFirmwareExists(QByteArray)), Qt::QueuedConnection);

    if(username != NULL) {
        movesCount->setUsername(username);
    }

    // don't upload logs in the background for now
    movesCount->setUploadLogs(false);

    return movesCount;
}

void startSync(ambit_object_t *deviceObject, ambit_personal_settings_t *currentPersonalSettings, MovesCount *movesCount,
               bool readAllLogs, bool syncTime, bool syncOrbit, bool syncSportMode, bool syncNavigation,
               bool writeLogs, bool writeSettingsJSON, const char *settingsInputFile, const char *appInputFile, Task *task)
{
    ambit_personal_settings_t *movecountPersonalSettings = libambit_personal_settings_alloc();

    if (deviceObject != NULL) {
        // Reading personal settings + waypoints
        int res = libambit_personal_settings_get(deviceObject, currentPersonalSettings);
        int waypoint_sync_res = libambit_navigation_read(deviceObject, currentPersonalSettings);

        libambit_sync_display_show(deviceObject);

        if (syncTime && res != -1) {
            qDebug() << "Start time sync...";

            time_t current_time = time(NULL);
            struct tm *local_time = localtime(&current_time);
            res = libambit_date_time_set(deviceObject, local_time);
            if (res == -1) {
                qDebug() << "ERROR: Failed to sync time";

                task->hasError();
            }

            qDebug() << "End time sync";
        }

        if (res != -1) {
            if (!readAllLogs) {
                qDebug() << "Not reading log for now...";
            } else {
                qDebug() << "Start reading log...";

                // Exit with an exit-code if uploading fails
                QObject::connect(movesCount, SIGNAL(uploadError(QByteArray)), task, SLOT(error()));

                syncData_t syncData;
                syncData.deviceObject = deviceObject;
                syncData.currentPersonalSettings = currentPersonalSettings;
                syncData.syncMovescount = writeLogs;
                syncData.movesCount = movesCount;

                res = libambit_log_read(deviceObject, &log_skip_cb, &log_data_cb, NULL, &syncData);
                if (res == -1) {
                    qDebug() << "ERROR: Failed to read logs";

                    task->hasError();
                }

                qDebug() << "End reading log...";
            }
        }

        if (waypoint_sync_res != -1 && syncNavigation) {
            qDebug() << "Start reading navigation...";

            qDebug() << "Get Personal Settings";
            if((movesCount->getPersonalSettings(movecountPersonalSettings, true)) != -1) {
                movesCount->applyPersonalSettingsFromDevice(movecountPersonalSettings, currentPersonalSettings);
                movesCount->writePersonalSettings(movecountPersonalSettings);
                libambit_navigation_write(deviceObject, movecountPersonalSettings);
            } else {
                qDebug() << "ERROR: Failed to read navigation";

                task->hasError();
            }
            qDebug() << "End reading navigation...";
        }

        if ((syncSportMode || (settingsInputFile != NULL && appInputFile != NULL)) && res != -1) {
            qDebug() << "Start sport mode";

            ambit_app_rules_t* ambitApps = liblibambit_malloc_app_rules();
            if(appInputFile != NULL) {
                qDebug() << "Reading apps from '" << appInputFile << "'";
                res = readAppsFromFile(appInputFile, ambitApps);
            } else {
                qDebug() << "Reading apps from Movescount";
                res = movesCount->getAppsData(ambitApps);
            }

            ambit_sport_mode_device_settings_t *ambitDeviceSettings = libambit_malloc_sport_mode_device_settings();

            if (res != -1) {
                if (settingsInputFile != NULL) {
                    qDebug() << "Reading sport-modes from '" << settingsInputFile << "'";
                    res = readSportModesFromFile(settingsInputFile, ambitDeviceSettings);
                } else {
                    qDebug() << "Reading sport-modes from Movescount";
                    res = movesCount->getCustomModeData(ambitDeviceSettings);
                }
            }

            if (res != -1) {
                qDebug() << "Writing " << ambitDeviceSettings->sport_modes_count << " sport modes and " <<
                         ambitDeviceSettings->sport_mode_groups_count << " sport mode groups";

                res = libambit_sport_mode_write(deviceObject, ambitDeviceSettings);
                if (res == -1) {
                    qDebug() << "ERROR: Failed to write sport mode";

                    task->hasError();
                }

                qDebug() << "Writing " << ambitApps->app_rules_count << " applications";
                res = libambit_app_data_write(deviceObject, ambitDeviceSettings, ambitApps);
                if (res == -1) {
                    qDebug() << "ERROR: Failed to write app data";

                    task->hasError();
                }
            } else {
                qDebug() << "ERROR: Could not read custom mode data";

                task->hasError();
            }

            libambit_sport_mode_device_settings_free(ambitDeviceSettings);
            libambit_app_rules_free(ambitApps);

            qDebug() << "End reading/writing sport mode";
        }

        if (syncOrbit && res != -1) {
            qDebug() << "Start sync orbit data";
            uint8_t *orbitData = NULL;
            int orbitDataLen;
            if ((orbitDataLen = movesCount->getOrbitalData(&orbitData)) != -1) {
                res = libambit_gps_orbit_write(deviceObject, orbitData, orbitDataLen);
                if (res == -1) {
                    qDebug() << "ERROR: Failed to write orbit data";

                    task->hasError();
                }
                free(orbitData);
            }
            else {
                qDebug() << "ERROR: Failed to sync orbit data";

                task->hasError();
            }

            qDebug() << "End orbit data sync";
        }

        if (writeSettingsJSON) {
            qDebug() << "Start sync watch apps to JSON";

            ambit_app_rules_t *ambitApps = liblibambit_malloc_app_rules();
            int ret = movesCount->getWatchAppConfig(ambitApps);
            if (ret != -1) {
                ambit_sport_mode_device_settings_t *ambitDeviceSettings = libambit_malloc_sport_mode_device_settings();
                ret = movesCount->getWatchModeConfig(ambitDeviceSettings);
                if (ret == -1) {
                    qDebug() << "ERROR: Failed to get WatchModeConfig to JSON";

                    task->hasError();
                }

                qDebug() << "End sync watch apps to JSON";

                libambit_sport_mode_device_settings_free(ambitDeviceSettings);
                libambit_app_rules_free(ambitApps);
            } else {
                qDebug() << "ERROR: Failed to sync watch apps to JSON";

                task->hasError();
            }
        }

        libambit_sync_display_clear(deviceObject);
    }

    libambit_personal_settings_free(movecountPersonalSettings);
}

int readSportModesFromFile(const char *settingsInputFile, ambit_sport_mode_device_settings_t *ambitDeviceSettings) {
    MovescountSettings settings = MovescountSettings();

    MovesCountJSON jsonParser;

    QFile jsonFile(settingsInputFile);
    jsonFile.open(QFile::ReadOnly);
    QByteArray data = jsonFile.read(9999999);

    if (data.length() == 0) {
        qDebug() << "Failed to read settings file from " << settingsInputFile;
        return -1;
    } else {
        if(jsonParser.parseDeviceSettingsReply(data, settings) == -1) {
            return -1;
        }
    }

    settings.toAmbitData(ambitDeviceSettings);

    return 0;
}

int readAppsFromFile(const char *settingsInputFile, ambit_app_rules_t* ambitApps) {
    MovescountSettings settings = MovescountSettings();

    MovesCountJSON jsonParser;

    QFile jsonFile(settingsInputFile);
    jsonFile.open(QFile::ReadOnly);
    QByteArray data = jsonFile.read(9999999);

    if (data.length() == 0) {
        qDebug() << "Failed to read settings file from " << settingsInputFile;
        return -1;
    } else {
        if (jsonParser.parseAppRulesReply(data, ambitApps) == -1) {
            return -1;
        }
    }

    return 0;
}

static int log_skip_cb(void *object, ambit_log_header_t *log_header)
{
    syncData_t *syncData = static_cast<syncData_t *>(object);

    bool exists = logStore.logExists(syncData->deviceObject->device_info.serial, log_header);

    printf("Got log header \"%20s\" %d-%02d-%02d %02d:%02d:%02d - exists: %d\n",
            log_header->activity_name, log_header->date_time.year, log_header->date_time.month,
            log_header->date_time.day, log_header->date_time.hour, log_header->date_time.minute,
            log_header->date_time.msec/1000, exists);

    if (exists) {
        return 0;
    }

    return 1;
}

static void log_data_cb(void *object, ambit_log_entry_t *log_entry)
{
    syncData_t *syncData = static_cast<syncData_t *>(object);

    printf("Storing log entry \"%17s\" %d-%02d-%02d %02d:%02d:%02d\n", log_entry->header.activity_name, log_entry->header.date_time.year, log_entry->header.date_time.month, log_entry->header.date_time.day, log_entry->header.date_time.hour, log_entry->header.date_time.minute, log_entry->header.date_time.msec/1000);

    DeviceInfo deviceInfo;
    deviceInfo = syncData->deviceObject->device_info;

    LogEntry *entry = logStore.store(deviceInfo, syncData->currentPersonalSettings, log_entry);
    if (entry != NULL) {
        movesCountXML.writeLog(entry);

        if (syncData->syncMovescount) {
            syncData->movesCount->writeLog(entry);
        } else {
            printf("Not sending log to movescount");
        }

	libambit_log_synced(syncData->deviceObject, log_entry);
	
        delete entry;
    }
}
