# - Resolve what hidapi driver to use
# This module is affected by the following defines
#  HIDAPI_DRIVER (possible values: libudev, libusb, pcapsimulate, mac, windows)
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
    elseif (HIDAPI_DRIVER STREQUAL "mac")
# Mac is still untested ...
#        #find_package(PCAP REQUIRED)
        set (HIDAPI_INCLUDE_DIR "hidapi" "")
        set (HIDAPI_SOURCE_FILES "hidapi/hid-mac.c")
        set (HIDAPI_LIBS "")
    elseif (HIDAPI_DRIVER STREQUAL "windows")
        find_package(iconv REQUIRED)
        set (HIDAPI_INCLUDE_DIR "hidapi" ${ICONV_INCLUDE_DIR})
        set (HIDAPI_SOURCE_FILES "hidapi/hid-windows.c")
        set (HIDAPI_LIBS ${ICONV_LIBRARY} setupapi)
    else (HIDAPI_DRIVER STREQUAL "libusb")
        find_package(UDev REQUIRED)
        set (HIDAPI_INCLUDE_DIR "hidapi" ${UDEV_INCLUDE_DIR})
        set (HIDAPI_SOURCE_FILES "hidapi/hid-linux.c")
        set (HIDAPI_LIBS ${UDEV_LIBS})
    endif (HIDAPI_DRIVER STREQUAL "libusb")

    mark_as_advanced(HIDAPI_INCLUDE_DIR HIDAPI_SOURCE_FILES HIDAPI_LIBS)
    set (HIDAPI_RESOLVED TRUE)
    message(STATUS "Found hidapi: ${HIDAPI_DRIVER} ${HIDAPI_INCLUDE_DIR} ${HIDAPI_SOURCE_FILES} ${HIDAPI_LIBS}")
endif (NOT HIDAPI_RESOLVED)
