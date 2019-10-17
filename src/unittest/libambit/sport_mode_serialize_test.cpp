extern "C" {
#include <libambit/sport_mode_serialize.h>
}
#include "doctest.h"

TEST_SUITE_BEGIN("sport_mode_serializet");

TEST_CASE("test calculate and write empty") {
    ambit_sport_mode_device_settings_t *ambit_device_settings = libambit_malloc_sport_mode_device_settings();
    int size = calculate_size_for_serialize_sport_mode_device_settings(ambit_device_settings);
    CHECK(size == 18);

    uint8_t *data = (uint8_t*)malloc(size);
    int written = serialize_sport_mode_device_settings(ambit_device_settings, data);
    CHECK(size == written);

    free(data);

    libambit_sport_mode_device_settings_free(ambit_device_settings);
}

TEST_CASE("test calculate and write with sport modes") {
    ambit_sport_mode_device_settings_t *ambit_device_settings = libambit_malloc_sport_mode_device_settings();

    bool ret = libambit_malloc_sport_modes(20, ambit_device_settings);
    REQUIRE(ret);

    int size = calculate_size_for_serialize_sport_mode_device_settings(ambit_device_settings);
    CHECK(size == 7938);

    ret = libambit_malloc_sport_mode_displays(5, &ambit_device_settings->sport_modes[0]);
    REQUIRE(ret);

    size = calculate_size_for_serialize_sport_mode_device_settings(ambit_device_settings);
    CHECK(size == 8178);

    for(int i = 0;i < 5;i++) {
        ambit_device_settings->sport_modes[0].display[i].row1 = 10;
        ambit_device_settings->sport_modes[0].display[i].row2 = 13;
        ambit_device_settings->sport_modes[0].display[i].type = 13;
        ret = libambit_malloc_sport_mode_view(2, &ambit_device_settings->sport_modes[0].display[i]);
        ambit_device_settings->sport_modes[0].display[i].view[0] = 321;
        REQUIRE(ret);
    }

    size = calculate_size_for_serialize_sport_mode_device_settings(ambit_device_settings);
    CHECK(size == 8238);

    uint8_t *data = (uint8_t*)malloc(size);
    int written = serialize_sport_mode_device_settings(ambit_device_settings, data);
    CHECK(size == written + 220);       // calculate currently estimated too much here

    free(data);

    libambit_sport_mode_device_settings_free(ambit_device_settings);
}

/*

int calculate_size_for_serialize_app_data(ambit_sport_mode_device_settings_t *ambit_settings, ambit_app_rules_t *ambit_apps);
int serialize_app_data(ambit_sport_mode_device_settings_t *ambit_settings, ambit_app_rules_t *ambit_apps, uint8_t *data);
 */

TEST_SUITE_END();
