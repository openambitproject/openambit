extern "C" {
#include <libambit/device_support.h>
}
#include "doctest.h"

TEST_SUITE_BEGIN("device_support");

TEST_CASE("test device support known") {
    CHECK(!libambit_device_support_known(432, 433));
    CHECK(libambit_device_support_known(0x1493, 0x002d));
    CHECK(libambit_device_support_known(0x1493, 0x0019));
}

TEST_CASE("test device support find") {
    uint8_t fw_version;
    ambit_known_device_t *nullDevice = NULL;
    CHECK(libambit_device_support_find(543, 653, "any", &fw_version) == nullDevice);
}

TEST_CASE("test device support find 3") {
    uint8_t fw_version = 0;
    ambit_known_device_t *nullDevice = NULL;
    CHECK(libambit_device_support_find(0x1493, 0x002d, "Loon", &fw_version) == nullDevice);
}

TEST_CASE("test device support find 2") {
    uint8_t fw_version = 255;
    ambit_known_device_t *nullDevice = NULL;
    CHECK(libambit_device_support_find(0x1493, 0x002d, "Loon", &fw_version) != nullDevice);
}

TEST_CASE("test device komposti") {
    const uint8_t *noKomposti = libambit_device_komposti(432, 9343, 0);
    const uint8_t *nullKomposti = NULL;
    CHECK(noKomposti == nullKomposti);

    const uint8_t *komposti = libambit_device_komposti(0x1493, 0x0019, 0);
    CHECK(komposti[0] == 0x02);
    CHECK(komposti[1] == 0x00);
    CHECK(komposti[2] == 0x2d);
    CHECK(komposti[3] == 0x00);
}

TEST_CASE("test fw version number") {
    const uint8_t version[4] = { 0, 0, 0, 0};
    uint32_t combinedVersion = libambit_fw_version_number(version);
    CHECK(combinedVersion == 0);
}

TEST_SUITE_END();
