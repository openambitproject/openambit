#include <movescount/movescount.h>
#include <movescount/movescountsettings.h>
#include <movescount/movescountjson.h>
#include <QtCore/QFile>
#include "doctest.h"

TEST_SUITE_BEGIN("MovescountSettings");

TEST_CASE("empty device settings") {
    MovescountSettings settings = MovescountSettings();

    QVariantMap map = QVariantMap();
    settings.parse(map);

    ambit_sport_mode_device_settings_t *ambitDeviceSettings = libambit_malloc_sport_mode_device_settings();

    settings.toAmbitData(ambitDeviceSettings);

    CHECK(ambitDeviceSettings->sport_modes_count == 0);
    CHECK(ambitDeviceSettings->sport_mode_groups_count == 0);

    libambit_sport_mode_device_settings_free(ambitDeviceSettings);
}

TEST_CASE("device settings with data from test-settings.json") {
    MovescountSettings settings = MovescountSettings();

    MovesCountJSON jsonParser;


    QFile jsonFile("test-data/test-settings.json");
    jsonFile.open(QFile::ReadOnly);
    QByteArray data = jsonFile.read(9999999);

    REQUIRE(data.length() > 0);

    REQUIRE(jsonParser.parseDeviceSettingsReply(data, settings) == 0);

    ambit_sport_mode_device_settings_t *ambitDeviceSettings = libambit_malloc_sport_mode_device_settings();

    settings.toAmbitData(ambitDeviceSettings);

    CHECK(ambitDeviceSettings->sport_modes_count == 10);
    CHECK(ambitDeviceSettings->sport_mode_groups_count == 10);

    libambit_sport_mode_device_settings_free(ambitDeviceSettings);
}

TEST_CASE("device settings with data from test-settings-nologgedrules.json") {
    MovescountSettings settings = MovescountSettings();

    MovesCountJSON jsonParser;


    QFile jsonFile("test-data/test-settings-nologgedrules.json");
    jsonFile.open(QFile::ReadOnly);
    QByteArray data = jsonFile.read(9999999);

    REQUIRE(data.length() > 0);

    REQUIRE(jsonParser.parseDeviceSettingsReply(data, settings) == 0);

    ambit_sport_mode_device_settings_t *ambitDeviceSettings = libambit_malloc_sport_mode_device_settings();

    settings.toAmbitData(ambitDeviceSettings);

    CHECK(ambitDeviceSettings->sport_modes_count == 10);
    CHECK(ambitDeviceSettings->sport_mode_groups_count == 10);

    libambit_sport_mode_device_settings_free(ambitDeviceSettings);
}

TEST_SUITE_END();
