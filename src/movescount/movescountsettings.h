#ifndef MOVESCOUNTSETTINGS_H
#define MOVESCOUNTSETTINGS_H

#include <QObject>
#include <QList>

#include "custommode.h"
#include "custommodegroup.h"

class MovescountSettings : public QObject
{
    Q_OBJECT
public:
    explicit MovescountSettings(QObject *parent = 0);

    void parse(QVariantMap settingsMap);
    MovescountSettings(const MovescountSettings &other);

    MovescountSettings& operator=(MovescountSettings &rhs);

    uint serializeCustomMode(u_int8_t **data);
signals:

public slots:

private:
    uint serializeCustomModeGroups(u_int8_t *data);
    uint serializeCustomModes(u_int8_t *data);
    void serializeHeader(u_int16_t header_nbr, u_int16_t length, u_int8_t *data);
    uint serializeUnknownDataField(u_int8_t *data);

    QList<CustomModeGroup> customModeGroups;
    QList<CustomMode> customModes;
};

#endif // MOVESCOUNTSETTINGS_H
