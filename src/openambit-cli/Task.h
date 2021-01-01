#ifndef OPENAMBIT_TASK_H
#define OPENAMBIT_TASK_H

#include <QtCore>

class Task : public QObject
{
    Q_OBJECT
public:
    Task(QObject *parent, const char *username, const char *userkey,
         bool readAllLogs, bool syncTime, bool syncOrbit, bool syncSportMode, bool syncNavigation,
         bool writeLogs, bool writeSettingsJSON, const char *settingsInputFile) : QObject(parent) {
        this->username = username;
        this->userkey = userkey;
        this->readAllLogs = readAllLogs;
        this->syncTime = syncTime;
        this->syncOrbit = syncOrbit;
        this->syncSportMode = syncSportMode;
        this->syncNavigation = syncNavigation;
        this->writeLogs = writeLogs;
        this->writeSettingsJSON = writeSettingsJSON;
        this->settingsInputFile = settingsInputFile;
    }

public slots:
    void run();

signals:
    void finished();

private:
    const char* username;
    const char* userkey;
    bool readAllLogs;
    bool syncTime;
    bool syncOrbit;
    bool syncSportMode;
    bool syncNavigation;
    bool writeLogs;
    bool writeSettingsJSON;
    const char* settingsInputFile;
};

#endif //OPENAMBIT_TASK_H
