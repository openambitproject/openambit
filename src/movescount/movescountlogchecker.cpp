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
        // set to running right here before actually starting the thread to avoid
        // race-conditions while the thread is not fully started yet
        running = true;

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
    MovesCount *movescount = MovesCount::instance();

    QList<LogStore::LogDirEntry> entries = logStore.dir();

    qDebug() << "Found:" << entries.count() << "logs in directory" <<
        QString(getenv("HOME"))<<"/.openambit"; // NOLINT(concurrency-mt-unsafe)

    // look at all logs stored locally
    foreach(LogStore::LogDirEntry entry, entries) {
        // This is a long operation, exit if application want to quit
        if (cancelRun) {
            cancelRun = false;
            return;
        }

        LogEntry *logEntry = logStore.read(entry);
        if (logEntry != NULL) {
            // take a closer look if there is no movescountId yet
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

    qDebug() << "Found: " << missingEntries.count() << " logs that are not uploaded";

    if (missingEntries.count() > 0) {
        // This is a long operation, exit if application want to quit
        if (cancelRun) {
            cancelRun = false;
            return;
        }

        // match the missing moves with all the known moves in that time-frame
        QList<MovesCountLogDirEntry> movescountEntries = movescount->getMovescountEntries(firstUnknown.date(), lastUnknown.date());
        foreach(MovesCountLogDirEntry entry, movescountEntries) {
            // This is a long operation, exit if application want to quit
            if (cancelRun) {
                cancelRun = false;
                return;
            }
            foreach(LogEntry *logEntry, missingEntries) {
                if (entry.time == logEntry->time) {
                    // we found a move already uploaded, so no need to upload and update the movescount-id in the logfile
                    missingEntries.removeOne(logEntry);
                    logStore.storeMovescountId(logEntry->device, logEntry->time, entry.moveId);
                    delete logEntry;
                    break;
                }
            }
        }

        qDebug() << "Having: " << missingEntries.count() << " remaining entries after pruning found logs";

        // Try to upload the remaining entries
        while (missingEntries.count() > 0) {
            // This is a long operation, exit if application want to quit
            if (cancelRun) {
                cancelRun = false;
                return;
            }
            LogEntry *logEntry = missingEntries.first();

            qDebug() << "Storing missing move from: " << logEntry->time;

            movescount->writeLog(logEntry);
            missingEntries.removeOne(logEntry);
            delete logEntry;
        }
    }

    cancelRun = false;
    running = false;
}

QString MovesCountLogChecker::status() {
    return QString("Running: ") + (running ? "true" : "false") +
        ", cancelled: " + (cancelRun ? "true" : "false") +
        ", entries to process: " + QString::number(missingEntries.count());
}
