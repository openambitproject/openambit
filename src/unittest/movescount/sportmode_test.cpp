#include <movescount/sportmode.h>
#include "doctest.h"

TEST_SUITE_BEGIN("CustomMode");

TEST_CASE("testing activity-name") {
    QVariantMap map = QVariantMap();

    map.insert(CustomMode::ACTIVITY_NAME, "test-name");

    CustomMode mode = CustomMode(map);

    char ambitName[CustomMode::NAME_SIZE];
    mode.toAmbitName(ambitName);
    CHECK(strcmp(ambitName, "test-name") == 0);

    // copy-construct and check again
    CustomMode copy = CustomMode(mode);
    copy.toAmbitName(ambitName);
    CHECK(strcmp(ambitName, "test-name") == 0);

    // assign and check again
    QVariantMap emptyMap = QVariantMap();
    CustomMode assign = CustomMode(emptyMap);
    assign = mode;
    assign.toAmbitName(ambitName);
    CHECK(strcmp(ambitName, "test-name") == 0);
}

TEST_CASE("testing members") {
    QVariantMap map = QVariantMap();

    map.insert(CustomMode::SPORT_MODE_ID, 23);

    CustomMode mode = CustomMode(map);

    CHECK(mode.getCustomModeId() == 23);
}

TEST_CASE("to ambit custom mode data") {
    QVariantMap map = QVariantMap();

    map.insert(CustomMode::SPORT_MODE_ID, 23);
    map.insert(CustomMode::ACTIVITY_NAME, "test-name");

    CustomMode mode = CustomMode(map);

    ambit_sport_mode_t *ambit_sport_modes = (ambit_sport_mode_t *)malloc(sizeof(ambit_sport_mode_t) * 1);
    ambit_sport_mode_device_settings_t *settings = libambit_malloc_sport_mode_device_settings();

    mode.toAmbitCustomModeData(ambit_sport_modes, settings);

    CHECK(strcmp(ambit_sport_modes->settings.activity_name, "test-name") == 0);
    CHECK(ambit_sport_modes->settings.sport_mode_id == 23);

    free(ambit_sport_modes);
    libambit_sport_mode_device_settings_free(settings);
}


TEST_CASE("to ambit settings") {
    QVariantMap map = QVariantMap();

    map.insert(CustomMode::SPORT_MODE_ID, 23);
    map.insert(CustomMode::ACTIVITY_NAME, "test-name");

    CustomMode mode = CustomMode(map);

    ambit_sport_mode_settings_t *settings = (ambit_sport_mode_settings_t *)malloc(sizeof(ambit_sport_mode_settings_t));
    mode.toAmbitSettings(settings);

    CHECK(strcmp(settings->activity_name, "test-name") == 0);
    CHECK(settings->sport_mode_id == 23);

    free(settings);
}

/*
class CustomModeDisplay : public QObject
{
public:
    explicit CustomModeDisplay(QVariantMap &displayMap, QObject *parent = 0);
    CustomModeDisplay(const CustomModeDisplay &other);

    CustomModeDisplay& operator=(const CustomModeDisplay &rhs);

    void toAmbitCustomModeData(ambit_sport_mode_display_t *ambitDisplay);

};
*/

TEST_SUITE_END();
