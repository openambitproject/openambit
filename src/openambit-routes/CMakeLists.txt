cmake_minimum_required(VERSION 3.0.0)
project (OPENAMBITROUTES CXX)

set (OPENAMBIT_VERSION 0.5)

# Where to lookup modules
set(CMAKE_MODULE_PATH "${OPENAMBIT_SOURCE_DIR}/cmake")

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

set(OPENAMBITROUTES_HDRS Task.h)

set(OPENAMBITROUTES_SRCS main.cpp Task.cpp)

set(CMAKE_AUTOMOC ON)

add_definitions(-DAPP_VERSION="${OPENAMBIT_VERSION}")

add_executable(openambit-routes ${OPENAMBITROUTES_HDRS} ${OPENAMBITROUTES_SRCS})

target_link_libraries(openambit-routes ${LIBAMBIT_LIBS} ${MOVESCOUNT_LIBS} ${UDEV_LIBS} Qt5::Core )

install(TARGETS openambit-routes DESTINATION ${CMAKE_INSTALL_BINDIR})
