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

include_directories(
  hidapi
)
