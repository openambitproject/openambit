#include "custommodegroup.h"
#include "libambit.h"

#include <QDebug>

const QString CustomModeGroup::ACTIVITY_ID = "ActivityID";
const QString CustomModeGroup::CUSTOM_MODE_GROUPS_ID = "CustomModeGroupsID";
const QString CustomModeGroup::CUSTOM_MODE_IDS = "CustomModeIDs";
const QString CustomModeGroup::IS_VISIBLE = "IsVisible";
const QString CustomModeGroup::ACTIVITY_NAME = "Name";

CustomModeGroup::CustomModeGroup(QVariantMap &customModeGroupMap, QObject *parent) :
    QObject(parent)
{
    activityId = customModeGroupMap[ACTIVITY_ID].toUInt();
    customModeGroupsId = customModeGroupMap[CUSTOM_MODE_GROUPS_ID].toInt();
    isVisible = customModeGroupMap[IS_VISIBLE].toBool();
    activityName = customModeGroupMap[ACTIVITY_NAME].toString();

    foreach (QVariant customModeIdVar, customModeGroupMap[CUSTOM_MODE_IDS].toList()) {
        customModeIds.append(customModeIdVar.toInt());
    }
}

CustomModeGroup::CustomModeGroup(const CustomModeGroup &other) :
    QObject(other.parent()),
    activityId(other.activityId),
    customModeGroupsId(other.customModeGroupsId),
    customModeIds(other.customModeIds),
    isVisible(other.isVisible),
    activityName(other.activityName),
    customModeIndex(other.customModeIndex)
{
}

CustomModeGroup &CustomModeGroup::operator=(const CustomModeGroup &rhs)
{
    activityId = rhs.activityId;
    customModeGroupsId = rhs.customModeGroupsId;
    customModeIds = rhs.customModeIds;
    isVisible = rhs.isVisible;
    activityName = rhs.activityName;
    customModeIndex = rhs.customModeIndex;

    return *this;
}

void CustomModeGroup::setCustomModeIndex(const QList<u_int16_t> &customModesList)
{
    foreach (int customModeId, customModeIds) {
        customModeIndex.append(customModesList.indexOf(customModeId));
    }
}

void CustomModeGroup::serializeStartHeader(u_int16_t length ,u_int8_t *data)
{
    ambit_write_header_t *header = (ambit_write_header_t*)data;
    header->header = START_HEADER;
    header->length = length;
}

uint CustomModeGroup::serialize(u_int8_t *data)
{
    u_int8_t *dataWrite = data + HEADER_SIZE;

    dataWrite += serializeName(dataWrite);
    dataWrite += serializeActivityId(dataWrite);
    dataWrite += serializeModesId(dataWrite);

    serializeHeader(GROUP_HEADER, dataWrite - data - HEADER_SIZE, data);

    return dataWrite - data;
}

void CustomModeGroup::serializeHeader(u_int16_t header_nbr, u_int16_t length, u_int8_t *dataWrite)
{
    ambit_write_header_t *header = (ambit_write_header_t*)dataWrite;
    header->header = header_nbr;
    header->length = length;
}

uint CustomModeGroup::serializeName(u_int8_t *data)
{
    u_int8_t *dataWrite = data + HEADER_SIZE;

    char *str = (char*)dataWrite;
    const char *source = activityName.toStdString().c_str();
    int strLen = activityName.length() < NAME_PAYLOAD_SIZE ? activityName.length() : NAME_PAYLOAD_SIZE;
    memset(str, 0, NAME_PAYLOAD_SIZE);
    memcpy(str, source, strLen);

    serializeHeader(NAME_HEADER, NAME_PAYLOAD_SIZE, data);

    return NAME_PAYLOAD_SIZE + HEADER_SIZE;
}

uint CustomModeGroup::serializeActivityId(u_int8_t *data)
{
    u_int8_t *dataWrite = data + HEADER_SIZE;

    *(u_int16_t*)dataWrite = activityId;
    serializeHeader(ACTIVITY_ID_HEADER, ACTIVITY_ID_PAYLOAD_SIZE, data);

    return ACTIVITY_ID_PAYLOAD_SIZE + HEADER_SIZE;
}

uint CustomModeGroup::serializeModesId(u_int8_t *data)
{
    u_int8_t *dataWrite = data;// + HEADER_SIZE;

    foreach (u_int16_t index, customModeIndex) {
        serializeHeader(MODES_ID_HEADER, MODES_ID_PAYLOAD_SIZE, dataWrite);
        dataWrite += HEADER_SIZE;

        // Write the IDs/position/index for all the custom mode that is in this group.
        *(u_int16_t*)dataWrite = index;
        dataWrite += 2;
    }

    return dataWrite - data;
}
