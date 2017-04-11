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

void MovescountSettings::toAmbitData(ambit_sport_mode_device_settings_t *ambitSettings)
{
    // Copy Custom modes
    if (libambit_malloc_sport_modes(customModes.count(), ambitSettings)) {
        ambit_sport_mode_t *ambitCustomModes = ambitSettings->sport_modes;

        foreach(CustomMode customMode, customModes)
        {
            customMode.toAmbitCustomModeData(ambitCustomModes, ambitSettings);
            ambitCustomModes++;
        }
    }

    // Copy Custom mode Groups
    if (libambit_malloc_sport_mode_groups(customModeGroups.count(), ambitSettings)) {
        ambit_sport_mode_group_t *ambitCustomModeGroups = ambitSettings->sport_mode_groups;

        foreach(CustomModeGroup customModeGroup, customModeGroups)
        {
            customModeGroup.toAmbitData(ambitCustomModeGroups);
            ambitCustomModeGroups++;
        }
    }
}
