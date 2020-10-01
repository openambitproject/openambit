#include "sportmodegroup.h"
#include "libambit.h"

const QString CustomModeGroup::ACTIVITY_ID = "ActivityID";
const QString CustomModeGroup::SPORT_MODE_GROUPS_ID = "CustomModeGroupsID";
const QString CustomModeGroup::SPORT_MODE_IDS = "CustomModeIDs";
const QString CustomModeGroup::IS_VISIBLE = "IsVisible";
const QString CustomModeGroup::ACTIVITY_NAME = "Name";

CustomModeGroup::CustomModeGroup(QVariantMap &customModeGroupMap, QObject *parent) :
    QObject(parent)
{
    activityId = customModeGroupMap[ACTIVITY_ID].toUInt();
    customModeGroupsId = customModeGroupMap[SPORT_MODE_GROUPS_ID].toInt();
    isVisible = customModeGroupMap[IS_VISIBLE].toBool();
    activityName = customModeGroupMap[ACTIVITY_NAME].toString();

    foreach (QVariant customModeIdVar, customModeGroupMap[SPORT_MODE_IDS].toList()) {
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

void CustomModeGroup::toAmbitData(ambit_sport_mode_group_t *ambitCustomModeGroups)
{
    ambitCustomModeGroups->activity_id = activityId;
    ambitCustomModeGroups->sport_mode_group_id = customModeGroupsId;
    ambitCustomModeGroups->is_visible = isVisible;
    toAmbitName(ambitCustomModeGroups->activity_name);

    if (libambit_malloc_sport_mode_index(customModeIndex.count(), ambitCustomModeGroups)) {
        uint16_t *ambitCustomModeIndex = ambitCustomModeGroups->sport_mode_index;

        foreach (u_int16_t index, customModeIndex) {
            // Write the IDs/position/index for all the custom mode that is in this group.
            *ambitCustomModeIndex = index;
            ambitCustomModeIndex++;
        }
    }
}

void CustomModeGroup::toAmbitName(char ambitName[GROUP_NAME_SIZE])
{
    const QByteArray &source = activityName.toLatin1();
    int strLen = activityName.length() < GROUP_NAME_SIZE ? activityName.length() : GROUP_NAME_SIZE;
    memset(ambitName, 0x00, GROUP_NAME_SIZE);
    memcpy(ambitName, source.data(), strLen);
}
