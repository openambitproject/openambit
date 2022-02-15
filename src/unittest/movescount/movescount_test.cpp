#include <movescount/movescount.h>
#include "doctest.h"

TEST_SUITE_BEGIN("MovesCount");

TEST_CASE("testing fetching the MovesCount instance") {
    MovesCount *movesCount = MovesCount::instance();

    CHECK_FALSE_MESSAGE(movesCount == nullptr, "MovesCount instance should not be null");

    movesCount->exit();
}

TEST_CASE("testing invoking some methods") {
    MovesCount *movesCount = MovesCount::instance();

    movesCount->checkAuthorization();
    movesCount->checkLatestFirmwareVersion();
    movesCount->getDeviceSettings();
    movesCount->setUsername("bla");
    movesCount->setUserkey("bla");
    movesCount->setAppkey("bla");
    movesCount->setUploadLogs(true);
    movesCount->setDevice(DeviceInfo());
    movesCount->setBaseAddress("bla");

    CHECK_FALSE_MESSAGE(movesCount->generateUserkey() == nullptr, "UserKey should not be null");
    CHECK_FALSE_MESSAGE(movesCount->isAuthorized(), "Should not be authorized");
}

TEST_CASE("testing fetching orbital data") {
    MovesCount *movesCount = MovesCount::instance();

    uint8_t *orbitData = NULL;
    CHECK_MESSAGE(-1 == movesCount->getOrbitalData(&orbitData), "Did not expect to read orbital data");
    CHECK_MESSAGE(orbitData == NULL, "Should not get back data here");
}

TEST_CASE("testing reading personal settings") {
    MovesCount *movesCount = MovesCount::instance();

    ambit_personal_settings_t *moveCountPersonalSettings = libambit_personal_settings_alloc();
    CHECK_MESSAGE(-1 == movesCount->getPersonalSettings(moveCountPersonalSettings, false), "Did not expect to read personal settings");
    CHECK_MESSAGE(-1 == movesCount->getPersonalSettings(moveCountPersonalSettings, true), "Did not expect to read personal settings");
    libambit_personal_settings_free(moveCountPersonalSettings);

    movesCount->exit();
}

TEST_CASE("testing fetching routes") {
    MovesCount *movesCount = MovesCount::instance();

    ambit_route_t* data = libambit_route_alloc(1);
    ambit_personal_settings_t *moveCountPersonalSettings = libambit_personal_settings_alloc();
    CHECK_MESSAGE(-1 == movesCount->getRoute(data, moveCountPersonalSettings, QString("bla")), "Did not expect to read route data");
    libambit_personal_settings_free(moveCountPersonalSettings);
    libambit_route_free(data, 1);
}

TEST_CASE("testing fetching routes from files") {
    MovesCount *movesCount = MovesCount::instance();

    ambit_route_t* data = libambit_route_alloc(1);
    ambit_personal_settings_t *moveCountPersonalSettings = libambit_personal_settings_alloc();
    CHECK_MESSAGE(-1 == movesCount->getRouteFromFile(data, moveCountPersonalSettings, QString("bla"), QString("dir")), "Did not expect to read route data");
    libambit_personal_settings_free(moveCountPersonalSettings);
    libambit_route_free(data, 1);
}

TEST_SUITE_END();
