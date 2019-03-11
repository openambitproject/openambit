cmake_minimum_required (VERSION 3.1.3)
project (LIBAMBIT C)

# Where to lookup modules
set(CMAKE_MODULE_PATH "${LIBAMBIT_SOURCE_DIR}/cmake")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

# Debug defines
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG_PRINT_ERROR -DDEBUG_PRINT_WARNING")
IF (DEFINED DEBUG_PRINT_INFO)
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG_PRINT_INFO=${DEBUG_PRINT_INFO}")
ENDIF()
IF (DEFINED DEBUG_PRINT_FILE_LINE)
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG_PRINT_FILE_LINE=${DEBUG_PRINT_FILE_LINE}")
ENDIF()

include(HidapiDriver)
include(GNUInstallDirs)

add_library (
  ambit
  SHARED
  crc16.c
  debug.c
  device_driver_ambit.c
  device_driver_ambit_navigation.c
  device_driver_ambit3.c
  device_driver_common.c
  device_support.c
  distance.c
  libambit.c
  personal.c
  pmem20.c
  protocol.c
  sbem0102.c
  sha256.c
  utils.c
  sport_mode_serialize.c
  ${HIDAPI_SOURCE_FILES}
)

target_link_libraries(
  ambit
  ${HIDAPI_LIBS}
  m
)

set_target_properties(ambit PROPERTIES VERSION 0.4.0 SOVERSION 0)

include_directories(
  ${HIDAPI_INCLUDE_DIR}
)

install(TARGETS ambit DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES libambit.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

set(CMAKE_INSTALL_UDEVRULESDIR /etc/udev/rules.d
    CACHE PATH "Where to install udev rules")
mark_as_advanced(CMAKE_INSTALL_UDEVRULESDIR)

install(FILES libambit.rules
        DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_UDEVRULESDIR}
        COMPONENT system)
