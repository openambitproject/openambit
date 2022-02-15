#ifndef OPENAMBIT_TASK_H
#define OPENAMBIT_TASK_H

#include <QtCore>

class Task : public QObject
{
    Q_OBJECT
public:
    Task(QObject *parent, const char *directory) : QObject(parent) {
        this->directory = directory;
    }

    void hasError();

public slots:
    void run();
    void error(QByteArray data);

signals:
    void finished();

private:
    const char* directory;

    bool isError = false;
};

#endif //OPENAMBIT_TASK_H
