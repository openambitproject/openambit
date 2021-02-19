cmake_minimum_required(VERSION 2.8.5)
project (OPENAMBITCLI CXX)

set (OPENAMBIT_VERSION 0.5)

# Where to lookup modules
set(CMAKE_MODULE_PATH "${OPENAMBITCLI_SOURCE_DIR}/cmake")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

find_package(libambit REQUIRED)
find_package(Movescount REQUIRED)
IF (!WIN32)
find_package(UDev REQUIRED)
ELSE (!WIN32)
set(UDEV_LIBS "")
ENDIF (!WIN32)
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

set(openambitcli_HDRS Task.h)

set(openambitcli_SRCS main.cpp Task.cpp)

set(CMAKE_AUTOMOC ON)

add_definitions(-DAPP_VERSION="${OPENAMBIT_VERSION}")

add_executable(openambit-cli ${openambitcli_HDRS} ${openambitcli_SRCS})

target_link_libraries(openambit-cli ${LIBAMBIT_LIBS} ${MOVESCOUNT_LIBS} ${UDEV_LIBS} Qt5::Core )

install(TARGETS openambit-cli DESTINATION ${CMAKE_INSTALL_BINDIR})
