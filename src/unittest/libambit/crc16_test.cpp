extern "C" {
#include "crc16.h"
}
#include "doctest.h"

TEST_SUITE_BEGIN ("crc16");

TEST_CASE ("testing computing crc16") {
    unsigned char *text = (unsigned char *) "testtext";

    uint16_t crc1 = crc16_ccitt_false(text, 8);
    CHECK(crc1 == 7394);
    uint16_t crc2 = crc16_ccitt_false_init(text, 8, 1234);
    CHECK(crc2 == 828);
}

TEST_SUITE_END();
