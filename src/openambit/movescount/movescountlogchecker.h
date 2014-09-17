#ifndef MOVESCOUNTLOGCHECKER_H
#define MOVESCOUNTLOGCHECKER_H

#include <QObject>
#include <QThread>

#include <libambit.h>

#include "logentry.h"
#include "logstore.h"

class MovesCountLogChecker : public QObject
{
    Q_OBJECT
public:
    explicit MovesCountLogChecker(QObject *parent = 0);
    ~MovesCountLogChecker();
    void run();
    bool isRunning();
    void cancel();
private slots:
    void checkUploadedLogs();

private:
    bool running;
    bool cancelRun;

    LogStore logStore;
    QThread workerThread;
};

#endif // MOVESCOUNTLOGCHECKER_H
