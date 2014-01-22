#include "movescountlogdirentry.h"

MovesCountLogDirEntry::MovesCountLogDirEntry(QString moveId, QDateTime time, u_int8_t activityId, QObject *parent) :
    QObject(parent), moveId(moveId), time(time), activityId(activityId)
{
}

MovesCountLogDirEntry::MovesCountLogDirEntry(const MovesCountLogDirEntry &other) :
    QObject(other.parent()), moveId(other.moveId), time(other.time), activityId(other.activityId)
{
}

MovesCountLogDirEntry::~MovesCountLogDirEntry()
{
}

MovesCountLogDirEntry& MovesCountLogDirEntry::operator=(const MovesCountLogDirEntry &rhs)
{
    moveId = rhs.moveId;
    time = rhs.time;
    activityId = rhs.activityId;

    return *this;
}
