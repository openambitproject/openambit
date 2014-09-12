#include "movescountlogchecker.h"
#include "movescount.h"

MovesCountLogChecker::MovesCountLogChecker(QObject *parent) :
    QObject(parent), running(false), cancelRun(false)
{
    this->moveToThread(&workerThread);
    workerThread.start();
}

MovesCountLogChecker::~MovesCountLogChecker()
{
    cancel();
    workerThread.exit();
    workerThread.wait();
}

void MovesCountLogChecker::run()
{
    if (!running) {
        QMetaObject::invokeMethod(this, "checkUploadedLogs", Qt::AutoConnection);
    }
}

bool MovesCountLogChecker::isRunning()
{
    return running;
}

void MovesCountLogChecker::cancel()
{
    cancelRun = true;
}

void MovesCountLogChecker::checkUploadedLogs()
{
    QDateTime firstUnknown = QDateTime::currentDateTime();
    QDateTime lastUnknown = QDateTime::fromTime_t(0);
    QList<LogEntry*> missingEntries;
    MovesCount *movescount = MovesCount::instance();

    running = true;

    QList<LogStore::LogDirEntry> entries = logStore.dir();
    foreach(LogStore::LogDirEntry entry, entries) {
        // This is a long operation, exit if application want to quit
        if (cancelRun) {
            cancelRun = false;
            return;
        }
        LogEntry *logEntry = logStore.read(entry);
        if (logEntry != NULL) {
            if (logEntry->movescountId.length() == 0) {
                missingEntries.append(logEntry);
                if (logEntry->time < firstUnknown) {
                    firstUnknown = logEntry->time;
                }
                if (logEntry->time > lastUnknown) {
                    lastUnknown = logEntry->time;
                }
            }
            else {
                delete logEntry;
            }
        }
    }

    if (missingEntries.count() > 0) {
        // This is a long operation, exit if application want to quit
        if (cancelRun) {
            cancelRun = false;
            return;
        }
        QList<MovesCountLogDirEntry> movescountEntries = movescount->getMovescountEntries(firstUnknown.date(), lastUnknown.date());
        foreach(MovesCountLogDirEntry entry, movescountEntries) {
            // This is a long operation, exit if application want to quit
            if (cancelRun) {
                cancelRun = false;
                return;
            }
            foreach(LogEntry *logEntry, missingEntries) {
                if (entry.time == logEntry->time) {
                    missingEntries.removeOne(logEntry);
                    logStore.storeMovescountId(logEntry->device, logEntry->time, entry.moveId);
                    delete logEntry;
                    break;
                }
            }
        }

        // Delete remaining entries
        while (missingEntries.count() > 0) {
            // This is a long operation, exit if application want to quit
            if (cancelRun) {
                cancelRun = false;
                return;
            }
            LogEntry *logEntry = missingEntries.first();
            movescount->writeLog(logEntry);
            missingEntries.removeOne(logEntry);
            delete logEntry;
        }
    }

    cancelRun = false;
    running = false;
}
