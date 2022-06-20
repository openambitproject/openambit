#include <movescount/movescount.h>
#include <movescount/movescountjson.h>
#include <QFile>
#include "doctest.h"

TEST_SUITE_BEGIN("MovesCountJSON");

ambit_personal_settings_t *readSettings(MovesCountJSON* xml, MovesCount *movesCount) {
    QFile routeFile("test-data/test-settings.json");
    CHECK(routeFile.open(QIODevice::ReadOnly));

    QByteArray _data = routeFile.readAll();

    ambit_personal_settings_t *settings = libambit_personal_settings_alloc();
    memset(settings, 0, sizeof(ambit_personal_settings_t));

    xml->parsePersonalSettings(_data, settings, movesCount, NULL);

    return settings;
}

TEST_CASE("testing parsing settings") {
    MovesCountJSON* xml = new MovesCountJSON();
    MovesCount *movescount = MovesCount::instance();

    ambit_personal_settings_t *settings = readSettings(xml, movescount);
    CHECK_FALSE_MESSAGE(settings == nullptr, "Settings should not be null");

    movescount->exit();
    libambit_personal_settings_free(settings);
    delete xml;
}

TEST_CASE("apply settings from device") {
    MovesCountJSON* xml = new MovesCountJSON();

    MovesCount *movescount = MovesCount::instance();

    ambit_personal_settings_t *settings1 = readSettings(xml, movescount);
    CHECK_FALSE_MESSAGE(settings1 == nullptr, "Settings should not be null");

    ambit_personal_settings_t *settings2 = readSettings(xml, movescount);
    CHECK_FALSE_MESSAGE(settings2 == nullptr, "Settings should not be null");

    movescount->applyPersonalSettingsFromDevice(settings1, settings2);

    movescount->exit();
    libambit_personal_settings_free(settings1);
    libambit_personal_settings_free(settings2);
    delete xml;
}

TEST_SUITE_END();
