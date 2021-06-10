#include <movescount/movescountlogchecker.h>
#include <QtCore/QCoreApplication>
#include <chrono>
#include <thread>
#include "doctest.h"

TEST_SUITE_BEGIN("MovesCountLogChecker");

TEST_CASE("testing MovesCountLogChecker") {
    MovesCountLogChecker *checker = new MovesCountLogChecker();

    CHECK_FALSE_MESSAGE(checker->isRunning(), "checker should not be running initially");

    checker->run();

    // wait for it to start running
    while (!checker->isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    CHECK_MESSAGE(checker->isRunning(), "checker should be running after been started");

    checker->cancel();

    /* The thread is not actually started because QT is not fully
     * up and running in this test currently

    // wait for it to finish
    while (checker->isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }*/

    delete checker;
}

/*
    explicit MovesCountLogChecker(QObject *parent = 0);
    ~MovesCountLogChecker();
    void run();
    bool isRunning();
    void cancel();
    QString status();
*/

TEST_SUITE_END();