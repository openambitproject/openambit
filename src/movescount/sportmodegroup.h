#ifndef CUSTOMMODEGROUP_H
#define CUSTOMMODEGROUP_H

#include <QObject>
#include <QVariantMap>
#include <QList>
#include "libambit.h"

class CustomModeGroup : public QObject
{
    Q_OBJECT
public:
    explicit CustomModeGroup(QVariantMap &customModeGroupMap, QObject *parent = 0);
    CustomModeGroup(const CustomModeGroup &other);

    CustomModeGroup& operator=(const CustomModeGroup &rhs);

    void setCustomModeIndex(const QList<u_int16_t> &customModesList);

    void toAmbitData(ambit_sport_mode_group_t *ambitCustomModeGroups);

signals:

public slots:

private:
    void toAmbitName(char ambitName[]);

    int activityId;
    int customModeGroupsId;
    QList<int> customModeIds;
    bool isVisible;
    QString activityName;

    QList<uint> customModeIndex;

    static const QString ACTIVITY_ID;
    static const QString SPORT_MODE_GROUPS_ID;
    static const QString SPORT_MODE_IDS;
    static const QString IS_VISIBLE;
    static const QString ACTIVITY_NAME;

    static const int GROUP_NAME_SIZE = 24;
};

#endif // CUSTOMMODEGROUP_H
