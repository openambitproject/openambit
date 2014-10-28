# - Resolve what hidapi driver to use
# This module is affected by the following defines
#  HIDAPI_DRIVER (possible values: usbraw, libusb, pcapsimulate)
#
# This module defines
#  HIDAPI_INCLUDE_DIR
#  HIDAPI_SOURCE_FILES
#  HIDAPI_LIBS

if (NOT HIDAPI_RESOLVED)
    if (HIDAPI_DRIVER STREQUAL "libusb")
        find_package(libusb REQUIRED)
        set (HIDAPI_INCLUDE_DIR "hidapi" ${LIBUSB_INCLUDE_DIR})
        set (HIDAPI_SOURCE_FILES "hidapi/hid-libusb.c")
        set (HIDAPI_LIBS ${LIBUSB_LIBRARIES})
    elseif (HIDAPI_DRIVER STREQUAL "pcapsimulate")
        find_package(PCAP REQUIRED)
        set (HIDAPI_INCLUDE_DIR "hidapi" ${PCAP_INCLUDE_DIR})
        set (HIDAPI_SOURCE_FILES "hidapi/hid-pcapsimulate.c")
        set (HIDAPI_LIBS ${PCAP_LIBRARY})
    else (HIDAPI_DRIVER STREQUAL "libusb")
        find_package(UDev REQUIRED)
        set (HIDAPI_INCLUDE_DIR "hidapi" ${UDEV_INCLUDE_DIR})
        set (HIDAPI_SOURCE_FILES "hidapi/hid-linux.c")
        set (HIDAPI_LIBS ${UDEV_LIBS})
    endif (HIDAPI_DRIVER STREQUAL "libusb")

    mark_as_advanced(HIDAPI_INCLUDE_DIR HIDAPI_SOURCE_FILES HIDAPI_LIBS)
    set (HIDAPI_RESOLVED TRUE)
endif (NOT HIDAPI_RESOLVED)
