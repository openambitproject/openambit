cmake_minimum_required (VERSION 2.8.5)
project (LIBAMBIT)

# Where to lookup modules
set(CMAKE_MODULE_PATH "${LIBAMBIT_SOURCE_DIR}/cmake")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

# Debug defines
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG_PRINT_ERROR -DDEBUG_PRINT_WARNING")
IF (DEFINED DEBUG_PRINT_INFO)
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG_PRINT_INFO=${DEBUG_PRINT_INFO}")
ENDIF()
IF (DEFINED DEBUG_PRINT_FILE_LINE)
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG_PRINT_FILE_LINE=${DEBUG_PRINT_FILE_LINE}")
ENDIF()

find_package(libusb REQUIRED)
find_package(UDev REQUIRED)
include(GNUInstallDirs)

add_library (
  ambit
  SHARED
  crc16.c
  debug.c
  hid.c
  libambit.c
  personal.c
  pmem20.c
  protocol.c
)

target_link_libraries(
  ambit
  ${UDEV_LIBS}
  ${LIBUSB_LIBRARIES}
  m
)

set_target_properties(ambit PROPERTIES VERSION 0.2.0 SOVERSION 0)

include_directories(
  hidapi
)

install(TARGETS ambit DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES libambit.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
