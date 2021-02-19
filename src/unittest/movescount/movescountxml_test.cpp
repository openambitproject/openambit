#include <movescount/movescountxml.h>
#include "doctest.h"

TEST_SUITE_BEGIN("MovesCountXML");

TEST_CASE("testing writing LogEntry") {
    MovesCountXML* xml = new MovesCountXML();

    LogEntry entry = LogEntry();

    // store
    xml->writeLog(&entry);

    delete xml;
}

TEST_SUITE_END();
