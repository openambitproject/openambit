find_package(libusb REQUIRED)
find_package(UDev REQUIRED)

add_library (
  libambit
  SHARED
  crc16.c
  libambit.c
  hid.c
  protocol.c
)

target_link_libraries(
  libambit
  ${UDEV_LIBS}
  ${LIBUSB_LIBRARIES}
  m
)

include_directories(
  hidapi
)
