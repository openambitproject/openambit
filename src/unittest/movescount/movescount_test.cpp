#include <movescount/movescount.h>
#include "doctest.h"

TEST_SUITE_BEGIN("MovesCount");

TEST_CASE("testing fetching the MovesCount instance") {
    MovesCount *movesCount = MovesCount::instance();

    CHECK_FALSE_MESSAGE(movesCount == nullptr, "MovesCount instance should not be null");

    movesCount->exit();
}

TEST_SUITE_END();
