# - Try to find libiconv include dirs and libraries
#
# Usage of this module as follows:
#
#     find_package(iconv)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  ICONV_ROOT_DIR             Set this variable to the root installation of
#                            libiconv if the module has problems finding the
#                            proper installation path.
#
# Variables defined by this module:
#
#  ICONV_FOUND                System has libiconv, include and library dirs found
#  ICONV_INCLUDE_DIR          The libiconv include directories.
#  ICONV_LIBRARY              The libiconv library

find_path(ICONV_ROOT_DIR
    NAMES include/iconv.h
)

find_path(ICONV_INCLUDE_DIR
    NAMES iconv.h
    HINTS ${ICONV_ROOT_DIR}/include
)

find_library(ICONV_LIBRARY
    NAMES iconv
    HINTS ${ICONV_ROOT_DIR}/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ICONV DEFAULT_MSG
    ICONV_LIBRARY
    ICONV_INCLUDE_DIR
)

include(CheckCSourceCompiles)
set(CMAKE_REQUIRED_LIBRARIES ${ICONV_LIBRARY})
check_c_source_compiles("int main() { return 0; }" ICONV_LINKS_SOLO)
set(CMAKE_REQUIRED_LIBRARIES)

include(CheckFunctionExists)
set(CMAKE_REQUIRED_LIBRARIES ${ICONV_LIBRARY})
set(CMAKE_REQUIRED_LIBRARIES)

mark_as_advanced(
    ICONV_ROOT_DIR
    ICONV_INCLUDE_DIR
    ICONV_LIBRARY
)
