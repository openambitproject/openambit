#include "movescountsettings.h"

MovescountSettings::MovescountSettings(QObject *parent) :
    QObject(parent)
{

}

void MovescountSettings::parse(QVariantMap settingsMap)
{
    QList<u_int16_t> customModeIndex;
    foreach (QVariant customModeVar, settingsMap["CustomModes"].toList()) {
        QVariantMap customModeMap = customModeVar.toMap();

        CustomMode customMode(customModeMap, this);
        customModes.append(customMode);

        customModeIndex.append(customMode.getCustomModeId());
    }
    foreach (QVariant customModeGroupVar, settingsMap["CustomModeGroups"].toList()) {
        QVariantMap customModeGroupMap = customModeGroupVar.toMap();

        CustomModeGroup customModeGroup(customModeGroupMap, this);
        customModeGroups.append(customModeGroup);
    }

    for (int i = 0; i < customModeGroups.count(); i++) {
        customModeGroups[i].setCustomModeIndex(customModeIndex);
    }
}

MovescountSettings::MovescountSettings(const MovescountSettings &other) :
    QObject(other.parent()),
    customModeGroups(other.customModeGroups),
    customModes(other.customModes)
{
}

MovescountSettings& MovescountSettings::operator=(MovescountSettings &rhs)
{
    customModeGroups = rhs.customModeGroups;
    customModes = rhs.customModes;

    return *this;
}

uint MovescountSettings::serializeCustomMode(u_int8_t **data)
{
    u_int8_t *writePosition;
    writePosition = *data + 4; //Save space for header.

    // TODO Check for every write that it is inside the range.
    writePosition += serializeCustomModes(writePosition);

    writePosition += serializeCustomModeGroups(writePosition);

    serializeHeader(0x0003, writePosition - *data - 4, *data);

    return writePosition - *data;
}

uint MovescountSettings::serializeCustomModes(u_int8_t *data)
{
    u_int8_t *writePosition = data + 4; //Save space for header.

    writePosition += serializeUnknownDataField(writePosition);

    foreach (CustomMode customMode, customModes) {
        writePosition += customMode.serialize(writePosition);
    }

    // Write header at the end, when total data size is known.
    serializeHeader(CustomMode::START_HEADER, writePosition - data - 4, data);

    return writePosition - data;
}

uint MovescountSettings::serializeCustomModeGroups(u_int8_t *data)
{
    u_int8_t *writePosition = data + 4; //Save space for section header.

    foreach (CustomModeGroup customModeGroup, customModeGroups) {
        writePosition += customModeGroup.serialize(writePosition);
    }

    // Write header at the end, when total data size is known.
    CustomModeGroup::serializeStartHeader(writePosition - data - 4, data);

    return writePosition - data;
}

void MovescountSettings::serializeHeader(u_int16_t header_nbr, u_int16_t length ,u_int8_t *data)
{
    ambit_write_header_t *header = (ambit_write_header_t*)data;
    header->header = header_nbr;
    header->length = length;
}

uint MovescountSettings::serializeUnknownDataField(u_int8_t *data)
{
    serializeHeader(0x010b, 2, data);
    data += 4;
    *(u_int16_t *)data = 2;

    return 4 + 2;
}
