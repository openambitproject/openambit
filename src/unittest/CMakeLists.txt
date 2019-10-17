cmake_minimum_required(VERSION 2.8.5)
project (TEST CXX)

set (OPENAMBIT_VERSION 0.5)

# Where to lookup modules
set(CMAKE_MODULE_PATH "${TEST_SOURCE_DIR}/cmake")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

find_package(libambit REQUIRED)
find_package(Movescount REQUIRED)
find_package(UDev REQUIRED)
find_package(Qt5Core REQUIRED)

include(GNUInstallDirs)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${LIBAMBIT_INCLUDE_DIR}
  ..
)

link_directories(
  ${LIBAMBIT_LIBS_DIR}
  ${MOVESCOUNT_LIBS_DIR}
)

set(TEST_HDRS
)

set(TEST_SRCS
        main.cpp

        # movescount
        movescount/logentry_test.cpp
        movescount/logstore_test.cpp
        movescount/movescountxml_test.cpp
        movescount/movescount_test.cpp
        movescount/movescountsettings_test.cpp
        movescount/sportmode_test.cpp

        # libambit
        libambit/crc16_test.cpp
        libambit/device_support_test.cpp
        libambit/sport_mode_serialize_test.cpp
        libambit/utils-test.cpp)

add_definitions(-DAPP_VERSION="${OPENAMBIT_VERSION}")

add_executable(unittest ${TEST_HDRS} ${TEST_SRCS})

target_link_libraries(unittest ${LIBAMBIT_LIBS} ${MOVESCOUNT_LIBS} ${UDEV_LIBS} Qt5::Core )
