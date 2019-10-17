extern "C" {
#include "utils.h"
}

#include <malloc.h>
#include "doctest.h"

TEST_SUITE_BEGIN ("utils");

TEST_CASE ("testing strptime") {
    tm timestamp = {};
    char * nullChar = NULL;
    char* ret = libambit_strptime("2019-10-03", "%Y-%m-%d", &timestamp);
    CHECK(ret != nullChar);
}

TEST_CASE ("testing strptime failure") {
    tm timestamp = {};
    char * nullChar = NULL;
    char* ret = libambit_strptime("2019-103", "%a%b%c%d%234%sdkelsadsY-%m-%d", &timestamp);
    CHECK(ret == nullChar);
}

TEST_CASE ("testing htob") {
    uint8_t binary[6];
    int ret = libambit_htob("00112233AABB", binary, 6);
    REQUIRE(ret == 6);
    CHECK(binary[0] == 0);
    CHECK(binary[1] == 0x11);
    CHECK(binary[2] == 0x22);
    CHECK(binary[3] == 0x33);
    CHECK(binary[4] == 0xAA);
    CHECK(binary[5] == 0xBB);
}

TEST_CASE ("testing htob-fail") {
    uint8_t binary[6];
    memset(&binary, 0, 6);
    int ret = libambit_htob("ZYB00112233AABB", binary, 6);
    REQUIRE(ret == -1);
    CHECK(binary[0] == 0);
    CHECK(binary[1] == 0);
    CHECK(binary[2] == 0);
    CHECK(binary[3] == 0);
    CHECK(binary[4] == 0);
    CHECK(binary[5] == 0);
}

TEST_CASE ("testing utf8memconv ASCII") {
    char* charNull = NULL;
    char* ret = utf8memconv("01234567", 8, "UTF-8");
    REQUIRE(ret != charNull);
    CHECK_MESSAGE(strcmp(ret, "01234567") == 0, ret);

    free(ret);
}

TEST_CASE ("testing utf8memconv special chars") {
    char* charNull = NULL;
    char* ret = utf8memconv("AB", 8, "ASCII");
    REQUIRE(ret != charNull);
    CHECK_MESSAGE(strcmp(ret, "AB") == 0, ret);

    free(ret);
}

TEST_CASE ("testing utf8memconv invalid encoding") {
    char* charNull = NULL;
    char* ret = utf8memconv("01234567", 8, "bla");
    CHECK(ret == charNull);
}

TEST_CASE ("testing utf8wcsconv") {
    char* charNull = NULL;
    wchar_t *input = (wchar_t*)malloc(sizeof(wchar_t)*8);
    memset(input, 0, sizeof(wchar_t)*8);
    input[0] = 65;
    input[1] = 66;
    char* ret = utf8wcsconv(input);
    CHECK(ret != charNull);
    CHECK(strcmp(ret, "AB") == 0);

    free(ret);
    free(input);
}

TEST_CASE ("testing read8") {
    uint8_t data[2];
    data[0] = 1;
    data[1] = 2;
    uint8_t *buf = data;

    uint8_t ret = read8(buf, 0);
    CHECK(ret == 1);

    ret = read8(buf, 1);
    CHECK(ret == 2);

    // now with "inc"
    size_t offset = 0;
    ret = read8inc(buf, &offset);
    CHECK(ret == 1);
    REQUIRE(offset == 1);
    ret = read8inc(buf, &offset);
    CHECK(ret == 2);
    REQUIRE(offset == 2);
}

TEST_CASE ("testing read16") {
    uint8_t data[4];
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;
    data[3] = 4;
    uint8_t *buf = data;

    uint16_t ret = read16(buf, 0);
    CHECK(ret == 513);

    ret = read16(buf, 2);
    CHECK(ret == 1027);

    // now with "inc"
    size_t offset = 0;
    ret = read16inc(buf, &offset);
    CHECK(ret == 513);
    REQUIRE(offset == 2);
    ret = read16inc(buf, &offset);
    CHECK(ret == 1027);
    REQUIRE(offset == 4);
}

TEST_CASE ("testing read32") {
    uint8_t data[8];
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;
    data[3] = 4;
    data[4] = 5;
    data[5] = 6;
    data[6] = 7;
    data[7] = 8;
    uint8_t *buf = data;

    uint32_t ret = read32(buf, 0);
    CHECK(ret == 67305985);

    ret = read32(buf, 4);
    CHECK(ret == 134678021);

    // now with "inc"
    size_t offset = 0;
    ret = read32inc(buf, &offset);
    CHECK(ret == 67305985);
    REQUIRE(offset == 4);
    ret = read32inc(buf, &offset);
    CHECK(ret == 134678021);
    REQUIRE(offset == 8);
}

TEST_SUITE_END();
