#include <movescount/logstore.h>
#include "doctest.h"

TEST_SUITE_BEGIN("LogStore");

TEST_CASE("testing storing LogEntry") {
    LogStore* store = new LogStore();

    LogEntry entry = LogEntry();

    // store
    LogEntry *back = store->store(&entry);
    delete back;

    delete store;
}

/*
    class LogDirEntry
    {
    public:
        QString device;
        QDateTime time;
        QString filename;
    };

    LogEntry *store(const DeviceInfo& deviceInfo, ambit_personal_settings_t *personalSettings, ambit_log_entry_t *logEntry);
    LogEntry *store(LogEntry *entry);
    void storeMovescountId(QString device, QDateTime time, QString movescountId);
    bool logExists(QString device, ambit_log_header_t *logHeader);
    LogEntry *read(QString device, QDateTime time);
    LogEntry *read(LogDirEntry dirEntry);
    LogEntry *read(QString filename);
    QList<LogDirEntry> dir(QString device = "");
*/

TEST_SUITE_END();