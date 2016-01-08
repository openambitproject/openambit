#ifndef CUSTOMMODEGROUP_H
#define CUSTOMMODEGROUP_H

#include <QObject>
#include <QVariantMap>
#include <QList>

class CustomModeGroup : public QObject
{
    Q_OBJECT
public:
    explicit CustomModeGroup(QVariantMap &customModeGroupMap, QObject *parent = 0);
    CustomModeGroup(const CustomModeGroup &other);

    CustomModeGroup& operator=(const CustomModeGroup &rhs);

    void setCustomModeIndex(const QList<u_int16_t> &customModesList);

    uint serialize(u_int8_t *data);
    static void serializeStartHeader(u_int16_t length, u_int8_t *data);
signals:

public slots:

private:
    uint serializeName(u_int8_t *data);
    void serializeHeader(u_int16_t header_nbr, u_int16_t length, u_int8_t *dataWrite);
    uint serializeActivityId(u_int8_t *data);
    uint serializeModesId(u_int8_t *data);

    int activityId;
    int customModeGroupsId;
    QList<int> customModeIds;
    bool isVisible;
    QString activityName;

    QList<uint> customModeIndex;

    static const QString ACTIVITY_ID;
    static const QString CUSTOM_MODE_GROUPS_ID;
    static const QString CUSTOM_MODE_IDS;
    static const QString IS_VISIBLE;
    static const QString ACTIVITY_NAME;

    static const int HEADER_SIZE = 4;
    static const int NAME_PAYLOAD_SIZE = 24;
    static const int ACTIVITY_ID_PAYLOAD_SIZE = 2;
    static const int MODES_ID_PAYLOAD_SIZE = 2;

    static const int START_HEADER = 0x0200;
    static const int GROUP_HEADER = 0x0210;
    static const int NAME_HEADER = 0x0212;
    static const int ACTIVITY_ID_HEADER = 0x0213;
    static const int MODES_ID_HEADER = 0x0214;
};

#endif // CUSTOMMODEGROUP_H
