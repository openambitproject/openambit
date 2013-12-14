cmake_minimum_required (VERSION 2.6)
project (LIBAMBIT)

# Where to lookup modules
set (CMAKE_MODULE_PATH "${LIBAMBIT_SOURCE_DIR}/cmake")

find_package(libusb REQUIRED)
find_package(UDev REQUIRED)

add_library (
  ambit
  SHARED
  crc16.c
  libambit.c
  hid.c
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

set_target_properties(ambit PROPERTIES VERSION 0.0.1 SOVERSION 1) 

include_directories(
  hidapi
)
