#ifndef MOVESCOUNTLOGDIRENTRY_H
#define MOVESCOUNTLOGDIRENTRY_H

#include <QObject>
#include <QDateTime>

class MovesCountLogDirEntry : public QObject
{
    Q_OBJECT
public:
    explicit MovesCountLogDirEntry(QString moveId, QDateTime time, u_int8_t activityId, QObject *parent = 0);
    MovesCountLogDirEntry(const MovesCountLogDirEntry &other);
    ~MovesCountLogDirEntry();

    MovesCountLogDirEntry& operator=(const MovesCountLogDirEntry &rhs);

    QString moveId;
    QDateTime time;
    u_int8_t activityId = 0;
signals:

public slots:

};

#endif // MOVESCOUNTLOGDIRENTRY_H
