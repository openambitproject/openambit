cmake_minimum_required(VERSION 3.0.0)
project (EXAMPLE C)

# Where to lookup modules
set(CMAKE_MODULE_PATH "${EXAMPLE_SOURCE_DIR}/cmake")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

include(${CMAKE_SOURCE_DIR}/Mac.cmake)
find_package(libambit REQUIRED)

include_directories(
  ${LIBAMBIT_INCLUDE_DIR}
)

link_directories(
  ${LIBAMBIT_LIBS_DIR}
)

add_executable(
  ambitconsole ambitconsole.c
)

target_link_libraries(
  ambitconsole ${LIBAMBIT_LIBS}
)
