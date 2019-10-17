#include <movescount/logentry.h>
#include <libambit.h>
#include "doctest.h"

TEST_SUITE_BEGIN("LogEntry");

static void populate(LogEntry *pEntry) {
    pEntry->device = "blabla";
    pEntry->movescountId = "id";

    pEntry->personalSettings = (ambit_personal_settings_t*)malloc(sizeof(ambit_personal_settings_t));
    memset(pEntry->personalSettings, 0, sizeof(ambit_personal_settings_t));

    pEntry->logEntry = (ambit_log_entry_t*)malloc(sizeof(ambit_log_entry_t));
    memset(pEntry->logEntry, 0, sizeof(ambit_log_entry_t));
}

TEST_CASE("testing constructing empty LogEntry") {
    LogEntry entry = LogEntry();
}

TEST_CASE("testing constructing LogEntry with data"){
    LogEntry entry = LogEntry();

    CHECK_EQ(entry.device, "");
    CHECK_EQ(entry.movescountId, "");
    // how to compare with NULL? CHECK_EQ(entry.personalSettings, NULL);
    // how to compare with NULL? CHECK_EQ(entry.logEntry, NULL);

    populate(&entry);

    CHECK_EQ(entry.device, "blabla");
    CHECK_EQ(entry.movescountId, "id");
    // how to compare with NULL? CHECK_NE(entry.personalSettings, NULL);
    // how to compare with NULL? CHECK_NE(entry.logEntry, NULL);
}

TEST_CASE("testing copying empty LogEntry") {
    LogEntry entry = LogEntry();
    LogEntry copy = LogEntry(entry);
}

TEST_CASE("testing copying LogEntry with some data") {
    LogEntry entry = LogEntry();

    entry.personalSettings = libambit_personal_settings_alloc();
    entry.personalSettings->alti_baro_mode = 23;
    entry.personalSettings->weight = 77;

    ambit_waypoint_t waypoints;
    strcpy(waypoints.name, "test");
    libambit_waypoint_append(entry.personalSettings, &waypoints, 1);

    strcpy(entry.personalSettings->waypoints.data[0].name, "testname");

    ambit_route_t *routes = libambit_route_alloc(2);
    entry.personalSettings->routes.data = routes;
    entry.personalSettings->routes.count = 2;

    entry.personalSettings->routes.data[0].activity_id = 54;
    entry.personalSettings->routes.data[0].points_count = 1;
    entry.personalSettings->routes.data[0].points = (ambit_routepoint_t*)malloc(sizeof(ambit_routepoint_t));
    entry.personalSettings->routes.data[0].points[0].altitude = 432;
    entry.personalSettings->routes.data[0].points[0].distance = 743;

    LogEntry copy = LogEntry(entry);

    CHECK(copy.personalSettings->alti_baro_mode == 23);
    CHECK(copy.personalSettings->weight == 77);
    CHECK(strcmp(copy.personalSettings->waypoints.data[0].name, "testname") == 0);
    CHECK(copy.personalSettings->routes.data[0].activity_id == 54);
    CHECK(copy.personalSettings->routes.data[0].points[0].altitude == 432);
    CHECK(copy.personalSettings->routes.data[0].points[0].distance == 743);
}

TEST_CASE("testing copying LogEntry with data") {
    LogEntry entry = LogEntry();

    populate(&entry);

    LogEntry copy = LogEntry(entry);
    CHECK_EQ(copy.device, "blabla");
    CHECK_EQ(copy.movescountId, "id");

    // should be copied so should not be equal
    CHECK_NE(copy.personalSettings, entry.personalSettings);
    CHECK_NE(copy.logEntry, entry.logEntry);
}

TEST_SUITE_END();